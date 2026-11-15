// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MotionSuspension.h"

#include <algorithm>
#include <unordered_map>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObserver.h>
#include <App/PropertyLinks.h>

#include "AssemblyLink.h"
#include "AssemblyObject.h"
#include "Groups.h"

namespace Assembly
{
struct GroundRelease
{
    GroundRelease(App::DocumentObject* target, App::DocumentObject* owner)
        : target(target)
        , objectId(target->getID())
        , owner(owner)
        , ownerId(owner ? owner->getID() : 0)
    {}

    App::DocumentObjectWeakPtrT target;
    long objectId;
    App::DocumentObjectWeakPtrT owner;
    long ownerId;
};

struct MotionSuspensionState
{
    size_t depth = 0;
    bool alive = true;
    std::vector<GroundRelease> groundReleases;
};

namespace
{
bool directlyContains(App::DocumentObject* owner, App::DocumentObject* object)
{
    auto* group = owner ? owner->getPropertyByName<App::PropertyLinkList>("Group") : nullptr;
    return group && std::ranges::find(group->getValues(), object) != group->getValues().end();
}

bool isOccurrence(App::DocumentObject* object)
{
    return object
        && (object->isDerivedFrom<AssemblyObject>() || object->isDerivedFrom<AssemblyLink>());
}

App::DocumentObject* groundReleaseOwner(App::DocumentObject* target, App::DocumentObject* record)
{
    std::unordered_set<App::DocumentObject*> owners;
    if (record) {
        for (auto* parent : record->getInList()) {
            if (!parent->isDerivedFrom<JointGroup>() || !directlyContains(parent, record)) {
                continue;
            }
            for (auto* owner : parent->getInList()) {
                if (isOccurrence(owner) && directlyContains(owner, parent)) {
                    owners.insert(owner);
                }
            }
        }
    }
    else {
        // Compatibility for old private two-argument callers. Real grounded-joint
        // callbacks supply the record so a root-owned ground on a descendant
        // retains the root's authority rather than the target's local owner.
        for (auto* owner : target->getInList()) {
            if (isOccurrence(owner) && directlyContains(owner, target)) {
                owners.insert(owner);
            }
        }
    }
    return owners.size() == 1 ? *owners.begin() : nullptr;
}

bool locallyContains(App::DocumentObject* owner, App::DocumentObject* target)
{
    // Examine containment only, never linked source definitions. The old owner
    // may be outside the just-validated root; bound traversal even if it is malformed.
    std::vector<App::DocumentObject*> pending {owner};
    std::unordered_set<App::DocumentObject*> visited;
    while (!pending.empty()) {
        auto* object = pending.back();
        pending.pop_back();
        if (!object || !visited.insert(object).second) {
            continue;
        }
        if (object == target) {
            return true;
        }
        for (const char* property : {"Group", "ElementList"}) {
            if (auto* children = object->getPropertyByName<App::PropertyLinkList>(property)) {
                pending.insert(pending.end(), children->getValues().begin(), children->getValues().end());
            }
        }
    }
    return false;
}

class SuspensionRegistry
{
public:
    SuspensionRegistry()
        : deleteConnection(
              App::GetApplication().signalDeleteDocument.connect([this](const App::Document& document) {
                  auto found = states.find(&document);
                  if (found != states.end()) {
                      // Restore/clear also sends this notification. Only the
                      // old object identities expire, not the live document scope.
                      found->second->groundReleases.clear();
                  }
              })
          )
        , newConnection(
              App::GetApplication().signalNewDocument.connect(
                  [this](const App::Document& document, bool) {
                      // Clear/restore callbacks can queue releases while old objects
                      // are being removed. Drop them before replacement objects can
                      // reuse both an address and the reset document object ID.
                      auto found = states.find(&document);
                      if (found != states.end()) {
                          found->second->groundReleases.clear();
                      }
                  }
              )
          )
        , deletedConnection(App::GetApplication().signalDeletedDocument.connect([this]() {
            // Only actual close sends this after removing the document from
            // the application map. Never dereference potentially stale keys.
            const auto documents = App::GetApplication().getDocuments();
            std::unordered_set<const App::Document*> live(documents.begin(), documents.end());
            std::erase_if(states, [&](auto& entry) {
                if (live.contains(entry.first)) {
                    return false;
                }
                entry.second->alive = false;
                entry.second->depth = 0;
                entry.second->groundReleases.clear();
                return true;
            });
        }))
    {}

