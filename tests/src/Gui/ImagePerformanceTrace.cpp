// SPDX-License-Identifier: LGPL-2.1-or-later
#include "ImagePerformanceTrace.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <map>
#include <mutex>
#include <stdexcept>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <Inventor/SoRenderManager.h>
#include <Inventor/fields/SoSFImage.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/ImagePlane.h>
#include <Gui/Tree.h>

namespace Gui::ImagePerformanceTrace
{
namespace
{
struct Store
{
    const void* bytes {};
    qint64 size {};
};
std::mutex mutex;
bool enabled = false;
bool balanced = true;
int visibility = -1;
thread_local int loadDepth = 0;
std::map<const SoSFImage*, Store> fields;
std::map<const void*, std::pair<qint64, int>> images;
std::map<const void*, bool> providers;
qint64 decodedPeak = 0, coinPeak = 0, attempts = 0, unavailable = 0;
qint64 successful = 0, failed = 0, emptyCalls = 0;
qint64 restored = 0, opened = 0, flushes = 0;
struct Timing
{
    qint64 nanoseconds = 0;
    qint64 calls = 0;
    int active = 0;
    int maximumDepth = 0;
};
std::array<Timing, 4> timings;
bool renderImageOverlap = false;
fastsignals::scoped_connection restoreConnection, openConnection;

qint64 coinBytes()
{
    std::map<const void*, qint64> unique;
    for (const auto& [field, data] : fields) {
        (void)field;
        if (data.bytes) {
            unique[data.bytes] = data.size;
        }
    }
    qint64 total = 0;
    for (const auto& [pointer, bytes] : unique) {
        (void)pointer;
        total += bytes;
    }
    return total;
}

qint64 decodedBytes()
{
    qint64 total = 0;
    for (const auto& [pointer, data] : images) {
        (void)pointer;
        total += data.first;
    }
    return total;
}

void refresh(const SoSFImage* field)
{
    SbVec2s size;
    int components = 0;
    const auto* bytes = field->getValue(size, components);
    fields[field] = {bytes, qint64(size[0]) * size[1] * components};
    coinPeak = std::max(coinPeak, coinBytes());
}
}  // namespace

void begin(int initialVisibility)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (!fields.empty() || !images.empty() || !providers.empty() || loadDepth != 0
        || std::any_of(timings.begin(), timings.end(), [](const Timing& t) { return t.active; })
        || initialVisibility < -1 || initialVisibility > 1) {
        throw std::runtime_error("Invalid image trace reset");
    }
    enabled = true;
    balanced = true;
    visibility = initialVisibility;
    decodedPeak = coinPeak = attempts = unavailable = restored = opened = flushes = 0;
    successful = failed = emptyCalls = 0;
    timings = {};
    renderImageOverlap = false;
    restoreConnection = App::GetApplication().signalFinishRestoreDocument.connect(
        [](const App::Document&) { ++restored; }
    );
    openConnection = App::GetApplication().signalFinishOpenDocument.connect([]() { ++opened; });
}

int initialVisibility()
{
    return enabled ? visibility : -1;
}

bool beginInterval(Phase phase)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (!enabled || (phase == Phase::Convert && loadDepth == 0)) {
        return false;
    }
    const auto index = static_cast<std::size_t>(phase);
    if (phase == Phase::Render) {
        renderImageOverlap |= timings[0].active || timings[1].active || timings[2].active;
    }
    else {
        renderImageOverlap |= timings[3].active != 0;
    }
    auto& timing = timings.at(index);
    ++timing.active;
    timing.maximumDepth = std::max(timing.maximumDepth, timing.active);
    return true;
}

void endInterval(Phase phase, qint64 nanoseconds)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto& timing = timings.at(static_cast<std::size_t>(phase));
    if (timing.active <= 0 || nanoseconds < 0) {
        balanced = false;
        return;
    }
    --timing.active;
    ++timing.calls;
    timing.nanoseconds += nanoseconds;
}

void registerField(const SoSFImage* field)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (enabled) {
        refresh(field);
    }
}

void updateField(const SoSFImage* field)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (enabled && fields.contains(field)) {
        refresh(field);
    }
}

void removeField(const SoSFImage* field)
{
    std::lock_guard<std::mutex> lock(mutex);
    fields.erase(field);
}

void provider(const void* pointer, bool pending)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (enabled) {
        providers[pointer] = pending;
    }
}

