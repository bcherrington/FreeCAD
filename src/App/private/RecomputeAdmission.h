// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 FreeCAD contributors
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <FCGlobal.h>
#include <Base/Exception.h>

namespace App
{
class Document;
class DocInfo;
class DocumentObject;
class Property;
class PropertyContainer;
class RecomputeProgressHandle;
namespace RecomputeDetail
{
using Generation = std::uint64_t;
using OperationId = std::uint64_t;

// Internal transport, deliberately mapped to the existing public Exception result.
class AppExport AdmissionRefusal: public Base::RuntimeError
{
public:
    enum class Reason
    {
        Busy,
        Unavailable
    };
    explicit AdmissionRefusal(const char* message, Reason = Reason::Busy);
    Reason reason() const noexcept
    {
        return failureReason;
    }

private:
    Reason failureReason;
};

class AppExport Retirement
{
public:
    bool retired() const noexcept
    {
        return done.load(std::memory_order_acquire);
    }
    void markRetired() noexcept
    {
        done.store(true, std::memory_order_release);
    }

private:
    std::atomic<bool> done {false};
};
using RetirementPtr = std::shared_ptr<Retirement>;

struct AppExport Binding
{
    const Generation documentGeneration;
    const Generation attachmentGeneration;
    const std::string documentName;
    const std::string objectName;
    const bool objectTarget;
    const bool missing;

    Binding(Generation, Generation, std::string, std::string, bool, bool);
};
using BindingPtr = std::shared_ptr<const Binding>;

enum class Route
{
    Owner,
    Worker
};
enum class Disposition
{
    Ready,
    Parked,
    Unavailable
};
enum class RouteDisposition
{
    WorkerEligible,
    OwnerReady,
    KnownUnsafe,
    Parked,
    Unavailable
};
struct OperationState;
struct ScopeState;

class AppExport OperationScope
{
public:
    OperationScope(OperationScope&&) noexcept;
    ~OperationScope();
    OperationScope(const OperationScope&) = delete;
    OperationScope& operator=(const OperationScope&) = delete;

private:
    friend class Coordinator;
    friend class NativeClaim;
    friend class ObservationScope;
    friend class MutationScope;
    OperationScope(std::shared_ptr<OperationState>, bool authority, bool workerDispatch = false);
    struct Impl;
    std::unique_ptr<Impl> impl;
};

class AppExport NativeClaim
{
public:
    NativeClaim() = default;
    NativeClaim(NativeClaim&&) noexcept = default;
    NativeClaim& operator=(NativeClaim&&) noexcept = default;
    ~NativeClaim();
    NativeClaim(const NativeClaim&) = delete;
    NativeClaim& operator=(const NativeClaim&) = delete;
    explicit operator bool() const noexcept;
    Document* document() const noexcept;
    DocumentObject* object() const noexcept;
    OperationId operation() const noexcept;
    OperationScope enter() const;
    void reset() noexcept;
    void registerProgress(std::shared_ptr<RecomputeProgressHandle>);
    bool cancellationRequested() const noexcept;

private:
    friend class Coordinator;
    std::shared_ptr<OperationState> state;
    Document* rootDocument {};
    DocumentObject* rootObject {};
};

struct AppExport ClaimResult
{
    Disposition disposition {Disposition::Parked};
    std::size_t candidate {};
    NativeClaim claim;
    std::uint64_t epoch {};
};

class AppExport MutationScope
{
public:
    MutationScope() = default;
    MutationScope(MutationScope&&) noexcept = default;
    MutationScope& operator=(MutationScope&&) noexcept;
    ~MutationScope();
    MutationScope(const MutationScope&) = delete;
    MutationScope& operator=(const MutationScope&) = delete;
    explicit operator bool() const noexcept;
    // Expansion is for preflight only: call BEFORE the first native write.
    void include(const std::vector<DocumentObject*>&, const std::vector<Document*>& = {});
    void reserveResource(std::shared_ptr<void>);
    std::vector<DocumentObject*> pinBindings(const std::vector<BindingPtr>&);

private:
    friend class Coordinator;
    friend class PropertySnapshot;
    std::shared_ptr<ScopeState> state;
    std::unique_ptr<OperationScope> context;
};

class AppExport CaptureScope
{
public:
    explicit CaptureScope(Document*, bool planning = false);
    ~CaptureScope();
    CaptureScope(const CaptureScope&) = delete;
    CaptureScope& operator=(const CaptureScope&) = delete;
    void pin(DocumentObject*);
    void finish(const std::vector<DocumentObject*>&);
    static void pinCurrent(DocumentObject*);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

using Observation = std::shared_ptr<OperationState>;
class AppExport ObservationScope
{
public:
    explicit ObservationScope(Observation);

private:
    OperationScope scope;
};

// Index lock is ONLY for copying/updating the small shared metadata containers.
// Release it before getters, callbacks, signals, destruction or registry entry.
class AppExport IndexLock
{
public:
    IndexLock();
    ~IndexLock();
    IndexLock(const IndexLock&) = delete;
    IndexLock& operator=(const IndexLock&) = delete;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

class AppExport PropertySnapshot
{
public:
    PropertySnapshot() = default;
    PropertySnapshot(PropertySnapshot&&) noexcept = default;
    PropertySnapshot& operator=(PropertySnapshot&&) noexcept = default;
    ~PropertySnapshot();
    const std::vector<Property*>& properties() const noexcept;
    void releaseStorage();
    void requireExclusiveStorage() const;
    void requireExclusiveStorage(const std::vector<Property*>&) const;

private:
    friend class Coordinator;
    std::vector<Property*> values;
    MutationScope reservation;
};

struct AppExport ClosureToken
{
    Generation serial {};
    int transactionId {};
};

class AppExport CloseScope
{
public:
    CloseScope() = default;
    CloseScope(CloseScope&&) noexcept = default;
    CloseScope& operator=(CloseScope&&) noexcept = default;
    ~CloseScope();
    explicit operator bool() const noexcept;
    bool ready() const;
    bool affects(OperationId) const;
    void cancelAffected();
    void ensureWaitAllowed() const;
    void commit();

private:
    friend class Coordinator;
    struct Impl;
    std::shared_ptr<Impl> impl;
};

class AppExport Coordinator
{
public:
    static Coordinator& instance();
    void registerDocument(Document*, const std::string&);
    void retireDocument(Document*);
    void registerAttachment(Document*, DocumentObject*, const std::string&);
    void retireAttachment(DocumentObject*);
    BindingPtr bindDocument(const Document*) const;
    BindingPtr bindObject(const DocumentObject*) const;
    BindingPtr bindNames(const std::string&, const std::string&, bool objectTarget) const;
    bool coherent(const BindingPtr&, const std::string&, const std::string&) const;
    Document* resolveDocument(const BindingPtr&) const;
    DocumentObject* resolveObject(const BindingPtr&) const;
    // One R phase chooses/pins the first admissible snapshot entry, BEFORE Q pop.
    ClaimResult claimFirst(const std::vector<BindingPtr>&, Route);
    ClaimResult claim(const BindingPtr&, Route);
    RouteDisposition classify(const BindingPtr&, bool recursive = true, bool checkWorker = true);
    // Native owner entrypoints called outside Application still participate.
    NativeClaim claimDocument(Document*, DocumentObject* = nullptr);
    void pinRoots(const std::vector<DocumentObject*>&);

