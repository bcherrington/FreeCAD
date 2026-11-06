// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <vector>

#include <QApplication>
#include <QDir>
#include <QImage>
#include <QMetaEnum>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QPainter>
#include <QSurfaceFormat>
#include <QSvgRenderer>
#include <QTest>

#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/nodes/SoComplexity.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTexture2.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/ImagePlane.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/SoFCOffscreenRenderer.h>
#include <Gui/ViewProviderImagePlane.h>
#include <src/App/InitApplication.h>

namespace
{
class RenderGuiApplication: public Gui::Application
{
public:
    RenderGuiApplication()
        : Gui::Application(false)
    {}
    using Gui::Application::slotDeleteDocument;
    using Gui::Application::slotNewDocument;
};

struct Scene
{
    Scene()
        : root(new SoSeparator)
    {
        root->ref();
    }
    ~Scene()
    {
        root->unref();
    }
    SoSeparator* root;
};

QImage pattern()
{
    QImage image(4, 4, QImage::Format_ARGB32);
    const std::array<QColor, 4> colors {
        QColor(255, 0, 0, 255),
        QColor(0, 255, 0, 128),
        QColor(0, 0, 255, 255),
        QColor(255, 255, 0, 0)
    };
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            image.setPixelColor(x, y, colors[(y / 2) * 2 + x / 2]);
        }
    }
    return image;
}

void scalarReference(const QImage& source, SoSFImage& image)
{
    // Independently sample QColor as the historical converter did. Canonical
    // luminance/LA/RGB/RGBA components avoid the old stride-based inference.
    const bool gray = source.allGray();
    const bool alpha = source.hasAlphaChannel();
    const int components = (gray ? 1 : 3) + (alpha ? 1 : 0);
    std::vector<unsigned char> bytes;
    for (int y = source.height() - 1; y >= 0; --y) {
        for (int x = 0; x < source.width(); ++x) {
            const QColor color = source.pixelColor(x, y);
            bytes.push_back(color.red());
            if (!gray) {
                bytes.push_back(color.green());
                bytes.push_back(color.blue());
            }
            if (alpha) {
                bytes.push_back(color.alpha());
            }
        }
    }
    image.setValue(SbVec2s(source.width(), source.height()), components, bytes.data());
}
}  // namespace

