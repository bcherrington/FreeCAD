// SPDX-License-Identifier: LGPL-2.1-or-later

#include <algorithm>
#include <atomic>
#include <functional>
#include <utility>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QThread>
#include <App/FeatureTest.h>
#include <Gui/Document.h>
#include <Gui/TreeParams.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Inventor/nodes/SoSeparator.h>

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

// Native callback fixtures; each lifecycle selection runs in a private process.
namespace Spec012
{
struct Update
{
    std::string property;
    std::string value;
    const void* propertyAddress {};
    const void* objectAddress {};
    long objectId {};
    bool recomputing {};
};
struct GuiPropertyTrace
{
    std::atomic<unsigned> copies {0};
    std::atomic<unsigned> savedCopiesDestroyed {0};
    std::atomic<unsigned> originalDestroyed {0};
    std::atomic<unsigned> savedCollectsAfterOwnerDestroyed {0};
};
struct ProviderTrace
{
    unsigned generation {};
    unsigned claims {};
    unsigned claims3D {};
    unsigned destroyed {};
    std::vector<Update> updates;
    std::vector<long> lastChildren;
};
}  // namespace Spec012
namespace App
{
class Spec012GuiTransactionLink: public PropertyLink
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    std::shared_ptr<Spec012::GuiPropertyTrace> trace = std::make_shared<Spec012::GuiPropertyTrace>();
    bool savedCopy = false;
    std::shared_ptr<Spec012::ProviderTrace> ownerTrace;
    void collectAdmissionTargets(std::vector<DocumentObject*>& targets) const override
    {
        PropertyLink::collectAdmissionTargets(targets);
        if (savedCopy && ownerTrace && ownerTrace->destroyed) {
            ++trace->savedCollectsAfterOwnerDestroyed;
        }
    }
    ~Spec012GuiTransactionLink() override
    {
        if (savedCopy) {
            ++trace->savedCopiesDestroyed;
        }
        else {
            ++trace->originalDestroyed;
        }
    }
    Property* Copy() const override
    {
        ++trace->copies;
        auto copy = std::make_unique<Spec012GuiTransactionLink>();
        copy->trace = trace;
        copy->savedCopy = true;
        copy->ownerTrace = ownerTrace;
        copy->setValue(getValue());
        return copy.release();
    }
};

class Spec012DrainFeature: public FeatureTest
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::Spec012DrainFeature);

public:
    std::function<void()> execution;
    mutable std::function<DocumentObject*(Base::Matrix4D*)> onSubObject;
    DocumentObject* getSubObject(
        const char* subname,
        PyObject** pyObject = nullptr,
        Base::Matrix4D* matrix = nullptr,
        bool transform = true,
        int depth = 0
    ) const override
    {
        auto callback = std::exchange(onSubObject, {});
        if (callback) {
            // The callback owns its complete result. Do not inspect this after
            // it may have deleted the model object or closed its document.
            return callback(matrix);
        }
        return FeatureTest::getSubObject(subname, pyObject, matrix, transform, depth);
    }
    DocumentObjectExecReturn* execute() override
    {
        auto callback = execution;
        if (callback) {
            callback();
        }
        return DocumentObject::StdReturn;
    }
};
}  // namespace App
TYPESYSTEM_SOURCE(App::Spec012GuiTransactionLink, App::PropertyLink)
PROPERTY_SOURCE(App::Spec012DrainFeature, App::FeatureTest)
namespace Gui
{
class Spec012DrainProvider: public ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::Spec012DrainProvider);

public:
    Spec012DrainProvider()
    {
        static unsigned nextGeneration = 0;
        trace->generation = ++nextGeneration;
    }
    ~Spec012DrainProvider() override
    {
        ++trace->destroyed;
    }
    bool allowOverride(const App::DocumentObject&) const override
    {
        return true;
    }
    std::shared_ptr<Spec012::ProviderTrace> trace = std::make_shared<Spec012::ProviderTrace>();
    mutable std::function<void()> onClaim;
    mutable std::function<void()> onClaim3D;
    std::function<void(const App::Property*)> onUpdate;
    bool children3DEnabled = false;
    Gui::SoRefPtr<SoSeparator> fixtureChildRoot {new SoSeparator};
    SoGroup* getChildRoot() const override
    {
        return children3DEnabled ? fixtureChildRoot.get() : nullptr;
    }
    std::vector<App::DocumentObject*> claimChildren3D() const override
    {
        auto state = trace;
        ++state->claims3D;
        auto callback = std::exchange(onClaim3D, {});
        if (callback) {
            callback();
        }
        // No provider/model/node access after a destructive native callback.
        return {};
    }
    std::vector<App::DocumentObject*> claimChildren() const override
    {
        ++trace->claims;
        // Consume before entering the callback: source recursion cannot fire it twice.
        auto callback = std::exchange(onClaim, {});
        if (callback) {
            callback();
        }
        const auto* feature = dynamic_cast<const App::FeatureTest*>(getObject());
        auto children = feature ? feature->SourceN.getValues() : std::vector<App::DocumentObject*>();
        trace->lastChildren.clear();
        for (auto* child : children) {
            trace->lastChildren.push_back(child->getID());
        }
        return children;
    }
    void updateData(const App::Property* property) override
    {
        const auto* object = getObject();
        const char* propertyName = property ? property->getName() : nullptr;
        Spec012::Update update;
        update.property = propertyName ? propertyName : "<null>";
        update.propertyAddress = property;
        update.objectAddress = object;
        update.objectId = object ? object->getID() : 0;
        update.recomputing = object && object->getDocument()->testStatus(App::Document::Recomputing);
        if (const auto* integer = dynamic_cast<const App::PropertyInteger*>(property)) {
            update.value = std::to_string(integer->getValue());
        }
        if (const auto* string = dynamic_cast<const App::PropertyString*>(property)) {
            update.value = string->getValue();
        }
        trace->updates.push_back(std::move(update));
        // Copy before invoking: destructive probes may destroy this provider.
        auto callback = onUpdate;
        if (callback) {
            callback(property);
        }
        // Deliberately no member access after native callback. Production
        // ViewProvider::update / Gui::Document may still access the provider.
    }
};
}  // namespace Gui
PROPERTY_SOURCE(Gui::Spec012DrainProvider, Gui::ViewProviderDocumentObject)

class testTreeWidgetLifecycle final: public QObject
{
    Q_OBJECT

public:
    explicit testTreeWidgetLifecycle(Gui::MainWindow& window)
        : mainWindow(window)
    {}

private Q_SLOTS:
    void initTestCase()  // NOLINT
    {
        App::Spec012GuiTransactionLink::init();
        App::Spec012DrainFeature::init();
        Gui::Spec012DrainProvider::init();
    }

    void cleanup()  // NOLINT
    {
        Gui::Selection().clearSelection();
        auto& application = App::GetApplication();
        for (const auto& name : documentNames) {
            if (auto* document = application.getDocument(name.c_str())) {
                spec012Disarm(*document);
                application.closeDocument(name.c_str());
            }
        }
        documentNames.clear();
        removePrimaryTree();
    }

