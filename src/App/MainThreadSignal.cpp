// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include "MainThreadSignal.h"

#include <atomic>

namespace
{
struct MainThreadHooks
{
    App::MainThreadSignalConfig::IsMainThreadFn isMainThread;
    App::MainThreadSignalConfig::InvokeSyncFn invokeSync;
    App::MainThreadSignalConfig::InvokeFn invoke;
    App::MainThreadSignalConfig::PumpEventsFn pumpEvents;
};

MainThreadHooks installedHooks {};
std::atomic<const MainThreadHooks*> activeHooks {nullptr};
}

namespace App
{

void MainThreadSignalConfig::installHooks(
    IsMainThreadFn isMainThread,
    InvokeSyncFn invokeSync,
    InvokeFn invoke,
    PumpEventsFn pumpEvents
)
{
    if (!isMainThread || !invokeSync) {
        throw std::invalid_argument(
            "Main-thread hooks must provide both an affinity predicate and a synchronous invoker"
        );
    }

    // Concurrent replacement is prohibited by the lifecycle contract. Publish
    // the complete hook pair with one release-store so readers never observe a
    // partially installed configuration.
    installedHooks = {isMainThread, invokeSync, invoke, pumpEvents};
    activeHooks.store(&installedHooks, std::memory_order_release);
}

void MainThreadSignalConfig::setHooks(
    IsMainThreadFn isMainThread,
    InvokeFn invoke,
    PumpEventsFn pumpEvents
)
{
    if (!isMainThread && !invoke && !pumpEvents) {
        clearHooks();
        return;
    }
    if (!isMainThread || !invoke) {
        throw std::invalid_argument(
            "Main-thread hooks must provide both an affinity predicate and an invoker"
        );
    }

    installedHooks = {isMainThread, nullptr, invoke, pumpEvents};
    activeHooks.store(&installedHooks, std::memory_order_release);
}

void MainThreadSignalConfig::clearHooks()
{
    // Emitters must already be stopped. Remove the complete hook snapshot with
    // one store and restore App-only inline behaviour.
    activeHooks.store(nullptr, std::memory_order_release);
}

bool MainThreadSignalConfig::isMainThread()
{
    if (const auto* hooks = activeHooks.load(std::memory_order_acquire)) {
        return hooks->isMainThread();
    }
    return true;  // no GUI hooks: preserve same-thread behavior
}

bool MainThreadSignalConfig::hasHooks()
{
    return activeHooks.load(std::memory_order_acquire) != nullptr;
}

bool MainThreadSignalConfig::canPumpEvents()
{
    if (const auto* hooks = activeHooks.load(std::memory_order_acquire)) {
        return hooks->pumpEvents != nullptr;
    }
    return false;
}

void MainThreadSignalConfig::invoke(std::function<void()>&& fn, bool blocking)
{
    if (const auto* hooks = activeHooks.load(std::memory_order_acquire)) {
        if (hooks->invoke) {
            hooks->invoke(std::move(fn), blocking);
            return;
        }
        if (hooks->invokeSync) {
            auto task = [](void* context) { (*static_cast<std::function<void()>*>(context))(); };
            hooks->invokeSync(task, &fn);
            return;
        }
    }

    fn();
}

void MainThreadSignalConfig::pumpEvents()
{
    if (const auto* hooks = activeHooks.load(std::memory_order_acquire)) {
        if (hooks->pumpEvents) {
            hooks->pumpEvents();
        }
    }
}

bool MainThreadSignalConfig::invokeSync(TaskFn task, void* context)
{
    if (const auto* hooks = activeHooks.load(std::memory_order_acquire)) {
        if (hooks->invokeSync) {
            return hooks->invokeSync(task, context);
        }
        if (hooks->invoke) {
            hooks->invoke([task, context] { task(context); }, true);
            return true;
        }
    }

    task(context);
    return true;
}

}  // namespace App
