// SPDX-License-Identifier: LGPL-2.1-or-later

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <QApplication>
#include <QDir>
#include <QEvent>
#include <QPointer>
#include <QTemporaryDir>
#include <QStringList>
#include <QTest>
#include <QTimer>
#include <QTreeWidgetItemIterator>
#include <QTreeWidget>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/ComboView.h>
#include <Gui/DockWindowManager.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <src/App/InitApplication.h>

class testTreeWidgetLifecycle final: public QObject
{
    Q_OBJECT

public:
    explicit testTreeWidgetLifecycle(Gui::MainWindow& window)
        : mainWindow(window)
    {}

private Q_SLOTS:
    void cleanup()  // NOLINT
    {
        Gui::Selection().clearSelection();
        auto& application = App::GetApplication();
        for (const auto& name : documentNames) {
            if (application.getDocument(name.c_str())) {
                application.closeDocument(name.c_str());
            }
        }
        documentNames.clear();
        removePrimaryTree();
    }

    void subscriptionLifetime()  // NOLINT
    {
        auto& application = App::GetApplication();
        auto treePreferences = application.GetUserParameter().GetGroup(
            "BaseApp/Preferences/DockWindows/TreeView"
        );
        const auto startBaseline = application.signalStartOpenDocument.num_slots();
        const auto finishBaseline = application.signalFinishOpenDocument.num_slots();

        for (int cycle = 0; cycle < 3; ++cycle) {
            treePreferences->SetBool("Enabled", true);
            mainWindow.initDockWindows(false);
            auto* dock = Gui::DockWindowManager::instance()->findRegisteredDockWindow("Std_TreeView");
            QVERIFY(dock);

            QPointer<QTreeWidget> receiver;
            int receiverCount = 0;
            for (auto* candidate : dock->findChildren<QTreeWidget*>()) {
                if (qstrcmp(candidate->metaObject()->className(), "Gui::TreeWidget") == 0) {
                    receiver = candidate;
                    ++receiverCount;
                }
            }
            QCOMPARE(receiverCount, 1);

            const auto liveStart = application.signalStartOpenDocument.num_slots();
            const auto liveFinish = application.signalFinishOpenDocument.num_slots();
            QCOMPARE(liveStart, startBaseline + 1);
            QCOMPARE(liveFinish, finishBaseline + 1);

            treePreferences->SetBool("Enabled", false);
            mainWindow.initDockWindows(false);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            QVERIFY(receiver.isNull());

            // Measure both before asserting. On the unfixed source these counts
            // fail without ever invoking either callback on the dead receiver.
            const auto destroyedStart = application.signalStartOpenDocument.num_slots();
            const auto destroyedFinish = application.signalFinishOpenDocument.num_slots();
            const auto detail
                = QStringLiteral("Cycle %1: start subscribers %2 -> %3; finish subscribers %4 -> %5")
                      .arg(cycle)
                      .arg(qulonglong(startBaseline))
                      .arg(qulonglong(destroyedStart))
                      .arg(qulonglong(finishBaseline))
                      .arg(qulonglong(destroyedFinish));
            QVERIFY2(
                destroyedStart == startBaseline && destroyedFinish == finishBaseline,
                qPrintable(detail)
            );
        }
    }

