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

#include <Base/Exception.h>
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
        svgPath = directory.filePath(QStringLiteral("lifecycle.svg")).toStdString();
        QFile svg(QString::fromStdString(svgPath));
        QVERIFY(svg.open(QIODevice::WriteOnly));
        QVERIFY(
            svg.write(
                "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"40\" height=\"20\">"
                "<rect width=\"40\" height=\"20\" fill=\"red\"/></svg>"
            )
            > 0
        );
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
        QTest::addColumn<bool>("svg");
        QTest::newRow("visible") << true << false;
        QTest::newRow("hidden-first-show") << false << false;
        QTest::newRow("svg-visible") << true << true;
        QTest::newRow("svg-hidden-first-show") << false << true;
    }

    void calibratedReopen()
    {
        QFETCH(bool, visible);
        QFETCH(bool, svg);
        auto* doc = createDocument();
        auto* plane = createPlane(doc, svg ? svgPath.c_str() : nullptr);
        QVERIFY(plane);
        auto* vp = provider(plane);
        QVERIFY(vp);
        const double dpmX = plane->XPixelsPerMeter;
        const double dpmY = plane->YPixelsPerMeter;
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
        assertTexture(vp, (visible || eagerBaseline()) ? 40 : 0, (visible || eagerBaseline()) ? 20 : 0);
        vp->Visibility.setValue(true);
        assertCalibration(plane);
        assertTexture(vp, 40, 20);
        assertGeometry(vp);
        QCOMPARE(plane->XPixelsPerMeter, dpmX);
        QCOMPARE(plane->YPixelsPerMeter, dpmY);
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
        assertRestoredHiddenTexture(vp);
        closeDocument(source);
        vp->Visibility.setValue(true);
        assertCalibration(restored);
        assertTexture(vp, 40, 20);
        assertGeometry(vp);
        vp->Visibility.setValue(false);
        vp->Visibility.setValue(true);
        assertCalibration(restored);
        assertTexture(vp, 40, 20);
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

    void earlyShowDefersUntilCompletion_data()
    {
        QTest::addColumn<bool>("importing");
        QTest::addColumn<bool>("latePayload");
        QTest::newRow("gui-restore-ready") << false << false;
        QTest::newRow("gui-restore-late") << false << true;
        QTest::newRow("app-import-ready") << true << false;
        QTest::newRow("app-import-late") << true << true;
    }

    void earlyShowDefersUntilCompletion()
    {
        QFETCH(bool, importing);
        QFETCH(bool, latePayload);
        auto* plane = dynamic_cast<Image::ImagePlane*>(
            createDocument()->addObject("Image::ImagePlane", "Plane")
        );
        QVERIFY(plane);
        auto* vp = provider(plane);
        QVERIFY(vp);
        vp->setStatus(Gui::isRestoring, !importing);
        plane->setStatus(App::ObjectStatus::ObjImporting, importing);
        plane->ImageFile.setValue(rasterPath.c_str());
        calibrate(plane);
        const QString payload = QString::fromUtf8(plane->ImageFile.getValue());
        const QString delayed = payload + QStringLiteral(".late");
        if (latePayload) {
            QVERIFY(QFile::rename(payload, delayed));
        }
        vp->show();
        assertTexture(vp, 0, 0);
        assertCalibration(plane);
        if (latePayload) {
            QVERIFY(QFile::rename(delayed, payload));
        }
        vp->setStatus(Gui::isRestoring, false);
        // In real imports payload extraction finishes before ObjImporting clears.
        vp->finishRestoring();
        assertTexture(vp, 40, 20);
        assertCalibration(plane);
        assertGeometry(vp);
        plane->setStatus(App::ObjectStatus::ObjImporting, false);
    }

    void failedPendingPayloadClearsOldTexture_data()
    {
        QTest::addColumn<bool>("missing");
        QTest::newRow("removed-included-payload") << true;
        QTest::newRow("corrupt-included-payload") << false;
    }

    void failedPendingPayloadClearsOldTexture()
    {
        QFETCH(bool, missing);
        auto* plane = createPlane(createDocument());
        QVERIFY(plane);
        auto* vp = provider(plane);
        QVERIFY(vp);
        calibrate(plane);
        vp->setStatus(Gui::isRestoring, true);
        vp->updateData(&plane->ImageFile);
        const QString payload = QString::fromUtf8(plane->ImageFile.getValue());
        QVERIFY(QFile::setPermissions(payload, QFileDevice::ReadOwner | QFileDevice::WriteOwner));
        if (missing) {
            QVERIFY(QFile::remove(payload));
        }
        else {
            QFile file(payload);
            QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
            QVERIFY(file.write("not an image") > 0);
        }
        vp->setStatus(Gui::isRestoring, false);
        vp->finishRestoring();
        assertTexture(vp, 0, 0);
        assertCalibration(plane);
        assertGeometry(vp);
        // Explicitly selecting a valid replacement is a new image assignment.
        plane->ImageFile.setValue(rasterPath.c_str());
        QCOMPARE(plane->XSize.getValue(), 20.0);
        QCOMPARE(plane->YSize.getValue(), 5.0);
        assertTexture(vp, 40, 20);
        QVERIFY(plane->Placement.getValue().isSame(calibrationPlacement(), 1e-10));
    }

    void invalidExternalAssignmentPreservesValidSource()
    {
        auto* plane = createPlane(createDocument());
        QVERIFY(plane);
        calibrate(plane);
        const std::string original = plane->ImageFile.getValue();
        const auto missing = directory.filePath(QStringLiteral("absent.png")).toStdString();
        QVERIFY_THROWS_EXCEPTION(Base::FileSystemError, plane->ImageFile.setValue(missing.c_str()));
        QCOMPARE(std::string(plane->ImageFile.getValue()), original);
        assertCalibration(plane);
        assertGeometry(provider(plane));
        assertTexture(provider(plane), 40, 20);
    }

    void clearImageAndReopenPreserveCalibration()
    {
        auto* doc = createDocument();
        auto* plane = createPlane(doc);
        QVERIFY(plane);
        calibrate(plane);
        App::PropertyFileIncluded empty;
        plane->ImageFile.Paste(empty);
        QVERIFY(std::string(plane->ImageFile.getValue()).empty());
        assertTexture(provider(plane), 0, 0);
        assertCalibration(plane);
        assertGeometry(provider(plane));
        const auto path = directory.filePath(QStringLiteral("cleared.FCStd")).toStdString();
        QVERIFY(doc->saveAs(path.c_str()));
        closeDocument(doc);
        doc = createDocument();
        doc->restore(path.c_str());
        plane = dynamic_cast<Image::ImagePlane*>(doc->getObject("Plane"));
        QVERIFY(plane);
        assertCalibration(plane);
        assertTexture(provider(plane), 0, 0);
    }

    void unsupportedImageAssignmentPreservesCalibration()
    {
        QImage unsupported(65537, 1, QImage::Format_ARGB32);
        QVERIFY(!unsupported.isNull());
        unsupported.fill(Qt::red);
        const auto path = directory.filePath(QStringLiteral("unsupported.png"));
        QVERIFY(unsupported.save(path));
        auto* plane = createPlane(createDocument());
        QVERIFY(plane);
        calibrate(plane);
        plane->ImageFile.setValue(path.toUtf8().constData());
        assertTexture(provider(plane), 0, 0);
        assertCalibration(plane);
        assertGeometry(provider(plane));
    }

    void removedExternalSourceUsesEmbeddedPayload_data()
    {
        QTest::addColumn<bool>("visible");
        QTest::newRow("visible") << true;
        QTest::newRow("hidden") << false;
    }

    void removedExternalSourceUsesEmbeddedPayload()
    {
        QFETCH(bool, visible);
        const QString external = directory.filePath(QStringLiteral("external.png"));
        QVERIFY(QFile::copy(QString::fromStdString(rasterPath), external));
        auto* doc = createDocument();
        auto* plane = createPlane(doc, external.toUtf8().constData());
        QVERIFY(plane);
        calibrate(plane);
        auto* vp = provider(plane);
        vp->Visibility.setValue(visible);
        QVERIFY(QFile::remove(external));
        QVERIFY(QFile::exists(QString::fromUtf8(plane->ImageFile.getValue())));
        const auto path = directory.filePath(QStringLiteral("embedded.FCStd")).toStdString();
        QVERIFY(doc->saveAs(path.c_str()));
        closeDocument(doc);
        doc = createDocument();
        doc->restore(path.c_str());
        plane = dynamic_cast<Image::ImagePlane*>(doc->getObject("Plane"));
        QVERIFY(plane);
        assertCalibration(plane);
        vp = provider(plane);
        assertTexture(vp, (visible || eagerBaseline()) ? 40 : 0, (visible || eagerBaseline()) ? 20 : 0);
        vp->Visibility.setValue(true);
        assertTexture(vp, 40, 20);
        assertCalibration(plane);
    }

    void hiddenResizeAndSaveBeforeFirstShow()
    {
        auto* doc = createDocument();
        auto* plane = createPlane(doc);
        QVERIFY(plane);
        calibrate(plane);
        provider(plane)->Visibility.setValue(false);
        const auto first = directory.filePath(QStringLiteral("pending-first.FCStd")).toStdString();
        QVERIFY(doc->saveAs(first.c_str()));
        closeDocument(doc);
        doc = createDocument();
        doc->restore(first.c_str());
        plane = dynamic_cast<Image::ImagePlane*>(doc->getObject("Plane"));
        QVERIFY(plane);
        assertRestoredHiddenTexture(provider(plane));
        plane->XSize.setValue(80);
        plane->YSize.setValue(30);
        assertCalibration(plane, 80, 30);
        assertGeometry(provider(plane), 80, 30);
        const auto second = directory.filePath(QStringLiteral("pending-second.FCStd")).toStdString();
        QVERIFY(doc->saveAs(second.c_str()));
        assertRestoredHiddenTexture(provider(plane));
        closeDocument(doc);
        doc = createDocument();
        doc->restore(second.c_str());
        plane = dynamic_cast<Image::ImagePlane*>(doc->getObject("Plane"));
        QVERIFY(plane);
        assertRestoredHiddenTexture(provider(plane));
        provider(plane)->Visibility.setValue(true);
        assertCalibration(plane, 80, 30);
        assertGeometry(provider(plane), 80, 30);
        assertTexture(provider(plane), 40, 20);
    }

    void materializedRepeatedShowDoesNotReloadMissingBackingFile()
    {
        auto* plane = createPlane(createDocument());
        QVERIFY(plane);
        calibrate(plane);
        auto* vp = provider(plane);
        QVERIFY(QFile::remove(QString::fromUtf8(plane->ImageFile.getValue())));
        for (int i = 0; i < 3; ++i) {
            vp->Visibility.setValue(false);
            vp->Visibility.setValue(true);
            assertCalibration(plane);
            assertTexture(vp, 40, 20);
        }
    }

private:
    static bool eagerBaseline()
    {
#ifdef FC_IMAGE_PERFORMANCE_TRACE
        // Only the opt-in benchmark build may qualify the deliberately eager
        // performance baseline. Restore-time safety expectations stay identical.
        return qEnvironmentVariableIntValue("SPEC003_EAGER_BASELINE") == 1;
#else
        return false;
#endif
    }

    void assertRestoredHiddenTexture(Gui::ViewProviderImagePlane* vp)
    {
        assertTexture(vp, eagerBaseline() ? 40 : 0, eagerBaseline() ? 20 : 0);
    }

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

    Image::ImagePlane* createPlane(App::Document* doc, const char* file = nullptr)
    {
        auto* plane = dynamic_cast<Image::ImagePlane*>(doc->addObject("Image::ImagePlane", "Plane"));
        if (plane) {
            plane->ImageFile.setValue(file ? file : rasterPath.c_str());
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

    void assertCalibration(Image::ImagePlane* plane, double x = 123.25, double y = 67.5)
    {
        QCOMPARE(plane->XSize.getValue(), x);
        QCOMPARE(plane->YSize.getValue(), y);
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

    void assertGeometry(Gui::ViewProviderImagePlane* vp, float x = 123.25F, float y = 67.5F)
    {
        QCOMPARE(vp->getTransformNode()->translation.getValue(), SbVec3f(3.25F, -4.5F, 8.0F));
        SoSearchAction search;
        search.setType(SoCoordinate3::getClassTypeId());
        search.setSearchingAll(true);
        search.apply(vp->getRoot());
        QVERIFY(search.getPath());
        auto* coords = static_cast<SoCoordinate3*>(search.getPath()->getTail());
        QCOMPARE(coords->point[0], SbVec3f(-x / 2, -y / 2, 0));
        QCOMPARE(coords->point[2], SbVec3f(x / 2, y / 2, 0));
    }

    std::unique_ptr<ImagePlaneGuiApplication> gui;
    QTemporaryDir directory;
    std::string rasterPath;
    std::string svgPath;
};

QTEST_MAIN(ImagePlaneTest)
#include "ImagePlane.moc"
