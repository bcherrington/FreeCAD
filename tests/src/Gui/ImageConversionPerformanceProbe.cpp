// SPDX-License-Identifier: LGPL-2.1-or-later
// Opt-in, one-conversion worker. Setup, hashing and fidelity checks are untimed.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <sys/resource.h>

#include <QApplication>
#include <QColorSpace>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>
#include <QPainter>
#include <QSvgRenderer>

#include <Inventor/fields/SoSFImage.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <src/App/InitApplication.h>
#include "ImagePerformanceTrace.h"

namespace Trace = Gui::ImagePerformanceTrace;
namespace
{
void require(bool value, const char* message)
{
    if (!value) {
        throw std::runtime_error(message);
    }
}
QString hash(const unsigned char* data, qsizetype length)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(
            QByteArrayView(reinterpret_cast<const char*>(data), length),
            QCryptographicHash::Sha256
        )
            .toHex()
    );
}
qint64 peakRss()
{
    rusage usage {};
    require(getrusage(RUSAGE_SELF, &usage) == 0, "RSS unavailable");
    return qint64(usage.ru_maxrss) * 1024;
}
QImage generate(const QString& recipe, int extent)
{
    require(extent == 512 || extent == 4096, "Unsupported recipe extent");
    if (recipe == "svg") {
        QImage image(extent, extent, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QSvgRenderer renderer(QByteArray(
            "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 256 256\">"
            "<rect width=\"256\" height=\"256\" fill=\"#2367ab\" fill-opacity=\"0.7\"/>"
            "<rect x=\"3\" y=\"7\" width=\"71\" height=\"119\" fill=\"red\"/>"
            "<circle cx=\"180\" cy=\"201\" r=\"39\" fill=\"lime\" fill-opacity=\"0.4\"/>"
            "<path d=\"M200 5L250 47L211 81Z\" fill=\"blue\"/></svg>"
        ));
        require(renderer.isValid(), "Invalid SVG recipe");
        QPainter painter(&image);
        renderer.render(&painter);
        require(painter.end(), "SVG painter completion failed");
        return image;
    }
    const bool indexed = recipe == "indexed-gray" || recipe == "indexed-gray-alpha";
    require(indexed || recipe == "premultiplied" || recipe == "gray16", "Unknown recipe");
    QImage image(
        extent,
        extent,
        indexed                  ? QImage::Format_Indexed8
            : recipe == "gray16" ? QImage::Format_Grayscale16
                                 : QImage::Format_ARGB32_Premultiplied
    );
    image.fill(0);  // Deterministic padding as well as pixels.
    if (indexed) {
        QList<QRgb> palette;
        for (int i = 0; i < 256; ++i) {
            palette.append(qRgba(i, i, i, recipe == "indexed-gray-alpha" ? (13 * i) % 256 : 255));
        }
        image.setColorTable(palette);
    }
    for (int y = 0; y < extent; ++y) {
        for (int x = 0; x < extent; ++x) {
            const int gray = (x + 3 * y) % 256;
            if (indexed) {
                image.scanLine(y)[x] = gray;
            }
            else if (recipe == "gray16") {
                reinterpret_cast<quint16*>(image.scanLine(y))[x] = quint16(gray * 257);
            }
            else {
                image.setPixelColor(
                    x,
                    y,
                    QColor(
                        (17 * x + 3 * y) % 256,
                        (5 * x + 11 * y) % 256,
                        (x + 7 * y) % 256,
                        (13 * x + y) % 256
                    )
                );
            }
        }
    }
    // Asymmetric corner witnesses; indexed palettes remain gray/gray-alpha.
    const std::array<int, 4> labels {17, 89, 163, 241};
    for (int i = 0; i < 4; ++i) {
        const int x = i % 2 ? extent - 1 : 0;
        const int y = i / 2 ? extent - 1 : 0;
        if (indexed) {
            image.setPixel(x, y, labels[i]);
        }
        else if (recipe == "gray16") {
            reinterpret_cast<quint16*>(image.scanLine(y))[x] = quint16(labels[i] * 257);
        }
        else {
            image.setPixelColor(x, y, QColor(labels[i], 255 - labels[i], 31 + i * 41, 64 + i * 63));
        }
    }
    return image;
}
}  // namespace

