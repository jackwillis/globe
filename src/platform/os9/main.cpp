// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Classic PowerPC proof of life: Toolbox window, RGB555 GWorld and wake-driven input loop.
#include "classic_present.hpp"
#include "classic_wait.hpp"
#include "clock.hpp"
#include "globe/app.hpp"
#include "globe/embedded_atlas.hpp"
#include "globe/embedded_elevation.hpp"
#include "globe/embedded_landclasses.hpp"
#include "globe/terrain.hpp"
#include "pixel_format.hpp"
#include <Dialogs.h>
#include <Events.h>
#include <Menus.h>
#include <OSUtils.h>
#include <Quickdraw.h>
#include <TextEdit.h>
#include <Windows.h>
#include <algorithm>
#include <optional>
// These Appearance Manager declarations are absent from the installed Multiversal interfaces.
// AppearanceLib exports the function; OS 9 provides arrow=0, closed hand=8, open hand=9.
extern "C" std::int32_t SetThemeCursor(std::uint32_t cursor);
using namespace globe;
static_assert(next_terrain_style(TerrainStyle::Map) == TerrainStyle::Relief);
static_assert(next_terrain_style(TerrainStyle::Relief) == TerrainStyle::Map);
namespace {
// Prefer short foreground turns to maximum settle throughput. Copy time counts in the slice.
constexpr std::uint64_t kWorkSliceMs = 16;
constexpr unsigned kWorkUnitsPerStep = 64;
// Inside Macintosh, Event Manager: high byte 1 is suspend/resume; low bit 1 is resume.
// The installed Multiversal Events.h omits these two constants.
constexpr unsigned kSuspendResumeMessage = 1;
constexpr unsigned kResumeFlag = 1;
std::uint64_t now_ms() {
    return monotonic_ns() / 1000000;
}
class Application {
  public:
    ~Application() {
        if (world_)
            DisposeGWorld(world_);
        if (mouse_region_)
            DisposeRgn(mouse_region_);
        if (window_)
            DisposeWindow(window_);
    }
    bool init() {
        auto atlas = Atlas::parse(kEmbeddedAtlasData, kEmbeddedAtlasSize);
        if (!atlas)
            return false;
        scene_ = Scene::create(*atlas);
        if (!scene_)
            return false;
        auto elevation = Elevation::parse(kEmbeddedElevationData, kEmbeddedElevationSize);
        if (!elevation)
            return false;
        classes_ = LandClasses::decode(kEmbeddedLandclassesData, kEmbeddedLandclassesSize);
        if (!classes_)
            return false;
        terrain_ = TerrainScene::create(*elevation, &*classes_);
        if (!terrain_)
            return false;
        Rect bounds{50, 30, 530, 670};
        const unsigned char title[] = {5, 'G', 'l', 'o', 'b', 'e'};
        window_ = NewCWindow(nullptr, &bounds, title, true, documentProc,
                             reinterpret_cast<WindowPtr>(-1), true, 0);
        mouse_region_ = NewRgn();
        if (!window_ || !mouse_region_)
            return false;
        SetPortWindowPort(window_);
        if (!buffers(640, 480))
            return false;
        apply(Tick{now_ms()});
        return true;
    }
    void run() {
        bool quit = false;
        auto turn_start = now_ms();
        while (!quit) {
            const auto before = now_ms();
            if (foreground_ && wake_ && before >= wake_) {
                wake_ = 0;
                apply(Tick{before});
            }
            if (foreground_ && redraw_) {
                render_globe(*fb_, model_.camera, *scene_, kDefaultPalette,
                             effective_layers(model_), true, quality_for(model_), &*terrain_);
                copy(0, fb_->height());
                redraw_ = false;
            }
            // Give presentation a bounded share before geometry can consume the whole turn.
            if (foreground_ && !presentation_held(model_) && now_ms() - turn_start < kWorkSliceMs)
                present_step();
            const bool work = foreground_ && job_ && job_->active() && !job_waiting_;
            if (work && now_ms() - turn_start < kWorkSliceMs) {
                // Check elapsed time after every bounded step AND presentation. A slow step or
                // Toolbox copy can still overrun: measure the full turn, not just geometry.
                const auto slice_end = turn_start + kWorkSliceMs;
                auto step_start = now_ms();
                unsigned steps = 0;
                do {
                    const auto band = job_->step(*fb_, model_.generation, kWorkUnitsPerStep);
                    auto step_end = now_ms();
                    max_step_ms_ = std::max(max_step_ms_, step_end - step_start);
                    if (band.band_ready) {
                        present(band.first_row, band.end_row);
                        step_end = now_ms();
                    }
                    if (band.wake_after_ms) {
                        job_waiting_ = true;
                        wake_ = step_end + band.wake_after_ms;
                        break;
                    }
                    if (!job_->active() && model_.refine_active) {
                        apply(RefinementFinished{model_.generation, model_.refine_level});
                        break;
                    }
                    step_start = step_end;
                    EventRecord pending{};
                    if (++steps % 16 == 0 && EventAvail(everyEvent, &pending))
                        break; // dispatch queued input before spending another geometry budget
                } while (job_->active() && step_start < slice_end);
                // Finish the last queued image through the same presentation continuation.
                if (!job_->active() && dirty_ && !presentation_held(model_))
                    presentation_.request(model_.generation);
            }
            if (foreground_)
                max_turn_ms_ = std::max(max_turn_ms_, now_ms() - turn_start);
            if (foreground_ && model_.stage == QualityStage::Refine && job_ && !job_->active() &&
                !titled_ && !presentation_.pending(model_.generation) && !dirty_)
                show_timing();
            Point pointer;
            GetMouse(&pointer);
            LocalToGlobal(&pointer);
            Rect mouse{pointer.v, pointer.h, static_cast<short>(pointer.v + 1),
                       static_cast<short>(pointer.h + 1)};
            RectRgn(mouse_region_, &mouse);
            EventRecord event{};
            WaitNextEvent(
                everyEvent, &event,
                static_cast<long>(classic_wait_ticks(
                    now_ms(), wake_,
                    (job_ && job_->active() && !job_waiting_) ||
                        (!presentation_held(model_) && presentation_.pending(model_.generation)),
                    foreground_)),
                foreground_ ? mouse_region_ : nullptr);
            const auto time = now_ms();
            turn_start = time;
            SetPortWindowPort(window_);
            Point local = event.where;
            GlobalToLocal(&local);
            if (event.what == osEvt && ((event.message >> 24) & 0xff) == kSuspendResumeMessage) {
                foreground_ = (event.message & kResumeFlag) != 0;
                if (!foreground_) {
                    // End an interrupted drag through update(), but retain terrain progress.
                    apply(PointerUp{time});
                    cursor_ = CursorIntent::Arrow;
                } else {
                    // The bitmap may have advanced before suspension. Re-present on resume.
                    dirty_ = true;
                    if (!redraw_ && !presentation_held(model_))
                        presentation_.request(model_.generation);
                }
            } else if (event.what == mouseDown) {
                WindowPtr hit = nullptr;
                const auto part = FindWindow(event.where, &hit);
                if (hit == window_) {
                    if (part == inContent) {
                        if (FrontWindow() != window_)
                            SelectWindow(window_);
                        else
                            quit = apply(PointerDown{time, double(local.h), double(local.v)});
                    } else if (part == inDrag) {
                        Rect limit{20, -32000, 32000, 32000};
                        DragWindow(window_, event.where, &limit);
                        SetPortWindowPort(window_);
                    } else if (part == inGoAway)
                        quit = TrackGoAway(window_, event.where);
                    else if (part == inGrow) {
                        Rect limit{120, 160, 1201, 1601};
                        const auto size = GrowWindow(window_, event.where, &limit);
                        if (size) {
                            const int width = static_cast<unsigned long>(size) & 0xffffU;
                            const int height = (static_cast<unsigned long>(size) >> 16) & 0xffffU;
                            if (buffers(width, height)) {
                                SizeWindow(window_, width, height, true);
                                apply(Resize{width, height});
                            } else
                                quit = true;
                        }
                    }
                } else if (part == inSysWindow)
                    SystemClick(&event, hit);
            } else if (event.what == mouseUp) {
                quit = apply(PointerUp{time});
                // A refinement finished (or advanced) while the button was down: show it now.
                if (dirty_ && model_.stage != QualityStage::Moving && !redraw_)
                    presentation_.request(model_.generation);
            } else if (event.what == keyDown || event.what == autoKey)
                quit = key(event.message, time);
            else if (event.what == updateEvt) {
                if (reinterpret_cast<WindowPtr>(event.message) == window_) {
                    BeginUpdate(window_);
                    // Background update events repaint the cached GWorld, never convert terrain.
                    copy(0, fb_->height(),
                         foreground_ && !presentation_.pending(model_.generation));
                    EndUpdate(window_);
                }
            } else if (event.what == activateEvt && !(event.modifiers & activeFlag))
                apply(PointerUp{time});
            // A one-pixel mouse region wakes on motion, even with no button held. No periodic poll.
            Point current;
            GetMouse(&current);
            const ScreenPoint position{double(current.h), double(current.v)};
            if (foreground_ && model_.dragging &&
                (position.x != model_.drag_last.x || position.y != model_.drag_last.y))
                apply(PointerMove{time, position.x, position.y});
            const auto intent =
                FrontWindow() == window_ ? cursor_for(model_, position) : CursorIntent::Arrow;
            if (foreground_ && intent != cursor_) {
                cursor_ = intent;
                const auto native = intent == CursorIntent::OpenHand     ? 9U
                                    : intent == CursorIntent::ClosedHand ? 8U
                                                                         : 0U;
                if (SetThemeCursor(native) != 0)
                    InitCursor();
            }
        }
    }