    void spec012TreeUnarmedControl()  // NOLINT
    {
        spec012TreeClaimMutation(false);
    }

    void spec012TreeClaimMutation()  // NOLINT
    {
        spec012TreeClaimMutation(true);
    }

    void spec012DeferredControl()  // NOLINT
    {
        auto* dock = createPrimaryTree();
        QPointer<QTreeWidget> tree = modelTree(dock);
        QVERIFY(tree);
        auto* document = newDocument("Spec012Defer");
        auto* object = spec012Feature(*document, "Parent");
        QVERIFY(object);
        auto* provider = spec012Provider(*object);
        QVERIFY(provider);
        spec012PassiveDrain(100);
        provider->trace->updates.clear();
        bool realRecomputing = false;
        bool noSynchronousUpdate = false;
        object->execution = [&] {
            realRecomputing = document->testStatus(App::Document::Recomputing);
            object->Integer.setValue(11);
            object->Integer.setValue(22);
            object->String.setValue("Spec012 final display");
            object->Label.setValue("Spec012 final displayed label");
            noSynchronousUpdate = provider->trace->updates.empty();
        };
        object->touch();
        QVERIFY(document->recompute());
        QVERIFY(realRecomputing);
        QVERIFY(noSynchronousUpdate);
        spec012PassiveDrain(250);
        QVERIFY(spec012LastValue(*provider->trace, "Integer") == "22");
        QVERIFY(spec012LastValue(*provider->trace, "String") == "Spec012 final display");
        QVERIFY(itemWithLabel(*tree, QStringLiteral("Spec012 final displayed label")));
        QCOMPARE(spec012Updates(*provider->trace, "Integer"), 1);
        const auto before = provider->trace->updates.size();
        spec012PassiveDrain(100);
        QCOMPARE(provider->trace->updates.size(), before);
        QVERIFY(tree);
    }

    void spec012DeferredMutation_data()  // NOLINT
    {
        QTest::addColumn<bool>("nestedEventLoop");
        QTest::newRow("post-recompute-immediate-convergence") << false;
        QTest::newRow("native-execute-nested-loop-real-requeue") << true;
    }

    void spec012DeferredMutation()  // NOLINT
    {
        QFETCH(bool, nestedEventLoop);
        auto* document = newDocument("Spec012MapMutation");
        auto* object = spec012Feature(*document, "Parent");
        QVERIFY(object);
        auto* provider = spec012Provider(*object);
        QVERIFY(provider);
        spec012PassiveDrain(100);
        provider->trace->updates.clear();
        int mutations = 0;
        bool enteredWhileRecomputing = false;
        bool callbackMutationDeferred = false;
        provider->onUpdate = [&](const App::Property* property) {
            if (mutations != 0 || property != &object->Integer) {
                return;
            }
            ++mutations;
            enteredWhileRecomputing = document->testStatus(App::Document::Recomputing);
            const auto before = provider->trace->updates.size();
            object->Integer.setValue(33);  // Same map key.
            object->String.setValue("new queued key");
            callbackMutationDeferred = provider->trace->updates.size() == before;
        };
        object->execution = [&] {
            object->Integer.setValue(11);
            if (nestedEventLoop) {
                // A real native feature entering a Qt event loop is the entry
                // port here. No synthetic Recomputing flag or direct flush.
                spec012PassiveDrain(30);
            }
        };
        object->touch();
        QVERIFY(document->recompute());
        spec012PassiveDrain(150);
        QCOMPARE(mutations, 1);
        QCOMPARE(enteredWhileRecomputing, nestedEventLoop);
        QCOMPARE(callbackMutationDeferred, nestedEventLoop);
        QVERIFY(spec012LastValue(*provider->trace, "Integer") == "33");
        QVERIFY(spec012LastValue(*provider->trace, "String") == "new queued key");
        QVERIFY(spec012Updates(*provider->trace, "Integer") >= 2);
        // This is callback/final-state evidence. Private map movement itself
        // requires debugger/source instrumentation and is not asserted here.
    }

    void spec012PropertyGeneration_data()  // NOLINT
    {
        QTest::addColumn<bool>("replaceProperty");
        QTest::newRow("unarmed-property-control") << false;
        QTest::newRow("remove-readd-later-property") << true;
    }

    void spec012PropertyGeneration()  // NOLINT
    {
        QFETCH(bool, replaceProperty);
        auto* document = newDocument("Spec012PropertyGeneration");
        auto* object = spec012Feature(*document, "Parent");
        QVERIFY(object);
        auto* provider = spec012Provider(*object);
        QVERIFY(provider);
        auto* original = dynamic_cast<App::PropertyInteger*>(
            object->addDynamicProperty("App::PropertyInteger", "ZLifecycle")
        );
        QVERIFY(original);
        spec012PassiveDrain(100);
        provider->trace->updates.clear();
        int mutations = 0;
        bool removed = false;
        App::PropertyInteger* replacement = nullptr;
        provider->onUpdate = [&](const App::Property* property) {
            if (mutations != 0 || property != &object->Integer) {
                return;
            }
            ++mutations;
            if (!replaceProperty) {
                return;
            }
            removed = object->removeDynamicProperty("ZLifecycle");
            replacement = dynamic_cast<App::PropertyInteger*>(
                object->addDynamicProperty("App::PropertyInteger", "ZLifecycle")
            );
            if (replacement) {
                replacement->setValue(99);
            }
        };
        object->execution = [&] {
            object->Integer.setValue(11);
            original->setValue(41);
        };
        object->touch();
        QVERIFY(document->recompute());
        spec012PassiveDrain(150);
        QCOMPARE(mutations, 1);
        QCOMPARE(removed, replaceProperty);
        if (replaceProperty) {
            QVERIFY(replacement);
            QCOMPARE(replacement->getValue(), 99L);
        }
        QVERIFY(spec012LastValue(*provider->trace, "ZLifecycle") == (replaceProperty ? "99" : "41"));
        // Allocators may reuse an address: lifecycle events are generation
        // evidence; pointer inequality is intentionally not the oracle.
    }

    void spec012ObjectGeneration_data()  // NOLINT
    {
        QTest::addColumn<bool>("replaceObject");
        QTest::newRow("unarmed-object-control") << false;
        QTest::newRow("replace-later-object-name") << true;
    }