void removeProvider(const void* pointer)
{
    std::lock_guard<std::mutex> lock(mutex);
    providers.erase(pointer);
}

void load(bool missing, bool succeeded, bool hasPayload)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (enabled) {
        if (!hasPayload) {
            ++emptyCalls;
            return;
        }
        ++attempts;
        successful += succeeded;
        failed += !succeeded;
        unavailable += missing && !succeeded;
    }
}

void loadContext(bool enter)
{
    loadDepth += enter ? 1 : -1;
}

const void* addDecoded(const QImage& image)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (!enabled || loadDepth == 0 || image.isNull()) {
        return nullptr;
    }
    const void* pointer = image.constBits();
    auto& data = images[pointer];
    data.first = image.sizeInBytes();
    ++data.second;
    decodedPeak = std::max(decodedPeak, decodedBytes());
    return pointer;
}

void removeDecoded(const void* pointer, const QImage& image)
{
    if (!pointer) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex);
    auto it = images.find(pointer);
    // A changed backing store is unqualified, rather than silently losing an
    // allocation or terminating from a throwing RAII destructor.
    if (image.constBits() != pointer || it == images.end()) {
        balanced = false;
    }
    if (it != images.end() && --it->second.second == 0) {
        images.erase(it);
    }
}

QJsonObject snapshot()
{
    std::lock_guard<std::mutex> lock(mutex);
    qint64 pending = 0, materialized = 0;
    for (const auto& [pointer, value] : providers) {
        (void)pointer;
        pending += value;
    }
    for (const auto& [field, data] : fields) {
        (void)field;
        materialized += data.size > 0;
    }
    QJsonObject result {
        {"decoded_image_bytes", decodedPeak},
        {"coin_image_bytes", coinPeak},
        {"live_decoded_bytes", decodedBytes()},
        {"live_coin_bytes", coinBytes()},
        {"pending_textures", pending},
        {"materialized_image_fields", materialized},
        {"image_plane_providers", qint64(providers.size())},
        {"load_attempts", attempts},
        {"successful_loads", successful},
        {"failed_loads", failed},
        {"empty_load_calls", emptyCalls},
        {"failed_pre_restore_loads", unavailable},
        {"restore_finished_signals", restored},
        {"open_finished_signals", opened},
        {"required_tree_flushes", flushes},
        {"storage_trace_balanced", balanced}
    };
    const std::array<const char*, 4> names {"load", "decode", "convert", "render"};
    qint64 active = 0;
    bool nested = false;
    for (std::size_t i = 0; i < timings.size(); ++i) {
        const auto prefix = QStringLiteral("diagnostic_") + QString::fromLatin1(names[i]);
        result.insert(prefix + QStringLiteral("_ns"), timings[i].nanoseconds);
        result.insert(prefix + QStringLiteral("_calls"), timings[i].calls);
        active += timings[i].active;
        nested |= timings[i].maximumDepth > 1;
    }
    result.insert(QStringLiteral("diagnostic_active_intervals"), active);
    result.insert(QStringLiteral("diagnostic_nested_same_phase"), nested);
    result.insert(QStringLiteral("diagnostic_render_image_overlap"), renderImageOverlap);
    return result;
}
}  // namespace Gui::ImagePerformanceTrace

extern "C" bool fc_image_trace_begin(int visibility)
{
    try {
        Gui::ImagePerformanceTrace::begin(visibility);
        return true;
    }
    catch (...) {
        return false;
    }
}

extern "C" const char* fc_image_trace_snapshot()
{
    static thread_local QByteArray result;
    try {
        result = QJsonDocument(Gui::ImagePerformanceTrace::snapshot()).toJson(QJsonDocument::Compact);
        return result.constData();
    }
    catch (...) {
        return nullptr;
    }
}

extern "C" bool fc_image_trace_flush_tree()
{
    try {
        Gui::TreeWidget::updateStatus(false);
        ++Gui::ImagePerformanceTrace::flushes;
        return true;
    }
    catch (...) {
        return false;
    }
}