class ImagePlaneRenderTest: public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        QOpenGLContext context;
        if (!context.create()) {
            QSKIP("OpenGL context unavailable: rendered acceptance not executed");
        }
        QOffscreenSurface surface;
        surface.setFormat(context.format());
        surface.create();
        QVERIFY(surface.isValid());
        QVERIFY(context.makeCurrent(&surface));
        qInfo() << "GL vendor:"
                << reinterpret_cast<const char*>(context.functions()->glGetString(GL_VENDOR));
        qInfo() << "GL renderer:"
                << reinterpret_cast<const char*>(context.functions()->glGetString(GL_RENDERER));
        qInfo() << "GL version:"
                << reinterpret_cast<const char*>(context.functions()->glGetString(GL_VERSION));
        qInfo() << "Qt backend:" << QGuiApplication::platformName()
                << "DPR:" << qApp->devicePixelRatio();
        context.doneCurrent();
        tests::initApplication();
        Gui::Application::initOpenInventor();
        Gui::Application::initApplication();
        gui = std::make_unique<RenderGuiApplication>();
    }

    void init()
    {
        doc = App::GetApplication().newDocument("RenderedImagePlane");
        gui->slotNewDocument(*doc, false);
        plane = dynamic_cast<Image::ImagePlane*>(doc->addObject("Image::ImagePlane", "Plane"));
        QVERIFY(plane);
        vp = dynamic_cast<Gui::ViewProviderImagePlane*>(gui->getDocument(doc)->getViewProvider(plane));
        QVERIFY(vp);
        plane->XSize.setValue(20);
        plane->YSize.setValue(10);
        vp->setDisplayMode("No shading");
        SoSearchAction search;
        search.setType(SoTexture2::getClassTypeId());
        search.setSearchingAll(true);
        search.apply(vp->getRoot());
        QVERIFY(search.getPath());
        texture = static_cast<SoTexture2*>(search.getPath()->getTail());
    }

    void cleanup()
    {
        if (doc) {
            gui->slotDeleteDocument(*doc);
            App::GetApplication().closeDocument(doc);
            doc = nullptr;
        }
    }

    void cleanupTestCase()
    {
        gui.reset();
    }

    void formats_data()
    {
        QTest::addColumn<int>("format");
        const auto names = QMetaEnum::fromType<QImage::Format>();
        for (int format = QImage::Format_Mono; format < QImage::NImageFormats; ++format) {
            QTest::newRow(names.valueToKey(format)) << format;
        }
        QTest::newRow("indexed-gray-alpha") << -1;
        QTest::newRow("indexed-color-alpha") << -2;
        QTest::newRow("svg-derived") << -3;
    }

    void formats()
    {
        QFETCH(int, format);
        QImage source;
        if (format >= 0) {
            source = pattern().convertToFormat(static_cast<QImage::Format>(format));
        }
        else if (format == -3) {
            QSvgRenderer svg(QByteArray(
                "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\">"
                "<rect width=\"2\" height=\"2\" fill=\"red\"/>"
                "<rect x=\"2\" width=\"2\" height=\"2\" fill=\"#00ff00\" fill-opacity=\"0.5\"/>"
                "<rect y=\"2\" width=\"2\" height=\"2\" fill=\"blue\"/></svg>"
            ));
            QVERIFY(svg.isValid());
            source = QImage(4, 4, QImage::Format_ARGB32_Premultiplied);
            source.fill(Qt::transparent);
            QPainter painter(&source);
            svg.render(&painter);
        }
        else {
            source = QImage(4, 4, QImage::Format_Indexed8);
            source.setColorTable(
                format == -1
                    ? QList<QRgb> {qRgba(20, 20, 20, 255), qRgba(100, 100, 100, 128), qRgba(180, 180, 180, 255), qRgba(0, 0, 0, 0)}
                    : QList<QRgb> {
                          qRgba(255, 0, 0, 255),
                          qRgba(0, 255, 0, 128),
                          qRgba(0, 0, 255, 255),
                          qRgba(255, 255, 0, 0)
                      }
            );
            for (int y = 0; y < 4; ++y) {
                for (int x = 0; x < 4; ++x) {
                    source.setPixel(x, y, (y / 2) * 2 + x / 2);
                }
            }
        }
        QVERIFY(!source.isNull());
        std::array<QColor, 16> expected;
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                expected[y * 4 + x] = source.pixelColor(x, y);
            }
        }
        SoSFImage reference;
        scalarReference(source, reference);
        SoSFImage converted;
        Gui::BitmapFactory().convert(source, converted);
        texture->image = converted;
        // Render only after both the caller's QImage and temporary Coin image die.
        std::fill_n(source.bits(), source.sizeInBytes(), 0xA5);
        source = QImage();
        converted.setValue(SbVec2s(0, 0), 0, nullptr);
        const std::vector<unsigned char> reuseStorage(65536, 0xCD);
        QImage actual = render();
        QVERIFY(!actual.isNull());
        texture->image = reference;
        QImage baseline = render();
        QVERIFY(!baseline.isNull());
        QCOMPARE(actual.size(), QSize(320, 240));
        QCOMPARE(actual.size(), baseline.size());
        // Fixed before sampling: <=2 levels for reference normalization + GL
        // blending; sample centers <=3 levels from analytic source-over color.
        for (int y = 0; y < 240; ++y) {
            for (int x = 0; x < 320; ++x) {
                const QColor a = actual.pixelColor(x, y), b = baseline.pixelColor(x, y);
                QVERIFY2(
                    std::abs(a.red() - b.red()) <= 2 && std::abs(a.green() - b.green()) <= 2
                        && std::abs(a.blue() - b.blue()) <= 2,
                    "Rendered scalar-reference mismatch"
                );
            }
        }
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                const QColor color = expected[y * 4 + x];
                const QColor pixel = actual.pixelColor(120 + x * 20 + 10, 100 + y * 10 + 5);
                const double alpha = color.alpha() / 255.0;
                QVERIFY(std::abs(pixel.red() - qRound(color.red() * alpha + 32 * (1 - alpha))) <= 3);
                QVERIFY(
                    std::abs(pixel.green() - qRound(color.green() * alpha + 64 * (1 - alpha))) <= 3
                );
                QVERIFY(std::abs(pixel.blue() - qRound(color.blue() * alpha + 96 * (1 - alpha))) <= 3);
            }
        }
        QCOMPARE(actual.pixelColor(20, 20), QColor(32, 64, 96));
        saveEvidence(actual, QString::fromLatin1(QTest::currentDataTag()));
    }

    void calibratedScaleAndPlacement()
    {
        QImage solid(4, 4, QImage::Format_RGB32);
        solid.fill(Qt::red);
        Gui::BitmapFactory().convert(solid, texture->image);
        const QImage initial = render();
        QCOMPARE(coloredBounds(initial), QRect(120, 100, 80, 40));
        // C++ axis-angle constructor takes radians.
        plane->Placement.setValue(
            Base::Placement(
                Base::Vector3d(3, -2, 0),
                Base::Rotation(Base::Vector3d(0, 0, 1), std::acos(-1.0) / 2)
            )
        );
        const QImage transformed = render();
        QCOMPARE(coloredBounds(transformed), QRect(152, 88, 40, 80));
        saveEvidence(initial, QStringLiteral("scale-initial"));
        saveEvidence(transformed, QStringLiteral("scale-placed"));
    }