    void liveRestoreCompletion()  // NOLINT
    {
        auto& application = App::GetApplication();
        QTemporaryDir storage;
        QVERIFY(storage.isValid());
        const auto path = storage.filePath(QStringLiteral("restore.FCStd")).toUtf8();
        const auto label = QStringLiteral("Spec010 restored object");
        auto* source = newDocument("TreeLifetimeRestore");
        auto* sourceObject = source->addObject("App::FeatureTest", "Feature");
        QVERIFY(sourceObject);
        sourceObject->Label.setValue(label.toUtf8().constData());
        QVERIFY(source->saveAs(path.constData()));
        closeDocument(*source);

        const auto startBaseline = application.signalStartOpenDocument.num_slots();
        const auto finishBaseline = application.signalFinishOpenDocument.num_slots();
        auto* dock = createPrimaryTree();
        QVERIFY(dock);
        QPointer<QTreeWidget> receiver = modelTree(dock);
        QVERIFY(receiver);

        // Hiding preserves a live receiver and its application subscriptions.
        dock->show();
        QVERIFY(!dock->isHidden());
        dock->hide();
        QVERIFY(dock->isHidden());
        QCOMPARE(application.signalStartOpenDocument.num_slots(), startBaseline + 1);
        QCOMPARE(application.signalFinishOpenDocument.num_slots(), finishBaseline + 1);
        dock->show();

        int finishes = 0;
        bool restoringAtFinish = true;
        fastsignals::scoped_connection finished(application.signalFinishOpenDocument.connect([&] {
            ++finishes;
            restoringAtFinish = application.isRestoring();
        }));
        auto* restored = application.openDocument(path.constData(), noViewFlags());
        QVERIFY(restored);
        documentNames.emplace_back(restored->getName());
        QCOMPARE(finishes, 1);
        QVERIFY(!restoringAtFinish);
        QVERIFY(!application.isRestoring());
        // This must be populated by finish itself, before an unrelated timer runs.
        QVERIFY(itemWithLabel(*receiver, label));
        finished.disconnect();
        closeDocument(*restored);

        removePrimaryTree();
        QVERIFY(receiver.isNull());
        const auto startAfter = application.signalStartOpenDocument.num_slots();
        const auto finishAfter = application.signalFinishOpenDocument.num_slots();
        QVERIFY(startAfter == startBaseline && finishAfter == finishBaseline);
        application.signalStartOpenDocument();
        application.signalFinishOpenDocument();
        restored = application.openDocument(path.constData(), noViewFlags());
        QVERIFY(restored);
        documentNames.emplace_back(restored->getName());
    }

    void finishOnUnwind()  // NOLINT
    {
        auto& application = App::GetApplication();
        auto* dock = createPrimaryTree();
        QVERIFY(dock);
        QPointer<QTreeWidget> receiver = modelTree(dock);
        QVERIFY(receiver);
        QTemporaryDir storage;
        QVERIFY(storage.isValid());
        const auto label = QStringLiteral("Spec010 pending on unwind");
        bool callbackEntered = false;
        bool restoringInsideCallback = false;
        bool documentItemInsideCallback = false;
        bool objectAbsentInsideCallback = false;
        fastsignals::scoped_connection started(application.signalStartOpenDocument.connect([&] {
            callbackEntered = true;
            restoringInsideCallback = application.isRestoring();
            auto* pending = newDocument("TreeLifetimeUnwind");
            auto* object = pending->addObject("App::FeatureTest", "Pending");
            if (object) {
                object->Label.setValue(label.toUtf8().constData());
            }
            documentItemInsideCallback
                = itemWithLabel(*receiver, QString::fromUtf8(pending->Label.getValue())) != nullptr;
            objectAbsentInsideCallback = object && !itemWithLabel(*receiver, label);
            throw std::runtime_error("Spec010 intentional open unwind");
        }));

        bool caughtExpected = false;
        try {
            application.openDocument(
                storage.filePath(QStringLiteral("not-reached.FCStd")).toUtf8().constData(),
                noViewFlags()
            );
        }
        catch (const std::runtime_error& error) {
            caughtExpected = std::string(error.what()) == "Spec010 intentional open unwind";
        }
        started.disconnect();
        QVERIFY(caughtExpected);
        QVERIFY(callbackEntered);
        QVERIFY(restoringInsideCallback);
        QVERIFY(documentItemInsideCallback);
        QVERIFY(objectAbsentInsideCallback);
        QVERIFY(!application.isRestoring());
        // No processEvents here: the actual App guard must finish pending work.
        QVERIFY(itemWithLabel(*receiver, label));
    }