    void spec012ObjectGeneration()  // NOLINT
    {
        QFETCH(bool, replaceObject);
        auto* document = newDocument("Spec012ObjectGeneration");
        auto* first = spec012Feature(*document, "AFirst");
        auto* target = spec012Feature(*document, "ZTarget");
        QVERIFY(first);
        QVERIFY(target);
        auto* firstProvider = spec012Provider(*first);
        auto oldTrace = spec012Provider(*target)->trace;
        const auto oldId = target->getID();
        spec012PassiveDrain(100);
        int mutations = 0;
        App::Spec012DrainFeature* replacement = nullptr;
        std::shared_ptr<Spec012::ProviderTrace> replacementTrace;
        firstProvider->onUpdate = [&](const App::Property* property) {
            if (mutations != 0 || property != &first->Integer) {
                return;
            }
            ++mutations;
            if (!replaceObject) {
                replacement = target;
                replacementTrace = oldTrace;
                return;
            }
            document->removeObject("ZTarget");
            replacement = spec012Feature(*document, "ZTarget");
            if (replacement) {
                replacementTrace = spec012Provider(*replacement)->trace;
                replacement->Integer.setValue(99);
            }
        };
        first->execution = [&] {
            first->Integer.setValue(11);
        };
        target->execution = [&] {
            target->Integer.setValue(41);
        };
        first->touch();
        target->touch();
        QVERIFY(document->recompute());
        spec012PassiveDrain(150);
        QCOMPARE(mutations, 1);
        QVERIFY(replacement);
        QCOMPARE(std::string(replacement->getNameInDocument()), std::string("ZTarget"));
        QCOMPARE(replacement->getID() != oldId, replaceObject);
        QVERIFY(replacementTrace);
        QCOMPARE(replacementTrace->generation != oldTrace->generation, replaceObject);
        QVERIFY(spec012LastValue(*replacementTrace, "Integer") == (replaceObject ? "99" : "41"));
        // A fresh lookup applying current state to the replacement is recorded,
        // not automatically called wrong-generation work.
    }

    void spec012DocumentNameReuse_data()  // NOLINT
    {
        QTest::addColumn<bool>("reuseDocumentName");
        QTest::newRow("own-deferred-work-control") << false;
        QTest::newRow("close-recreate-document-name") << true;
    }

    void spec012DocumentNameReuse()  // NOLINT
    {
        QFETCH(bool, reuseDocumentName);
        auto* oldDocument = newDocument("Spec012DocumentGeneration");
        const std::string name = oldDocument->getName();
        auto* oldObject = spec012Feature(*oldDocument, "Parent");
        QVERIFY(oldObject);
        auto oldTrace = spec012Provider(*oldObject)->trace;
        oldTrace->updates.clear();
        oldObject->execution = [&] {
            oldObject->Integer.setValue(41);
        };
        oldObject->touch();
        QVERIFY(oldDocument->recompute());
        QVERIFY(oldTrace->updates.empty());  // Actual deferred old work exists.
        if (reuseDocumentName) {
            closeDocument(*oldDocument);
        }
        auto* replacementDocument = reuseDocumentName
            ? App::GetApplication().newDocument(name.c_str(), "testUser", noViewFlags())
            : oldDocument;
        QVERIFY(replacementDocument);
        auto* replacement = reuseDocumentName ? spec012Feature(*replacementDocument, "Parent")
                                              : oldObject;
        QVERIFY(replacement);
        auto replacementTrace = spec012Provider(*replacement)->trace;
        replacementTrace->updates.clear();
        replacement->execution = [&] {
            replacement->Integer.setValue(99);
        };
        replacement->touch();
        QVERIFY(replacementDocument->recompute());
        QVERIFY(replacementTrace->updates.empty());
        spec012PassiveDrain(150);
        QVERIFY(spec012LastValue(*replacementTrace, "Integer") == "99");
        if (reuseDocumentName) {
            QVERIFY(oldTrace->updates.empty());
        }
        QCOMPARE(replacementTrace->generation != oldTrace->generation, reuseDocumentName);
        // Old timer dispatch versus new timer dispatch needs instrumentation;
        // new own-work convergence alone is not proof of an identity defect.
    }

    void spec012NativeDestruction_data()  // NOLINT
    {
        QTest::addColumn<int>("destructionMode");
        QTest::newRow("non-destructive-native-callback") << 0;
        QTest::newRow("delete-current-provider") << 1;
        QTest::newRow("close-current-document") << 2;
    }

    void spec012NativeDestruction()  // NOLINT
    {
        // Each data row must be invoked in a separate externally bounded
        // process. Never select this whole function alongside another case.
        QFETCH(int, destructionMode);
        auto* document = newDocument("Spec012NativeDestruction");
        const std::string documentName = document->getName();
        auto* object = spec012Feature(*document, "Parent");
        QVERIFY(object);
        auto* provider = spec012Provider(*object);
        auto trace = provider->trace;
        spec012PassiveDrain(100);
        int mutations = 0;
        provider->onUpdate = [&](const App::Property* property) {
            if (mutations != 0 || property != &object->Integer) {
                return;
            }
            ++mutations;
            std::fprintf(
                stderr,
                "SPEC012 native updateData callback entered; destructionMode=%d generation=%u\n",
                destructionMode,
                trace->generation
            );
            std::fflush(stderr);
            if (destructionMode == 2) {
                App::GetApplication().closeDocument(documentName.c_str());
            }
            else if (destructionMode == 1) {
                document->removeObject("Parent");
            }
            std::fprintf(
                stderr,
                "SPEC012 native callback deletion returned; destroyed=%u\n",
                trace->destroyed
            );
            std::fflush(stderr);
        };
        object->execution = [&] {
            object->Integer.setValue(11);
        };
        object->touch();
        QVERIFY(document->recompute());
        spec012PassiveDrain(100);
        QCOMPARE(mutations, 1);
        // A crash before this boundary belongs first to the earliest native
        // stack frame proven by debugger/sanitizer, not assumed Document code.
        std::fprintf(stderr, "SPEC012 native destructive deferred drain returned\n");
        std::fflush(stderr);
    }

    void spec012VisibleLifecycle_data()  // NOLINT
    {
        QTest::addColumn<int>("mode");
        QTest::newRow("visible-native-control") << 0;
        QTest::newRow("visible-delete-current-provider") << 1;
        QTest::newRow("visible-close-current-document") << 2;
        QTest::newRow("visible-delete-then-throw") << 3;
        QTest::newRow("visible-ordinary-exception") << 4;
    }

