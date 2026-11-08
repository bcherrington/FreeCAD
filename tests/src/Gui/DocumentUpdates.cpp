// SPDX-License-Identifier: LGPL-2.1-or-later

#include <memory>

#include <QCoreApplication>
#include <QStringList>
#include <QTest>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <src/App/InitApplication.h>

namespace
{
class RecordingProvider: public Gui::ViewProviderDocumentObject
{
public:
    void updateData(const App::Property* property) override
    {
        if (!property) {
            ++nullUpdates;
            return;
        }
        updates.append(QString::fromUtf8(property->getName()));
        if (auto* value = freecad_cast<const App::PropertyInteger*>(property)) {
            values.append(value->getValue());
        }
    }

    int nullUpdates = 0;
    QStringList updates;
    QList<int> values;
};
}  // namespace

class DocumentUpdates: public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        tests::initApplication();
        Gui::Application::initOpenInventor();
        Gui::Application::initApplication();
        gui = std::make_unique<Gui::Application>(true);
        mainWindow = std::make_unique<Gui::MainWindow>();
    }

    void init()
    {
        App::DocumentInitFlags flags;
        flags.createView = false;
        document = App::GetApplication().newDocument("QueuedUpdates", "testUser", flags);
        QVERIFY(document);
        object = document->addObject("App::DocumentObject", "Object");
        QVERIFY(object);
        stale = freecad_cast<App::PropertyInteger*>(
            object->addDynamicProperty("App::PropertyInteger", "Stale")
        );
        value = freecad_cast<App::PropertyInteger*>(
            object->addDynamicProperty("App::PropertyInteger", "Value")
        );
        QVERIFY(stale);
        QVERIFY(value);
        auto* guiDocument = gui->getDocument(document);
        QVERIFY(guiDocument);
        QVERIFY(!guiDocument->getViewProvider(object));
        provider = new RecordingProvider;
        provider->attach(object);
        guiDocument->addViewProvider(provider);
        provider->updates.clear();
        provider->values.clear();
    }

    void cleanup()
    {
        if (document) {
            document->setStatus(App::Document::Recomputing, false);
            App::GetApplication().closeDocument(document);
            document = nullptr;
        }
        QCoreApplication::processEvents();
    }

    void cleanupTestCase()
    {
        mainWindow.reset();
        gui.reset();
    }

    void disappearedProperty_data()
    {
        QTest::addColumn<bool>("rename");
        QTest::newRow("removed") << false;
        QTest::newRow("renamed") << true;
    }

    void disappearedProperty()
    {
        QFETCH(bool, rename);
        document->setStatus(App::Document::Recomputing, true);
        stale->setValue(1);
        if (rename) {
            QVERIFY(object->renameDynamicProperty(stale, "Renamed"));
        }
        else {
            QVERIFY(object->removeDynamicProperty("Stale"));
        }
        value->setValue(1);
        value->setValue(2);
        QVERIFY(provider->updates.isEmpty());

        // Progress processing can deliver the timer before recompute finishes.
        QCoreApplication::processEvents();
        QCOMPARE(provider->nullUpdates, 0);
        QCOMPARE(provider->updates, QStringList {QStringLiteral("Value")});
        QCOMPARE(provider->values, QList<int> {2});

        // The queue must still deliver subsequent changes normally.
        value->setValue(3);
        QCoreApplication::processEvents();
        QCOMPARE(provider->updates.count(QStringLiteral("Value")), 2);
        QCOMPARE(provider->values, (QList<int> {2, 3}));
    }

    void closedDocumentDropsQueuedUpdates()
    {
        document->setStatus(App::Document::Recomputing, true);
        value->setValue(1);
        QVERIFY(provider->updates.isEmpty());
        document->setStatus(App::Document::Recomputing, false);
        App::GetApplication().closeDocument(document);
        document = nullptr;
        QCoreApplication::processEvents();
        QVERIFY(App::GetApplication().getDocument("QueuedUpdates") == nullptr);
    }

private:
    std::unique_ptr<Gui::Application> gui;
    std::unique_ptr<Gui::MainWindow> mainWindow;
    App::Document* document = nullptr;
    App::DocumentObject* object = nullptr;
    App::PropertyInteger* stale = nullptr;
    App::PropertyInteger* value = nullptr;
    RecordingProvider* provider = nullptr;
};

QTEST_MAIN(DocumentUpdates)
#include "DocumentUpdates.moc"
