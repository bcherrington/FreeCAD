// SPDX-License-Identifier: LGPL-2.1-or-later

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QTest>

#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTransform.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/ImagePlane.h>
#include <App/MergeDocuments.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/ViewProviderImagePlane.h>
#include <src/App/InitApplication.h>

namespace
{
// Use the real GUI document/controller hooks without creating a native viewer.
class ImagePlaneGuiApplication: public Gui::Application
{
public:
    ImagePlaneGuiApplication()
        : Gui::Application(false)
    {}

    using Gui::Application::slotDeleteDocument;
    using Gui::Application::slotNewDocument;
};
}  // namespace

class ImagePlaneTest: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        tests::initApplication();
        Gui::Application::initOpenInventor();
        Gui::Application::initApplication();
        gui = std::make_unique<ImagePlaneGuiApplication>();
        QVERIFY(directory.isValid());
        QImage image(40, 20, QImage::Format_ARGB32);
        image.fill(QColor(10, 20, 30, 255));
        image.setDotsPerMeterX(2000);
        image.setDotsPerMeterY(4000);
        rasterPath = directory.filePath(QStringLiteral("source.png")).toStdString();
        QVERIFY(image.save(QString::fromStdString(rasterPath)));
    }

    void cleanup()
    {
        for (auto* doc : App::GetApplication().getDocuments()) {
            gui->slotDeleteDocument(*doc);
            App::GetApplication().closeDocument(doc);
        }
    }

    void cleanupTestCase()
    {
        gui.reset();
    }

    void calibratedReopen_data()
    {
        QTest::addColumn<bool>("visible");
        QTest::newRow("visible") << true;
        QTest::newRow("hidden-first-show") << false;
    }

    void calibratedReopen()
    {
        QFETCH(bool, visible);
        auto* doc = createDocument();
        auto* plane = createPlane(doc);
        QVERIFY(plane);
        auto* vp = provider(plane);
        QVERIFY(vp);
        calibrate(plane);
        vp->Visibility.setValue(visible);
        const auto path = directory.filePath(QStringLiteral("calibrated.FCStd")).toStdString();
        QVERIFY(doc->saveAs(path.c_str()));
        closeDocument(doc);

        // The archive includes its image payload: reopening must not depend on this source path.
        doc = createDocument();
        doc->restore(path.c_str());
        plane = dynamic_cast<Image::ImagePlane*>(doc->getObject("Plane"));
        QVERIFY(plane);
        vp = provider(plane);
        QVERIFY(vp);
        QVERIFY(std::string(plane->ImageFile.getValue()) != rasterPath);
        QVERIFY(QFile::exists(QString::fromUtf8(plane->ImageFile.getValue())));
        QVERIFY(!vp->isRestoring());
        QVERIFY(!plane->testStatus(App::ObjectStatus::ObjImporting));
        QCOMPARE(vp->Visibility.getValue(), visible);
        assertCalibration(plane);
        assertTexture(vp, visible ? 40 : 0, visible ? 20 : 0);
        vp->Visibility.setValue(true);
        assertCalibration(plane);
        assertTexture(vp, 40, 20);
        assertGeometry(vp);
        QCOMPARE(plane->XPixelsPerMeter, 2000.0);
        QCOMPARE(plane->YPixelsPerMeter, 4000.0);
        vp->Visibility.setValue(false);
        vp->Visibility.setValue(true);
        assertCalibration(plane);
        assertTexture(vp, 40, 20);
        const auto second = directory.filePath(QStringLiteral("second.FCStd")).toStdString();
        QVERIFY(doc->saveAs(second.c_str()));
        closeDocument(doc);
        doc = createDocument();
        doc->restore(second.c_str());
        plane = dynamic_cast<Image::ImagePlane*>(doc->getObject("Plane"));
        QVERIFY(plane);
        assertCalibration(plane);
        assertTexture(provider(plane), 40, 20);
        assertGeometry(provider(plane));
    }

    void pendingVisibleFinishAfterFlagsClear()
    {
        auto* doc = createDocument();
        auto* plane = dynamic_cast<Image::ImagePlane*>(doc->addObject("Image::ImagePlane", "Plane"));
        QVERIFY(plane);
        auto* vp = provider(plane);
        QVERIFY(vp);
        vp->setStatus(Gui::isRestoring, true);
        plane->ImageFile.setValue(rasterPath.c_str());
        calibrate(plane);
        assertTexture(vp, 0, 0);
        vp->setStatus(Gui::isRestoring, false);
        QVERIFY(!plane->testStatus(App::ObjectStatus::ObjImporting));
        vp->finishRestoring();
        assertCalibration(plane);
        assertTexture(vp, 40, 20);
        assertGeometry(vp);
    }

    void hiddenImport_data()
    {
        QTest::addColumn<bool>("copy");
        QTest::newRow("copy-paste") << true;
        QTest::newRow("merge-archive") << false;
    }

    void hiddenImport()
    {
        QFETCH(bool, copy);
        auto* source = createDocument();
        auto* plane = createPlane(source);
        QVERIFY(plane);
        calibrate(plane);
        provider(plane)->Visibility.setValue(false);
        auto* target = createDocument();
        std::vector<App::DocumentObject*> imported;
        if (copy) {
            imported = target->copyObject({plane});
        }
        else {
            const auto path = directory.filePath(QStringLiteral("import.FCStd")).toStdString();
            QVERIFY(source->saveAs(path.c_str()));
            std::ifstream input(path, std::ios::binary);
            QVERIFY(input.is_open());
            App::MergeDocuments merge(target);
            imported = merge.importObjects(input);
        }
        QCOMPARE(imported.size(), std::size_t(1));
        auto* restored = dynamic_cast<Image::ImagePlane*>(imported.front());
        QVERIFY(restored);
        auto* vp = provider(restored);
        QVERIFY(vp);
        QVERIFY(!vp->isRestoring());
        QVERIFY(!restored->testStatus(App::ObjectStatus::ObjImporting));
        QVERIFY(!vp->Visibility.getValue());
        assertCalibration(restored);
        assertTexture(vp, 0, 0);
        closeDocument(source);
        vp->Visibility.setValue(true);
        assertCalibration(restored);
        assertTexture(vp, 40, 20);
        assertGeometry(vp);
        const auto path = directory.filePath(QStringLiteral("imported-shown.FCStd")).toStdString();
        QVERIFY(target->saveAs(path.c_str()));
        const std::string name = restored->getNameInDocument();
        closeDocument(target);
        target = createDocument();
        target->restore(path.c_str());
        restored = dynamic_cast<Image::ImagePlane*>(target->getObject(name.c_str()));
        QVERIFY(restored);
        assertCalibration(restored);
        assertTexture(provider(restored), 40, 20);
    }

    void freshImageAndExplicitResize()
    {
        auto* plane = createPlane(createDocument());
        QVERIFY(plane);
        QCOMPARE(plane->XSize.getValue(), 20.0);
        QCOMPARE(plane->YSize.getValue(), 5.0);
        calibrate(plane);
        auto* vp = provider(plane);
        vp->Visibility.setValue(false);
        vp->Visibility.setValue(true);
        assertCalibration(plane);
        assertGeometry(vp);
        assertTexture(vp, 40, 20);

        // Explicit file replacement still initializes the new image's physical dimensions.
        QImage replacement(60, 32, QImage::Format_RGB32);
        replacement.fill(Qt::red);
        replacement.setDotsPerMeterX(2000);
        replacement.setDotsPerMeterY(4000);
        const auto path = directory.filePath(QStringLiteral("replacement.png"));
        QVERIFY(replacement.save(path));
        plane->ImageFile.setValue(path.toUtf8().constData());
        QCOMPARE(plane->XSize.getValue(), 30.0);
        QCOMPARE(plane->YSize.getValue(), 8.0);
        assertTexture(vp, 60, 32);
        QVERIFY(plane->Placement.getValue().isSame(calibrationPlacement(), 1e-10));
    }

    void svgExplicitResize()
    {
        QFile file(directory.filePath(QStringLiteral("source.svg")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(
            file.write(
                "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"40\" height=\"20\">"
                "<rect width=\"40\" height=\"20\" fill=\"red\"/></svg>"
            )
            > 0
        );
        file.close();
        auto* doc = createDocument();
        auto* plane = dynamic_cast<Image::ImagePlane*>(doc->addObject("Image::ImagePlane", "Plane"));
        QVERIFY(plane);
        plane->ImageFile.setValue(file.fileName().toUtf8().constData());
        QVERIFY(plane->XSize.getValue() > 0);
        QVERIFY(plane->YSize.getValue() > 0);
        calibrate(plane);
        auto* vp = provider(plane);
        vp->Visibility.setValue(false);
        vp->Visibility.setValue(true);
        assertCalibration(plane);
        assertGeometry(vp);
        SoSearchAction search;
        search.setType(SoTexture2::getClassTypeId());
        search.setSearchingAll(true);
        search.apply(vp->getRoot());
        QVERIFY(search.getPath());
        auto* texture = static_cast<SoTexture2*>(search.getPath()->getTail());
        SbVec2s size;
        int components;
        QVERIFY(texture->image.getValue(size, components));
        QVERIFY(size[0] > 0 && size[1] > 0);
    }

private:
    App::Document* createDocument()
    {
        auto* doc = App::GetApplication().newDocument("ImagePlaneLifecycle");
        gui->slotNewDocument(*doc, false);
        return doc;
    }

    void closeDocument(App::Document* doc)
    {
        gui->slotDeleteDocument(*doc);
        App::GetApplication().closeDocument(doc);
    }

    Image::ImagePlane* createPlane(App::Document* doc)
    {
        auto* plane = dynamic_cast<Image::ImagePlane*>(doc->addObject("Image::ImagePlane", "Plane"));
        if (plane) {
            plane->ImageFile.setValue(rasterPath.c_str());
        }
        return plane;
    }

    Gui::ViewProviderImagePlane* provider(Image::ImagePlane* plane)
    {
        return dynamic_cast<Gui::ViewProviderImagePlane*>(
            gui->getDocument(plane->getDocument())->getViewProvider(plane)
        );
    }

    Base::Placement calibrationPlacement() const
    {
        return {Base::Vector3d(3.25, -4.5, 8.0), Base::Rotation(Base::Vector3d(1, 2, 3), 37.0)};
    }

    void calibrate(Image::ImagePlane* plane)
    {
        plane->XSize.setValue(123.25);
        plane->YSize.setValue(67.5);
        plane->Placement.setValue(calibrationPlacement());
    }

    void assertCalibration(Image::ImagePlane* plane)
    {
        QCOMPARE(plane->XSize.getValue(), 123.25);
        QCOMPARE(plane->YSize.getValue(), 67.5);
        QVERIFY(plane->Placement.getValue().isSame(calibrationPlacement(), 1e-10));
    }

    void assertTexture(Gui::ViewProviderImagePlane* vp, int width, int height)
    {
        SoSearchAction search;
        search.setType(SoTexture2::getClassTypeId());
        search.setSearchingAll(true);
        search.apply(vp->getRoot());
        QVERIFY(search.getPath());
        auto* texture = static_cast<SoTexture2*>(search.getPath()->getTail());
        SbVec2s size;
        int components;
        const auto* bytes = texture->image.getValue(size, components);
        QCOMPARE(int(size[0]), width);
        QCOMPARE(int(size[1]), height);
        if (width) {
            QVERIFY(bytes);
            QCOMPARE(components, 4);
        }
        else {
            QVERIFY(!bytes);
        }
    }

    void assertGeometry(Gui::ViewProviderImagePlane* vp)
    {
        QCOMPARE(vp->getTransformNode()->translation.getValue(), SbVec3f(3.25F, -4.5F, 8.0F));
        SoSearchAction search;
        search.setType(SoCoordinate3::getClassTypeId());
        search.setSearchingAll(true);
        search.apply(vp->getRoot());
        QVERIFY(search.getPath());
        auto* coords = static_cast<SoCoordinate3*>(search.getPath()->getTail());
        QCOMPARE(coords->point[0], SbVec3f(-61.625F, -33.75F, 0));
        QCOMPARE(coords->point[2], SbVec3f(61.625F, 33.75F, 0));
    }

    std::unique_ptr<ImagePlaneGuiApplication> gui;
    QTemporaryDir directory;
    std::string rasterPath;
};

QTEST_MAIN(ImagePlaneTest)
#include "ImagePlane.moc"