    void spec012VisibleLifecycle()  // NOLINT
    {
        // Select each row in its own externally bounded native process.
        QFETCH(int, mode);
        auto* document = newDocument("Spec012VisibleLifecycle");
        const std::string documentName = document->getName();
        auto* first = spec012Feature(*document, "AFirst");
        auto* later = spec012Feature(*document, "ZLater");
        QVERIFY(first && later);
        auto* provider = spec012Provider(*first);
        auto* laterProvider = spec012Provider(*later);
        QVERIFY(provider && laterProvider);
        const auto trace = provider->trace;
        const auto laterTrace = laterProvider->trace;
        provider->addDisplayMaskMode(new SoSeparator, "Spec012Visible");
        provider->setDisplayMaskMode("Spec012Visible");
        provider->ViewProvider::show();
        spec012PassiveDrain(100);
        QVERIFY(provider->ViewProvider::isShow());
        trace->updates.clear();
        laterTrace->updates.clear();
        int mutations = 0;
        bool hiddenDuringCallback = false;
        bool deletionReturned = false;
        provider->onUpdate = [&](const App::Property* property) {
            if (mutations || property != &first->Integer) {
                return;
            }
            ++mutations;
            hiddenDuringCallback = !provider->ViewProvider::isShow();
            std::fprintf(
                stderr,
                "SPEC012 visible updateData mode=%d hidden=%d generation=%u\n",
                mode,
                hiddenDuringCallback,
                trace->generation
            );
            std::fflush(stderr);
            if (mode == 2) {
                App::GetApplication().closeDocument(documentName.c_str());
            }
            else if (mode == 1 || mode == 3) {
                document->removeObject("AFirst");
            }
            deletionReturned = true;
            std::fprintf(
                stderr,
                "SPEC012 visible callback mutation returned destroyed=%u\n",
                trace->destroyed
            );
            std::fflush(stderr);
            if (mode == 3 || mode == 4) {
                throw std::runtime_error("Spec012 ordinary native update exception");
            }
        };
        first->execution = [&] {
            first->Integer.setValue(11);
        };
        later->execution = [&] {
            later->Integer.setValue(73);
        };
        first->touch();
        later->touch();
        QVERIFY(document->recompute());
        QVERIFY(trace->updates.empty());
        QVERIFY(laterTrace->updates.empty());
        spec012PassiveDrain(150);
        QCOMPARE(mutations, 1);
        QVERIFY(hiddenDuringCallback);
        QVERIFY(deletionReturned);
        QCOMPARE(trace->destroyed, (mode >= 1 && mode <= 3) ? 1U : 0U);
        if (mode == 2) {
            QVERIFY(!App::GetApplication().getDocument(documentName.c_str()));
            QVERIFY(laterTrace->updates.empty());
            QCOMPARE(laterTrace->destroyed, 1U);
        }
        else {
            QVERIFY(App::GetApplication().getDocument(documentName.c_str()));
            QVERIFY(spec012LastValue(*laterTrace, "Integer") == "73");
            QCOMPARE(spec012Updates(*laterTrace, "Integer"), 1);
            if (mode == 0) {
                QVERIFY(provider->ViewProvider::isShow());
            }
        }
        const auto before = laterTrace->updates.size();
        spec012PassiveDrain(100);
        QCOMPARE(laterTrace->updates.size(), before);
        std::fprintf(
            stderr,
            "SPEC012 visible lifecycle normal drain mode=%d laterUpdates=%zu\n",
            mode,
            before
        );
        std::fflush(stderr);
    }

    void spec012CurrentPropertyGeneration_data()  // NOLINT
    {
        QTest::addColumn<bool>("replaceProperty");
        QTest::newRow("current-property-control") << false;
        QTest::newRow("remove-readd-current-property") << true;
    }

    void spec012CurrentPropertyGeneration()  // NOLINT
    {
        QFETCH(bool, replaceProperty);
        auto* document = newDocument("Spec012CurrentProperty");
        auto* object = spec012Feature(*document, "Parent");
        QVERIFY(object);
        auto* provider = spec012Provider(*object);
        QVERIFY(provider);
        auto* original = dynamic_cast<App::PropertyInteger*>(
            object->addDynamicProperty("App::PropertyInteger", "CurrentLifecycle")
        );
        QVERIFY(original);
        spec012PassiveDrain(100);
        provider->trace->updates.clear();
        unsigned retirements = 0;
        int mutations = 0;
        bool removed = false;
        App::PropertyInteger* replacement = nullptr;
        bool replacingInsideUpdate = false;
        unsigned nestedNewPropertySignals = 0;
        unsigned outerPropertySignals = 0;
        auto* guiDocument = Gui::Application::Instance->getDocument(document);
        QVERIFY(guiDocument);
        fastsignals::scoped_connection changed(guiDocument->signalChangedObject.connect(
            [&](const Gui::ViewProviderDocumentObject& changedProvider, const App::Property& property) {
                if (&changedProvider != provider
                    || (&property != original && &property != replacement)) {
                    return;
                }
                // Address comparison is only an event key: the callback-depth
                // marker distinguishes the nested new-property update from the
                // outer retired-property tail even if the allocator reuses it.
                if (replacingInsideUpdate) {
                    ++nestedNewPropertySignals;
                }
                else {
                    ++outerPropertySignals;
                }
            }
        ));
        fastsignals::scoped_connection removal(
            App::GetApplication().signalRemoveDynamicProperty.connect([&](const App::Property& property) {
                if (&property == original) {
                    ++retirements;
                }
            })
        );
        provider->onUpdate = [&](const App::Property* property) {
            if (mutations || property != original) {
                return;
            }
            ++mutations;
            if (!replaceProperty) {
                return;
            }
            replacingInsideUpdate = true;
            removed = object->removeDynamicProperty("CurrentLifecycle");
            replacement = dynamic_cast<App::PropertyInteger*>(
                object->addDynamicProperty("App::PropertyInteger", "CurrentLifecycle")
            );
            if (replacement) {
                replacement->setValue(99);
            }
            replacingInsideUpdate = false;
            std::fprintf(
                stderr,
                "SPEC012 current property removed=%d retirements=%u replacement=%p\n",
                removed,
                retirements,
                static_cast<void*>(replacement)
            );
            std::fflush(stderr);
            // Never dereference the borrowed original property after retirement.
        };
        object->execution = [&] {
            original->setValue(41);
        };
        object->touch();
        QVERIFY(document->recompute());
        QVERIFY(provider->trace->updates.empty());
        spec012PassiveDrain(150);
        QCOMPARE(mutations, 1);
        QCOMPARE(removed, replaceProperty);
        QCOMPARE(retirements, replaceProperty ? 1U : 0U);
        auto* current = dynamic_cast<App::PropertyInteger*>(
            object->getPropertyByName("CurrentLifecycle")
        );
        QVERIFY(current);
        QCOMPARE(current->getValue(), replaceProperty ? 99L : 41L);
        QVERIFY(
            spec012LastValue(*provider->trace, "CurrentLifecycle") == (replaceProperty ? "99" : "41")
        );
        std::fprintf(
            stderr,
            "SPEC012 current property signal oracle replaced=%d nestedNew=%u outer=%u\n",
            replaceProperty,
            nestedNewPropertySignals,
            outerPropertySignals
        );
        std::fflush(stderr);
        QCOMPARE(nestedNewPropertySignals, replaceProperty ? 1U : 0U);
        QCOMPARE(outerPropertySignals, replaceProperty ? 0U : 1U);
        const auto before = provider->trace->updates.size();
        spec012PassiveDrain(100);
        QCOMPARE(provider->trace->updates.size(), before);
        removal.disconnect();
        changed.disconnect();
    }

    void spec012TreeNewObject_data()  // NOLINT
    {
        QTest::addColumn<bool>("armed");
        QTest::newRow("new-object-before-snapshot-control") << false;
        QTest::newRow("claim-creates-object-after-snapshot") << true;
    }