  private:
    bool buffers(int width, int height) {
        auto fb = Framebuffer::create(width, height);
        if (!fb)
            return false;
        Rect rect{0, 0, static_cast<short>(height), static_cast<short>(width)};
        GWorldPtr next = nullptr;
        if (NewGWorld(&next, 16, &rect, nullptr, nullptr, 0) != 0 || !next)
            return false;
        if (job_)
            job_->cancel();
        presentation_.cancel();
        if (world_)
            DisposeGWorld(world_);
        world_ = next;
        fb_ = std::move(fb);
        return true;
    }
    // Coalesce updates while a stable GWorld snapshot is submitted in bounded row ranges.
    void present(int, int) {
        dirty_ = true;
        if (foreground_ && !presentation_held(model_))
            presentation_.request(model_.generation);
    }
    void present_step() {
        if (presentation_.needs_capture(model_.generation)) {
            if (!capture())
                return;
            presentation_.begin(model_.generation, fb_->height());
            dirty_ = false;
            return; // conversion and submission never share the same turn
        }
        if (presentation_.active(model_.generation)) {
            const int first = presentation_.first(), end = presentation_.end();
            if (copy(first, end, false))
                presentation_.submitted(model_.generation);
        }
    }
    bool capture() {
        const auto start = now_ms();
        const auto pix = GetGWorldPixMap(world_);
        if (!LockPixels(pix))
            return false;
        const PixelFormat format{16, 0x7c00, 0x03e0, 0x001f, true};
        const auto stride = static_cast<std::uint16_t>((**pix).rowBytes) & 0x3fffU;
        const bool ok = convert_pixels(
            *fb_, format, reinterpret_cast<std::uint8_t*>(GetPixBaseAddr(pix)), stride);
        UnlockPixels(pix); // no locked handles survive a yield
        copy_ms_ += now_ms() - start;
        return ok;
    }
    // The window title reports the last settle: first refine step to last, and time spent copying.
    void show_timing() {
        titled_ = true;
        unsigned char title[96];
        unsigned n = 0;
        const auto put = [&](const char* text) {
            while (*text && n < 95)
                title[++n] = static_cast<unsigned char>(*text++);
        };
        const auto number = [&](std::uint64_t v) {
            char digits[12];
            int d = 0;
            do {
                digits[d++] = static_cast<char>('0' + v % 10);
                v /= 10;
            } while (v && d < 11);
            while (d)
                title[++n] = static_cast<unsigned char>(digits[--d]);
        };
        put("Globe  ");
        number(now_ms() - refine_t0_);
        put(" ms, copy ");
        number(copy_ms_);
        put(" ms, turn ");
        number(max_turn_ms_);
        put(" ms, step ");
        number(max_step_ms_);
        put(" ms");
        title[0] = static_cast<unsigned char>(n);
        SetWTitle(window_, title);
    }
    bool copy(int first, int end, bool convert = true) {
        if (!world_ || !fb_)
            return false;
        if (convert)
            presentation_.cancel(); // synchronous moving frame replaces any queued old snapshot
        const auto copy_t0 = now_ms();
        struct Timer {
            std::uint64_t& total;
            std::uint64_t t0;
            ~Timer() { total += now_ms() - t0; }
        } timer{copy_ms_, copy_t0};
        const auto pix = GetGWorldPixMap(world_);
        if (!LockPixels(pix))
            return false;
        const PixelFormat format{16, 0x7c00, 0x03e0, 0x001f, true};
        const auto stride = static_cast<std::uint16_t>((**pix).rowBytes) & 0x3fffU;
        const bool converted =
            !convert ||
            convert_pixel_rows(*fb_, format, reinterpret_cast<std::uint8_t*>(GetPixBaseAddr(pix)),
                               stride, first, end);
        if (converted) {
            SetPortWindowPort(window_);
            Rect rect{static_cast<short>(first), 0, static_cast<short>(end),
                      static_cast<short>(fb_->width())};
            CopyBits(reinterpret_cast<BitMap*>(*pix), &window_->portBits, &rect, &rect, srcCopy,
                     nullptr);
            if (convert)
                dirty_ = false;
        }
        UnlockPixels(pix);
        return converted;
    }
    bool apply(const Msg& msg) {
        if (std::holds_alternative<Tick>(msg) && job_waiting_) {
            job_waiting_ = false;
            wake_ = 0;
        }
        const auto generation = model_.generation;
        const auto stage = model_.stage;
        auto fx = update(model_, msg);
        if (generation != model_.generation) {
            presentation_.cancel();
            job_waiting_ = false;
            if (job_)
                job_->cancel();
            if (stage == QualityStage::Refine)
                fx.redraw = true;
        }
        if (fx.refine && stage != QualityStage::Refine) {
            refine_t0_ = now_ms();
            copy_ms_ = 0;
            max_turn_ms_ = max_step_ms_ = 0;
            dirty_ = false;
            titled_ = false;
        }
        if (fx.refine) {
            job_.emplace(model_.camera, *scene_, kDefaultPalette, effective_layers(model_),
                         quality_for(model_), model_.generation, &*terrain_);
        }
        if (std::holds_alternative<Resize>(msg))
            fx.wake_after_ms = update(model_, Tick{now_ms()}).wake_after_ms;
        if (fx.wake_after_ms)
            wake_ = now_ms() + fx.wake_after_ms;
        redraw_ |= fx.redraw;
        return fx.quit;
    }
    bool key(unsigned long message, std::uint64_t time) {
        const auto code = (message >> 8) & 0xff;
        const auto ch = message & 0xff;
        std::optional<Command> command;
        if (code == 0x7b)
            command = Command::Left;
        else if (code == 0x7c)
            command = Command::Right;
        else if (code == 0x7d)
            command = Command::Down;
        else if (code == 0x7e)
            command = Command::Up;
        else
            switch (ch) {
            case '+':
            case '=':
                command = Command::ZoomIn;
                break;
            case '-':
                command = Command::ZoomOut;
                break;
            case 'r':
            case 'R':
                command = Command::Reset;
                break;
            case 'g':
            case 'G':
                command = Command::ToggleGraticule;
                break;
            case 'b':
            case 'B':
                command = Command::ToggleBorders;
                break;
            case 's':
            case 'S':
                command = Command::CycleStyle;
                break;
            case 't':
            case 'T':
                command = Command::ToggleDragMode;
                break;
            case 'n':
            case 'N':
                command = Command::AlignNorth;
                break;
            case 'q':
            case 'Q':
            case 27:
                command = Command::Quit;
                break;
            }
        return command ? apply(CommandIssued{time, *command}) : false;
    }
    WindowPtr window_ = nullptr;
    GWorldPtr world_ = nullptr;
    RgnHandle mouse_region_ = nullptr;
    Model model_ = initial_model(640, 480);
    std::optional<Scene> scene_;
    std::optional<LandClasses> classes_;
    std::optional<TerrainScene> terrain_;
    std::optional<Framebuffer> fb_;
    std::optional<RefinementJob> job_;
    bool job_waiting_ = false;
    std::uint64_t wake_ = 0;
    std::uint64_t refine_t0_ = 0, copy_ms_ = 0;
    bool titled_ = true, dirty_ = false;
    ClassicPresentation presentation_;
    bool foreground_ = true;
    std::uint64_t max_turn_ms_ = 0, max_step_ms_ = 0;
    bool redraw_ = true;
    CursorIntent cursor_ = CursorIntent::Arrow;
};
} // namespace
int main() {
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(nullptr);
    InitCursor();
    Application app;
    if (!app.init()) {
        SysBeep(10);
        return 1;
    }
    app.run();
    return 0;
}