    void survivingTreeAndPendingDeletion()  // NOLINT
    {
        auto& application = App::GetApplication();
        QTemporaryDir storage;
        QVERIFY(storage.isValid());
        const auto path = storage.filePath(QStringLiteral("survivor.FCStd")).toUtf8();
        const auto restoredLabel = QStringLiteral("Spec010 survivor restore");
        auto* saved = newDocument("TreeLifetimeSaved");
        auto* savedObject = saved->addObject("App::FeatureTest", "SavedFeature");
        QVERIFY(savedObject);
        savedObject->Label.setValue(restoredLabel.toUtf8().constData());
        QVERIFY(saved->saveAs(path.constData()));
        closeDocument(*saved);

        const auto startBaseline = application.signalStartOpenDocument.num_slots();
        const auto finishBaseline = application.signalFinishOpenDocument.num_slots();
        auto* dock = createPrimaryTree();
        QVERIFY(dock);
        QPointer<QTreeWidget> removedReceiver = modelTree(dock);
        QVERIFY(removedReceiver);
        auto combo = std::make_unique<Gui::DockWnd::ComboView>(nullptr, &mainWindow);
        QPointer<QTreeWidget> survivor = modelTree(combo.get());
        QVERIFY(survivor);
        // ComboView also owns a PropertyView: stabilize after its observers attach.
        const auto bothStart = application.signalStartOpenDocument.num_slots();
        const auto bothFinish = application.signalFinishOpenDocument.num_slots();
        QVERIFY(bothStart >= startBaseline + 2);
        QVERIFY(bothFinish >= finishBaseline + 2);

        auto* document = newDocument("TreeLifetimeSurvivor");
        auto* object = document->addObject("App::FeatureTest", "SurvivingFeature");
        QVERIFY(object);
        const auto cleanLabel = QStringLiteral("Spec010 surviving clean object");
        object->purgeTouched();
        object->Label.setValue(cleanLabel.toUtf8().constData());
        QTRY_VERIFY_WITH_TIMEOUT(itemWithLabel(*removedReceiver, cleanLabel), 2000);
        QTRY_VERIFY_WITH_TIMEOUT(itemWithLabel(*survivor, cleanLabel), 2000);
        auto* survivorItem = itemWithLabel(*survivor, cleanLabel);
        const auto cleanIcon = survivorItem->icon(0).cacheKey();
        object->touch();
        QTRY_VERIFY_WITH_TIMEOUT(survivorItem->icon(0).cacheKey() != cleanIcon, 2000);
        QVERIFY(Gui::Selection().addSelection(document->getName(), object->getNameInDocument()));
        QTRY_VERIFY_WITH_TIMEOUT(survivorItem->isSelected(), 2000);

        const auto pendingLabel = QStringLiteral("Spec010 pending before deletion");
        object->Label.setValue(pendingLabel.toUtf8().constData());
        Gui::Selection().clearSelection(document->getName());
        std::vector<QPointer<QTimer>> timers;
        bool timerActive = false;
        for (auto* timer : removedReceiver->findChildren<QTimer*>()) {
            timerActive = timerActive || timer->isActive();
            timers.emplace_back(timer);
        }
        QVERIFY(timerActive);
        QVERIFY(!timers.empty());
        removePrimaryTree();
        QVERIFY(removedReceiver.isNull());
        for (const auto& timer : timers) {
            QVERIFY(timer.isNull());
        }
        QVERIFY(survivor);
        const auto oneStart = application.signalStartOpenDocument.num_slots();
        const auto oneFinish = application.signalFinishOpenDocument.num_slots();
        QVERIFY(oneStart == bothStart - 1 && oneFinish == bothFinish - 1);

        application.signalStartOpenDocument();
        application.signalFinishOpenDocument();
        QTRY_VERIFY_WITH_TIMEOUT(itemWithLabel(*survivor, pendingLabel), 2000);
        auto* restored = application.openDocument(path.constData(), noViewFlags());
        QVERIFY(restored);
        documentNames.emplace_back(restored->getName());
        QVERIFY(itemWithLabel(*survivor, restoredLabel));
        combo.reset();
        QVERIFY(survivor.isNull());
        const auto noneStart = application.signalStartOpenDocument.num_slots();
        const auto noneFinish = application.signalFinishOpenDocument.num_slots();
        QVERIFY(noneStart == startBaseline && noneFinish == finishBaseline);
        application.signalStartOpenDocument();
        application.signalFinishOpenDocument();
    }

private:
    static void closeDocument(const App::Document& document)
    {
        const std::string name = document.getName();
        App::GetApplication().closeDocument(name.c_str());
    }

    static App::DocumentInitFlags noViewFlags()
    {
        App::DocumentInitFlags flags;
        flags.createView = false;
        return flags;
    }

    App::Document* newDocument(const char* prefix)
    {
        auto& application = App::GetApplication();
        const auto name = application.getUniqueDocumentName(prefix);
        auto* document = application.newDocument(name.c_str(), "testUser", noViewFlags());
        documentNames.push_back(name);
        return document;
    }