    ~SuspensionRegistry()
    {
        deleteConnection.disconnect();
        newConnection.disconnect();
        deletedConnection.disconnect();
    }

    std::unordered_map<const App::Document*, std::shared_ptr<MotionSuspensionState>> states;

private:
    fastsignals::connection deleteConnection;
    fastsignals::connection newConnection;
    fastsignals::connection deletedConnection;
};

SuspensionRegistry& registry()
{
    static SuspensionRegistry instance;
    return instance;
}
}  // namespace

AutomaticMotionSuspension::AutomaticMotionSuspension(App::Document& document)
{
    auto& entry = registry().states[&document];
    if (!entry) {
        entry = std::make_shared<MotionSuspensionState>();
    }
    state = entry;
    ++state->depth;
}

AutomaticMotionSuspension::~AutomaticMotionSuspension()
{
    release();
}

void AutomaticMotionSuspension::release() noexcept
{
    if (state && state->alive && state->depth) {
        --state->depth;
    }
    state.reset();
}

bool isAutomaticMotionSuspended(const App::Document* document)
{
    if (!document) {
        return false;
    }
    const auto& states = registry().states;
    const auto found = states.find(document);
    return found != states.end() && found->second->depth != 0;
}

void deferGroundedRelease(App::Document& document, App::DocumentObject* target, App::DocumentObject* record)
{
    if (!target || target->getDocument() != &document || !isAutomaticMotionSuspended(&document)) {
        return;
    }
    if (record && record->getDocument() != &document) {
        return;
    }
    auto* owner = groundReleaseOwner(target, record);
    auto& pending = registry().states.at(&document)->groundReleases;
    // Deletion can notify again after containment has already been removed.
    // Keep the provenance captured by the earlier callback instead of adding
    // an unowned duplicate that could later acquire broader release authority.
    if (!owner && std::ranges::any_of(pending, [target](const auto& item) {
            return *item.target == target && item.objectId == target->getID();
        })) {
        return;
    }
    if (std::ranges::none_of(pending, [target, owner](const auto& item) {
            return *item.target == target && item.objectId == target->getID()
                && *item.owner == owner && item.ownerId == (owner ? owner->getID() : 0);
        })) {
        pending.emplace_back(target, owner);
    }
}

void reconcileDeferredGroundedReleases(
    App::Document& document,
    App::DocumentObject* root,
    const std::unordered_set<App::DocumentObject*>& admitted,
    const std::vector<App::DocumentObject*>& grounded
)
{
    auto found = registry().states.find(&document);
    if (found == registry().states.end()) {
        return;
    }
    std::vector<App::DocumentObject*> releases;
    auto& pending = found->second->groundReleases;
    std::erase_if(pending, [&](const auto& item) {
        auto* target = *item.target;
        if (!target || target->getID() != item.objectId) {
            return true;
        }
        if (!admitted.contains(target)) {
            return false;
        }
        // Unknown or ambiguous provenance never grants release authority. This
        // differs from a known owner that has since expired, whose live intent
        // can transfer to a new directly-owning root.
        if (item.ownerId == 0) {
            return false;
        }
        auto* owner = *item.owner;
        if (owner && owner->getID() != item.ownerId) {
            owner = nullptr;
        }
        // A root solve cannot consume a nested occurrence's grounding intent,
        // even when that occurrence is flexible and its components are active.
        // Preserve live intents across owner deletion. An explicit move out of
        // the old owner permits a new directly-owning root to adopt the release.
        if (owner != root
            && (!directlyContains(root, target) || (owner && locallyContains(owner, target)))) {
            return false;
        }
        if (std::ranges::find(grounded, target) == grounded.end()) {
            releases.push_back(target);
        }
        return true;
    });
    for (auto* target : releases) {
        for (const char* name : {"Placement", "LinkPlacement"}) {
            if (auto* property = target->getPropertyByName(name)) {
                property->setReadOnly(false);
            }
        }
    }
}
}  // namespace Assembly
