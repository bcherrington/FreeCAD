// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <FCGlobal.h>
#include <QElapsedTimer>
#include <QImage>
#include <QJsonObject>

class SoSFImage;
namespace Gui::ImagePerformanceTrace
{
// Compiled only in an explicitly enabled developer benchmark build. Storage
// means logical image-plane payload bytes, excluding Qt/driver allocator scratch.
GuiExport void begin(int initialVisibility);
GuiExport int initialVisibility();
GuiExport void registerField(const SoSFImage*);
GuiExport void updateField(const SoSFImage*);
GuiExport void removeField(const SoSFImage*);
GuiExport void provider(const void*, bool pending);
GuiExport void removeProvider(const void*);
GuiExport void load(bool unavailableDuringRestore, bool succeeded, bool hasPayload = true);
GuiExport QJsonObject snapshot();
GuiExport void loadContext(bool enter);
GuiExport const void* addDecoded(const QImage&);
GuiExport void removeDecoded(const void*, const QImage&);

enum class Phase
{
    Load,
    Decode,
    Convert,
    Render
};
GuiExport bool beginInterval(Phase);
GuiExport void endInterval(Phase, qint64 nanoseconds);
GuiExport void renderInterval(bool enter);

class Interval
{
public:
    explicit Interval(Phase phase)
        : phase(phase)
        , active(beginInterval(phase))
    {
        if (active) {
            timer.start();
        }
    }
    ~Interval()
    {
        if (active) {
            endInterval(phase, timer.nsecsElapsed());
        }
    }
    Interval(const Interval&) = delete;
    Interval& operator=(const Interval&) = delete;

private:
    Phase phase;
    bool active;
    QElapsedTimer timer;
};

class Load
{
public:
    Load()
    {
        loadContext(true);
    }
    ~Load()
    {
        loadContext(false);
    }
    Load(const Load&) = delete;
    Load& operator=(const Load&) = delete;

private:
    Interval interval {Phase::Load};
};

class Decoded
{
public:
    explicit Decoded(const QImage& image)
        : image(image)
        , storage(addDecoded(image))
    {}
    ~Decoded()
    {
        removeDecoded(storage, image);
    }
    Decoded(const Decoded&) = delete;
    Decoded& operator=(const Decoded&) = delete;

private:
    const QImage& image;
    const void* storage;
};

class Field
{
public:
    explicit Field(const SoSFImage& field)
        : field(field)
    {
        registerField(&field);
    }
    ~Field()
    {
        removeField(&field);
    }
    Field(const Field&) = delete;
    Field& operator=(const Field&) = delete;

private:
    const SoSFImage& field;
};
}  // namespace Gui::ImagePerformanceTrace

// Small C ABI for the native FreeCAD Python macro. No pointers into QImage/Coin
// storage cross this ABI. Failure never unwinds a C++ exception through ctypes.
extern "C" {
GuiExport bool fc_image_trace_begin(int initialVisibility);
GuiExport const char* fc_image_trace_snapshot();
GuiExport bool fc_image_trace_flush_tree();
GuiExport bool fc_image_trace_watch_renderer(void* manager);
GuiExport unsigned long long fc_image_trace_render_count();
GuiExport const char* fc_image_trace_field_info(const void* field);
GuiExport const char* fc_image_trace_plane_spacing(const char* document, const char* object);
GuiExport bool fc_image_trace_replace_field(
    void* field,
    int width,
    int height,
    const unsigned char* bytes,
    qint64 length
);
}