    void spec012TreeNewObject()  // NOLINT
    {
        QFETCH(bool, armed);
        Gui::TreeParams::setStatusTimeout(1);
        auto* dock = createPrimaryTree();
        QVERIFY(dock);
        QPointer<QTreeWidget> tree = modelTree(dock);
        QVERIFY(tree);
        auto* document = newDocument("Spec012TreeNewObject");
        auto* parent = spec012Feature(*document, "Parent");
        QVERIFY(parent);
        spec012PassiveDrain(150);
        tree->expandAll();  // Setup only; no expansion or rescue after arming.
        spec012PassiveDrain(150);
        auto* provider = spec012Provider(*parent);
        QVERIFY(provider);
        unsigned hooks = 0;
        App::Spec012DrainFeature* created = nullptr;
        std::shared_ptr<Spec012::ProviderTrace> createdTrace;
        auto create = [&] {
            created = spec012Feature(*document, "NativeNewObject");
            if (created) {
                createdTrace = spec012Provider(*created)->trace;
            }
        };
        provider->onClaim = [&] {
            ++hooks;
            if (armed) {
                create();
            }
        };
        if (!armed) {
            create();
        }
        parent->Integer.setValue(11);
        std::vector<QPointer<QTimer>> timers;
        for (auto* timer : tree->findChildren<QTimer*>(QString(), Qt::FindDirectChildrenOnly)) {
            if (!timer->isSingleShot()) {
                timers.emplace_back(timer);
            }
        }
        QCOMPARE(timers.size(), size_t(1));
        QPointer<QTimer> statusTimer = timers.front();
        QCOMPARE(statusTimer->interval(), 1);
        QVERIFY(statusTimer->isActive());
        QObject observations;
        unsigned timeouts = 0;
        QObject::connect(statusTimer.data(), &QTimer::timeout, &observations, [&] { ++timeouts; });
        spec012PassiveDrain(250);
        QCOMPARE(hooks, 1U);
        QVERIFY(created);
        QVERIFY(createdTrace);
        QCOMPARE(createdTrace->destroyed, 0U);
        QVERIFY(tree && statusTimer);
        QVERIFY(timeouts > 0);
        QVERIFY(!statusTimer->isActive());
        int displayed = 0;
        bool underDocument = false;
        const auto label = QString::fromUtf8(created->Label.getValue());
        const auto documentLabel = QString::fromUtf8(document->Label.getValue());
        for (QTreeWidgetItemIterator item(tree.data()); *item; ++item) {
            if ((*item)->text(0) == label) {
                ++displayed;
                underDocument = (*item)->parent() && (*item)->parent()->text(0) == documentLabel;
            }
        }
        std::fprintf(
            stderr,
            "SPEC012 NewObjects armed=%d hooks=%u generation=%u id=%ld items=%d "
            "timerTimeouts=%u timerActive=%d\n",
            armed,
            hooks,
            createdTrace->generation,
            created->getID(),
            displayed,
            timeouts,
            statusTimer->isActive()
        );
        std::fflush(stderr);
        QCOMPARE(displayed, 1);
        QVERIFY(underDocument);
        const auto after = timeouts;
        spec012PassiveDrain(100);
        QCOMPARE(timeouts, after);
        QVERIFY(!statusTimer->isActive());
        // The new object has no link to the claiming parent: child traversal
        // cannot incidentally rescue its NewObjects notification.
    }

    void spec012ChildClaimLifecycle_data()  // NOLINT
    {
        QTest::addColumn<int>("mode");
        QTest::newRow("child-claim-control") << 0;
        QTest::newRow("child-claim-delete-provider") << 1;
        QTest::newRow("child-claim-close-document") << 2;
    }

    void spec012ChildClaimLifecycle()  // NOLINT
    {
        QFETCH(int, mode);
        auto* document = newDocument("Spec012ChildClaim");
        const std::string name = document->getName();
        auto* object = spec012Feature(*document, "AFirst");
        auto* later = spec012Feature(*document, "ZLater");
        QVERIFY(object && later);
        auto* provider = spec012Provider(*object);
        auto* laterProvider = spec012Provider(*later);
        QVERIFY(provider && laterProvider);
        const auto trace = provider->trace;
        const auto laterTrace = laterProvider->trace;
        provider->children3DEnabled = true;
        spec012PassiveDrain(100);
        // Keep the independent inner visible-update endpoint unarmed, so this
        // selection qualifies the actual child callback continuation.
        QVERIFY(!provider->ViewProvider::isShow());
        trace->updates.clear();
        laterTrace->updates.clear();
        const auto beforeClaims = trace->claims3D;
        unsigned hooks = 0;
        bool callbackReturned = false;
        provider->onClaim3D = [&] {
            ++hooks;
            std::fprintf(
                stderr,
                "SPEC012 claimChildren3D entered mode=%d generation=%u\n",
                mode,
                trace->generation
            );
            std::fflush(stderr);
            if (mode == 1) {
                document->removeObject("AFirst");
            }
            else if (mode == 2) {
                App::GetApplication().closeDocument(name.c_str());
            }
            callbackReturned = true;
            std::fprintf(
                stderr,
                "SPEC012 claimChildren3D mutation returned destroyed=%u\n",
                trace->destroyed
            );
            std::fflush(stderr);
        };
        object->execution = [&] {
            object->Integer.setValue(11);
        };
        later->execution = [&] {
            later->Integer.setValue(73);
        };
        object->touch();
        later->touch();
        QVERIFY(document->recompute());
        QVERIFY(trace->updates.empty());
        QVERIFY(laterTrace->updates.empty());
        spec012PassiveDrain(150);
        QCOMPARE(hooks, 1U);
        QVERIFY(callbackReturned);
        QVERIFY(trace->claims3D > beforeClaims);
        QCOMPARE(trace->destroyed, mode ? 1U : 0U);
        if (mode == 2) {
            QVERIFY(!App::GetApplication().getDocument(name.c_str()));
            QVERIFY(laterTrace->updates.empty());
            QCOMPARE(laterTrace->destroyed, 1U);
        }
        else {
            QVERIFY(spec012LastValue(*laterTrace, "Integer") == "73");
            QCOMPARE(spec012Updates(*laterTrace, "Integer"), 1);
        }
        const auto after = laterTrace->updates.size();
        spec012PassiveDrain(100);
        QCOMPARE(laterTrace->updates.size(), after);
        std::fprintf(
            stderr,
            "SPEC012 claimChildren3D normal drain mode=%d laterUpdates=%zu\n",
            mode,
            after
        );
        std::fflush(stderr);
    }

    void spec012EditSubObjectLifecycle_data()  // NOLINT
    {
        QTest::addColumn<int>("mode");
        QTest::newRow("edit-subobject-control") << 0;
        QTest::newRow("edit-subobject-reset-edit") << 1;
        QTest::newRow("edit-subobject-delete-provider") << 2;
        QTest::newRow("edit-subobject-close-document") << 3;
    }

