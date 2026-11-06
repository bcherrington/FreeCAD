// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Python.h>
#include <fstream>
#include <QCryptographicHash>
#include <QDir>
#include <QDropEvent>
#include <QMimeData>
#include <QMessageBox>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>
#include <App/DocumentObjectGroup.h>
#include <Gui/Selection/Selection.h>
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
#include <Gui/MainWindow.h>
#include <Gui/WorkbenchManager.h>
#include <Gui/Tree.h>
#include <Gui/TreeParams.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <src/App/InitApplication.h>

namespace
{
class BehaviorGuiApplication: public Gui::Application
{
public:
    BehaviorGuiApplication()
        : Gui::Application(true)
    {}
    using Gui::Application::slotDeleteDocument;
    using Gui::Application::slotNewDocument;
};
}  // namespace

class BehaviorTree: public Gui::TreeWidget
{
public:
    explicit BehaviorTree(QWidget* parent = nullptr)
        : Gui::TreeWidget("BehaviorTree", parent)
    {}
    using Gui::TreeWidget::dropEvent;
    void dragging(bool enabled)
    {
        setState(enabled ? DraggingState : NoState);
    }
};

class TreeBehaviorTest: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        tests::initApplication();
        Gui::Application::initOpenInventor();
        Gui::Application::initApplication();
        gui = std::make_unique<BehaviorGuiApplication>();
        mainWindow = std::make_unique<Gui::MainWindow>();
        QVERIFY(Gui::WorkbenchManager::instance()->activate("TreeBehaviorTest", "Gui::NoneWorkbench"));
        auto& app = App::GetApplication();
        QVERIFY(directory.isValid());
        auto* doc = newNoView("EmptyFixture");
        changedBaseline = gui->getDocument(doc)->signalChangedObject.num_slots();
        touchedBaseline = doc->signalTouchedObject.num_slots();
        emptyArchive = directory.filePath(QStringLiteral("empty.FCStd")).toStdString();
        QVERIFY(doc->saveAs(emptyArchive.c_str()));
        app.closeDocument(doc);
        const QString output = qEnvironmentVariable("FREECAD_TREE_FIXTURE_OUTPUT");
        if (!output.isEmpty()) {
            QVERIFY(QDir().mkpath(output));
            for (int count : {8, 512}) {
                doc = newNoView("TreeFixture");
                doc->Label.setValue(count == 8 ? "Small tree fixture" : "Larger tree fixture");
                auto* group = static_cast<App::DocumentObjectGroup*>(
                    doc->addObject("App::DocumentObjectGroup", "Assembly")
                );
                auto* nested = static_cast<App::DocumentObjectGroup*>(
                    doc->addObject("App::DocumentObjectGroup", "Nested")
                );
                group->addObject(nested);
                for (int i = 0; i < count; ++i) {
                    const std::string name = "Plane" + std::to_string(i);
                    auto* plane = doc->addObject("Image::ImagePlane", name.c_str());
                    if (i < count / 2) {
                        nested->addObject(plane);
                    }
                }
                const auto path = QDir(output).filePath(QStringLiteral("tree-%1.FCStd").arg(count));
                QVERIFY(doc->saveAs(path.toUtf8().constData()));
                app.closeDocument(doc);
            }
            fixtureRoot = output;
        }
        else {
            fixtureRoot = QString::fromUtf8(TREE_BEHAVIOR_DATA_DIR);
        }
        for (int count : {8, 512}) {
            QFile input(fixture(count));
            QVERIFY(input.open(QIODevice::ReadOnly));
            const auto digest
                = QCryptographicHash::hash(input.readAll(), QCryptographicHash::Sha256).toHex();
            if (output.isEmpty()) {
                QCOMPARE(
                    digest,
                    count == 8
                        ? QByteArrayLiteral(
                              "b664fd8eed579b0d1f1b96dcba204bc8070bfe576b62d8c73040c33f7f34028a"
                          )
                        : QByteArrayLiteral(
                              "f0849e08d5453e10846fcb6b7c0f67563766ed6f7bfebc08b7ac5febb218d2b5"
                          )
                );
            }
        }
        invalidArchive = directory.filePath(QStringLiteral("invalid.FCStd")).toStdString();
        QFile invalid(QString::fromStdString(invalidArchive));
        QVERIFY(invalid.open(QIODevice::WriteOnly));
        QCOMPARE(invalid.write("invalid"), qint64(7));
        previousTimeout = Gui::TreeParams::getStatusTimeout();
        previousSyncView = Gui::TreeParams::getSyncView();
        previousRecompute = Gui::TreeParams::getRecomputeOnDrop();
        Gui::TreeParams::setSyncView(false);
        Gui::TreeParams::setRecomputeOnDrop(false);
        Gui::TreeParams::setStatusTimeout(5);
    }

    void cleanup()
    {
        tree.reset();
        treeWindow.reset();
        Gui::TreeParams::setStatusTimeout(5);
        App::GetApplication().closeAllDocuments();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QTest::qWait(20);  // Drain native activation and deferred view deletion between fixtures.
    }

    void cleanupTestCase()
    {
        Gui::TreeParams::setStatusTimeout(previousTimeout);
        Gui::TreeParams::setSyncView(previousSyncView);
        Gui::TreeParams::setRecomputeOnDrop(previousRecompute);
        mainWindow.reset();
        gui.reset();
    }

    void idleRestoreCompletes_data()
    {
        QTest::addColumn<QString>("operation");
        QTest::addColumn<int>("planeCount");
        for (int count : {8, 512}) {
            for (const char* name :
                 {"zero-timeout-restore",
                  "app-open",
                  "empty-app-open",
                  "direct-restore",
                  "python-load",
                  "delayed-restore",
                  "copy-paste",
                  "merge-import",
                  "failed-direct"}) {
                const auto tag
                    = QStringLiteral("%1-%2").arg(count).arg(QString::fromLatin1(name)).toLatin1();
                QTest::newRow(tag.constData()) << QString::fromLatin1(name) << count;
            }
        }
    }

    void idleRestoreCompletes()
    {
        QFETCH(QString, operation);
        QFETCH(int, planeCount);
        archive = fixture(planeCount).toStdString();
        if (operation == QStringLiteral("zero-timeout-restore")) {
            Gui::TreeParams::setStatusTimeout(0);
        }
        auto& app = App::GetApplication();
        tree = std::make_unique<BehaviorTree>();
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
            target = openNoView((empty ? emptyArchive : archive).c_str());
            QVERIFY(target);
            // Application open must flush synchronously at its completion barrier.
            QCOMPARE(tree->topLevelItemCount(), 1);
            QCOMPARE(tree->topLevelItem(0)->childCount(), empty ? 0 : planeCount / 2 + 1);
            QCOMPARE(gui->getDocument(target)->signalChangedObject.num_slots(), changedBaseline + 1);
            QCOMPARE(target->signalTouchedObject.num_slots(), touchedBaseline + 1);
        }
        else if (
            operation == QStringLiteral("copy-paste") || operation == QStringLiteral("merge-import")
        ) {
            auto* source = openNoView(archive.c_str());
            QVERIFY(source);
            target = newNoView("Target");
            QVERIFY(!timer->isActive());
            std::vector<App::DocumentObject*> imported;
            if (operation == QStringLiteral("copy-paste")) {
                imported = target->copyObject(source->getObjects());
            }
            else {
                std::ifstream input(archive, std::ios::binary);
                QVERIFY(input.is_open());
                App::MergeDocuments merge(target);
                imported = merge.importObjects(input);
            }
            QCOMPARE(imported.size(), std::size_t(planeCount + 2));
            // Keep source alive: closing it must not kick the target scheduler.
        }
        else {
            target = newNoView("Target");
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
        const int expected = (failed || operation == QStringLiteral("empty-app-open"))
            ? 0
            : planeCount / 2 + 1;
        // No updateStatus/edit/open here: only the normal event loop can finish this work.
        QTRY_COMPARE_WITH_TIMEOUT(item->childCount(), expected, 1500);
        QTRY_COMPARE_WITH_TIMEOUT(gdoc->signalChangedObject.num_slots(), changedBaseline + 1, 1500);
        QTRY_COMPARE_WITH_TIMEOUT(target->signalTouchedObject.num_slots(), touchedBaseline + 1, 1500);
        QTRY_VERIFY_WITH_TIMEOUT(!timer->isActive(), 1500);
        const auto completedTicks = ticks.count();
        QTest::qWait(20);
        QCOMPARE(ticks.count(), completedTicks);  // No stranded periodic retry once complete.
        if (expected) {
            auto* plane = target->getObject("Plane0");
            QVERIFY(plane);
            plane->Label.setValue("Observer attached");
            tree->expandAll();
            QTRY_VERIFY(findItem(plane));
            QCOMPARE(findItem(plane)->text(0), QStringLiteral("Observer attached"));
            auto* vp = static_cast<Gui::ViewProviderDocumentObject*>(gdoc->getViewProvider(plane));
            QVERIFY(vp);
            item->setShowHidden(false);
            vp->ShowInTree.setValue(false);
            QTRY_VERIFY(findItem(plane)->isHidden());
            vp->ShowInTree.setValue(true);
            QTRY_VERIFY(!findItem(plane)->isHidden());
        }
    }

    void groupDeletionPreservesEffects_data()
    {
        fixtureRows();
    }

    void groupDeletionPreservesEffects()
    {
        QFETCH(int, planeCount);
        auto* doc = openFixture(planeCount);
        QVERIFY(doc);
        QTRY_COMPARE(objectItems(), planeCount + 2);
        auto* group = doc->getObject("Nested");
        auto* child = doc->getObject("Plane0");
        auto* keep = doc->getObject(("Plane" + std::to_string(planeCount - 2)).c_str());
        auto* remove = doc->getObject(("Plane" + std::to_string(planeCount - 1)).c_str());
        QVERIFY(group && child && keep && remove);
        tree->raise();
        tree->activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(treeWindow.get()));
        tree->setFocus();
        QTRY_VERIFY2(
            tree->hasFocus(),
            qPrintable(QStringLiteral("visible=%1 active=%2 focus=%3 policy=%4")
                           .arg(tree->isVisible())
                           .arg(tree->isActiveWindow())
                           .arg(
                               QApplication::focusWidget()
                                   ? QApplication::focusWidget()->metaObject()->className()
                                   : "null"
                           )
                           .arg(int(tree->focusPolicy())))
        );
        tree->scrollToItem(findItem(remove));
        QTest::mouseClick(
            tree->viewport(),
            Qt::LeftButton,
            Qt::NoModifier,
            tree->visualItemRect(findItem(remove)).center()
        );
        QTRY_VERIFY(Gui::Selection().isSelected(doc->getName(), remove->getNameInDocument()));
        QVERIFY(Gui::Selection().addSelection(doc->getName(), keep->getNameInDocument()));
        const std::string removedName = remove->getNameInDocument();
        doc->removeObject(removedName.c_str());
        QTRY_VERIFY(!findItem(remove));
        QVERIFY(!Gui::Selection().isSelected(doc->getName(), removedName.c_str()));
        QVERIFY(Gui::Selection().isSelected(doc->getName(), keep->getNameInDocument()));
        QTRY_VERIFY(tree->hasFocus());
        doc->removeObject("Nested");
        QTRY_VERIFY(!findItem(group));
        QTRY_COMPARE(objectItems(), planeCount);
        auto* item = tree->getDocumentItem(gui->getDocument(doc));
        QVERIFY(item);
        for (int i = 0; i < planeCount / 2; ++i) {
            auto* survivor = doc->getObject(("Plane" + std::to_string(i)).c_str());
            QVERIFY(survivor);
            QVERIFY(findItem(survivor));
            QCOMPARE(findItem(survivor)->parent(), static_cast<QTreeWidgetItem*>(item));
            QVERIFY(item->getViewProvider(survivor));
        }
        child->Label.setValue("Reparented child");
        QCOMPARE(findItem(child)->text(0), QStringLiteral("Reparented child"));
        QVERIFY(Gui::Selection().isSelected(doc->getName(), keep->getNameInDocument()));
        QTRY_VERIFY(tree->hasFocus());
    }

    void expansionAndReload_data()
    {
        fixtureRows();
    }

    void expansionAndReload()
    {
        QFETCH(int, planeCount);
        auto* doc = openFixture(planeCount);
        QVERIFY(doc);
        auto* outer = findItem(doc->getObject("Assembly"));
        auto* nested = findItem(doc->getObject("Nested"));
        QVERIFY(outer && nested);
        outer->setExpanded(true);
        nested->setExpanded(false);
        doc->getObject("Plane0")->Label.setValue("Saved child");
        const auto path = directory.filePath(QStringLiteral("roundtrip.FCStd")).toStdString();
        QVERIFY(doc->saveAs(path.c_str()));
        doc->restore(path.c_str());
        QTRY_VERIFY(!statusTimer()->isActive());
        QTRY_COMPARE(doc->getObjects().size(), std::size_t(planeCount + 2));
        outer = findItem(doc->getObject("Assembly"));
        nested = findItem(doc->getObject("Nested"));
        QVERIFY(outer && nested);
        QVERIFY(outer->isExpanded());
        QVERIFY(!nested->isExpanded());
        nested->setExpanded(true);
        QTRY_VERIFY(findItem(doc->getObject("Plane0")));
        QCOMPARE(findItem(doc->getObject("Plane0"))->text(0), QStringLiteral("Saved child"));
        tree.reset();
        App::GetApplication().closeDocument(doc);
        tree = std::make_unique<BehaviorTree>(treeWindow.get());
        treeWindow->layout()->addWidget(tree.get());
        doc = openNoView(path.c_str());
        QVERIFY(doc);
        QCOMPARE(tree->topLevelItemCount(), 1);
        QVERIFY(findItem(doc->getObject("Assembly"))->isExpanded());
        tree->expandAll();
        QTRY_COMPARE(objectItems(), planeCount + 2);
    }

    void dragDefersAndDropMovesObject_data()
    {
        fixtureRows();
    }

    void dragDefersAndDropMovesObject()
    {
        QFETCH(int, planeCount);
        auto* doc = openFixture(planeCount);
        QVERIFY(doc);
        auto* plane = doc->getObject(("Plane" + std::to_string(planeCount - 1)).c_str());
        auto* nested = static_cast<App::DocumentObjectGroup*>(doc->getObject("Nested"));
        QVERIFY(plane && nested);
        auto* selected = findItem(plane);
        QVERIFY(selected);
        tree->setCurrentItem(selected);
        selected->setSelected(true);
        QTRY_VERIFY(Gui::Selection().isSelected(doc->getName(), plane->getNameInDocument()));
        tree->dragging(true);
        auto* added = doc->addObject("Image::ImagePlane", "DuringDrag");
        QVERIFY(added);
        QTest::qWait(30);
        QVERIFY(!findItem(added));
        QVERIFY(statusTimer()->isActive());
        QVERIFY(selected->isSelected());
        tree->dragging(false);
        QTRY_VERIFY(findItem(added));
        QTRY_VERIFY(!statusTimer()->isActive());
        QVERIFY(selected->isSelected());
        tree->scrollToItem(findItem(nested));
        QCoreApplication::processEvents();
        QMimeData mime;
        QDropEvent drop(
            tree->visualItemRect(findItem(nested)).center(),
            Qt::MoveAction,
            &mime,
            Qt::LeftButton,
            Qt::NoModifier
        );
        bool modal = false;
        QTimer watchdog;
        connect(&watchdog, &QTimer::timeout, this, [&]() {
            for (auto* window : QApplication::topLevelWidgets()) {
                if (auto* box = qobject_cast<QMessageBox*>(window)) {
                    modal = true;
                    box->reject();
                }
            }
        });
        watchdog.start(200);
        tree->dropEvent(&drop);  // Production drop handler with the real selected source and target.
        watchdog.stop();
        QVERIFY(!modal);
        QTRY_VERIFY(nested->hasObject(plane));
        tree->expandAll();
        QTRY_VERIFY(findItem(plane));
        QTRY_COMPARE(findItem(plane)->parent(), findItem(nested));
        QTRY_COMPARE(objectItems(), planeCount + 3);
    }

    void failedAppOpenResumesScheduler()
    {
        auto& app = App::GetApplication();
        tree = std::make_unique<BehaviorTree>();
        auto* timer = statusTimer();
        QVERIFY(timer);
        QVERIFY(!timer->isActive());
        bool failed = false;
        try {
            openNoView(invalidArchive.c_str());
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
    static App::Document* newNoView(const char* name)
    {
        App::DocumentInitFlags flags;
        flags.createView = false;
        return App::GetApplication().newDocument(name, "TreeBehaviorTest", flags);
    }

    static App::Document* openNoView(const char* path)
    {
        App::DocumentInitFlags flags;
        flags.createView = false;
        return App::GetApplication().openDocument(path, flags);
    }

    void fixtureRows()
    {
        QTest::addColumn<int>("planeCount");
        QTest::newRow("small-8") << 8;
        QTest::newRow("larger-512") << 512;
    }

    App::Document* openFixture(int count)
    {
        treeWindow = std::make_unique<QWidget>();
        auto* layout = new QVBoxLayout(treeWindow.get());
        tree = std::make_unique<BehaviorTree>(treeWindow.get());
        layout->addWidget(tree.get());
        treeWindow->resize(800, 600);
        treeWindow->show();
        treeWindow->activateWindow();
        auto* doc = openNoView(fixture(count).toUtf8().constData());
        tree->expandAll();
        QCoreApplication::processEvents();
        if (!QTest::qWaitForWindowExposed(treeWindow.get())) {
            return nullptr;
        }
        // A task-owned Xvfb need not have a window manager to activate the Qt receiver.
        treeWindow->activateWindow();
        if (!QTest::qWaitForWindowActive(treeWindow.get())) {
            return nullptr;
        }
        tree->setFocus();
        return doc;
    }

    int objectItems() const
    {
        int count = 0;
        for (QTreeWidgetItemIterator it(tree.get()); *it; ++it) {
            if ((*it)->type() == Gui::TreeWidget::ObjectType) {
                ++count;
            }
        }
        return count;
    }

    QString fixture(int count) const
    {
        return QDir(fixtureRoot).filePath(QStringLiteral("tree-%1.FCStd").arg(count));
    }

    QTreeWidgetItem* findItem(const App::DocumentObject* object) const
    {
        for (QTreeWidgetItemIterator it(tree.get()); *it; ++it) {
            if ((*it)->type() == Gui::TreeWidget::ObjectType) {
                auto* item = static_cast<Gui::DocumentObjectItem*>(*it);
                if (item->object()->getObject() == object) {
                    return item;
                }
            }
        }
        return nullptr;
    }

    QTimer* statusTimer() const
    {
        for (auto* timer : tree->findChildren<QTimer*>(QString(), Qt::FindDirectChildrenOnly)) {
            if (!timer->isSingleShot()) {
                return timer;
            }
        }
        return nullptr;
    }

    std::unique_ptr<BehaviorGuiApplication> gui;
    std::unique_ptr<Gui::MainWindow> mainWindow;
    std::unique_ptr<QWidget> treeWindow;
    std::unique_ptr<BehaviorTree> tree;
    QString fixtureRoot;
    QTemporaryDir directory;
    std::string archive;
    std::string emptyArchive;
    std::string invalidArchive;
    std::size_t changedBaseline = 0;
    std::size_t touchedBaseline = 0;
    int previousTimeout = 0;
    bool previousSyncView = false;
    bool previousRecompute = false;
};

QTEST_MAIN(TreeBehaviorTest)
#include "TreeBehavior.moc"