private:
    QImage render()
    {
        Scene scene;
        auto* camera = new SoOrthographicCamera;
        camera->position.setValue(0, 0, 100);
        camera->height = 60;
        camera->nearDistance = 1;
        camera->farDistance = 200;
        scene.root->addChild(camera);
        auto* complexity = new SoComplexity;
        complexity->textureQuality = 0.1F;
        scene.root->addChild(complexity);
        scene.root->addChild(vp->getRoot());
        Gui::SoQtOffscreenRenderer renderer(SbViewportRegion(320, 240));
        renderer.setNumPasses(0);
        renderer.setBackgroundColor(SbColor4f(32 / 255.0F, 64 / 255.0F, 96 / 255.0F, 1));
        if (!renderer.render(scene.root)) {
            return {};
        }
        QImage image;
        renderer.writeToImage(image);
        return image;
    }

    QRect coloredBounds(const QImage& image)
    {
        int minX = 320, minY = 240, maxX = -1, maxY = -1;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (image.pixelColor(x, y) == QColor(32, 64, 96)) {
                    continue;
                }
                minX = std::min(minX, x);
                minY = std::min(minY, y);
                maxX = std::max(maxX, x);
                maxY = std::max(maxY, y);
            }
        }
        return QRect(QPoint(minX, minY), QPoint(maxX, maxY));
    }

    void saveEvidence(const QImage& image, const QString& name)
    {
        const QString directory = qEnvironmentVariable("FREECAD_IMAGE_TEST_OUTPUT");
        if (directory.isEmpty()) {
            return;
        }
        QVERIFY(QDir().mkpath(directory));
        QVERIFY(image.save(QDir(directory).filePath(name + QStringLiteral(".png"))));
    }

    std::unique_ptr<RenderGuiApplication> gui;
    App::Document* doc = nullptr;
    Image::ImagePlane* plane = nullptr;
    Gui::ViewProviderImagePlane* vp = nullptr;
    SoTexture2* texture = nullptr;
};

int main(int argc, char** argv)
{
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setOption(QSurfaceFormat::DeprecatedFunctions, true);
    format.setSamples(0);
    QSurfaceFormat::setDefaultFormat(format);
    QApplication application(argc, argv);
    ImagePlaneRenderTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "ImagePlaneRender.moc"