    void spec012EditSubObjectLifecycle()  // NOLINT
    {
        QFETCH(int, mode);
        auto* document = newDocument("Spec012EditSubObject");
        const std::string name = document->getName();
        auto* object = spec012Feature(*document, "AEditing");
        auto* later = spec012Feature(*document, "ZLater");
        QVERIFY(object && later);
        auto* provider = spec012Provider(*object);
        auto* laterProvider = spec012Provider(*later);
        QVERIFY(provider && laterProvider);
        const auto trace = provider->trace;
        const auto laterTrace = laterProvider->trace;
        auto* guiDocument = Gui::Application::Instance->getDocument(document);
        QVERIFY(guiDocument);
        QPointer<Gui::View3DInventor> view = dynamic_cast<Gui::View3DInventor*>(
            guiDocument->createView(Gui::View3DInventor::getClassTypeId())
        );
        QVERIFY(view);
        QPointer<Gui::View3DInventorViewer> viewer = view->getViewer();
        QVERIFY(viewer);
        QVERIFY(guiDocument->setEdit(provider, 0, ""));
        QVERIFY(guiDocument->getInEdit() == provider);
        QVERIFY(guiDocument->getEditingViewOfViewProvider(provider) == view.data());
        spec012PassiveDrain(100);
        const Base::Matrix4D beforeTransform = guiDocument->getEditingTransform();
        Base::Matrix4D proposedTransform;
        proposedTransform.move(7.0, 0.0, 0.0);
        QVERIFY(beforeTransform != proposedTransform);
        trace->updates.clear();
        laterTrace->updates.clear();
        unsigned hooks = 0;
        bool callbackReturned = false;
        object->onSubObject = [&](Base::Matrix4D* matrix) -> App::DocumentObject* {
            ++hooks;
            std::fprintf(
                stderr,
                "SPEC012 edit getSubObject entered mode=%d generation=%u matrix=%d\n",
                mode,
                trace->generation,
                matrix != nullptr
            );
            std::fflush(stderr);
            if (matrix) {
                *matrix = proposedTransform;
            }
            App::DocumentObject* result = object;
            if (mode == 1) {
                guiDocument->resetEdit();
            }
            else if (mode == 2) {
                document->removeObject("AEditing");
                result = nullptr;
            }
            else if (mode == 3) {
                App::GetApplication().closeDocument(name.c_str());
                result = nullptr;
            }
            callbackReturned = true;
            std::fprintf(
                stderr,
                "SPEC012 edit getSubObject mutation returned destroyed=%u viewerAlive=%d\n",
                trace->destroyed,
                !viewer.isNull()
            );
            std::fflush(stderr);
            return result;
        };
        object->execution = [&] {
            object->Placement.setValue(
                Base::Placement(Base::Vector3d(3.0, 0.0, 0.0), Base::Rotation())
            );
        };
        later->execution = [&] {
            later->Integer.setValue(73);
        };
        object->touch();
        later->touch();
        QVERIFY(document->recompute());
        QVERIFY(trace->updates.empty());
        QVERIFY(laterTrace->updates.empty());
        spec012PassiveDrain(150);
        QCOMPARE(hooks, 1U);
        QVERIFY(callbackReturned);
        QCOMPARE(trace->destroyed, mode >= 2 ? 1U : 0U);
        if (mode == 3) {
            QVERIFY(!App::GetApplication().getDocument(name.c_str()));
            QVERIFY(laterTrace->updates.empty());
        }
        else {
            QVERIFY(view && viewer);
            QVERIFY(spec012LastValue(*laterTrace, "Integer") == "73");
            if (mode == 0) {
                QVERIFY(guiDocument->getInEdit() == provider);
                QVERIFY(guiDocument->getEditingTransform() == proposedTransform);
            }
            else {
                QVERIFY(!guiDocument->getInEdit());
                // The callback ended edit ownership: its prospective transform
                // must not be installed on the former viewer after return.
                QVERIFY(guiDocument->getEditingTransform() == beforeTransform);
            }
        }
        std::fprintf(stderr, "SPEC012 edit getSubObject normal drain mode=%d hooks=%u\n", mode, hooks);
        std::fflush(stderr);
    }

    void spec012RetainedProviderUndo_data()  // NOLINT
    {
        QTest::addColumn<bool>("removeObject");
        QTest::addColumn<bool>("originalUser1");
        QTest::newRow("retained-undo-unarmed-control") << false << false;
        QTest::newRow("retained-undo-original-bit-false") << true << false;
        QTest::newRow("retained-undo-original-bit-true") << true << true;
    }

    void spec012RetainedProviderUndo()  // NOLINT
    {
        QFETCH(bool, removeObject);
        QFETCH(bool, originalUser1);
        auto* document = newDocument("Spec012RetainedProviderUndo");
        auto* object = spec012Feature(*document, "AFirst");
        auto* later = spec012Feature(*document, "ZLater");
        QVERIFY(object && later);
        auto* provider = spec012Provider(*object);
        auto* laterProvider = spec012Provider(*later);
        QVERIFY(provider && laterProvider);
        const auto trace = provider->trace;
        const auto laterTrace = laterProvider->trace;
        const auto generation = trace->generation;
        const auto* originalInteger = &object->Integer;
        auto* guiDocument = Gui::Application::Instance->getDocument(document);
        QVERIFY(guiDocument);
        spec012PassiveDrain(100);
        provider->Visibility.setStatus(App::Property::User1, originalUser1);
        QCOMPARE(provider->Visibility.testStatus(App::Property::User1), originalUser1);
        trace->updates.clear();
        laterTrace->updates.clear();
        unsigned callbacks = 0;
        unsigned sourceSignals = 0;
        bool temporaryUser1 = false;
        fastsignals::scoped_connection changed(guiDocument->signalChangedObject.connect(
            [&](const Gui::ViewProviderDocumentObject& changedProvider, const App::Property& property) {
                if (&changedProvider == provider && &property == originalInteger) {
                    ++sourceSignals;
                }
            }
        ));
        provider->onUpdate = [&](const App::Property* property) {
            if (callbacks || property != originalInteger) {
                return;
            }
            ++callbacks;
            temporaryUser1 = provider->Visibility.testStatus(App::Property::User1);
            if (removeObject) {
                document->removeObject("AFirst");
            }
            // Only the independently owned trace is accessed after removal.
            std::fprintf(
                stderr,
                "SPEC012 retained provider callback remove=%d originalUser1=%d "
                "temporaryUser1=%d destroyed=%u generation=%u\n",
                removeObject,
                originalUser1,
                temporaryUser1,
                trace->destroyed,
                generation
            );
            std::fflush(stderr);
        };
        document->setMaxUndoStackSize(10);
        document->openTransaction("Spec012 provider retained during callback removal");
        object->execution = [&] {
            object->Integer.setValue(11);
        };
        later->execution = [&] {
            later->Integer.setValue(73);
        };
        object->touch();
        later->touch();
        QVERIFY(document->recompute());
        QVERIFY(trace->updates.empty());
        spec012PassiveDrain(150);
        QCOMPARE(callbacks, 1U);
        QVERIFY(temporaryUser1);
        QCOMPARE(document->getObject("AFirst") == nullptr, removeObject);
        // Establish native storage survival BEFORE touching the retained raw
        // provider. Removal must retain this exact instance in undo history.
        QCOMPARE(trace->destroyed, 0U);
        const bool detachedUser1 = provider->Visibility.testStatus(App::Property::User1);
        const auto signalsBeforeUndo = sourceSignals;
        QVERIFY(spec012LastValue(*laterTrace, "Integer") == "73");
        QCOMPARE(spec012Updates(*laterTrace, "Integer"), 1);
        provider->onUpdate = {};  // Undo must not reenter the armed callback.
        document->commitTransaction();
        QVERIFY(document->getAvailableUndos() > 0);
        QVERIFY(document->undo());
        spec012PassiveDrain(100);
        auto* restoredObject = document->getObject("AFirst");
        QVERIFY(restoredObject);
        auto* restoredProvider = spec012Provider(*restoredObject);
        QVERIFY(restoredProvider);
        QCOMPARE(trace->destroyed, 0U);
        QVERIFY(restoredProvider == provider);
        QCOMPARE(restoredProvider->trace->generation, generation);
        const bool restoredUser1 = restoredProvider->Visibility.testStatus(App::Property::User1);
        std::fprintf(
            stderr,
            "SPEC012 retained provider undo remove=%d originalUser1=%d detachedUser1=%d "
            "restoredUser1=%d destroyed=%u sameProvider=%d generation=%u sourceSignals=%u\n",
            removeObject,
            originalUser1,
            detachedUser1,
            restoredUser1,
            trace->destroyed,
            restoredProvider == provider,
            restoredProvider->trace->generation,
            signalsBeforeUndo
        );
        std::fflush(stderr);
        // Explicitly clear an intentionally preset bit only AFTER both bit
        // observations. Visibility synchronization is independently meaningful.
        if (originalUser1) {
            restoredProvider->Visibility.setStatus(App::Property::User1, false);
        }
        QCOMPARE(restoredProvider->Visibility.getValue(), restoredObject->Visibility.getValue());
        const bool desiredVisibility = !restoredObject->Visibility.getValue();
        restoredProvider->Visibility.setValue(desiredVisibility);
        const bool visibilitySynced = restoredObject->Visibility.getValue() == desiredVisibility;
        std::fprintf(
            stderr,
            "SPEC012 retained provider visibility synced=%d desired=%d\n",
            visibilitySynced,
            desiredVisibility
        );
        std::fflush(stderr);
        // Assertions follow actual undo and synchronization, even on the
        // introduced stale-bit baseline. No wrong-bit gate skips that evidence.
        QCOMPARE(signalsBeforeUndo, removeObject ? 0U : 1U);
        QCOMPARE(detachedUser1, originalUser1);
        QCOMPARE(restoredUser1, originalUser1);
        QVERIFY(visibilitySynced);
    }

