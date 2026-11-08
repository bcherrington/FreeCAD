// SPDX-License-Identifier: LGPL-2.1-or-later
#include <array>
#if defined(__linux__)
# include <sys/resource.h>
#endif
#include <QImage>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QTest>
#include <QPainter>
#include <QSvgRenderer>
#include <Inventor/fields/SoSFImage.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <src/App/InitApplication.h>
#include "ImagePerformanceTrace.h"

namespace Trace = Gui::ImagePerformanceTrace;

class ImagePerformanceTraceTest: public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        tests::initApplication();
        Gui::Application::initOpenInventor();
    }

    void formats_data()
    {
        QTest::addColumn<int>("format");
        QTest::addColumn<int>("extent");
        QTest::newRow("direct-rgb32-512") << int(QImage::Format_RGB32) << 512;
        QTest::newRow("direct-rgb32-4096") << int(QImage::Format_RGB32) << 4096;
        QTest::newRow("direct-argb32-512") << int(QImage::Format_ARGB32) << 512;
        QTest::newRow("direct-argb32-4096") << int(QImage::Format_ARGB32) << 4096;
        QTest::newRow("direct-rgbx-512") << int(QImage::Format_RGBX8888) << 512;
        QTest::newRow("direct-rgbx-4096") << int(QImage::Format_RGBX8888) << 4096;
        QTest::newRow("svg-derived-512") << -1 << 512;
        QTest::newRow("svg-derived-4096") << -1 << 4096;
        QTest::newRow("direct-rgba-512") << int(QImage::Format_RGBA8888) << 512;
        QTest::newRow("direct-rgba-4096") << int(QImage::Format_RGBA8888) << 4096;
        QTest::newRow("one-component-gray-512") << int(QImage::Format_Grayscale8) << 512;
        QTest::newRow("one-component-gray-4096") << int(QImage::Format_Grayscale8) << 4096;
        QTest::newRow("indexed-gray-alpha-512") << int(QImage::Format_Indexed8) << 512;
        QTest::newRow("indexed-gray-alpha-4096") << int(QImage::Format_Indexed8) << 4096;
        QTest::newRow("premultiplied-512") << int(QImage::Format_ARGB32_Premultiplied) << 512;
        QTest::newRow("premultiplied-4096") << int(QImage::Format_ARGB32_Premultiplied) << 4096;
        QTest::newRow("gray16-512") << int(QImage::Format_Grayscale16) << 512;
        QTest::newRow("gray16-4096") << int(QImage::Format_Grayscale16) << 4096;
    }

    void formats()
    {
        QFETCH(int, format);
        Trace::begin(-1);
        QFETCH(int, extent);
        const qint64 rgbaBytes = qint64(extent) * extent * 4;
        {
            Trace::Load load;
            QImage source(
                extent,
                extent,
                format < 0 ? QImage::Format_ARGB32_Premultiplied : QImage::Format(format)
            );
            if (source.format() == QImage::Format_Indexed8) {
                QList<QRgb> colors;
                for (int i = 0; i < 256; ++i) {
                    colors.push_back(qRgba(i, i, i, 255 - i));
                }
                source.setColorTable(colors);
            }
            source.fill(0);
            if (format < 0) {
                QSvgRenderer svg(QByteArrayLiteral(
                    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"40\" height=\"40\"><rect "
                    "width=\"40\" height=\"40\" fill=\"red\"/><rect x=\"3\" y=\"7\" width=\"9\" "
                    "height=\"15\" fill=\"blue\" fill-opacity=\"0.5\"/></svg>"
                ));
                QVERIFY(svg.isValid());
                QPainter painter(&source);
                svg.render(&painter);
            }
            Trace::Decoded decoded(source);
            {
                QImage shared(source);
                Trace::Decoded alias(shared);
                QCOMPARE(Trace::snapshot()["live_decoded_bytes"].toInteger(), source.sizeInBytes());
                SoSFImage converted;
                Trace::Field field(converted);
                Gui::BitmapFactory().convert(source, converted);
                auto result = Trace::snapshot();
                QCOMPARE(result["live_decoded_bytes"].toInteger(), source.sizeInBytes());
                QCOMPARE(result["live_coin_bytes"].toInteger(), rgbaBytes);
                QCOMPARE(result["coin_image_bytes"].toInteger(), rgbaBytes);
                const bool direct = source.format() == QImage::Format_RGBA8888
                    || source.format() == QImage::Format_RGBX8888
                    || source.format() == QImage::Format_RGB32
                    || source.format() == QImage::Format_ARGB32;
                QCOMPARE(
                    result["decoded_image_bytes"].toInteger(),
                    source.sizeInBytes() + (direct ? 0 : rgbaBytes)
                );
                SoSFImage copied;
                Trace::Field other(copied);
                copied = converted;
                Trace::updateField(&copied);
                QCOMPARE(Trace::snapshot()["live_coin_bytes"].toInteger(), 2 * rgbaBytes);
                auto observation = Trace::snapshot();
                observation.insert("source_format", int(source.format()));
                observation.insert("extent", extent);
                observation.insert("source_bytes_per_line", source.bytesPerLine());
                observation.insert("source_pixel_bytes", source.sizeInBytes());
                observation.insert(
                    "source_pixel_sha256",
                    QString::fromLatin1(
                        QCryptographicHash::hash(
                            QByteArrayView(
                                reinterpret_cast<const char*>(source.constBits()),
                                source.sizeInBytes()
                            ),
                            QCryptographicHash::Sha256
                        )
                            .toHex()
                    )
                );
                observation.insert(
                    "source_effective_components",
                    (source.allGray() ? 1 : 3) + (source.hasAlphaChannel() ? 1 : 0)
                );
                observation.insert("scenario", "counter-qualification-image-plane-copy-overlap");
#if defined(__linux__)
                struct rusage usage {};
                QVERIFY(getrusage(RUSAGE_SELF, &usage) == 0);
                observation.insert("peak_rss_bytes", qint64(usage.ru_maxrss) * 1024);
#endif
                qInfo().noquote() << "IMAGE_TRACE_RESOURCE_ROW"
                                  << QJsonDocument(observation).toJson(QJsonDocument::Compact);
            }
            QCOMPARE(Trace::snapshot()["live_coin_bytes"].toInteger(), 0);
            QCOMPARE(Trace::snapshot()["live_decoded_bytes"].toInteger(), source.sizeInBytes());
        }
        auto result = Trace::snapshot();
        QCOMPARE(result["live_decoded_bytes"].toInteger(), 0);
        QCOMPARE(result["live_coin_bytes"].toInteger(), 0);
        QVERIFY(result["storage_trace_balanced"].toBool());
    }

    void cAbiAndLifecycleCounters()
    {
        QVERIFY(!fc_image_trace_begin(2));
        QVERIFY(fc_image_trace_begin(-1));
        QVERIFY(fc_image_trace_snapshot());
        QCOMPARE(
            QJsonDocument::fromJson(fc_image_trace_snapshot()).object()["pending_textures"].toInt(),
            0
        );
        int owner = 0;
        Trace::provider(&owner, true);
        QCOMPARE(Trace::snapshot()["pending_textures"].toInt(), 1);
        Trace::provider(&owner, false);
        QCOMPARE(Trace::snapshot()["pending_textures"].toInt(), 0);
        Trace::load(true, false);
        Trace::load(false, true);
        Trace::load(true, true);
        QCOMPARE(Trace::snapshot()["load_attempts"].toInt(), 3);
        QCOMPARE(Trace::snapshot()["failed_pre_restore_loads"].toInt(), 1);
        QCOMPARE(Trace::snapshot()["successful_loads"].toInt(), 2);
        QCOMPARE(Trace::snapshot()["failed_loads"].toInt(), 1);
        Trace::load(false, false, false);
        QCOMPARE(Trace::snapshot()["load_attempts"].toInt(), 3);
        QCOMPARE(Trace::snapshot()["empty_load_calls"].toInt(), 1);
        QVERIFY(!fc_image_trace_begin(-1));
        Trace::removeProvider(&owner);
        QCOMPARE(Trace::snapshot()["image_plane_providers"].toInt(), 0);

        QVERIFY(!fc_image_trace_field_info(nullptr));
        QVERIFY(!fc_image_trace_field_info(reinterpret_cast<const void*>(1)));
        const std::array<unsigned char, 8> bytes {0, 128, 254, 255, 17, 23, 65, 200};
        {
            SoSFImage field;
            Trace::Field tracked(field);
            QVERIFY(fc_image_trace_replace_field(&field, 2, 1, bytes.data(), bytes.size()));
            Trace::updateField(&field);
            const auto info = QJsonDocument::fromJson(fc_image_trace_field_info(&field)).object();
            QCOMPARE(info["width"].toInt(), 2);
            QCOMPARE(info["height"].toInt(), 1);
            QCOMPARE(info["components"].toInt(), 4);
            QCOMPARE(
                info["sha256"].toString().toLatin1(),
                QCryptographicHash::hash(
                    QByteArray(reinterpret_cast<const char*>(bytes.data()), bytes.size()),
                    QCryptographicHash::Sha256
                )
                    .toHex()
            );
            QVERIFY(!fc_image_trace_replace_field(&field, 2, 1, bytes.data(), bytes.size() - 1));
            SoSFImage alias;
            Trace::Field trackedAlias(alias);
            SbVec2s size;
            int components;
            const auto* data = field.getValue(size, components);
            alias.setValue(size, components, data, SoSFImage::NO_COPY);
            Trace::updateField(&alias);
            QCOMPARE(Trace::snapshot()["materialized_image_fields"].toInt(), 2);
            QCOMPARE(Trace::snapshot()["live_coin_bytes"].toInteger(), qint64(bytes.size()));
        }
        QCOMPARE(Trace::snapshot()["live_coin_bytes"].toInt(), 0);
        QVERIFY(fc_image_trace_watch_renderer(nullptr));
        QCOMPARE(fc_image_trace_render_count(), 0ULL);
    }

    void detectsUnobservedDetach()
    {
        Trace::begin(-1);
        {
            Trace::Load load;
            QImage source(8, 8, QImage::Format_RGBA8888);
            QImage shared(source);
            Trace::Decoded decoded(source);
            source.fill(Qt::red);
        }
        QVERIFY(!Trace::snapshot()["storage_trace_balanced"].toBool());
        QCOMPARE(Trace::snapshot()["live_decoded_bytes"].toInteger(), 0);
        Trace::begin(-1);
        QVERIFY(Trace::snapshot()["storage_trace_balanced"].toBool());
    }
};

QTEST_MAIN(ImagePerformanceTraceTest)
#include "ImagePerformanceTraceTest.moc"
