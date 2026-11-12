// SPDX-License-Identifier: LGPL-2.1-or-later

#include "MotionSuspension.h"

#include <algorithm>
#include <unordered_map>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObserver.h>

namespace Assembly
{
struct GroundRelease
{
    explicit GroundRelease(App::DocumentObject* target)
        : target(target)
        , objectId(target->getID())
    {}

    App::DocumentObjectWeakPtrT target;
    long objectId;
};

struct MotionSuspensionState
{
    size_t depth = 0;
    bool alive = true;
    std::vector<GroundRelease> groundReleases;
};

namespace
{
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

void deferGroundedRelease(App::Document& document, App::DocumentObject* target)
{
    if (!target || target->getDocument() != &document || !isAutomaticMotionSuspended(&document)) {
        return;
    }
    auto& pending = registry().states.at(&document)->groundReleases;
    if (std::ranges::none_of(pending, [target](const auto& item) {
            return *item.target == target && item.objectId == target->getID();
        })) {
        pending.emplace_back(target);
    }
}

void reconcileDeferredGroundedReleases(
    App::Document& document,
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