    void spec012GuiTransactionPropertyCleanup_data()  // NOLINT
    {
        QTest::addColumn<bool>("renameBorrowedProperty");
        QTest::newRow("owned-gui-link-saved-copy-cleanup") << false;
        QTest::newRow("borrowed-gui-property-rename-cleanup") << true;
    }

    void spec012GuiTransactionPropertyCleanup()  // NOLINT
    {
        QFETCH(bool, renameBorrowedProperty);
        auto* document = newDocument("Spec012GuiPropertyCleanup");
        const std::string documentName = document->getName();
        auto* object = spec012Feature(*document, "Owner");
        auto* oldTarget = document->addObject("App::FeatureTest", "OldTarget");
        auto* newTarget = document->addObject("App::FeatureTest", "NewTarget");
        QVERIFY(object && oldTarget && newTarget);
        auto* provider = spec012Provider(*object);
        QVERIFY(provider);
        const auto providerTrace = provider->trace;
        auto* property = dynamic_cast<App::Spec012GuiTransactionLink*>(
            provider->addDynamicProperty("App::Spec012GuiTransactionLink", "GuiLinkBeforeTransaction")
        );
        QVERIFY(property);
        const auto propertyTrace = property->trace;
        property->ownerTrace = providerTrace;
        property->setValue(oldTarget);
        spec012PassiveDrain(100);
        QVERIFY(property->getContainer() == provider);
        QVERIFY(property->getValue() == oldTarget);
        QCOMPARE(propertyTrace->copies.load(), 0U);
        QCOMPARE(propertyTrace->originalDestroyed.load(), 0U);
        document->setMaxUndoStackSize(10);
        document->openTransaction("Spec012 native GUI transaction property cleanup");
        QVERIFY(document->isTransactionEmpty());
        if (renameBorrowedProperty) {
            // GUI providers expose the native rename API inherited from
            // PropertyContainer. Its App transaction record uses the same public
            // Document helper as DocumentObject's native rename override.
            QVERIFY(provider->renameDynamicProperty(property, "GuiLinkAfterRename"));
            document->renamePropertyOfObject(provider, property, "GuiLinkBeforeTransaction");
            QVERIFY(!provider->getPropertyByName("GuiLinkBeforeTransaction"));
            QVERIFY(provider->getPropertyByName("GuiLinkAfterRename") == property);
            QVERIFY(property->getValue() == oldTarget);
            QCOMPARE(propertyTrace->copies.load(), 0U);  // nameOrig borrows the real property.
        }
        else {
            property->setValue(newTarget);
            QVERIFY(property->getValue() == newTarget);
            QCOMPARE(propertyTrace->copies.load(), 1U);  // Native onBeforeChange saved Copy().
        }
        QVERIFY(!document->isTransactionEmpty());
        QCOMPARE(propertyTrace->savedCopiesDestroyed.load(), 0U);
        QCOMPARE(propertyTrace->originalDestroyed.load(), 0U);
        document->commitTransaction();
        QCOMPARE(document->getAvailableUndos(), 1);
        QVERIFY(property->getValue() == (renameBorrowedProperty ? oldTarget : newTarget));
        std::fprintf(
            stderr,
            "SPEC012 GUI transaction before close rename=%d copies=%u savedDestroyed=%u "
            "originalDestroyed=%u providerDestroyed=%u undos=%d\n",
            renameBorrowedProperty,
            propertyTrace->copies.load(),
            propertyTrace->savedCopiesDestroyed.load(),
            propertyTrace->originalDestroyed.load(),
            providerTrace->destroyed,
            document->getAvailableUndos()
        );
        std::fflush(stderr);
        QVERIFY(App::GetApplication().closeDocument(documentName.c_str()));
        QVERIFY(!App::GetApplication().getDocument(documentName.c_str()));
        spec012PassiveDrain(150);
        // Only independent trace storage is inspected after native document and
        // provider destruction. Neither a transaction key nor a borrowed property
        // pointer is dereferenced after its owner is closed.
        std::fprintf(
            stderr,
            "SPEC012 GUI transaction after close rename=%d copies=%u savedDestroyed=%u "
            "originalDestroyed=%u providerDestroyed=%u\n",
            renameBorrowedProperty,
            propertyTrace->copies.load(),
            propertyTrace->savedCopiesDestroyed.load(),
            propertyTrace->originalDestroyed.load(),
            providerTrace->destroyed
        );
        std::fflush(stderr);
        QCOMPARE(providerTrace->destroyed, 1U);
        QCOMPARE(propertyTrace->originalDestroyed.load(), 1U);
        QCOMPARE(propertyTrace->savedCopiesDestroyed.load(), renameBorrowedProperty ? 0U : 1U);
        if (!renameBorrowedProperty) {
            std::fprintf(
                stderr,
                "SPEC012 GUI owned saved-copy collections after provider destruction=%u\n",
                propertyTrace->savedCollectsAfterOwnerDestroyed.load()
            );
            std::fflush(stderr);
            QVERIFY(propertyTrace->savedCollectsAfterOwnerDestroyed.load() > 0);
        }
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
    static void spec012PassiveDrain(int milliseconds)
    {
        QElapsedTimer elapsed;
        elapsed.start();
        while (elapsed.elapsed() < milliseconds) {
            // No qWait helper timer, model writes, paints requested by this
            // fixture, or explicit tree update. Deliver already scheduled Qt
            // work, excluding external input/socket activity.
            QCoreApplication::processEvents(
                QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers
            );
            QThread::msleep(1);
        }
    }

    static App::Spec012DrainFeature* spec012Feature(App::Document& document, const char* name)
    {
        return dynamic_cast<App::Spec012DrainFeature*>(
            document.addObject("App::Spec012DrainFeature", name, true, "Gui::Spec012DrainProvider")
        );
    }

    static Gui::Spec012DrainProvider* spec012Provider(const App::DocumentObject& object)
    {
        return dynamic_cast<Gui::Spec012DrainProvider*>(
            Gui::Application::Instance->getViewProvider(&object)
        );
    }

    static int spec012Updates(const Spec012::ProviderTrace& trace, const char* property)
    {
        return int(std::count_if(trace.updates.begin(), trace.updates.end(), [&](const auto& update) {
            return update.property == property;
        }));
    }

    static std::string spec012LastValue(const Spec012::ProviderTrace& trace, const char* property)
    {
        for (auto it = trace.updates.rbegin(); it != trace.updates.rend(); ++it) {
            if (it->property == property) {
                return it->value;
            }
        }
        return "<not observed>";
    }

    void spec012TreeClaimMutation(bool armed)
    {
        Gui::TreeParams::setStatusTimeout(1);
        auto* dock = createPrimaryTree();
        QVERIFY(dock);
        QPointer<QTreeWidget> tree = modelTree(dock);
        QVERIFY(tree);
        auto* document = newDocument("Spec012ClaimChildren");
        auto* first = spec012Feature(*document, "ParentA");
        auto* second = spec012Feature(*document, "ParentB");
        auto* childA = document->addObject("App::FeatureTest", "ChildA");
        auto* childOld = document->addObject("App::FeatureTest", "ChildOld");
        auto* childNew = document->addObject("App::FeatureTest", "ChildNew");
        QVERIFY(first && second && childA && childOld && childNew);
        first->SourceN.setValues({childA});
        second->SourceN.setValues({childOld});
        spec012PassiveDrain(150);
        tree->expandAll();  // Setup only, before arming or triggering work.
        spec012PassiveDrain(150);
        auto* secondItem = itemWithLabel(*tree, QString::fromUtf8(second->Label.getValue()));
        QVERIFY(secondItem);
        auto* oldItem = itemWithLabel(*tree, QString::fromUtf8(childOld->Label.getValue()));
        QVERIFY(oldItem);
        QVERIFY(oldItem->parent() == secondItem);
        auto* firstProvider = spec012Provider(*first);
        auto* secondProvider = spec012Provider(*second);
        QVERIFY(firstProvider && secondProvider);
        const auto beforeSecondClaims = secondProvider->trace->claims;
        unsigned hookCount = 0;
        unsigned mutations = 0;
        firstProvider->onClaim = [&] {
            ++hookCount;
            if (armed) {
                ++mutations;
                second->SourceN.setValues({childNew});
            }
        };
        if (!armed) {
            second->SourceN.setValues({childNew});
        }
        first->Integer.setValue(first->Integer.getValue() + 1);
        std::vector<QPointer<QTimer>> repeatingDirectTimers;
        for (auto* timer : tree->findChildren<QTimer*>(QString(), Qt::FindDirectChildrenOnly)) {
            // Source-qualified identity: Tree.cpp:726-745 constructs the
            // status timer as the repeating direct child. Selection,
            // preselection and visibility timers are single-shot. Activity
            // of legitimate other timers does not identify this receiver.
            if (!timer->isSingleShot()) {
                repeatingDirectTimers.emplace_back(timer);
            }
        }
        QCOMPARE(repeatingDirectTimers.size(), size_t(1));
        QPointer<QTimer> statusTimer = repeatingDirectTimers.front();
        QCOMPARE(statusTimer->interval(), 1);
        QVERIFY(statusTimer->isActive());
        QObject observations;
        unsigned deliveredTimeouts = 0;
        QObject::connect(statusTimer.data(), &QTimer::timeout, &observations, [&] {
            ++deliveredTimeouts;
        });
        spec012PassiveDrain(250);
        QVERIFY(tree);
        QVERIFY(statusTimer);
        secondItem = itemWithLabel(*tree, QString::fromUtf8(second->Label.getValue()));
        QVERIFY(secondItem);
        int finalChildren = 0;
        int staleChildren = 0;
        for (int index = 0; index < secondItem->childCount(); ++index) {
            const auto label = secondItem->child(index)->text(0);
            finalChildren += label == QString::fromUtf8(childNew->Label.getValue());
            staleChildren += label == QString::fromUtf8(childOld->Label.getValue());
        }
        std::fprintf(
            stderr,
            "SPEC012 G07/G08 armed=%d hook=%u mutations=%u timerTimeouts=%u timerActive=%d "
            "Bclaims=%u->%u finalChildren=%d staleChildren=%d nativeFinalChildId=%ld\n",
            armed,
            hookCount,
            mutations,
            deliveredTimeouts,
            statusTimer->isActive(),
            beforeSecondClaims,
            secondProvider->trace->claims,
            finalChildren,
            staleChildren,
            childNew->getID()
        );
        std::fflush(stderr);
        QCOMPARE(hookCount, 1U);
        QCOMPARE(mutations, armed ? 1U : 0U);
        QVERIFY(deliveredTimeouts > 0);
        QVERIFY(!statusTimer->isActive());
        QVERIFY(second->SourceN.getValues() == std::vector<App::DocumentObject*>({childNew}));
        // First final-state failure is intentionally actionable on the baseline.
        QCOMPARE(finalChildren, 1);
        QCOMPARE(staleChildren, 0);
        QVERIFY(secondProvider->trace->lastChildren == std::vector<long>({childNew->getID()}));
        const auto afterClaims = secondProvider->trace->claims;
        const auto afterTimeouts = deliveredTimeouts;
        spec012PassiveDrain(100);
        QCOMPARE(secondProvider->trace->claims, afterClaims);
        QCOMPARE(deliveredTimeouts, afterTimeouts);
        QVERIFY(!statusTimer->isActive());
        // Private ChangedObjects/NewObjects are not inspected. Final native
        // membership, observed provider children, displayed hierarchy, and a
        // stable inactive timer together form the observable completion oracle.
    }

    static void spec012Disarm(App::Document& document)
    {
        for (auto* object : document.getObjects()) {
            if (auto* provider = spec012Provider(*object)) {
                provider->onClaim = {};
                provider->onClaim3D = {};
                provider->onUpdate = {};
            }
            if (auto* feature = dynamic_cast<App::Spec012DrainFeature*>(object)) {
                feature->execution = {};
                feature->onSubObject = {};
            }
        }
    }

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
        if (argument.startsWith(QStringLiteral("spec012"))
            || argument == QStringLiteral("subscriptionLifetime")
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