int main(int argc, char** argv)
{
    try {
        QApplication application(argc, argv);
        require(argc == 4, "Expected recipe extent input-file-or-dash");
        tests::initApplication();
        Gui::Application::initOpenInventor();
        const QString recipe = QString::fromLocal8Bit(argv[1]);
        bool validExtent = false;
        const int extent = QString::fromLocal8Bit(argv[2]).toInt(&validExtent);
        require(validExtent, "Invalid extent");
        auto& factory = Gui::BitmapFactory();
        QElapsedTimer decode;
        decode.start();
        QImage source = recipe == "payload" ? QImage(QString::fromLocal8Bit(argv[3]))
                                            : generate(recipe, extent);
        const double setupMs = double(decode.nsecsElapsed()) / 1e6;
        require(!source.isNull(), "Image setup failed");
        QJsonObject result {
            {"schema", "freecad-image-conversion-observation/v1"},
            {"qt_version", qVersion()},
            {"qt_platform", QGuiApplication::platformName()},
            {"recipe", recipe},
            {"width", source.width()},
            {"height", source.height()},
            {"source_format", int(source.format())},
            {"source_format_name", QMetaEnum::fromType<QImage::Format>().valueToKey(source.format())},
            {"source_bytes_per_line", source.bytesPerLine()},
            {"source_bytes", source.sizeInBytes()},
            {"source_has_alpha", source.hasAlphaChannel()},
            {"source_color_space", source.colorSpace().description()},
            {"source_color_space_valid", source.colorSpace().isValid()},
            {"source_pixel_sha256", hash(source.constBits(), source.sizeInBytes())},
            {"source_palette_sha256",
             hash(
                 reinterpret_cast<const unsigned char*>(source.colorTable().constData()),
                 source.colorTable().size() * qsizetype(sizeof(QRgb))
             )},
            {"setup_decode_or_generate_ms", setupMs}
        };
        // Snapshot process high-water before conversion; setup remains included in RSS.
        result.insert("pre_conversion_peak_rss_bytes", peakRss());
        Trace::begin(-1);
        {
            Trace::Load context;
            SoSFImage image;
            Trace::Field field(image);
            {
                Trace::Decoded decoded(source);
                QElapsedTimer timer;
                timer.start();
                factory.convert(source, image);
                const double convertMs = double(timer.nsecsElapsed()) / 1e6;
                result.insert(
                    "metrics",
                    QJsonObject {
                        {"convert_ms", convertMs},
                        {"peak_rss_bytes", peakRss()},
                        {"coin_image_bytes", Trace::snapshot()["coin_image_bytes"]},
                        {"decoded_image_bytes", Trace::snapshot()["decoded_image_bytes"]}
                    }
                );
                auto trace = Trace::snapshot();
                require(
                    trace["diagnostic_convert_calls"].toInteger() == 1,
                    "Conversion counter mismatch"
                );
            }
            SbVec2s size;
            int components = 0;
            const auto* bytes = image.getValue(size, components);
            require(
                bytes && size[0] == source.width() && size[1] == source.height() && components >= 1
                    && components <= 4,
                "Coin dimensions/components invalid"
            );
            const qsizetype length = qsizetype(size[0]) * size[1] * components;
            const QString outputHash = hash(bytes, length);
            int maxError = 0;
            for (int y = 0; y < size[1]; ++y) {
                for (int x = 0; x < size[0]; ++x) {
                    const QColor expected = source.pixelColor(x, size[1] - y - 1);
                    const auto* pixel = bytes + (qsizetype(y) * size[0] + x) * components;
                    const std::array<int, 4> actual {
                        pixel[0],
                        components < 3 ? pixel[0] : pixel[1],
                        components < 3 ? pixel[0] : pixel[2],
                        components == 2       ? pixel[1]
                            : components == 4 ? pixel[3]
                                              : 255
                    };
                    const std::array<int, 4> reference {
                        expected.red(),
                        expected.green(),
                        expected.blue(),
                        expected.alpha()
                    };
                    for (int c = 0; c < 4; ++c) {
                        maxError = std::max(maxError, std::abs(actual[c] - reference[c]));
                    }
                }
            }
            result.insert("coin_components", components);
            result.insert("coin_pixel_sha256", outputHash);
            result.insert("max_rgba_error", maxError);
            // Force writes through all original storage, then destroy caller image.
            source.fill(0);
            source = QImage();
            const bool ownsPixels = hash(bytes, length) == outputHash;
            result.insert("owns_pixels_after_source_write_and_destruction", ownsPixels);
            result.insert("correctness_passed", maxError <= 1 && ownsPixels);
            require(maxError <= 1 && ownsPixels, "Pixel/orientation/ownership mismatch");
        }
        const auto finalTrace = Trace::snapshot();
        require(
            finalTrace["storage_trace_balanced"].toBool()
                && finalTrace["live_coin_bytes"].toInteger() == 0
                && finalTrace["live_decoded_bytes"].toInteger() == 0
                && finalTrace["diagnostic_active_intervals"].toInteger() == 0,
            "Unbalanced trace"
        );
        result.insert("trace", finalTrace);
        // Loaded map is private provenance; publisher must omit absolute paths.
        QFile maps(QStringLiteral("/proc/self/maps"));
        require(maps.open(QIODevice::ReadOnly), "Runtime map unavailable");
        result.insert("runtime_maps", QString::fromUtf8(maps.readAll()));
        std::fputs("IMAGE_CONVERSION_RESULT ", stdout);
        std::fputs(QJsonDocument(result).toJson(QJsonDocument::Compact).constData(), stdout);
        std::fputs("\n", stdout);
        return 0;
    }
    catch (const std::exception& error) {
        std::fprintf(stderr, "Image conversion worker failed: %s\n", error.what());
        return 1;
    }
}
