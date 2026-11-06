// SPDX-License-Identifier: LGPL-2.1-or-later

#include <array>
#include <memory>

#include <QCoreApplication>
#include <QEvent>
#include <QPointer>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Tree.h>
#include <src/App/InitApplication.h>

namespace
{
class TreeGuiApplication: public Gui::Application
{
public:
    TreeGuiApplication()
        : Gui::Application(false)
    {}

    using Gui::Application::slotDeleteDocument;
    using Gui::Application::slotNewDocument;
};
}  // namespace

class TreeWidgetLifetimeTest: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        tests::initApplication();
        Gui::Application::initOpenInventor();
        Gui::Application::initApplication();
        gui = std::make_unique<TreeGuiApplication>();
        QVERIFY(directory.isValid());
        auto& app = App::GetApplication();
        auto* doc = app.newDocument("TreeLifetimeFixture");
        archive = directory.filePath(QStringLiteral("tree.FCStd")).toStdString();
        QVERIFY(doc->saveAs(archive.c_str()));
        app.closeDocument(doc);
    }

    void cleanup()
    {
        for (auto* doc : App::GetApplication().getDocuments()) {
            if (gui->getDocument(doc)) {
                gui->slotDeleteDocument(*doc);
            }
            App::GetApplication().closeDocument(doc);
        }
        drainEvents();
    }

    void cleanupTestCase()
    {
        gui.reset();
    }

    void subscriptionsEndWithReceiver_data()
    {
        QTest::addColumn<bool>("deleteParent");
        QTest::newRow("direct-destruction") << false;
        QTest::newRow("parent-destruction") << true;
    }

    void subscriptionsEndWithReceiver()
    {
        QFETCH(bool, deleteParent);
        const auto baseline = subscriptionCounts();
        auto parent = std::make_unique<QWidget>();
        QPointer<Gui::TreeWidget> tree = new Gui::TreeWidget("Lifetime", parent.get());
        auto expected = baseline;
        for (auto& count : expected) {
            ++count;
        }
        QCOMPARE(subscriptionCounts(), expected);
        if (deleteParent) {
            parent.reset();
        }
        else {
            delete tree.data();
        }
        QVERIFY(tree.isNull());
        // Counts detect retained raw receivers without dereferencing freed storage.
        QCOMPARE(subscriptionCounts(), baseline);
    }

    void applicationOpenAfterDestruction()
    {
        auto& app = App::GetApplication();
        const auto baseline = subscriptionCounts();
        int starts = 0;
        int finishes = 0;
        fastsignals::scoped_connection start = app.signalStartOpenDocument.connect([&]() {
            ++starts;
        });
        fastsignals::scoped_connection finish = app.signalFinishOpenDocument.connect([&]() {
            ++finishes;
        });
        for (int i = 0; i < 3; ++i) {
            {
                auto oldTree = std::make_unique<Gui::TreeWidget>("OldTree");
            }
            // Keep the application alive and exercise the real open path with no tree.
            // Under Memcheck this also detects access through a destroyed receiver.
            auto* doc = app.openDocument(archive.c_str());
            QVERIFY(doc);
            QCOMPARE(starts, 2 * i + 1);
            QCOMPARE(finishes, 2 * i + 1);
            app.closeDocument(doc);
            {
                Gui::TreeWidget replacement("ReplacementTree");
                doc = app.openDocument(archive.c_str());
                QVERIFY(doc);
                gui->slotNewDocument(*doc, false);
                QCOMPARE(replacement.topLevelItemCount(), 1);
                QCOMPARE(replacement.topLevelItem(0)->text(0), QString::fromUtf8(doc->Label.getValue()));
                QCOMPARE(starts, 2 * i + 2);
                QCOMPARE(finishes, 2 * i + 2);
                gui->slotDeleteDocument(*doc);
                QCOMPARE(replacement.topLevelItemCount(), 0);
                app.closeDocument(doc);
            }
            drainEvents();
        }
        start.disconnect();
        finish.disconnect();
        QCOMPARE(subscriptionCounts(), baseline);
    }

    void queuedStatusWorkEndsWithReceiver()
    {
        const auto baseline = subscriptionCounts();
        auto tree = std::make_unique<Gui::TreeWidget>("QueuedTree");
        Gui::TreeWidget::updateStatus(true);
        QList<QPointer<QTimer>> timers;
        bool active = false;
        for (auto* timer : tree->findChildren<QTimer*>(QString(), Qt::FindDirectChildrenOnly)) {
            timers.push_back(timer);
            active = active || timer->isActive();
        }
        QVERIFY(active);
        QVERIFY(QMetaObject::invokeMethod(tree.get(), "onUpdateStatus", Qt::QueuedConnection));
        tree.reset();
        for (const auto& timer : timers) {
            QVERIFY(timer.isNull());
        }
        drainEvents();
        QCOMPARE(subscriptionCounts(), baseline);
        // A fresh receiver can still schedule and drain status work.
        Gui::TreeWidget replacement("QueuedReplacement");
        QVERIFY(QMetaObject::invokeMethod(&replacement, "onUpdateStatus", Qt::QueuedConnection));
        drainEvents();
    }

private:
    std::array<std::size_t, 9> subscriptionCounts() const
    {
        auto& app = App::GetApplication();
        return {
            app.signalStartOpenDocument.num_slots(),
            app.signalFinishOpenDocument.num_slots(),
            gui->signalNewDocument.num_slots(),
            gui->signalDeleteDocument.num_slots(),
            gui->signalRenameDocument.num_slots(),
            gui->signalActiveDocument.num_slots(),
            gui->signalRelabelDocument.num_slots(),
            gui->signalShowHidden.num_slots(),
            gui->signalChangedObject.num_slots(),
        };
    }

    static void drainEvents()
    {
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        QCoreApplication::processEvents();
    }

    std::unique_ptr<TreeGuiApplication> gui;
    QTemporaryDir directory;
    std::string archive;
};

QTEST_MAIN(TreeWidgetLifetimeTest)
#include "TreeWidgetLifetime.moc"