    MutationScope mutation(
        const std::vector<DocumentObject*>&,
        const std::vector<Document*>& = {},
        bool destructive = false
    );
    MutationScope mutation(const Property&, const std::vector<DocumentObject*>& proposed = {});
    // Lexical receiver pin within an existing admitted owner operation.
    MutationScope pinDocumentLifetime(Document*);
    std::optional<MutationScope> tryAtomic();
    MutationScope atomic();
    // Import creates new objects and may run inside a synchronous owner execute.
    // This does not relax native exclusion for opaque undo/abort/restore phases.
    MutationScope atomicForImport();
    MutationScope atomicForRestore(const PropertyContainer&);
    MutationScope atomicForRestore(const Property&);
    void retainMutationResource(std::shared_ptr<void>);
    bool hasAuthority() const;
    bool onWorker() const;
    Observation observation() const;
    OperationScope workerDispatch();
    void ensureWaitAllowed() const;
    void ensureSynchronousAllowed() const;
    void registerProperty(Property*, DocumentObject* = nullptr, Document* = nullptr);
    void setPropertyParent(Property*, Property*);
    void ensurePropertyStorageAvailable(Property*) const;
    bool retireProperty(Property*);  // false while leased; caller must defer storage destruction
    PropertySnapshot snapshotProperties(
        const std::vector<Property*>&,
        const Document* excludedDocument = nullptr,
        const DocumentObject* excludedObject = nullptr,
        bool collectTopology = true
    );
    void ensureFeatureAllowed(DocumentObject*) const;
    void ensureRootAvailable() const;

    bool deferRemoval(DocumentObject*);  // true only if a pinned exact attachment deferred
    void drainDeferred();                // outside Q; qualified GUI owner/native completion only
    CloseScope tryClose(const BindingPtr& documentBinding);
    bool isObserved(const BindingPtr&) const;
    std::uint64_t epoch() const noexcept;
    void setWake(std::function<void()>);
    void clearWake();
    void setPropertyDrain(std::function<void()>);
    void requestCleanup();
    bool hasDeferredWork() const noexcept;
    bool hasOwnerNativeWork() const noexcept;
    bool hasActiveOperations() const;
    bool tryBeginShutdown() noexcept;
    void shutdown();

    ClosureToken captureClosure(int transactionId, const std::vector<Document*>& = {});
    bool closureCurrent(const ClosureToken&) const;
    void retireClosure(int transactionId);
    void markClosureClosing(int transactionId, bool);
    bool isClosureClosing(int transactionId) const;
    void adoptClosure(const ClosureToken&, bool abort) noexcept;
    void drainClosures(const std::function<bool(const ClosureToken&, bool abort)>&);

private:
    std::optional<MutationScope> tryAtomicImpl(bool ownerNativeImport);
    MutationScope restoreImportedObject(const BindingPtr&, const MutationScope& importPreparation);
    bool matchesImportedRestore(const PropertyContainer*, const Property*) const;
    friend class App::Document;
    friend class App::DocInfo;
    Coordinator();
    ~Coordinator();
    struct Impl;
    std::unique_ptr<Impl> impl;
    friend class CaptureScope;
    friend class MutationScope;
    friend class CloseScope;
    friend class PropertySnapshot;
};
AppExport void throwIfBlockingRecomputeWaitIsCyclic();
}  // namespace RecomputeDetail
}  // namespace App