    static QTreeWidget* modelTree(QWidget* widget)
    {
        QTreeWidget* receiver = nullptr;
        for (auto* candidate : widget->findChildren<QTreeWidget*>()) {
            if (qstrcmp(candidate->metaObject()->className(), "Gui::TreeWidget") == 0) {
                if (receiver) {
                    return nullptr;
                }
                receiver = candidate;
            }
        }
        return receiver;
    }

    static QTreeWidgetItem* itemWithLabel(QTreeWidget& tree, const QString& label)
    {
        for (QTreeWidgetItemIterator item(&tree); *item; ++item) {
            if ((*item)->text(0) == label) {
                return *item;
            }
        }
        return nullptr;
    }

    QWidget* createPrimaryTree()
    {
        App::GetApplication()
            .GetUserParameter()
            .GetGroup("BaseApp/Preferences/DockWindows/TreeView")
            ->SetBool("Enabled", true);
        mainWindow.initDockWindows(false);
        return Gui::DockWindowManager::instance()->findRegisteredDockWindow("Std_TreeView");
    }

    void removePrimaryTree()
    {
        App::GetApplication()
            .GetUserParameter()
            .GetGroup("BaseApp/Preferences/DockWindows/TreeView")
            ->SetBool("Enabled", false);
        mainWindow.initDockWindows(false);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    Gui::MainWindow& mainWindow;
    std::vector<std::string> documentNames;
};

int main(int argc, char** argv)
{
    QApplication qtApplication(argc, argv);
    QTemporaryDir userHome;
    if (!userHome.isValid()) {
        return EXIT_FAILURE;
    }
    const auto dataPath = userHome.filePath(QStringLiteral("data"));
    const auto tempPath = userHome.filePath(QStringLiteral("temp"));
    if (!QDir().mkpath(dataPath) || !QDir().mkpath(tempPath)) {
        return EXIT_FAILURE;
    }
    qputenv("FREECAD_USER_HOME", userHome.path().toUtf8());
    qputenv("FREECAD_USER_DATA", dataPath.toUtf8());
    qputenv("FREECAD_USER_TEMP", tempPath.toUtf8());
    tests::initApplication();

    auto dockPreferences = App::GetApplication().GetUserParameter().GetGroup(
        "BaseApp/Preferences/DockWindows"
    );
    for (const char* name : {"TreeView", "ComboView", "PropertyView", "TaskWatcher", "DAGView"}) {
        dockPreferences->GetGroup(name)->SetBool("Enabled", false);
    }
    Gui::Application::initApplication();
    Gui::Application::initOpenInventor();
    auto guiApplication = std::make_unique<Gui::Application>(true);
    auto mainWindow = std::make_unique<Gui::MainWindow>();
    mainWindow->initDockWindows(false);

    Gui::Selection().disableCommandLog();
    auto arguments = qtApplication.arguments();
    int selectedFunctions = 0;
    for (const auto& argument : arguments) {
        if (argument == QStringLiteral("subscriptionLifetime")
            || argument == QStringLiteral("liveRestoreCompletion")
            || argument == QStringLiteral("finishOnUnwind")
            || argument == QStringLiteral("survivingTreeAndPendingDeletion")) {
            ++selectedFunctions;
        }
    }
    if (selectedFunctions == 0) {
        arguments.append(QStringLiteral("subscriptionLifetime"));
    }
    // Each CTest entry selects one function in a fresh process. Never continue
    // into an open-emitting test after a failed subscription assertion.
    if (selectedFunctions > 1) {
        std::fprintf(stderr, "Select only one TreeWidgetLifecycle function per process.\n");
        userHome.remove();
        std::fflush(nullptr);
        std::_Exit(EXIT_FAILURE);
    }
    testTreeWidgetLifecycle tests(*mainWindow);
    int result = QTest::qExec(&tests, arguments);
    if (result != 0) {
        // A failing count gate can leave stale callbacks on an unfixed build.
        // Remove the private profile, then avoid shutdown-time signal emission.
        userHome.remove();
        std::fflush(nullptr);
        std::_Exit(result);
    }
    mainWindow.reset();
    guiApplication.reset();
    if (!userHome.remove()) {
        result = EXIT_FAILURE;
    }
    return result;
}

#include "TreeWidgetLifecycle.moc"