extern "C" const char* fc_image_trace_field_info(const void* pointer)
{
    static thread_local QByteArray result;
    if (!pointer) {
        return nullptr;
    }
    try {
        const auto* field = static_cast<const SoSFImage*>(pointer);
        std::lock_guard<std::mutex> lock(Gui::ImagePerformanceTrace::mutex);
        if (!Gui::ImagePerformanceTrace::fields.contains(field)) {
            return nullptr;
        }
        SbVec2s size;
        int components = 0;
        const auto* bytes = field->getValue(size, components);
        const qint64 length = qint64(size[0]) * size[1] * components;
        const auto hash = QCryptographicHash::hash(
                              QByteArrayView(reinterpret_cast<const char*>(bytes), length),
                              QCryptographicHash::Sha256
        )
                              .toHex();
        result = QJsonDocument(
                     QJsonObject {
                         {"width", size[0]},
                         {"height", size[1]},
                         {"components", components},
                         {"sha256", QString::fromLatin1(hash)}
                     }
        ).toJson(QJsonDocument::Compact);
        return result.constData();
    }
    catch (...) {
        return nullptr;
    }
}

extern "C" bool fc_image_trace_replace_field(
    void* pointer,
    int width,
    int height,
    const unsigned char* bytes,
    qint64 length
)
{
    if (!pointer || !bytes || width <= 0 || height <= 0 || width > 32767 || height > 32767
        || qint64(width) * height * 4 != length) {
        return false;
    }
    try {
        auto* field = static_cast<SoSFImage*>(pointer);
        std::lock_guard<std::mutex> lock(Gui::ImagePerformanceTrace::mutex);
        if (!Gui::ImagePerformanceTrace::fields.contains(field)) {
            return false;
        }
        field->setValue(SbVec2s(width, height), 4, bytes);
        return true;
    }
    catch (...) {
        return false;
    }
}

namespace
{
SoRenderManager* watchedRenderer = nullptr;
std::atomic<unsigned long long> renderCount {0};
struct RenderInterval
{
    QElapsedTimer timer;
    bool traced = false;
};
std::array<RenderInterval, 64> renderIntervals;
std::size_t renderDepth = 0;
std::size_t renderOverflow = 0;
void startedRender(void*, SoRenderManager*)
{
    Gui::ImagePerformanceTrace::renderInterval(true);
}
void completedRender(void*, SoRenderManager*)
{
    Gui::ImagePerformanceTrace::renderInterval(false);
    ++renderCount;
}
}  // namespace
void Gui::ImagePerformanceTrace::renderInterval(bool enter)
{
    if (enter) {
        if (renderDepth == renderIntervals.size()) {
            ++renderOverflow;
            std::lock_guard<std::mutex> lock(mutex);
            balanced = false;
            return;
        }
        auto& interval = renderIntervals[renderDepth++];
        interval.traced = beginInterval(Phase::Render);
        if (interval.traced) {
            interval.timer.start();
        }
    }
    else if (renderOverflow) {
        --renderOverflow;
    }
    else if (renderDepth) {
        auto& interval = renderIntervals[--renderDepth];
        if (interval.traced) {
            endInterval(Phase::Render, interval.timer.nsecsElapsed());
        }
    }
    else {
        std::lock_guard<std::mutex> lock(mutex);
        balanced = false;
    }
}
extern "C" bool fc_image_trace_watch_renderer(void* pointer)
{
    try {
        if (renderDepth || renderOverflow) {
            return false;
        }
        if (watchedRenderer) {
            watchedRenderer->removePreRenderCallback(startedRender, nullptr);
            watchedRenderer->removePostRenderCallback(completedRender, nullptr);
        }
        watchedRenderer = static_cast<SoRenderManager*>(pointer);
        renderCount = 0;
        if (watchedRenderer) {
            watchedRenderer->addPreRenderCallback(startedRender, nullptr);
            watchedRenderer->addPostRenderCallback(completedRender, nullptr);
        }
        return true;
    }
    catch (...) {
        return false;
    }
}
extern "C" unsigned long long fc_image_trace_render_count()
{
    return renderCount;
}

extern "C" const char* fc_image_trace_plane_spacing(const char* document, const char* object)
{
    if (!document || !object) {
        return nullptr;
    }
    static thread_local QByteArray result;
    try {
        const auto* doc = App::GetApplication().getDocument(document);
        const auto* plane = doc ? dynamic_cast<Image::ImagePlane*>(doc->getObject(object)) : nullptr;
        if (!plane) {
            return nullptr;
        }
        result = QJsonDocument(
                     QJsonObject {
                         {"x_pixels_per_meter", plane->XPixelsPerMeter},
                         {"y_pixels_per_meter", plane->YPixelsPerMeter}
                     }
        ).toJson(QJsonDocument::Compact);
        return result.constData();
    }
    catch (...) {
        return nullptr;
    }
}
