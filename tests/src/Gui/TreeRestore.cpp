// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Python.h>
#include <fstream>
#include <memory>

#include <QCoreApplication>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

#include <Base/Exception.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/ImagePlane.h>
#include <App/MergeDocuments.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Tree.h>
#include <Gui/TreeParams.h>
#include <src/App/InitApplication.h>

namespace
{
class RestoreGuiApplication: public Gui::Application
{
public:
    RestoreGuiApplication()
        : Gui::Application(false)
    {}
    using Gui::Application::slotDeleteDocument;
    using Gui::Application::slotNewDocument;
};
}  // namespace

class TreeRestoreTest: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        tests::initApplication();
        Gui::Application::initOpenInventor();
        Gui::Application::initApplication();
        gui = std::make_unique<RestoreGuiApplication>();
        auto& app = App::GetApplication();
        newDocument = app.signalNewDocument.connect([this](const App::Document& doc, bool) {
            gui->slotNewDocument(doc, false);  // Real controller, without a native viewer.
        });
        deleteDocument = app.signalDeleteDocument.connect([this](const App::Document& doc) {
            gui->slotDeleteDocument(doc);
        });
        QVERIFY(directory.isValid());
        auto* doc = app.newDocument("RestoreFixture");
        changedBaseline = gui->getDocument(doc)->signalChangedObject.num_slots();
        touchedBaseline = doc->signalTouchedObject.num_slots();
        emptyArchive = directory.filePath(QStringLiteral("empty.FCStd")).toStdString();
        QVERIFY(doc->saveAs(emptyArchive.c_str()));
        QVERIFY(doc->addObject("Image::ImagePlane", "Plane"));
        archive = directory.filePath(QStringLiteral("populated.FCStd")).toStdString();
        QVERIFY(doc->saveAs(archive.c_str()));
        app.closeDocument(doc);
        invalidArchive = directory.filePath(QStringLiteral("invalid.FCStd")).toStdString();
        QFile invalid(QString::fromStdString(invalidArchive));
        QVERIFY(invalid.open(QIODevice::WriteOnly));
        QCOMPARE(invalid.write("invalid"), qint64(7));
        previousTimeout = Gui::TreeParams::getStatusTimeout();
        Gui::TreeParams::setStatusTimeout(5);
    }

    void cleanup()
    {
        tree.reset();
        Gui::TreeParams::setStatusTimeout(5);
        App::GetApplication().closeAllDocuments();
        QCoreApplication::processEvents();
    }

    void cleanupTestCase()
    {
        Gui::TreeParams::setStatusTimeout(previousTimeout);
        newDocument.disconnect();
        deleteDocument.disconnect();
        gui.reset();
    }

    void idleRestoreCompletes_data()
    {
        QTest::addColumn<QString>("operation");
        QTest::newRow("zero-timeout-restore") << QStringLiteral("zero-timeout-restore");
        for (const char* name :
             {"app-open",
              "empty-app-open",
              "direct-restore",
              "python-load",
              "delayed-restore",
              "copy-paste",
              "merge-import",
              "failed-direct"}) {
            QTest::newRow(name) << QString::fromLatin1(name);
        }
    }

    void idleRestoreCompletes()
    {
        QFETCH(QString, operation);
        if (operation == QStringLiteral("zero-timeout-restore")) {
            Gui::TreeParams::setStatusTimeout(0);
        }
        auto& app = App::GetApplication();
        tree = std::make_unique<Gui::TreeWidget>("RestoreTree");
        auto* timer = statusTimer();
        QVERIFY(timer);
        QVERIFY(!timer->isActive());
        QSignalSpy ticks(timer, &QTimer::timeout);
        App::Document* target = nullptr;
        bool failed = false;
        bool observedSuppression = false;
        bool populatedDuringRestore = false;
        fastsignals::scoped_connection restoreObservation;
        if (operation == QStringLiteral("app-open") || operation == QStringLiteral("empty-app-open")) {
            const bool empty = operation == QStringLiteral("empty-app-open");
            target = app.openDocument((empty ? emptyArchive : archive).c_str());
            QVERIFY(target);
            // Application open must flush synchronously at its completion barrier.
            QCOMPARE(tree->topLevelItemCount(), 1);
            QCOMPARE(tree->topLevelItem(0)->childCount(), empty ? 0 : 1);
            QCOMPARE(gui->getDocument(target)->signalChangedObject.num_slots(), changedBaseline + 1);
            QCOMPARE(target->signalTouchedObject.num_slots(), touchedBaseline + 1);
        }
        else if (
            operation == QStringLiteral("copy-paste") || operation == QStringLiteral("merge-import")
        ) {
            auto* source = app.newDocument("Source");
            auto* plane = source->addObject("Image::ImagePlane", "Plane");
            QVERIFY(plane);
            Gui::TreeWidget::updateStatus(false);
            target = app.newDocument("Target");
            QVERIFY(!timer->isActive());
            std::vector<App::DocumentObject*> imported;
            if (operation == QStringLiteral("copy-paste")) {
                imported = target->copyObject({plane});
            }
            else {
                std::ifstream input(archive, std::ios::binary);
                QVERIFY(input.is_open());
                App::MergeDocuments merge(target);
                imported = merge.importObjects(input);
            }
            QCOMPARE(imported.size(), std::size_t(1));
            // Keep source alive: closing it must not kick the target scheduler.
        }
        else {
            target = app.newDocument("Target");
            QVERIFY(!timer->isActive());
            restoreObservation = target->signalRestoreDocument.connect([&](Base::XMLReader&) {
                observedSuppression = app.isRestoring();
                QTest::qWait(15);  // Let scheduled ticks run while restore still owns the data.
                populatedDuringRestore = tree->getDocumentItem(gui->getDocument(target))->childCount()
                    != 0;
            });
            if (operation == QStringLiteral("failed-direct")) {
                // Force restore's delete/recreate controller path, then fail file validation.
                QVERIFY(target->addObject("Image::ImagePlane", "BeforeFailure"));
                Gui::TreeWidget::updateStatus(false);
                QVERIFY(!timer->isActive());
                try {
                    target->restore(invalidArchive.c_str());
                }
                catch (const Base::Exception&) {
                    failed = true;
                }
                QVERIFY(failed);
            }
            else if (operation == QStringLiteral("python-load")) {
                const auto gil = PyGILState_Ensure();
                PyObject* document = target->getPyObject();
                PyObject* result = PyObject_CallMethod(document, "load", "s", archive.c_str());
                const bool loaded = result != nullptr;
                if (!loaded) {
                    PyErr_Print();
                }
                Py_XDECREF(result);
                Py_DECREF(document);
                PyGILState_Release(gil);
                QVERIFY(loaded);
            }
            else {
                const bool delayed = operation == QStringLiteral("delayed-restore");
                target->restore(archive.c_str(), delayed);
                if (delayed) {
                    QVERIFY(target->afterRestore());
                }
            }
        }
        QVERIFY(!app.isRestoring());
        if (operation == QStringLiteral("direct-restore")
            || operation == QStringLiteral("python-load")
            || operation == QStringLiteral("delayed-restore")
            || operation == QStringLiteral("zero-timeout-restore")) {
            QVERIFY(observedSuppression);
            QVERIFY(!populatedDuringRestore);
        }
        auto* gdoc = gui->getDocument(target);
        QVERIFY(gdoc);
        auto* item = tree->getDocumentItem(gdoc);
        QVERIFY(item);
        const int expected = (failed || operation == QStringLiteral("empty-app-open")) ? 0 : 1;
        // No updateStatus/edit/open here: only the normal event loop can finish this work.
        QTRY_COMPARE_WITH_TIMEOUT(item->childCount(), expected, 1500);
        QTRY_COMPARE_WITH_TIMEOUT(gdoc->signalChangedObject.num_slots(), changedBaseline + 1, 1500);
        QTRY_COMPARE_WITH_TIMEOUT(target->signalTouchedObject.num_slots(), touchedBaseline + 1, 1500);
        QTRY_VERIFY_WITH_TIMEOUT(!timer->isActive(), 1500);
        const auto completedTicks = ticks.count();
        QTest::qWait(20);
        QCOMPARE(ticks.count(), completedTicks);  // No stranded periodic retry once complete.
        if (expected) {
            auto* plane = target->getObject("Plane");
            QVERIFY(plane);
            plane->Label.setValue("Observer attached");
            QCOMPARE(item->child(0)->text(0), QStringLiteral("Observer attached"));
        }
    }

    void failedAppOpenResumesScheduler()
    {
        auto& app = App::GetApplication();
        tree = std::make_unique<Gui::TreeWidget>("FailureTree");
        auto* timer = statusTimer();
        QVERIFY(timer);
        QVERIFY(!timer->isActive());
        bool failed = false;
        try {
            app.openDocument(invalidArchive.c_str());
        }
        catch (const Base::Exception&) {
            failed = true;
        }
        QVERIFY(failed);
        QVERIFY(!app.isRestoring());
        QTRY_VERIFY_WITH_TIMEOUT(!timer->isActive(), 1500);
        // No later open/edit is needed to release the failed operation's scheduler.
        for (auto* doc : app.getDocuments()) {
            auto* gdoc = gui->getDocument(doc);
            QVERIFY(gdoc);
            QTRY_COMPARE_WITH_TIMEOUT(gdoc->signalChangedObject.num_slots(), changedBaseline + 1, 1500);
        }
    }

private:
    QTimer* statusTimer() const
    {
        for (auto* timer : tree->findChildren<QTimer*>(QString(), Qt::FindDirectChildrenOnly)) {
            if (!timer->isSingleShot()) {
                return timer;
            }
        }
        return nullptr;
    }

    std::unique_ptr<RestoreGuiApplication> gui;
    fastsignals::scoped_connection newDocument;
    fastsignals::scoped_connection deleteDocument;
    std::unique_ptr<Gui::TreeWidget> tree;
    QTemporaryDir directory;
    std::string archive;
    std::string emptyArchive;
    std::string invalidArchive;
    std::size_t changedBaseline = 0;
    std::size_t touchedBaseline = 0;
    int previousTimeout = 0;
};

QTEST_MAIN(TreeRestoreTest)
#include "TreeRestore.moc"
