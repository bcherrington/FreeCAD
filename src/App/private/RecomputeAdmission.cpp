// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 FreeCAD contributors
#include "RecomputeAdmission.h"

#include <algorithm>
#include <condition_variable>
#include <App/MainThreadSignal.h>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Property.h>
#include <App/PropertyLinks.h>

namespace App::RecomputeDetail
{
namespace
{
struct DocumentRecord;
struct ObjectRecord
{
    DocumentObject* pointer {};
    std::shared_ptr<DocumentRecord> document;
    Generation generation {};
    std::string name;
    bool live {true};
    bool pendingRemoval {false};
    std::unordered_map<OperationId, unsigned> pins;
};
struct DocumentRecord
{
    Document* pointer {};
    Generation generation {};
    std::string name;
    bool live {true};
    bool retiring {false};
    OperationId closeOwner {};
    OperationId writer {};
    unsigned writerDepth {};
    unsigned readers {};
    OperationId executionOwner {};
    unsigned executionDepth {};
    std::unordered_map<OperationId, unsigned> pins;
};
struct PropertyRecord
{
    Property* pointer {};
    DocumentObject* objectKey {};
    Document* documentKey {};
    Property* parentKey {};
    std::shared_ptr<DocumentRecord> document;
    std::shared_ptr<ObjectRecord> object;
    bool live {true};
    unsigned pins {};
    // Preserve original native owners across detach/move until their frames retire.
    std::vector<std::shared_ptr<ObjectRecord>> storageObjects;
    std::vector<std::shared_ptr<DocumentRecord>> storageDocuments;
};
struct ClosureRecord
{
    ClosureToken token;
    bool live {true};
    bool pending {false};
    bool closing {false};
    bool abort {false};
    std::vector<Generation> documents;
};
struct Registry
{
    std::mutex mutex;
    Generation next {1};
    OperationId nextOperation {1};
    std::unordered_map<const Document*, std::shared_ptr<DocumentRecord>> documents;
    std::unordered_map<Generation, std::shared_ptr<DocumentRecord>> documentGenerations;
    std::unordered_map<std::string, std::shared_ptr<DocumentRecord>> names;
    std::unordered_map<const DocumentObject*, std::shared_ptr<ObjectRecord>> objects;
    std::unordered_map<Generation, std::shared_ptr<ObjectRecord>> objectGenerations;
    std::unordered_map<Property*, std::shared_ptr<PropertyRecord>> properties;
    std::unordered_map<int, ClosureRecord> closures;
    std::unordered_map<OperationId, unsigned> native;
    std::unordered_map<OperationId, std::vector<std::weak_ptr<OperationState>>> operations;
    unsigned writers {};
    std::unordered_map<OperationId, unsigned> writerOperations;
    std::unordered_map<void*, std::pair<OperationId, unsigned>> resourceWriters;
    bool drainScheduled {false};
    OperationId atomicOwner {};
    unsigned atomicDepth {};
    bool stopping {false};
    std::vector<std::shared_ptr<ObjectRecord>> removals;
    std::atomic<std::uint64_t> epoch {1};
    std::atomic<bool> ready {false};
    bool cleanupRequested {false};
    bool drainBlocked {false};
    unsigned wakeActive {};
    std::condition_variable wakeDone;
    std::atomic<unsigned> ownerNative {0};
    std::shared_ptr<std::function<void()>> wake;
    std::shared_ptr<std::function<void()>> propertyDrain;
};
std::recursive_mutex indexMutex;

bool pinned(const std::unordered_map<OperationId, unsigned>& pins)
{
    return std::any_of(pins.begin(), pins.end(), [](const auto& p) { return p.second != 0; });
}
bool foreignPins(const std::unordered_map<OperationId, unsigned>& pins, OperationId operation)
{
    return std::any_of(pins.begin(), pins.end(), [operation](const auto& p) {
        return p.first != operation && p.second != 0;
    });
}
void updateReady(Registry& registry)
{
    const bool pendingClosure
        = std::any_of(registry.closures.begin(), registry.closures.end(), [](const auto& item) {
              return item.second.live && item.second.pending;
          });
    registry.ready.store(
        !registry.drainScheduled && !registry.drainBlocked && !registry.atomicOwner
            && registry.native.empty() && !registry.writers
            && (!registry.removals.empty() || registry.cleanupRequested || pendingClosure),
        std::memory_order_release
    );
    registry.epoch.fetch_add(1, std::memory_order_release);
}
void wake(const std::shared_ptr<Registry>& registry) noexcept
{
    std::shared_ptr<std::function<void()>> callback;
    {
        std::lock_guard lock(registry->mutex);
        registry->drainBlocked = false;
        updateReady(*registry);
        callback = registry->wake;
        if (callback && *callback) {
            ++registry->wakeActive;
        }
    }
    if (callback && *callback) {
        try {
            (*callback)();
        }
        catch (...) { /* Notification must not escape a lifetime destructor. */
        }
        std::lock_guard lock(registry->mutex);
        --registry->wakeActive;
        registry->wakeDone.notify_all();
    }
}
}  // namespace

struct ProgressRegistration
{
    std::weak_ptr<RecomputeProgressHandle> progress;
    std::shared_ptr<ProgressRegistration> next;
};
struct OperationState
{
    std::shared_ptr<Registry> registry;
    OperationId id {};
    bool native {false};
    bool worker {false};
    BindingPtr binding;
    std::shared_ptr<OperationState> parent;
    std::atomic<bool> canceled {false};
    std::shared_ptr<ProgressRegistration> progress;
    std::vector<std::shared_ptr<DocumentRecord>> documents;
    std::vector<std::shared_ptr<ObjectRecord>> objects;
    std::vector<std::shared_ptr<DocumentRecord>> executionDocuments;
    ~OperationState()
    {
        if (!registry || (!native && objects.empty() && documents.empty())) {
            return;
        }
        {
            std::lock_guard lock(registry->mutex);
            for (const auto& document : executionDocuments) {
                if (document->executionDepth && !--document->executionDepth) {
                    document->executionOwner = 0;
                }
            }
            for (const auto& object : objects) {
                auto it = object->pins.find(id);
                if (it != object->pins.end() && !--it->second) {
                    object->pins.erase(it);
                }
            }
            for (const auto& document : documents) {
                auto it = document->pins.find(id);
                if (it != document->pins.end() && !--it->second) {
                    document->pins.erase(it);
                }
            }
            if (native) {
                if (!worker) {
                    registry->ownerNative.fetch_sub(1, std::memory_order_release);
                }
                auto it = registry->native.find(id);
                if (it != registry->native.end() && !--it->second) {
                    registry->native.erase(it);
                    registry->operations.erase(id);
                }
            }
        }
        wake(registry);
    }
};
namespace
{
struct ContextLink;
struct CurrentContext
{
    std::shared_ptr<OperationState> operation;
    bool authority {false};
    std::shared_ptr<const ContextLink> previous;
    std::weak_ptr<ScopeState> mutation;
    bool borrowed {false};
    BindingPtr importedRestore;
    std::weak_ptr<ScopeState> importPreparation;
};
struct ContextLink
{
    CurrentContext context;
};
thread_local CurrentContext current;
thread_local bool workerDispatcher = false;
thread_local CaptureScope* activeCapture = nullptr;
OperationId currentId()
{
    return current.operation ? current.operation->id : 0;
}
bool borrowedContext()
{
    for (const CurrentContext* context = &current; context;
         context = context->previous ? &context->previous->context : nullptr) {
        if (context->borrowed) {
            return true;
        }
    }
    return false;
}

bool observedOperation(OperationId id)
{
    for (const CurrentContext* context = &current; context;
         context = context->previous ? &context->previous->context : nullptr) {
        for (auto operation = context->operation; operation; operation = operation->parent) {
            if (operation->id == id) {
                return true;
            }
        }
    }
    return false;
}
bool observedDocument(const std::shared_ptr<DocumentRecord>& document)
{
    for (const auto& pin : document->pins) {
        if (pin.second && observedOperation(pin.first)) {
            return true;
        }
    }
    return false;
}

void pinDocument(
    const std::shared_ptr<OperationState>& operation,
    const std::shared_ptr<DocumentRecord>& document
)
{
    if (std::find(operation->documents.begin(), operation->documents.end(), document)
        == operation->documents.end()) {
        ++document->pins[operation->id];
        operation->documents.push_back(document);
    }
}
void reserveExecutionDocument(
    const std::shared_ptr<OperationState>& operation,
    const std::shared_ptr<DocumentRecord>& document
)
{
    if (document->executionOwner && document->executionOwner != operation->id) {
        throw AdmissionRefusal("Document has another native recompute operation");
    }
    if (std::find(operation->executionDocuments.begin(), operation->executionDocuments.end(), document)
        == operation->executionDocuments.end()) {
        document->executionOwner = operation->id;
        ++document->executionDepth;
        operation->executionDocuments.push_back(document);
    }
}
void pinObject(
    const std::shared_ptr<OperationState>& operation,
    const std::shared_ptr<ObjectRecord>& object
)
{
    pinDocument(operation, object->document);
    if (std::find(operation->objects.begin(), operation->objects.end(), object)
        == operation->objects.end()) {
        ++object->pins[operation->id];
        operation->objects.push_back(object);
    }
}
std::shared_ptr<OperationState> newOperation(const std::shared_ptr<Registry>& registry)
{
    auto operation = std::make_shared<OperationState>();
    operation->registry = registry;
    operation->parent = current.operation;
    operation->documents.reserve(2);
    operation->objects.reserve(2);
    operation->executionDocuments.reserve(2);
    std::lock_guard lock(registry->mutex);
    operation->id = current.authority && current.operation ? currentId() : registry->nextOperation++;
    return operation;
}
}  // namespace

struct ScopeState
{
    std::shared_ptr<Registry> registry;
    std::shared_ptr<OperationState> operation;
    std::vector<std::shared_ptr<DocumentRecord>> documents;
    std::vector<std::shared_ptr<PropertyRecord>> properties;
    std::vector<std::shared_ptr<ObjectRecord>> ownerObjects;
    std::vector<std::shared_ptr<DocumentRecord>> ownerDocuments;
    std::vector<std::shared_ptr<void>> resources;
    bool atomic {false};
    bool importPreparation {false};
    bool destructive {false};
    ~ScopeState()
    {
        if (!registry) {
            return;
        }
        {
            std::lock_guard lock(registry->mutex);
            for (const auto& property : properties) {
                --property->pins;
            }
            for (const auto& object : ownerObjects) {
                auto pin = object->pins.find(operation->id);
                if (pin != object->pins.end() && !--pin->second) {
                    object->pins.erase(pin);
                }
            }
            for (const auto& document : ownerDocuments) {
                auto pin = document->pins.find(operation->id);
                if (pin != document->pins.end() && !--pin->second) {
                    document->pins.erase(pin);
                }
            }
            for (const auto& document : documents) {
                if (document->writerDepth && !--document->writerDepth) {
                    document->writer = 0;
                }
            }
            for (const auto& resource : resources) {
                auto entry = registry->resourceWriters.find(resource.get());
                if (entry != registry->resourceWriters.end() && !--entry->second.second) {
                    registry->resourceWriters.erase(entry);
                }
            }
            if (atomic) {
                if (registry->atomicDepth && !--registry->atomicDepth) {
                    registry->atomicOwner = 0;
                }
            }
            else if (!documents.empty()) {
                --registry->writers;
                auto owner = registry->writerOperations.find(operation->id);
                if (owner != registry->writerOperations.end() && !--owner->second) {
                    registry->writerOperations.erase(owner);
                }
            }
        }
        wake(registry);
    }
};
namespace
{
void pinScopeOwner(
    const std::shared_ptr<ScopeState>& scope,
    const std::shared_ptr<ObjectRecord>& object,
    const std::shared_ptr<DocumentRecord>& document
)
{
    if (object
        && std::find(scope->ownerObjects.begin(), scope->ownerObjects.end(), object)
            == scope->ownerObjects.end()) {
        ++object->pins[scope->operation->id];
        scope->ownerObjects.push_back(object);
    }
    if (document
        && std::find(scope->ownerDocuments.begin(), scope->ownerDocuments.end(), document)
            == scope->ownerDocuments.end()) {
        ++document->pins[scope->operation->id];
        scope->ownerDocuments.push_back(document);
    }
}
}  // namespace
struct Coordinator::Impl
{
    std::shared_ptr<Registry> registry = std::make_shared<Registry>();
};
struct OperationScope::Impl
{
    CurrentContext previous;
    bool previousWorker {false};
};

AdmissionRefusal::AdmissionRefusal(const char* message, Reason reason)
    : Base::RuntimeError(message)
    , failureReason(reason)
{}
Binding::Binding(
    Generation doc,
    Generation object,
    std::string docName,
    std::string objName,
    bool target,
    bool unavailable
)
    : documentGeneration(doc)
    , attachmentGeneration(object)
    , documentName(std::move(docName))
    , objectName(std::move(objName))
    , objectTarget(target)
    , missing(unavailable)
{}
OperationScope::OperationScope(std::shared_ptr<OperationState> state, bool authority, bool dispatch)
    : impl(std::make_unique<Impl>(Impl {current, workerDispatcher}))
{
    auto previous = std::make_shared<ContextLink>(ContextLink {current});
    const bool borrowed = !authority && !dispatch && static_cast<bool>(state);
    current = {std::move(state), authority, std::move(previous), {}, borrowed, {}, {}};
    if (dispatch) {
        workerDispatcher = true;
    }
}
OperationScope::OperationScope(OperationScope&&) noexcept = default;
OperationScope::~OperationScope()
{
    if (impl) {
        current = std::move(impl->previous);
        workerDispatcher = impl->previousWorker;
    }
}
ObservationScope::ObservationScope(Observation value)
    : scope(std::move(value), false)
{}
NativeClaim::~NativeClaim() = default;
NativeClaim::operator bool() const noexcept
{
    return static_cast<bool>(state);
}
Document* NativeClaim::document() const noexcept
{
    return rootDocument;
}
DocumentObject* NativeClaim::object() const noexcept
{
    return rootObject;
}
OperationId NativeClaim::operation() const noexcept
{
    return state ? state->id : 0;
}
OperationScope NativeClaim::enter() const
{
    return OperationScope(state, true);
}
void NativeClaim::reset() noexcept
{
    state.reset();
    rootDocument = nullptr;
    rootObject = nullptr;
}
void NativeClaim::registerProgress(std::shared_ptr<RecomputeProgressHandle> progress)
{
    if (!state || !progress) {
        return;
    }
    auto registration = std::make_shared<ProgressRegistration>();
    registration->progress = progress;
    bool canceled;
    {
        std::lock_guard lock(state->registry->mutex);
        registration->next = state->progress;
        state->progress = std::move(registration);
        canceled = state->canceled.load(std::memory_order_relaxed);
    }
    if (canceled) {
        progress->cancel();
    }
}
bool NativeClaim::cancellationRequested() const noexcept
{
    return state && state->canceled.load(std::memory_order_acquire);
}
MutationScope& MutationScope::operator=(MutationScope&& other) noexcept
{
    if (this == &other) {
        return *this;
    }
    // A newly constructed replacement already installed its context. Splice out
    // the old lexical frame rather than restoring it over that new context.
    if (context && context->impl && other.context && other.context->impl) {
        other.context->impl->previous = context->impl->previous;
        context->impl.reset();
    }
    context.reset();
    state.reset();
    state = std::move(other.state);
    context = std::move(other.context);
    return *this;
}
MutationScope::~MutationScope() = default;
MutationScope::operator bool() const noexcept
{
    return static_cast<bool>(state);
}
PropertySnapshot::~PropertySnapshot() = default;
void PropertySnapshot::releaseStorage()
{
    if (!reservation.state) {
        return;
    }
    auto scope = reservation.state;
    {
        std::lock_guard lock(scope->registry->mutex);
        for (const auto& property : scope->properties) {
            --property->pins;
        }
        scope->properties.clear();
    }
    wake(scope->registry);
}
void PropertySnapshot::requireExclusiveStorage() const
{
    requireExclusiveStorage(values);
}
void PropertySnapshot::requireExclusiveStorage(const std::vector<Property*>& destructionValues) const
{
    if (!reservation.state) {
        return;
    }
    std::lock_guard lock(reservation.state->registry->mutex);
    for (const auto& property : reservation.state->properties) {
        if (std::find(destructionValues.begin(), destructionValues.end(), property->pointer)
                != destructionValues.end()
            && property->pins != 1) {
            throw AdmissionRefusal("Property storage has another live receiver or reader");
        }
    }
}

const std::vector<Property*>& PropertySnapshot::properties() const noexcept
{
    return values;
}
struct IndexLock::Impl
{
    std::lock_guard<std::recursive_mutex> lock {indexMutex};
};
IndexLock::IndexLock()
    : impl(std::make_unique<Impl>())
{}
IndexLock::~IndexLock() = default;
Coordinator::Coordinator()
    : impl(std::make_unique<Impl>())
{}
Coordinator::~Coordinator() = default;
Coordinator& Coordinator::instance()
{
    // App and Python static destructors may retire metadata after Application.
    static Coordinator* value = new Coordinator;
    return *value;
}

void Coordinator::registerDocument(Document* pointer, const std::string& name)
{
    auto record = std::make_shared<DocumentRecord>();
    record->pointer = pointer;
    record->name = name;
    auto registry = impl->registry;
    std::lock_guard lock(registry->mutex);
    auto previous = registry->documents.find(pointer);
    if (previous != registry->documents.end() && previous->second->live) {
        return;
    }
    record->generation = registry->next++;
    registry->documents[pointer] = record;
    registry->documentGenerations[record->generation] = record;
    registry->names[name] = record;
    for (auto& item : registry->properties) {
        if (item.second->documentKey == pointer) {
            item.second->document = record;
        }
    }
}
void Coordinator::retireDocument(Document* pointer)
{
    auto registry = impl->registry;
    {
        std::lock_guard lock(registry->mutex);
        auto found = registry->documents.find(pointer);
        if (found == registry->documents.end()) {
            return;
        }
        auto document = found->second;
        if (pinned(document->pins)) {
            throw AdmissionRefusal("Document is leased by recompute");
        }
        document->live = false;
        document->retiring = true;
        registry->documentGenerations.erase(document->generation);
        for (auto& item : registry->closures) {
            if (std::find(item.second.documents.begin(), item.second.documents.end(), document->generation)
                != item.second.documents.end()) {
                item.second.live = false;
                item.second.pending = false;
            }
        }
        auto name = registry->names.find(document->name);
        if (name != registry->names.end() && name->second == document) {
            registry->names.erase(name);
        }
        for (auto it = registry->objects.begin(); it != registry->objects.end();) {
            if (it->second->document == document) {
                it->second->live = false;
                registry->objectGenerations.erase(it->second->generation);
                it = registry->objects.erase(it);
            }
            else {
                ++it;
            }
        }
        registry->documents.erase(found);
    }
    wake(registry);
}
void Coordinator::registerAttachment(Document* doc, DocumentObject* pointer, const std::string& name)
{
    auto object = std::make_shared<ObjectRecord>();
    object->pointer = pointer;
    object->name = name;
    auto registry = impl->registry;
    std::lock_guard lock(registry->mutex);
    auto owner = registry->documents.find(doc);
    if (owner == registry->documents.end()) {
        return;
    }
    object->document = owner->second;
    object->generation = registry->next++;
    registry->objects[pointer] = object;
    registry->objectGenerations[object->generation] = object;
    for (auto& item : registry->properties) {
        if (item.second->objectKey == pointer) {
            item.second->object = object;
            item.second->document = object->document;
        }
    }
}
void Coordinator::retireAttachment(DocumentObject* pointer)
{
    auto registry = impl->registry;
    {
        std::lock_guard lock(registry->mutex);
        auto found = registry->objects.find(pointer);
        if (found == registry->objects.end()) {
            return;
        }
        if (pinned(found->second->pins)) {
            throw AdmissionRefusal("Object is leased by recompute");
        }
        found->second->live = false;
        registry->objectGenerations.erase(found->second->generation);
        registry->objects.erase(found);
    }
    wake(registry);
}
BindingPtr Coordinator::bindDocument(const Document* pointer) const
{
    auto registry = impl->registry;
    std::lock_guard lock(registry->mutex);
    auto found = registry->documents.find(pointer);
    if (found == registry->documents.end()) {
        return std::make_shared<Binding>(0, 0, "", "", false, true);
    }
    const auto& doc = found->second;
    return std::make_shared<Binding>(doc->generation, 0, doc->name, "", false, !doc->live || doc->retiring);
}
BindingPtr Coordinator::bindObject(const DocumentObject* pointer) const
{
    auto registry = impl->registry;
    std::lock_guard lock(registry->mutex);
    auto found = registry->objects.find(pointer);
    if (found == registry->objects.end()) {
        return std::make_shared<Binding>(0, 0, "", "", true, true);
    }
    const auto& object = found->second;
    return std::make_shared<Binding>(
        object->document->generation,
        object->generation,
        object->document->name,
        object->name,
        true,
        !object->live || object->pendingRemoval || !object->document->live || object->document->retiring
    );
}
BindingPtr Coordinator::bindNames(
    const std::string& document,
    const std::string& object,
    bool objectTarget
) const
{
    auto registry = impl->registry;
    std::lock_guard lock(registry->mutex);
    auto found = registry->names.find(document);
    if (found == registry->names.end() || !found->second->live || found->second->retiring) {
        return std::make_shared<Binding>(0, 0, document, object, objectTarget, true);
    }
    const auto& doc = found->second;
    if (!objectTarget) {
        return std::make_shared<Binding>(doc->generation, 0, document, object, false, false);
    }
    for (const auto& item : registry->objects) {
        const auto& target = item.second;
        if (target->document == doc && target->name == object && target->live
            && !target->pendingRemoval) {
            return std::make_shared<Binding>(
                doc->generation,
                target->generation,
                document,
                object,
                true,
                false
            );
        }
    }
    return std::make_shared<Binding>(doc->generation, 0, document, object, true, true);
}
bool Coordinator::coherent(const BindingPtr& binding, const std::string& doc, const std::string& object) const
{
    return binding && binding->documentName == doc && binding->objectName == object;
}
Document* Coordinator::resolveDocument(const BindingPtr& binding) const
{
    if (!binding || binding->missing) {
        return nullptr;
    }
    auto registry = impl->registry;
    std::lock_guard lock(registry->mutex);
    auto found = registry->documentGenerations.find(binding->documentGeneration);
    if (found == registry->documentGenerations.end() || !found->second->live
        || found->second->retiring) {
        return nullptr;
    }
    if (binding->objectTarget) {
        auto object = registry->objectGenerations.find(binding->attachmentGeneration);
        if (object == registry->objectGenerations.end() || !object->second->live
            || object->second->pendingRemoval) {
            return nullptr;
        }
    }
    return found->second->pointer;
}
DocumentObject* Coordinator::resolveObject(const BindingPtr& binding) const
{
    if (!binding || !binding->objectTarget || binding->missing) {
        return nullptr;
    }
    auto registry = impl->registry;
    std::lock_guard lock(registry->mutex);
    auto found = registry->objectGenerations.find(binding->attachmentGeneration);
    if (found == registry->objectGenerations.end() || !found->second->live
        || found->second->pendingRemoval || !found->second->document->live
        || found->second->document->retiring) {
        return nullptr;
    }
    return found->second->pointer;
}
ClaimResult Coordinator::claimFirst(const std::vector<BindingPtr>& candidates, Route route)
{
    if (route == Route::Owner && borrowedContext()) {
        return {};
    }
    auto registry = impl->registry;
    auto operation = newOperation(registry);
    ClaimResult result;
    std::lock_guard lock(registry->mutex);
    result.epoch = registry->epoch.load(std::memory_order_relaxed);
    if (registry->stopping || (registry->atomicOwner && registry->atomicOwner != operation->id)) {
        return result;
    }
    auto admitTerminal = [&]() {
        operation->native = true;
        operation->worker = workerDispatcher || route == Route::Worker
            || (current.authority && current.operation && current.operation->worker);
        ++registry->native[operation->id];
        registry->operations[operation->id].push_back(operation);
        if (!operation->worker) {
            registry->ownerNative.fetch_add(1, std::memory_order_release);
        }
        result.claim.state = operation;
        updateReady(*registry);
    };
    for (std::size_t i = 0; i != candidates.size(); ++i) {
        result.candidate = i;
        const auto& binding = candidates[i];
        if (!binding || binding->missing) {
            admitTerminal();
            result.disposition = Disposition::Unavailable;
            return result;
        }
        auto d = registry->documentGenerations.find(binding->documentGeneration);
        if (d == registry->documentGenerations.end() || !d->second->live || d->second->retiring) {
            admitTerminal();
            result.disposition = Disposition::Unavailable;
            return result;
        }
        auto document = d->second;
        std::shared_ptr<ObjectRecord> object;
        if (binding->objectTarget) {
            auto o = registry->objectGenerations.find(binding->attachmentGeneration);
            if (o == registry->objectGenerations.end() || !o->second->live
                || o->second->pendingRemoval) {
                admitTerminal();
                result.disposition = Disposition::Unavailable;
                return result;
            }
            object = o->second;
        }
        if ((document->executionOwner && document->executionOwner != operation->id)
            || document->readers) {
            continue;
        }
        if (document->writerDepth
            && !(route == Route::Owner && current.authority && document->writer == operation->id)) {
            continue;
        }
        operation->native = true;
        operation->worker = workerDispatcher || route == Route::Worker
            || (current.authority && current.operation && current.operation->worker);
        ++registry->native[operation->id];
        registry->operations[operation->id].push_back(operation);
        if (!operation->worker) {
            registry->ownerNative.fetch_add(1, std::memory_order_release);
        }
        updateReady(*registry);
        reserveExecutionDocument(operation, document);
        if (object) {
            pinObject(operation, object);
        }
        else {
            pinDocument(operation, document);
        }
        operation->binding = binding;
        result.claim.state = operation;
        result.claim.rootDocument = document->pointer;
        result.claim.rootObject = object ? object->pointer : nullptr;
        result.disposition = Disposition::Ready;
        return result;
    }
    return result;
}
ClaimResult Coordinator::claim(const BindingPtr& binding, Route route)
{
    return claimFirst({binding}, route);
}
NativeClaim Coordinator::claimDocument(Document* doc, DocumentObject* object)
{
    ensureSynchronousAllowed();
    if (current.authority && current.operation && current.operation->native) {
        {
            std::lock_guard lock(impl->registry->mutex);
            auto found = impl->registry->documents.find(doc);
            if (found == impl->registry->documents.end() || !found->second->live
                || (found->second->retiring && !found->second->pins.count(currentId()))) {
                throw AdmissionRefusal("Nested recompute document is unavailable");
            }
            reserveExecutionDocument(current.operation, found->second);
            pinDocument(current.operation, found->second);
        }
        NativeClaim claim;
        claim.state = current.operation;
        claim.rootDocument = doc;
        claim.rootObject = object;
        return claim;
    }
    auto result = claim(object ? bindObject(object) : bindDocument(doc), Route::Owner);
    if (result.disposition != Disposition::Ready) {
        throw AdmissionRefusal("Recompute target is busy or unavailable");
    }
    return std::move(result.claim);
}

void MutationScope::include(
    const std::vector<DocumentObject*>& objects,
    const std::vector<Document*>& documents
)
{
    if (!state) {
        return;
    }
    auto registry = state->registry;
    std::vector<std::shared_ptr<DocumentRecord>> additions;
    additions.reserve(objects.size() + documents.size());
    std::lock_guard lock(registry->mutex);
    auto add = [&](const std::shared_ptr<DocumentRecord>& document) {
        if (document
            && std::find(state->documents.begin(), state->documents.end(), document)
                == state->documents.end()
            && std::find(additions.begin(), additions.end(), document) == additions.end()) {
            additions.push_back(document);
        }
    };
    for (auto* pointer : documents) {
        auto found = registry->documents.find(pointer);
        if (found != registry->documents.end()) {
            add(found->second);
        }
    }
    for (auto* pointer : objects) {
        auto found = registry->objects.find(pointer);
        if (found == registry->objects.end()) {
            continue;
        }  // unattached construction
        auto object = found->second;
        if ((state->destructive && pinned(object->pins))
            || foreignPins(object->pins, state->operation->id)) {
            throw AdmissionRefusal("Topology endpoint is leased by recompute");
        }
        add(object->document);
    }
    const auto operation = state->operation->id;
    if (registry->atomicOwner && registry->atomicOwner != operation) {
        throw AdmissionRefusal("Topology mutation conflicts with an atomic operation");
    }
    for (const auto& document : additions) {
        if ((!document->live && document->closeOwner != operation)
            || (document->retiring && document->closeOwner != operation
                && !document->pins.count(operation))
            || document->readers || (document->writerDepth && document->writer != operation)) {
            throw AdmissionRefusal("Document topology is busy");
        }
    }
    if (state->documents.empty() && !additions.empty() && !state->atomic) {
        ++registry->writers;
        ++registry->writerOperations[operation];
    }
    for (const auto& document : additions) {
        document->writer = operation;
        ++document->writerDepth;
        state->documents.push_back(document);
    }
    updateReady(*registry);
}
namespace
{
void reserveResource(const std::shared_ptr<ScopeState>& scope, std::shared_ptr<void> resource)
{
    if (!scope || !resource) {
        throw AdmissionRefusal("Missing lexical resource admission scope");
    }
    if (std::any_of(scope->resources.begin(), scope->resources.end(), [&](const auto& value) {
            return value.get() == resource.get();
        })) {
        return;
    }
    scope->resources.reserve(scope->resources.size() + 1);
    std::lock_guard lock(scope->registry->mutex);
    auto found = scope->registry->resourceWriters.find(resource.get());
    if (found != scope->registry->resourceWriters.end()
        && found->second.first != scope->operation->id) {
        throw AdmissionRefusal("Shared dependency metadata has another writer");
    }
    auto& owner = scope->registry->resourceWriters[resource.get()];
    owner.first = scope->operation->id;
    ++owner.second;
    scope->resources.push_back(std::move(resource));
}
}  // namespace
void MutationScope::reserveResource(std::shared_ptr<void> resource)
{
    RecomputeDetail::reserveResource(state, std::move(resource));
}
std::vector<DocumentObject*> MutationScope::pinBindings(const std::vector<BindingPtr>& bindings)
{
    if (!state) {
        throw AdmissionRefusal("Binding pin requires a source mutation scope");
    }
    std::vector<DocumentObject*> result(bindings.size(), nullptr);
    std::vector<std::shared_ptr<ObjectRecord>> objects;
    std::vector<std::shared_ptr<DocumentRecord>> documents;
    objects.reserve(bindings.size());
    documents.reserve(bindings.size());
    state->documents.reserve(state->documents.size() + bindings.size());
    state->ownerObjects.reserve(state->ownerObjects.size() + bindings.size());
    state->ownerDocuments.reserve(state->ownerDocuments.size() + bindings.size());
    std::lock_guard lock(state->registry->mutex);
    const auto id = state->operation->id;
    if (state->registry->atomicOwner && state->registry->atomicOwner != id) {
        throw AdmissionRefusal("Target binding conflicts with an atomic operation");
    }
    for (std::size_t i = 0; i != bindings.size(); ++i) {
        const auto& binding = bindings[i];
        if (!binding || binding->missing) {
            throw AdmissionRefusal(
                "Mutation target is unavailable",
                AdmissionRefusal::Reason::Unavailable
            );
        }
        auto doc = state->registry->documentGenerations.find(binding->documentGeneration);
        if (doc == state->registry->documentGenerations.end() || !doc->second->live
            || doc->second->retiring) {
            throw AdmissionRefusal(
                "Mutation target document is unavailable",
                AdmissionRefusal::Reason::Unavailable
            );
        }
        auto document = doc->second;
        if (document->readers || (document->writerDepth && document->writer != id)) {
            throw AdmissionRefusal("Mutation target document is busy");
        }
        if (std::find(documents.begin(), documents.end(), document) == documents.end()) {
            documents.push_back(document);
        }
        if (binding->objectTarget) {
            auto object = state->registry->objectGenerations.find(binding->attachmentGeneration);
            if (object == state->registry->objectGenerations.end() || !object->second->live
                || object->second->pendingRemoval) {
                throw AdmissionRefusal(
                    "Mutation target attachment is unavailable",
                    AdmissionRefusal::Reason::Unavailable
                );
            }
            if (foreignPins(object->second->pins, id)) {
                throw AdmissionRefusal("Mutation target is leased by another operation");
            }
            objects.push_back(object->second);
            result[i] = object->second->pointer;
        }
    }
    const bool hadDocuments = !state->documents.empty();
    for (const auto& document : documents) {
        if (std::find(state->documents.begin(), state->documents.end(), document)
            == state->documents.end()) {
            document->writer = id;
            ++document->writerDepth;
            state->documents.push_back(document);
        }
    }
    if (!hadDocuments && !state->documents.empty() && !state->atomic) {
        ++state->registry->writers;
        ++state->registry->writerOperations[id];
    }
    for (const auto& object : objects) {
        pinScopeOwner(state, object, object->document);
    }
    for (const auto& document : documents) {
        pinScopeOwner(state, nullptr, document);
    }
    updateReady(*state->registry);
    return result;
}
void Coordinator::retainMutationResource(std::shared_ptr<void> resource)
{
    if (!current.authority) {
        throw AdmissionRefusal("Resource planning requires native writer authority");
    }
    RecomputeDetail::reserveResource(current.mutation.lock(), std::move(resource));
}
MutationScope Coordinator::mutation(
    const std::vector<DocumentObject*>& objects,
    const std::vector<Document*>& documents,
    bool destructive
)
{
    MutationScope result;
    result.state = std::make_shared<ScopeState>();
    result.state->registry = impl->registry;
    result.state->operation = current.authority && current.operation ? current.operation
                                                                     : newOperation(impl->registry);
    result.state->destructive = destructive;
    result.context = std::unique_ptr<OperationScope>(new OperationScope(result.state->operation, true));
    current.mutation = result.state;
    result.include(objects, documents);
    return result;
}
MutationScope Coordinator::mutation(const Property& property, const std::vector<DocumentObject*>& proposed)
{
    std::shared_ptr<PropertyRecord> record;
    {
        std::lock_guard lock(impl->registry->mutex);
        auto found = impl->registry->properties.find(const_cast<Property*>(&property));
        if (found != impl->registry->properties.end() && found->second->live) {
            record = found->second;
        }
    }
    if (!record || (!record->object && !record->document) || (record->object && !record->object->live)
        || (record->document && !record->document->live)) {
        // Detached flags/copies must not inspect their saved raw targets. A
        // constructor's explicit new endpoints still need backlink protection.
        if (record && record->objectKey && !proposed.empty()) {
            if (auto parent = current.mutation.lock()) {
                MutationScope parentView;
                parentView.state = parent;
                parentView.include(proposed);
            }
            return mutation(proposed);
        }
        return {};
    }
    std::vector<DocumentObject*> owners;
    std::vector<Document*> documents;
    if (record && record->object) {
        owners.push_back(record->object->pointer);
    }
    if (record && record->document) {
        documents.push_back(record->document->pointer);
    }
    auto scope = mutation(owners, documents);
    scope.state->properties.reserve(1);
    scope.state->ownerObjects.reserve(1);
    scope.state->ownerDocuments.reserve(1);
    {
        std::lock_guard lock(impl->registry->mutex);
        pinScopeOwner(scope.state, record->object, record->document);
        ++record->pins;
        scope.state->properties.push_back(record);
    }
    std::vector<DocumentObject*> targets;
    if (auto link = dynamic_cast<const PropertyLinkBase*>(&property)) {
        // Primary source is already reserved before this potentially virtual getter.
        link->collectAdmissionTargets(targets);
    }
    {
        std::lock_guard lock(impl->registry->mutex);
        for (auto* target : proposed) {
            auto found = impl->registry->objects.find(target);
            if (found != impl->registry->objects.end() && found->second->document->retiring
                && std::find(targets.begin(), targets.end(), target) == targets.end()) {
                throw AdmissionRefusal("Cannot introduce a dependency into a closing document");
            }
        }
    }
    targets.insert(targets.end(), proposed.begin(), proposed.end());
    scope.include(targets);
    if (auto link = dynamic_cast<const PropertyLinkBase*>(&property)) {
        const_cast<PropertyLinkBase*>(link)->registerAdmissionTargets(targets);
    }
    return scope;
}
std::optional<MutationScope> Coordinator::tryAtomic()
{
    return tryAtomicImpl(false);
}
std::optional<MutationScope> Coordinator::tryAtomicImpl(bool ownerNativeImport)
{
    MutationScope result;
    result.state = std::make_shared<ScopeState>();
    result.state->registry = impl->registry;
    result.state->operation = current.authority && current.operation ? current.operation
                                                                     : newOperation(impl->registry);
    result.state->atomic = true;
    result.state->importPreparation = ownerNativeImport;
    auto registry = impl->registry;
    {
        std::lock_guard lock(registry->mutex);
        const auto id = result.state->operation->id;
        const bool ownOwnerNative = ownerNativeImport && current.authority && current.operation
            && current.operation->native && !current.operation->worker && !borrowedContext()
            && !activeCapture;
        const bool nativeConflict = ownOwnerNative
            ? std::any_of(
                  registry->native.begin(),
                  registry->native.end(),
                  [id](const auto& permit) { return permit.first != id && permit.second; }
              )
            : !registry->native.empty();
        if (activeCapture || (registry->atomicOwner && registry->atomicOwner != id) || nativeConflict
            || std::any_of(
                registry->resourceWriters.begin(),
                registry->resourceWriters.end(),
                [id](const auto& owner) { return owner.second.first != id; }
            )
            || std::any_of(
                registry->writerOperations.begin(),
                registry->writerOperations.end(),
                [id](const auto& owner) { return owner.first != id && owner.second; }
            )) {
            // Do not let the unacquired scope decrement another atomic phase.
            result.state->registry.reset();
            return std::nullopt;
        }
        registry->atomicOwner = id;
        ++registry->atomicDepth;
        updateReady(*registry);
    }
    result.context = std::unique_ptr<OperationScope>(new OperationScope(result.state->operation, true));
    current.mutation = result.state;
    return result;
}
MutationScope Coordinator::atomic()
{
    auto scope = tryAtomic();
    if (!scope) {
        throw AdmissionRefusal("Atomic structural operation is busy with recompute");
    }
    return std::move(*scope);
}
MutationScope Coordinator::atomicForImport()
{
    auto scope = tryAtomicImpl(true);
    if (!scope) {
        throw AdmissionRefusal("Import preparation is busy with foreign native or topology work");
    }
    return std::move(*scope);
}
MutationScope Coordinator::restoreImportedObject(
    const BindingPtr& binding,
    const MutationScope& importPreparation
)
{
    if (!current.authority || !current.operation || onWorker() || borrowedContext() || activeCapture
        || !importPreparation.state || !importPreparation.state->importPreparation
        || importPreparation.state->registry != impl->registry
        || importPreparation.state->operation->id != currentId()) {
        throw AdmissionRefusal("Imported receiver restore requires its exact owner import phase");
    }
    auto receiver = mutation(std::vector<DocumentObject*> {}, {});
    receiver.pinBindings({binding});
    current.importedRestore = binding;
    current.importPreparation = importPreparation.state;
    return receiver;
}
bool Coordinator::matchesImportedRestore(const PropertyContainer* container, const Property* property) const
{
    if (!current.authority || !current.operation || !current.operation->native || onWorker()
        || borrowedContext() || activeCapture) {
        return false;
    }
    // The nearest explicit receiver scope masks outer receiver capabilities.
    // Promote its lexical phase outside R, including on every early return.
    BindingPtr binding;
    std::shared_ptr<ScopeState> phase;
    for (const CurrentContext* context = &current; context;
         context = context->previous ? &context->previous->context : nullptr) {
        if (context->importedRestore) {
            phase = context->importPreparation.lock();
            if (context->operation && context->operation->id == currentId() && phase
                && phase->importPreparation && phase->operation->id == currentId()) {
                binding = context->importedRestore;
            }
            break;
        }
    }
    if (!binding) {
        return false;
    }
    std::lock_guard lock(impl->registry->mutex);
    auto found = impl->registry->objectGenerations.find(binding->attachmentGeneration);
    if (found == impl->registry->objectGenerations.end() || !found->second->live
        || found->second->pendingRemoval || !found->second->document->live
        || found->second->document->retiring
        || found->second->document->generation != binding->documentGeneration) {
        return false;
    }
    if (container && static_cast<const PropertyContainer*>(found->second->pointer) == container) {
        return true;
    }
    if (property) {
        auto registered = impl->registry->properties.find(const_cast<Property*>(property));
        return registered != impl->registry->properties.end() && registered->second->live
            && registered->second->object == found->second;
    }
    return false;
}
MutationScope Coordinator::atomicForRestore(const PropertyContainer& receiver)
{
    return matchesImportedRestore(&receiver, nullptr) ? atomicForImport() : atomic();
}
MutationScope Coordinator::atomicForRestore(const Property& receiver)
{
    return matchesImportedRestore(nullptr, &receiver) ? atomicForImport() : atomic();
}
bool Coordinator::hasAuthority() const
{
    return current.authority;
}
bool Coordinator::onWorker() const
{
    return workerDispatcher || (current.operation && current.operation->worker && current.authority);
}
Observation Coordinator::observation() const
{
    return current.operation;
}
OperationScope Coordinator::workerDispatch()
{
    auto operation = newOperation(impl->registry);
    operation->worker = true;
    return OperationScope(std::move(operation), false, true);
}
void Coordinator::ensureSynchronousAllowed() const
{
    if (borrowedContext()) {
        throw AdmissionRefusal("Synchronous recompute is unavailable inside a borrowed GUI callback");
    }
}
void Coordinator::ensureWaitAllowed() const
{
    bool active = false;
    for (const CurrentContext* context = &current; context;
         context = context->previous ? &context->previous->context : nullptr) {
        if (context->operation) {
            active = true;
            break;
        }
    }
    if (active) {
        throw AdmissionRefusal(
            "Cannot wait for queued recompute from a native, writer or observed callback"
        );
    }
}
void throwIfBlockingRecomputeWaitIsCyclic()
{
    Coordinator::instance().ensureWaitAllowed();
}
void Coordinator::ensureRootAvailable() const
{
    if (!current.operation || !current.operation->binding) {
        return;
    }
    if (current.operation->canceled.load(std::memory_order_acquire)) {
        throw Base::UserAbortException();
    }
    const auto& binding = current.operation->binding;
    std::lock_guard lock(impl->registry->mutex);
    if (current.operation->canceled.load(std::memory_order_relaxed)) {
        throw Base::UserAbortException();
    }
    auto document = impl->registry->documentGenerations.find(binding->documentGeneration);
    if (binding->missing || document == impl->registry->documentGenerations.end()
        || !document->second->live) {
        throw AdmissionRefusal(
            "Original recompute document is unavailable",
            AdmissionRefusal::Reason::Unavailable
        );
    }
    if (binding->objectTarget) {
        auto object = impl->registry->objectGenerations.find(binding->attachmentGeneration);
        if (object == impl->registry->objectGenerations.end() || !object->second->live
            || object->second->pendingRemoval) {
            throw AdmissionRefusal(
                "Original recompute target is unavailable",
                AdmissionRefusal::Reason::Unavailable
            );
        }
    }
}
void Coordinator::ensureFeatureAllowed(DocumentObject* pointer) const
{
    ensureRootAvailable();
    if (activeCapture) {
        throw AdmissionRefusal("Cannot execute a feature during graph capture");
    }
    if (!onWorker()) {
        return;
    }
    bool live = false;
    {
        std::lock_guard lock(impl->registry->mutex);
        auto found = impl->registry->objects.find(pointer);
        live = found != impl->registry->objects.end() && found->second->live && current.operation
            && std::find(
                   current.operation->objects.begin(),
                   current.operation->objects.end(),
                   found->second
               ) != current.operation->objects.end();
    }
    if (!live || !pointer->canRecomputeOnWorker()) {
        throw AdmissionRefusal(
            "Scheduled feature is no longer worker eligible; execution may be partial"
        );
    }
    ensureRootAvailable();
}


struct CaptureScope::Impl
{
    std::shared_ptr<Registry> registry;
    std::shared_ptr<OperationState> operation;
    std::shared_ptr<ScopeState> planningPins;
    std::vector<std::shared_ptr<DocumentRecord>> readers;
    CaptureScope* previous {};
    bool planning {false};
    bool allowOwnedWriter {false};
};
CaptureScope::CaptureScope(Document* document, bool planning)
    : impl(std::make_unique<Impl>())
{
    impl->registry = Coordinator::instance().impl->registry;
    impl->operation = current.operation;
    impl->planning = planning;
    if (!impl->operation || !current.authority) {
        throw AdmissionRefusal("Graph capture requires a native owner operation");
    }
    auto registry = impl->registry;
    // A synchronous hasSetValue/onChanged callback may recompute its current
    // state before its lexical setter writer ends. Foreign work stays excluded.
    impl->allowOwnedWriter = planning
        || (!impl->operation->worker && !borrowedContext() && !activeCapture);
    if (planning) {
        // A setter still uses its planned endpoints after graph readers end.
        // Keep those borrows in the caller's lexical writer, never in the
        // enclosing opaque operation that may apply another undo step.
        impl->planningPins = current.mutation.lock();
        if (!impl->planningPins || impl->planningPins->operation != impl->operation) {
            impl->planningPins = std::make_shared<ScopeState>();
            impl->planningPins->registry = registry;
            impl->planningPins->operation = impl->operation;
        }
        impl->planningPins->ownerDocuments.reserve(impl->planningPins->ownerDocuments.size() + 1);
    }
    impl->readers.reserve(1);
    impl->operation->documents.reserve(impl->operation->documents.size() + 1);
    impl->operation->executionDocuments.reserve(impl->operation->executionDocuments.size() + 1);
    {
        std::lock_guard lock(registry->mutex);
        auto found = registry->documents.find(document);
        if (found == registry->documents.end() || !found->second->live
            || (found->second->retiring && !found->second->pins.count(impl->operation->id))
            || (found->second->writerDepth
                && !(impl->allowOwnedWriter && found->second->writer == impl->operation->id))) {
            throw AdmissionRefusal("Graph capture conflicts with a topology writer");
        }
        if (planning) {
            if (found->second->executionOwner
                && found->second->executionOwner != impl->operation->id) {
                throw AdmissionRefusal("Planning capture conflicts with another native operation");
            }
        }
        else {
            reserveExecutionDocument(impl->operation, found->second);
        }
        if (planning) {
            pinScopeOwner(impl->planningPins, nullptr, found->second);
        }
        else {
            pinDocument(impl->operation, found->second);
        }
        ++found->second->readers;
        impl->readers.push_back(found->second);
    }
    impl->previous = activeCapture;
    activeCapture = this;
}
CaptureScope::~CaptureScope()
{
    if (!impl) {
        return;
    }
    activeCapture = impl->previous;
    {
        std::lock_guard lock(impl->registry->mutex);
        for (const auto& document : impl->readers) {
            --document->readers;
        }
    }
    // Standalone planning borrows end here; setter borrows remain in its
    // lexical writer scope. Execution captures retain their native pins.
    impl->planningPins.reset();
    wake(impl->registry);
}
void CaptureScope::pin(DocumentObject* pointer)
{
    if (!pointer) {
        return;
    }
    auto operation = impl->operation;
    if (operation->canceled.load(std::memory_order_acquire)) {
        throw Base::UserAbortException();
    }
    operation->objects.reserve(operation->objects.size() + 1);
    operation->documents.reserve(operation->documents.size() + 1);
    operation->executionDocuments.reserve(operation->executionDocuments.size() + 1);
    impl->readers.reserve(impl->readers.size() + 1);
    if (impl->planningPins) {
        impl->planningPins->ownerObjects.reserve(impl->planningPins->ownerObjects.size() + 1);
        impl->planningPins->ownerDocuments.reserve(impl->planningPins->ownerDocuments.size() + 1);
    }
    std::lock_guard lock(impl->registry->mutex);
    if (operation->canceled.load(std::memory_order_relaxed)) {
        throw Base::UserAbortException();
    }
    auto found = impl->registry->objects.find(pointer);
    if (found == impl->registry->objects.end() || !found->second->live || !found->second->document->live
        || (found->second->document->retiring && !found->second->document->pins.count(operation->id))) {
        throw AdmissionRefusal("Dependency attachment retired during graph capture");
    }
    auto object = found->second;
    if (object->pendingRemoval
        && std::find(operation->objects.begin(), operation->objects.end(), object)
            == operation->objects.end()) {
        throw AdmissionRefusal("Dependency has a pending exact-generation removal");
    }
    auto doc = object->document;
    if (std::find(impl->readers.begin(), impl->readers.end(), doc) == impl->readers.end()) {
        if (doc->writerDepth && !(impl->allowOwnedWriter && doc->writer == operation->id)) {
            throw AdmissionRefusal("Dependency topology is busy");
        }
        if (impl->planning) {
            if (doc->executionOwner && doc->executionOwner != operation->id) {
                throw AdmissionRefusal("Dependency has another native operation");
            }
        }
        else {
            reserveExecutionDocument(operation, doc);
        }
        ++doc->readers;
        impl->readers.push_back(doc);
    }
    if (impl->planningPins) {
        pinScopeOwner(impl->planningPins, object, doc);
    }
    else {
        pinObject(operation, object);
    }
}
void CaptureScope::finish(const std::vector<DocumentObject*>& objects)
{
    for (auto* object : objects) {
        pin(object);
    }
    if (impl->operation->worker && !impl->planning) {
        for (auto* object : objects) {
            if (!object->canRecomputeOnWorker()) {
                throw AdmissionRefusal("Actual scheduled vector contains a worker-ineligible feature");
            }
        }
    }
}
void CaptureScope::pinCurrent(DocumentObject* object)
{
    if (activeCapture) {
        activeCapture->pin(object);
    }
}
RouteDisposition Coordinator::classify(const BindingPtr& binding, bool recursive, bool checkWorker)
{
    if (borrowedContext()) {
        return RouteDisposition::Parked;
    }
    {
        std::lock_guard lock(impl->registry->mutex);
        // Queue routing must not borrow its enclosing writer/atomic authority.
        // Explicit synchronous requests have a separate, qualified owner route.
        if (impl->registry->atomicOwner
            || (current.operation && impl->registry->writerOperations.count(currentId()))) {
            return RouteDisposition::Parked;
        }
    }
    auto result = claim(binding, Route::Owner);
    if (result.disposition == Disposition::Unavailable) {
        return RouteDisposition::Unavailable;
    }
    if (result.disposition != Disposition::Ready) {
        return RouteDisposition::Parked;
    }
    if (!checkWorker) {
        return RouteDisposition::OwnerReady;
    }
    auto context = result.claim.enter();
    try {
        CaptureScope capture(result.claim.document(), true);
        std::vector<DocumentObject*> roots;
        if (result.claim.object()) {
            roots.push_back(result.claim.object());
        }
        else {
            const auto& objects = result.claim.document()->getObjects();
            roots.assign(objects.begin(), objects.end());
        }
        auto objects = binding->objectTarget && !recursive
            ? roots
            : Document::getDependencyList(roots, Document::DepSort);
        capture.finish(objects);
        for (auto* object : objects) {
            if (!object->canRecomputeOnWorker()) {
                return RouteDisposition::KnownUnsafe;
            }
        }
        return RouteDisposition::WorkerEligible;
    }
    catch (const AdmissionRefusal&) {
        return RouteDisposition::Parked;
    }
    catch (const Base::BadGraphError&) {
        return RouteDisposition::KnownUnsafe;
    }
}

void Coordinator::registerProperty(Property* pointer, DocumentObject* object, Document* doc)
{
    auto record = std::make_shared<PropertyRecord>();
    record->pointer = pointer;
    auto registry = impl->registry;
    std::lock_guard lock(registry->mutex);
    auto previous = registry->properties.find(pointer);
    if (previous != registry->properties.end()) {
        record = previous->second;
    }
    const auto nativePinned = [&registry](const auto& owner) {
        return owner
            && std::any_of(owner->pins.begin(), owner->pins.end(), [&registry](const auto& pin) {
                   return pin.second && registry->native.count(pin.first);
               });
    };
    record->storageObjects.erase(
        std::remove_if(
            record->storageObjects.begin(),
            record->storageObjects.end(),
            [&](const auto& owner) { return !nativePinned(owner); }
        ),
        record->storageObjects.end()
    );
    record->storageDocuments.erase(
        std::remove_if(
            record->storageDocuments.begin(),
            record->storageDocuments.end(),
            [&](const auto& owner) { return !nativePinned(owner); }
        ),
        record->storageDocuments.end()
    );
    if (nativePinned(record->object)
        && std::find(record->storageObjects.begin(), record->storageObjects.end(), record->object)
            == record->storageObjects.end()) {
        record->storageObjects.push_back(record->object);
    }
    if (nativePinned(record->document)
        && std::find(record->storageDocuments.begin(), record->storageDocuments.end(), record->document)
            == record->storageDocuments.end()) {
        record->storageDocuments.push_back(record->document);
    }
    record->live = true;
    record->objectKey = object;
    record->documentKey = doc;
    record->object.reset();
    record->document.reset();
    if (object) {
        auto found = registry->objects.find(object);
        if (found != registry->objects.end()) {
            record->object = found->second;
            record->document = found->second->document;
        }
    }
    else if (doc) {
        auto found = registry->documents.find(doc);
        if (found != registry->documents.end()) {
            record->document = found->second;
        }
    }
    else {
        record->object.reset();
        record->document.reset();
    }
    registry->properties[pointer] = record;
}
bool Coordinator::retireProperty(Property* pointer)
{
    auto registry = impl->registry;
    std::lock_guard lock(registry->mutex);
    auto found = registry->properties.find(pointer);
    if (found == registry->properties.end()) {
        return true;
    }
    const auto& record = found->second;
    record->live = false;
    const auto nativePinned = [&registry](const auto& owner) {
        return owner
            && std::any_of(owner->pins.begin(), owner->pins.end(), [&registry](const auto& pin) {
                   return pin.second && registry->native.count(pin.first);
               });
    };
    if (record->pins || nativePinned(record->object) || nativePinned(record->document)
        || std::any_of(record->storageObjects.begin(), record->storageObjects.end(), nativePinned)
        || std::any_of(record->storageDocuments.begin(), record->storageDocuments.end(), nativePinned)) {
        return false;
    }
    registry->properties.erase(found);
    return true;
}
PropertySnapshot Coordinator::snapshotProperties(
    const std::vector<Property*>& pointers,
    const Document* excludedDocument,
    const DocumentObject* excludedObject,
    bool collectTopology
)
{
    PropertySnapshot snapshot;
    snapshot.values.reserve(pointers.size());
    snapshot.reservation.state = std::make_shared<ScopeState>();
    auto scope = snapshot.reservation.state;
    scope->registry = impl->registry;
    scope->operation = current.authority && current.operation ? current.operation
                                                              : newOperation(impl->registry);
    scope->properties.reserve(pointers.size() * 2);
    scope->operation->objects.reserve(scope->operation->objects.size() + pointers.size() * 2);
    scope->operation->documents.reserve(scope->operation->documents.size() + pointers.size() * 2);
    snapshot.reservation.context = std::unique_ptr<OperationScope>(
        new OperationScope(scope->operation, true)
    );
    current.mutation = scope;
    std::vector<std::shared_ptr<PropertyRecord>> records;
    std::vector<std::shared_ptr<DocumentRecord>> documents;
    std::vector<std::shared_ptr<ObjectRecord>> objects;
    std::unordered_set<Property*> excluded;
    records.reserve(pointers.size() * 2);
    documents.reserve(pointers.size());
    objects.reserve(pointers.size());
    {
        std::lock_guard lock(impl->registry->mutex);
        for (auto* pointer : pointers) {
            auto found = impl->registry->properties.find(pointer);
            if (found == impl->registry->properties.end() || !found->second->live) {
                continue;
            }
            auto record = found->second;
            const bool excludeOwner = (excludedDocument && record->document
                                       && record->document->pointer == excludedDocument)
                || (excludedObject && record->object && record->object->pointer == excludedObject);
            if (!excludeOwner
                && ((record->document && !record->document->live)
                    || (record->object && !record->object->live))) {
                continue;
            }
            snapshot.values.push_back(pointer);
            while (record && std::find(records.begin(), records.end(), record) == records.end()) {
                records.push_back(record);
                if (excludeOwner) {
                    excluded.insert(record->pointer);
                }
                else {
                    if (record->object) {
                        if (foreignPins(record->object->pins, scope->operation->id)) {
                            throw AdmissionRefusal("Property owner is leased by another operation");
                        }
                        objects.push_back(record->object);
                    }
                    if (record->document
                        && std::find(documents.begin(), documents.end(), record->document)
                            == documents.end()) {
                        documents.push_back(record->document);
                    }
                }
                auto parent = impl->registry->properties.find(record->parentKey);
                record = parent == impl->registry->properties.end() ? nullptr : parent->second;
            }
        }
        const auto id = scope->operation->id;
        if (impl->registry->atomicOwner && impl->registry->atomicOwner != id) {
            throw AdmissionRefusal("Property snapshot conflicts with an atomic operation");
        }
        for (const auto& document : documents) {
            if ((!document->live && document->closeOwner != id)
                || (document->retiring && document->closeOwner != id && !document->pins.count(id))
                || document->readers || (document->writerDepth && document->writer != id)) {
                throw AdmissionRefusal("Property snapshot source is busy");
            }
        }
        scope->documents.reserve(documents.size());
        scope->properties.reserve(records.size());
        scope->ownerObjects.reserve(objects.size());
        scope->ownerDocuments.reserve(documents.size());
        // No rejected snapshot publishes even a transient property/ancestor pin.
        if (!documents.empty()) {
            ++impl->registry->writers;
            ++impl->registry->writerOperations[id];
        }
        for (const auto& document : documents) {
            document->writer = id;
            ++document->writerDepth;
            scope->documents.push_back(document);
        }
        for (const auto& record : records) {
            ++record->pins;
            scope->properties.push_back(record);
        }
        for (const auto& object : objects) {
            pinScopeOwner(scope, object, object->document);
        }
        for (const auto& document : documents) {
            pinScopeOwner(scope, nullptr, document);
        }
        updateReady(*impl->registry);
    }
    if (collectTopology) {
        std::vector<DocumentObject*> endpoints;
        for (auto* property : snapshot.values) {
            if (auto* link = dynamic_cast<PropertyLinkBase*>(property)) {
                link->collectAdmissionTargets(endpoints);
            }
        }
        snapshot.reservation.include(endpoints);
    }
    return snapshot;
}

bool Coordinator::deferRemoval(DocumentObject* pointer)
{
    auto registry = impl->registry;
    bool result = false;
    {
        std::lock_guard lock(registry->mutex);
        auto found = registry->objects.find(pointer);
        if (found != registry->objects.end() && pinned(found->second->pins)) {
            if (!found->second->pendingRemoval) {
                found->second->pendingRemoval = true;
                registry->removals.push_back(found->second);
            }
            result = true;
        }
    }
    if (result) {
        wake(registry);
    }
    return result;
}
void Coordinator::drainDeferred()
{
    auto registry = impl->registry;
    if (MainThreadSignalConfig::hasHooks() && !MainThreadSignalConfig::isMainThread()) {
        {
            std::lock_guard lock(registry->mutex);
            if (registry->drainScheduled || !registry->ready.load(std::memory_order_relaxed)) {
                return;
            }
            registry->drainScheduled = true;
            updateReady(*registry);
        }
        try {
            MainThreadSignalConfig::invoke(
                [this, registry]() {
                    {
                        std::lock_guard lock(registry->mutex);
                        registry->drainScheduled = false;
                    }
                    drainDeferred();
                },
                false
            );
        }
        catch (...) {
            {
                std::lock_guard lock(registry->mutex);
                registry->drainScheduled = false;
            }
            wake(registry);
            throw;
        }
        return;
    }
    std::vector<std::shared_ptr<ObjectRecord>> removals;
    std::shared_ptr<std::function<void()>> propertyDrain;
    {
        std::lock_guard lock(registry->mutex);
        if (!registry->native.empty() || registry->atomicOwner || registry->writers) {
            return;
        }
        removals.swap(registry->removals);
        if (registry->cleanupRequested) {
            propertyDrain = registry->propertyDrain;
        }
        registry->cleanupRequested = false;
        updateReady(*registry);
    }
    for (const auto& object : removals) {
        bool remove = false;
        {
            std::lock_guard lock(registry->mutex);
            remove = object->live && object->document->live && !object->document->retiring
                && !pinned(object->pins);
            if (!remove && object->live && object->document->live) {
                registry->removals.push_back(object);
            }
        }
        if (!remove) {
            continue;
        }
        try {
            auto scope = mutation({object->pointer}, {object->document->pointer}, true);
            {
                std::lock_guard lock(registry->mutex);
                auto live = registry->objects.find(object->pointer);
                if (!object->live || !object->document->live || live == registry->objects.end()
                    || live->second != object) {
                    continue;
                }
            }
            // Writer reservation prevents a new lease/close while calling the normal API.
            object->document->pointer->removeObject(object->name.c_str());
        }
        catch (const AdmissionRefusal&) {
            std::lock_guard lock(registry->mutex);
            if (object->live && object->document->live) {
                registry->removals.push_back(object);
            }
        }
        catch (const Base::Exception& exception) {
            exception.reportException();
        }
    }
    if (propertyDrain && *propertyDrain) {
        (*propertyDrain)();
    }
    {
        std::lock_guard lock(registry->mutex);
        updateReady(*registry);
    }
}
struct CloseScope::Impl
{
    std::shared_ptr<Registry> registry;
    std::shared_ptr<DocumentRecord> document;
    OperationId owner {};
    std::unordered_set<OperationId> affected;
    bool committed {false};
    bool callerMayWait {false};
    std::shared_ptr<OperationState> operation;
    std::unique_ptr<OperationScope> context;
    std::optional<MutationScope> reservation;
    std::optional<PropertySnapshot> outgoing;
    std::optional<PropertySnapshot> incoming;
    ~Impl()
    {
        if (!document) {
            return;
        }
        if (!committed) {
            {
                std::lock_guard lock(registry->mutex);
                if (document->live && document->closeOwner == owner) {
                    document->retiring = false;
                    document->closeOwner = 0;
                }
            }
            wake(registry);
        }
    }
};
CloseScope::~CloseScope() = default;
CloseScope::operator bool() const noexcept
{
    return static_cast<bool>(impl);
}
bool CloseScope::affects(OperationId operation) const
{
    if (!impl || !operation) {
        return false;
    }
    std::lock_guard lock(impl->registry->mutex);
    return impl->affected.count(operation) || impl->document->pins.count(operation);
}
void CloseScope::ensureWaitAllowed() const
{
    if (!impl || !impl->callerMayWait) {
        throw AdmissionRefusal(
            "Cannot wait for document close from an enclosing native or writer operation"
        );
    }
}
void CloseScope::cancelAffected()
{
    if (!impl) {
        return;
    }
    // A promoted weak operation can become its last strong owner while its
    // native thread returns. Its destructor takes R, so keep every promotion
    // alive beyond R, including exceptional progress-snapshot unwinding.
    std::vector<std::shared_ptr<OperationState>> promoted;
    std::vector<std::shared_ptr<ProgressRegistration>> progress;
    {
        std::lock_guard lock(impl->registry->mutex);
        std::size_t capacity = 0;
        for (const auto& item : impl->registry->operations) {
            if (impl->affected.count(item.first) || impl->document->pins.count(item.first)) {
                capacity += item.second.size();
            }
        }
        // No promotion exists yet if allocation fails. The subsequent shared
        // pointer moves cannot allocate or release the final owner under R.
        promoted.reserve(capacity);
        for (auto& item : impl->registry->operations) {
            if (!impl->affected.count(item.first) && !impl->document->pins.count(item.first)) {
                continue;
            }
            for (auto& weak : item.second) {
                if (auto operation = weak.lock()) {
                    promoted.push_back(std::move(operation));
                    const auto& retained = promoted.back();
                    retained->canceled.store(true, std::memory_order_release);
                    if (retained->progress) {
                        progress.push_back(retained->progress);
                    }
                }
            }
        }
    }
    for (auto registration : progress) {
        for (; registration; registration = registration->next) {
            if (auto handle = registration->progress.lock()) {
                handle->cancel();
            }
        }
    }
}
bool CloseScope::ready() const
{
    if (!impl) {
        return false;
    }
    {
        std::lock_guard lock(impl->registry->mutex);
        if (pinned(impl->document->pins) || impl->document->readers
            || (impl->document->writerDepth && impl->document->writer != impl->owner)) {
            return false;
        }
    }
    if (impl->reservation) {
        return true;
    }
    try {
        auto reservation = Coordinator::instance().mutation({}, {impl->document->pointer});
        std::vector<DocumentObject*> targets;
        for (auto* object : impl->document->pointer->getObjects()) {
            targets.push_back(object);
        }
        std::vector<Property*> ownProperties;
        for (auto* object : targets) {
            object->getPropertyList(ownProperties);
        }
        auto outgoing = Coordinator::instance().snapshotProperties(
            ownProperties,
            impl->document->pointer,
            nullptr,
            false
        );
        std::vector<DocumentObject*> oldTargets;
        for (auto* property : outgoing.properties()) {
            if (auto* link = dynamic_cast<PropertyLinkBase*>(property)) {
                link->collectAdmissionTargets(oldTargets);
            }
        }
        {
            std::lock_guard lock(impl->registry->mutex);
            for (auto* target : oldTargets) {
                auto record = impl->registry->objects.find(target);
                if (record != impl->registry->objects.end()) {
                    for (const auto& pin : record->second->pins) {
                        if (pin.second) {
                            impl->affected.insert(pin.first);
                            if (observedOperation(pin.first)) {
                                return false;
                            }
                        }
                    }
                }
            }
        }
        reservation.include(oldTargets);
        auto incoming
            = PropertyLinkBase::incomingAdmissionProperties(targets, {impl->document->pointer});
        {
            std::lock_guard lock(impl->registry->mutex);
            std::erase_if(incoming, [&](Property* pointer) {
                auto record = impl->registry->properties.find(pointer);
                return record == impl->registry->properties.end() || !record->second->document
                    || record->second->document == impl->document;
            });
        }
        auto sources = Coordinator::instance().snapshotProperties(incoming, impl->document->pointer);
        // Source/storage reservation survives every delete-document observer and native unlink.
        outgoing.releaseStorage();
        sources.releaseStorage();
        impl->outgoing.emplace(std::move(outgoing));
        impl->incoming.emplace(std::move(sources));
        impl->reservation.emplace(std::move(reservation));
        return true;
    }
    catch (const AdmissionRefusal&) {
        return false;
    }
}
void CloseScope::commit()
{
    if (!impl) {
        throw AdmissionRefusal("Document close was not admitted");
    }
    if (impl->outgoing) {
        impl->outgoing->releaseStorage();
    }
    std::lock_guard lock(impl->registry->mutex);
    if (pinned(impl->document->pins) || impl->document->readers) {
        throw AdmissionRefusal("Document still has native recompute leases");
    }
    impl->committed = true;
    impl->document->live = false;
}
CloseScope Coordinator::tryClose(const BindingPtr& binding)
{
    CloseScope result;
    if (!binding || binding->missing) {
        return result;
    }
    auto state = std::make_shared<CloseScope::Impl>();
    state->registry = impl->registry;
    state->operation = newOperation(impl->registry);
    state->callerMayWait = true;
    for (const CurrentContext* context = &current; context;
         context = context->previous ? &context->previous->context : nullptr) {
        if (context->operation) {
            state->callerMayWait = false;
            break;
        }
    }
    std::vector<DocumentObject*> targets;
    {
        std::lock_guard lock(impl->registry->mutex);
        auto found = impl->registry->documentGenerations.find(binding->documentGeneration);
        if (found == impl->registry->documentGenerations.end() || !found->second->live
            || found->second->retiring || observedDocument(found->second)) {
            return result;
        }
        auto document = found->second;
        if (document->writer && observedOperation(document->writer)) {
            return result;
        }
        state->owner = state->operation->id;
        state->document = document;
        for (const auto& object : impl->registry->objects) {
            if (object.second->document == document && object.second->live) {
                targets.push_back(object.second->pointer);
            }
        }
        for (const auto& pin : document->pins) {
            if (pin.second) {
                state->affected.insert(pin.first);
            }
        }
        if (document->writerDepth) {
            state->affected.insert(document->writer);
        }
        document->retiring = true;
        document->closeOwner = state->owner;
    }
    // Copy the reverse index with I only, then inspect registered owners with R.
    // No native getter races an existing writer on the document being closed.
    auto incoming = PropertyLinkBase::incomingAdmissionProperties(targets, {state->document->pointer});
    bool ownIncoming = false;
    {
        std::lock_guard lock(impl->registry->mutex);
        for (auto* pointer : incoming) {
            auto property = impl->registry->properties.find(pointer);
            if (property == impl->registry->properties.end() || !property->second->live) {
                continue;
            }
            auto source = property->second->object;
            if (source) {
                for (const auto& pin : source->pins) {
                    if (pin.second) {
                        state->affected.insert(pin.first);
                        ownIncoming = ownIncoming || observedOperation(pin.first);
                    }
                }
            }
            auto document = property->second->document;
            if (document && document->writerDepth) {
                state->affected.insert(document->writer);
                ownIncoming = ownIncoming || observedOperation(document->writer);
            }
        }
    }
    if (ownIncoming) {
        return result;
    }  // Close intent rolls back before cancellation.
    state->context = std::unique_ptr<OperationScope>(new OperationScope(state->operation, true));
    result.impl = std::move(state);
    return result;
}
bool Coordinator::isObserved(const BindingPtr& binding) const
{
    if (!binding) {
        return false;
    }
    std::lock_guard lock(impl->registry->mutex);
    auto document = impl->registry->documentGenerations.find(binding->documentGeneration);
    return document != impl->registry->documentGenerations.end()
        && observedDocument(document->second);
}
std::uint64_t Coordinator::epoch() const noexcept
{
    return impl->registry->epoch.load(std::memory_order_acquire);
}
void Coordinator::setWake(std::function<void()> callback)
{
    auto owned = std::make_shared<std::function<void()>>(std::move(callback));
    std::shared_ptr<std::function<void()>> retired;
    {
        std::lock_guard lock(impl->registry->mutex);
        retired = std::exchange(impl->registry->wake, std::move(owned));
        impl->registry->stopping = false;
    }
}
void Coordinator::clearWake()
{
    std::shared_ptr<std::function<void()>> retired;
    std::unique_lock lock(impl->registry->mutex);
    retired.swap(impl->registry->wake);
    impl->registry->wakeDone.wait(lock, [this]() { return impl->registry->wakeActive == 0; });
    lock.unlock();
}
void Coordinator::setPropertyDrain(std::function<void()> callback)
{
    auto owned = std::make_shared<std::function<void()>>(std::move(callback));
    std::shared_ptr<std::function<void()>> retired;
    {
        std::lock_guard lock(impl->registry->mutex);
        retired = std::exchange(impl->registry->propertyDrain, std::move(owned));
    }
}
void Coordinator::requestCleanup()
{
    {
        std::lock_guard lock(impl->registry->mutex);
        impl->registry->cleanupRequested = true;
    }
    wake(impl->registry);
}
bool Coordinator::hasOwnerNativeWork() const noexcept
{
    return impl->registry->ownerNative.load(std::memory_order_acquire) != 0;
}
bool Coordinator::hasDeferredWork() const noexcept
{
    return impl->registry->ready.load(std::memory_order_acquire);
}
bool Coordinator::tryBeginShutdown() noexcept
{
    std::lock_guard lock(impl->registry->mutex);
    if (impl->registry->ownerNative.load(std::memory_order_relaxed) || impl->registry->atomicOwner
        || std::any_of(
            impl->registry->resourceWriters.begin(),
            impl->registry->resourceWriters.end(),
            [this](const auto& entry) { return !impl->registry->native.count(entry.second.first); }
        )
        || std::any_of(
            impl->registry->writerOperations.begin(),
            impl->registry->writerOperations.end(),
            [this](const auto& owner) { return !impl->registry->native.count(owner.first); }
        )) {
        return false;
    }
    impl->registry->stopping = true;
    updateReady(*impl->registry);
    return true;
}
void Coordinator::shutdown()
{
    {
        std::lock_guard lock(impl->registry->mutex);
        impl->registry->stopping = true;
    }
    wake(impl->registry);
}
ClosureToken Coordinator::captureClosure(int id, const std::vector<Document*>& documents)
{
    if (!id) {
        return {};
    }
    auto registry = impl->registry;
    std::lock_guard lock(registry->mutex);
    auto found = registry->closures.find(id);
    if (found == registry->closures.end() || !found->second.live) {
        registry->closures[id] = ClosureRecord {ClosureToken {registry->next++, id}};
    }
    auto& record = registry->closures.at(id);
    for (auto* document : documents) {
        auto doc = registry->documents.find(document);
        if (doc != registry->documents.end()
            && std::find(record.documents.begin(), record.documents.end(), doc->second->generation)
                == record.documents.end()) {
            record.documents.push_back(doc->second->generation);
        }
    }
    return record.token;
}
bool Coordinator::closureCurrent(const ClosureToken& token) const
{
    std::lock_guard lock(impl->registry->mutex);
    auto found = impl->registry->closures.find(token.transactionId);
    return found != impl->registry->closures.end() && found->second.live
        && found->second.token.serial == token.serial;
}
void Coordinator::retireClosure(int id)
{
    std::lock_guard lock(impl->registry->mutex);
    auto found = impl->registry->closures.find(id);
    if (found != impl->registry->closures.end()) {
        found->second.live = false;
        found->second.pending = false;
    }
}
void Coordinator::markClosureClosing(int id, bool closing)
{
    std::lock_guard lock(impl->registry->mutex);
    auto found = impl->registry->closures.find(id);
    if (found != impl->registry->closures.end()) {
        found->second.closing = closing;
    }
}
bool Coordinator::isClosureClosing(int id) const
{
    std::lock_guard lock(impl->registry->mutex);
    auto found = impl->registry->closures.find(id);
    return found != impl->registry->closures.end() && found->second.live && found->second.closing;
}
void Coordinator::adoptClosure(const ClosureToken& token, bool abort) noexcept
{
    {
        std::lock_guard lock(impl->registry->mutex);
        auto found = impl->registry->closures.find(token.transactionId);
        if (found == impl->registry->closures.end() || !found->second.live
            || found->second.token.serial != token.serial) {
            return;
        }
        found->second.pending = true;
        found->second.abort = abort;
    }
    wake(impl->registry);
}
void Coordinator::drainClosures(const std::function<bool(const ClosureToken&, bool)>& callback)
{
    std::vector<std::pair<ClosureToken, bool>> pending;
    {
        std::lock_guard lock(impl->registry->mutex);
        if (!impl->registry->native.empty() || impl->registry->writers
            || impl->registry->atomicOwner) {
            return;
        }
        for (const auto& item : impl->registry->closures) {
            if (item.second.live && item.second.pending) {
                pending.emplace_back(item.second.token, item.second.abort);
            }
        }
    }
    bool blocked = false;
    for (const auto& item : pending) {
        if (closureCurrent(item.first) && callback(item.first, item.second)) {
            std::lock_guard lock(impl->registry->mutex);
            auto found = impl->registry->closures.find(item.first.transactionId);
            if (found != impl->registry->closures.end()
                && found->second.token.serial == item.first.serial) {
                found->second.live = false;
                found->second.pending = false;
            }
        }
        else {
            blocked = true;
        }
    }
    {
        std::lock_guard lock(impl->registry->mutex);
        impl->registry->drainBlocked = blocked;
        updateReady(*impl->registry);
    }
}
void Coordinator::pinRoots(const std::vector<DocumentObject*>& roots)
{
    if (!current.operation || !current.authority) {
        throw AdmissionRefusal("No native root operation");
    }
    auto operation = current.operation;
    operation->objects.reserve(operation->objects.size() + roots.size());
    operation->documents.reserve(operation->documents.size() + roots.size());
    std::lock_guard lock(impl->registry->mutex);
    for (auto* pointer : roots) {
        auto object = impl->registry->objects.find(pointer);
        if (object == impl->registry->objects.end() || !object->second->live) {
            throw AdmissionRefusal("Recompute root attachment is unavailable");
        }
        if (object->second->pendingRemoval
            && std::find(operation->objects.begin(), operation->objects.end(), object->second)
                == operation->objects.end()) {
            throw AdmissionRefusal("Requested root has a pending exact-generation removal");
        }
        reserveExecutionDocument(operation, object->second->document);
        pinObject(operation, object->second);
    }
}
void Coordinator::setPropertyParent(Property* property, Property* parent)
{
    std::lock_guard lock(impl->registry->mutex);
    auto found = impl->registry->properties.find(property);
    if (found != impl->registry->properties.end()) {
        found->second->parentKey = parent;
    }
}
void Coordinator::ensurePropertyStorageAvailable(Property* property) const
{
    std::lock_guard lock(impl->registry->mutex);
    auto found = impl->registry->properties.find(property);
    if (found != impl->registry->properties.end() && found->second->pins) {
        throw AdmissionRefusal("Property storage is held by a native reader snapshot");
    }
}
}  // namespace App::RecomputeDetail
