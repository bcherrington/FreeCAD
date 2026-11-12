// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <memory>
#include <unordered_set>
#include <vector>

#include <Mod/Assembly/AssemblyGlobal.h>

namespace App
{
class Document;
class DocumentObject;
}  // namespace App

namespace Assembly
{
struct MotionSuspensionState;

/** Disable automatic assembly motion in one live document, without changing preferences.
 * Handles can outlive the document. Releasing a handle never solves or replays callbacks.
 * Like document mutation, this API is used on the application thread.
 */
class AssemblyExport AutomaticMotionSuspension
{
public:
    explicit AutomaticMotionSuspension(App::Document& document);
    ~AutomaticMotionSuspension();
    AutomaticMotionSuspension(const AutomaticMotionSuspension&) = delete;
    AutomaticMotionSuspension& operator=(const AutomaticMotionSuspension&) = delete;

    void release() noexcept;

private:
    std::shared_ptr<MotionSuspensionState> state;
};

AssemblyExport bool isAutomaticMotionSuspended(const App::Document* document);

// Internal grounded-joint callback coordination. These do not release suspension.
void deferGroundedRelease(App::Document& document, App::DocumentObject* target);
void reconcileDeferredGroundedReleases(
    App::Document& document,
    const std::unordered_set<App::DocumentObject*>& admitted,
    const std::vector<App::DocumentObject*>& grounded
);
}  // namespace Assembly
