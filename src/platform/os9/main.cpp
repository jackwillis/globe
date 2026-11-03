// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Classic PowerPC proof of life: Toolbox window, RGB555 GWorld and wake-driven input loop.
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
constexpr std::uint64_t kWorkSliceMs = 24;
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
        while (!quit) {
            const auto before = now_ms();
            if (wake_ && before >= wake_) {
                wake_ = 0;
                apply(Tick{before});
            }
            if (redraw_) {
                render_globe(*fb_, model_.camera, *scene_, kDefaultPalette,
                             effective_layers(model_), true, quality_for(model_), &*terrain_);
                copy(0, fb_->height());
                redraw_ = false;
            }
            const bool work = job_ && job_->active() && !job_waiting_;
            if (work) {
                // Yielding to the system costs a tick or more on Classic, so run refine steps in a
                // slice of kWorkSliceMs before each WaitNextEvent (each step is itself bounded).
                const auto slice_end = now_ms() + kWorkSliceMs;
                do {
                    const auto band = job_->step(*fb_, model_.generation);
                    if (band.band_ready)
                        present(band.first_row, band.end_row);
                    if (band.wake_after_ms) {
                        job_waiting_ = true;
                        wake_ = now_ms() + band.wake_after_ms;
                        break;
                    }
                } while (job_->active() && now_ms() < slice_end);
                if (!job_->active()) {
                    if (dirty_ &&
                        !presentation_held(model_)) { // the last throttled frame is always shown
                        copy(0, fb_->height());
                        dirty_ = false;
                    }
                    if (!titled_)
                        show_timing();
                }
            }
            Point pointer;
            GetMouse(&pointer);
            LocalToGlobal(&pointer);
            Rect mouse{pointer.v, pointer.h, static_cast<short>(pointer.v + 1),
                       static_cast<short>(pointer.h + 1)};
            RectRgn(mouse_region_, &mouse);
            EventRecord event{};
            WaitNextEvent(everyEvent, &event,
                          static_cast<long>(classic_wait_ticks(
                              now_ms(), wake_, job_ && job_->active() && !job_waiting_)),
                          mouse_region_);
            const auto time = now_ms();
            SetPortWindowPort(window_);
            Point local = event.where;
            GlobalToLocal(&local);
            if (event.what == mouseDown) {
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
                    copy(0, fb_->height());
            } else if (event.what == keyDown || event.what == autoKey)
                quit = key(event.message, time);
            else if (event.what == updateEvt) {
                if (reinterpret_cast<WindowPtr>(event.message) == window_) {
                    BeginUpdate(window_);
                    copy(0, fb_->height());
                    EndUpdate(window_);
                }
            } else if (event.what == activateEvt && !(event.modifiers & activeFlag))
                apply(PointerUp{time});
            // A one-pixel mouse region wakes on motion, even with no button held. No periodic poll.
            Point current;
            GetMouse(&current);
            const ScreenPoint position{double(current.h), double(current.v)};
            if (model_.dragging &&
                (position.x != model_.drag_last.x || position.y != model_.drag_last.y))
                apply(PointerMove{time, position.x, position.y});
            const auto intent =
                FrontWindow() == window_ ? cursor_for(model_, position) : CursorIntent::Arrow;
            if (intent != cursor_) {
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
        if (world_)
            DisposeGWorld(world_);
        world_ = next;
        fb_ = std::move(fb);
        return true;
    }
    // Copying a frame costs tens of milliseconds on Classic, so a refinement frame is shown only
    // when the last copies took less than a quarter of the time since they started; skipped frames
    // are caught up by the next shown one, and the final frame is always shown.
    void present(int first, int end) {
        const auto start = now_ms();
        if (presentation_held(model_) || start < next_copy_ms_) {
            dirty_ = true;
            return;
        }
        copy(first, end);
        dirty_ = false;
        next_copy_ms_ = start + 4 * (now_ms() - start);
    }
    // The window title reports the last settle: first refine step to last, and time spent copying.
    void show_timing() {
        titled_ = true;
        unsigned char title[40];
        unsigned n = 0;
        const auto put = [&](const char* text) {
            while (*text && n < 39)
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
        title[0] = static_cast<unsigned char>(n);
        SetWTitle(window_, title);
    }
    void copy(int first, int end) {
        if (!world_ || !fb_)
            return;
        dirty_ = false;
        const auto copy_t0 = now_ms();
        struct Timer {
            std::uint64_t& total;
            std::uint64_t t0;
            ~Timer() { total += now_ms() - t0; }
        } timer{copy_ms_, copy_t0};
        const auto pix = GetGWorldPixMap(world_);
        if (!LockPixels(pix))
            return;
        const PixelFormat format{16, 0x7c00, 0x03e0, 0x001f, true};
        const auto stride = static_cast<std::uint16_t>((**pix).rowBytes) & 0x3fffU;
        const bool converted = convert_pixels(
            *fb_, format, reinterpret_cast<std::uint8_t*>(GetPixBaseAddr(pix)), stride);
        if (converted) {
            SetPortWindowPort(window_);
            Rect rect{static_cast<short>(first), 0, static_cast<short>(end),
                      static_cast<short>(fb_->width())};
            CopyBits(reinterpret_cast<BitMap*>(*pix), &window_->portBits, &rect, &rect, srcCopy,
                     nullptr);
        }
        UnlockPixels(pix);
    }
    bool apply(const Msg& msg) {
        if (std::holds_alternative<Tick>(msg) && job_waiting_) {
            job_waiting_ = false;
            wake_ = 0;
        }
        const auto generation = model_.generation;
        const auto stage = model_.stage;
        auto fx = update(model_, msg);
        if (generation != model_.generation && job_) {
            job_waiting_ = false;
            job_->cancel();
            if (stage == QualityStage::Refine)
                fx.redraw = true;
        }
        if (model_.stage == QualityStage::Refine && stage != QualityStage::Refine) {
            refine_t0_ = now_ms();
            copy_ms_ = 0;
            next_copy_ms_ = 0;
            dirty_ = false;
            titled_ = false;
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
    std::uint64_t next_copy_ms_ = 0;
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
