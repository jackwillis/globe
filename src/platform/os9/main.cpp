// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Classic PowerPC proof of life: Toolbox window, RGB555 GWorld and wake-driven input loop.
#include "classic_present.hpp"
#include "classic_wait.hpp"
#include "clock.hpp"
#include "globe/app.hpp"
#include "globe/embedded_atlas.hpp"
#include "globe/embedded_countries.hpp"
#include "globe/embedded_elevation.hpp"
#include "globe/embedded_landclasses.hpp"
#include "globe/embedded_places.hpp"
#include "globe/overlays.hpp"
#include "globe/refine_runner.hpp"
#include "globe/shown_frame.hpp"
#include "globe/terrain.hpp"
#include "globe/zoom.hpp"
#include "info_card.hpp"
#include "pixel_format.hpp"
#include "wall_clock.hpp"
#include <Dialogs.h>
#include <Events.h>
#include <Menus.h>
#include <OSUtils.h>
#include <Quickdraw.h>
#include <TextEdit.h>
#include <Windows.h>
#include <algorithm>
#include <cstring>
#include <optional>
#include <vector>
// These Appearance Manager declarations are absent from the installed Multiversal interfaces.
// AppearanceLib exports the function (checked with nm on libAppearanceLib.a); OS 9 provides
// arrow=0, closed hand=8, open hand=9, pointing hand=10 (Appearance Manager reference, not in the
// headers).
extern "C" std::int32_t SetThemeCursor(std::uint32_t cursor);
using namespace globe;
static_assert(next_terrain_style(TerrainStyle::Map) == TerrainStyle::Relief);
static_assert(next_terrain_style(TerrainStyle::Relief) == TerrainStyle::Map);
namespace {
// Prefer short foreground turns to maximum settle throughput. Copy time counts in the slice.
constexpr std::uint64_t kWorkSliceMs = 16;
constexpr unsigned kWorkUnitsPerStep = 64;
// Aim each slice of a frame copied to the window at about this long, so a frame wipes in as fast as
// the machine allows without holding up input (the work slice above is 16 ms).
constexpr std::uint64_t kPresentSliceMs = 6;
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
        // Picking data are optional: without them clicks do nothing and the overlays still work.
        countries_ = Countries::parse(kEmbeddedCountriesData, kEmbeddedCountriesSize);
        if (countries_)
            country_scene_ = CountryScene::create(*countries_);
        places_ = Places::parse(kEmbeddedPlacesData, kEmbeddedPlacesSize);
        if (places_)
            place_scene_ = PlaceScene::create(*places_);
        overlays_.attach(countries_ ? &*countries_ : nullptr,
                         country_scene_ ? &*country_scene_ : nullptr, places_ ? &*places_ : nullptr,
                         place_scene_ ? &*place_scene_ : nullptr);
        runner_.set_scene(*scene_);
        runner_.set_places(place_scene_ ? &*place_scene_ : nullptr);
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
                runner_.render_moving(*fb_, model_, &*terrain_);
                copy(0, fb_->height());
                redraw_ = false;
            }
            // Give presentation a bounded share before geometry can consume the whole turn.
            if (foreground_ && !presentation_held(model_) && now_ms() - turn_start < kWorkSliceMs)
                present_step();
            const bool work = foreground_ && runner_.active();
            if (work && now_ms() - turn_start < kWorkSliceMs) {
                // Check elapsed time after every bounded step AND presentation. A slow step or
                // Toolbox copy can still overrun: measure the full turn, not just geometry.
                const auto slice_end = turn_start + kWorkSliceMs;
                auto step_start = now_ms();
                unsigned steps = 0;
                do {
                    const auto band = runner_.step(*fb_, model_, kWorkUnitsPerStep);
                    auto step_end = now_ms();
                    max_step_ms_ = std::max(max_step_ms_, step_end - step_start);
                    if (band.band_ready) {
                        present(band.first_row, band.end_row);
                        step_end = now_ms();
                    }
                    if (!runner_.active() && model_.refine_active) {
                        apply(RefinementFinished{model_.generation, model_.refine_level});
                        break;
                    }
                    step_start = step_end;
                    EventRecord pending{};
                    if (++steps % 16 == 0 && EventAvail(everyEvent, &pending))
                        break; // dispatch queued input before spending another geometry budget
                } while (runner_.active() && step_start < slice_end);
                // The final snapshot includes the full settled overlays.
                if (!runner_.active() && (dirty_ || shown_quick_ || bar_shown_) &&
                    !presentation_held(model_))
                    presentation_.request(model_.generation);
            }
            if (foreground_)
                max_turn_ms_ = std::max(max_turn_ms_, now_ms() - turn_start);
            if (foreground_ && model_.stage == QualityStage::Refine && runner_.job() != nullptr &&
                !runner_.active() && !titled_ && !presentation_.pending(model_.generation) &&
                !dirty_)
                show_timing();
            Point pointer;
            GetMouse(&pointer);
            LocalToGlobal(&pointer);
            Rect mouse{pointer.v, pointer.h, static_cast<short>(pointer.v + 1),
                       static_cast<short>(pointer.h + 1)};
            RectRgn(mouse_region_, &mouse);
            EventRecord event{};
            WaitNextEvent(everyEvent, &event,
                          static_cast<long>(classic_wait_ticks(
                              now_ms(), wake_,
                              runner_.active() || (!presentation_held(model_) &&
                                                   presentation_.pending(model_.generation)),
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
                } else if (hit == info_.window())
                    info_.mouse_down(event);
                else if (part == inSysWindow)
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
                } else if (reinterpret_cast<WindowPtr>(event.message) == info_.window()) {
                    BeginUpdate(info_.window());
                    info_.draw();
                    EndUpdate(info_.window());
                    SetPortWindowPort(window_);
                }
            } else if (event.what == activateEvt && !(event.modifiers & activeFlag))
                apply(PointerUp{time});
            else if (event.what != nullEvent && event.what != activateEvt &&
                     event.what != diskEvt && event.what != osEvt) {
                // Not an event this program knows (a scroll wheel, if the system sends one to a
                // non-Carbon program, would show up here): remember the code for the title.
                last_unknown_ = static_cast<unsigned>(event.what);
                ++unknown_count_;
                show_timing(); // refresh the title so the code is visible
            }
            // A one-pixel mouse region wakes on motion, even with no button held. No periodic poll.
            Point current;
            GetMouse(&current);
            const ScreenPoint position{double(current.h), double(current.v)};
            if (foreground_ && model_.dragging &&
                (position.x != model_.drag_last.x || position.y != model_.drag_last.y))
                apply(PointerMove{time, position.x, position.y});
            // Only the cheap hover test runs, and only when the pointer, view or selection moved:
            // the country pick that follows a click is the expensive one.
            const bool selected = overlays_.focused().has_value();
            if (!hover_valid_ || position.x != hover_at_.x || position.y != hover_at_.y ||
                selected != hover_.selection || generation_seen_ != model_.generation) {
                hover_ = {selected, overlays_.clickable_at(model_.camera, position)};
                hover_at_ = position;
                generation_seen_ = model_.generation;
                hover_valid_ = true;
            }
            const auto target = hover_;
            const auto intent = FrontWindow() == window_ ? cursor_for(model_, position, target)
                                                         : CursorIntent::Arrow;
            if (foreground_ && intent != cursor_) {
                cursor_ = intent;
                const auto native = intent == CursorIntent::OpenHand       ? 9U
                                    : intent == CursorIntent::ClosedHand   ? 8U
                                    : intent == CursorIntent::PointingHand ? 10U
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
        runner_.cancel(nullptr, model_.camera);
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
            if (!capture()) {
                presentation_.cancel();
                dirty_ = true; // retry on a later event/update, never busy-spin on a failed lock
                return;
            }
            presentation_.begin(model_.generation, fb_->height());
            dirty_ = false;
            return; // conversion and submission never share the same turn
        }
        if (presentation_.active(model_.generation)) {
            const int first = presentation_.first(), end = presentation_.end();
            const auto copy_start = now_ms();
            if (copy(first, end, false)) {
                presentation_.tune(end - first, now_ms() - copy_start, kPresentSliceMs);
                presentation_.submitted(model_.generation);
                ++present_bands_;
            } else {
                presentation_.cancel();
                dirty_ = true;
            }
        }
    }
    bool capture() {
        const auto start = now_ms();
        const auto pix = GetGWorldPixMap(world_);
        if (!LockPixels(pix))
            return false;
        const PixelFormat format{16, 0x7c00, 0x03e0, 0x001f, true};
        const auto stride = static_cast<std::uint16_t>((**pix).rowBytes) & 0x3fffU;
        const bool settled = frame_settled(model_, runner_.active());
        const auto& shown = shown_.begin(overlays_, *fb_, model_.camera, unix_seconds(), settled,
                                         bar_progress(model_, runner_.job()));
        shown_quick_ = shown_.quick();
        bar_shown_ = shown_.bar();
        const bool ok = convert_pixels(
            shown, format, reinterpret_cast<std::uint8_t*>(GetPixBaseAddr(pix)), stride);
        shown_.end();
        UnlockPixels(pix); // no locked handles survive a yield
        copy_ms_ += now_ms() - start;
        return ok;
    }
    // The window title reports the last settle: first refine step to last, and time spent
    // copying.
    void show_timing() {
        if (!titled_)
            settled_ms_ = now_ms() - refine_t0_;
        titled_ = true;
        unsigned char title[128];
        unsigned n = 0;
        const auto put = [&](const char* text) {
            while (*text && n < 127)
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
        put("Globe ");
        number(present_bands_);
        put(" bands, ");
        number(settled_ms_);
        put(" ms, copy ");
        number(copy_ms_);
        put(" ms, turn ");
        number(max_turn_ms_);
        put(" ms, step ");
        number(max_step_ms_);
        put(" ms");
        if (unknown_count_ != 0) {
            put(", ev ");
            number(last_unknown_);
            put("x");
            number(unknown_count_);
        }
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
        // Overlays (night, measure, focused country) go on a copy; the frame itself is
        // untouched. Frames that move or are still refining get the cheap overlays; the settled
        // one the full.
        bool converted = true;
        if (convert) { // else the rows were converted by capture(); only submit them
            const bool settled = frame_settled(model_, runner_.active());
            const auto& shown = shown_.begin(overlays_, *fb_, model_.camera, unix_seconds(),
                                             settled, bar_progress(model_, runner_.job()));
            shown_quick_ = shown_.quick();
            bar_shown_ = shown_.bar();
            converted = convert_pixel_rows(shown, format,
                                           reinterpret_cast<std::uint8_t*>(GetPixBaseAddr(pix)),
                                           stride, first, end);
            shown_.end();
        }
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
        const auto generation = model_.generation;
        const auto stage = model_.stage;
        auto fx = update(model_, msg);
        if (generation != model_.generation) {
            presentation_.cancel();
            runner_.cancel(fb_ ? &*fb_ : nullptr, model_.camera);
            if (stage == QualityStage::Refine)
                fx.redraw = true;
        }
        if (fx.refine && stage != QualityStage::Refine) {
            refine_t0_ = now_ms();
            copy_ms_ = 0;
            present_bands_ = 0;
            max_turn_ms_ = max_step_ms_ = 0;
            dirty_ = false;
            titled_ = false;
        }
        if (fx.refine) {
            runner_.start(model_, fb_ ? &*fb_ : nullptr, &*terrain_);
        }
        if (std::holds_alternative<Resize>(msg))
            fx.wake_after_ms = update(model_, Tick{now_ms()}).wake_after_ms;
        if (fx.wake_after_ms)
            wake_ = now_ms() + fx.wake_after_ms;
        redraw_ |= fx.redraw;
        if (fx.pick)
            pick(fx.pick_at);
        return fx.quit;
    }
    // A click that waited out the double-click window: pick, show a card, and draw the overlay.
    void pick(ScreenPoint at) {
        const bool changed = overlays_.click(model_.camera, at);
        if (const auto card = overlays_.take_card()) {
            SetPortWindowPort(window_);
            Point corner{window_->portRect.top, window_->portRect.right};
            LocalToGlobal(&corner);
            info_.show(*card, corner.h);
            SetPortWindowPort(window_);
        }
        if (changed)
            reshow();
    }
    // Shows the current frame again (an overlay changed); no render, no refine.
    void reshow() {
        if (!redraw_ && fb_)
            copy(0, fb_->height());
    }
    bool key(unsigned long message, std::uint64_t time) {
        const auto code = (message >> 8) & 0xff;
        const auto ch = static_cast<int>(message & 0xff);
        // Arrow and Page keys are decoded here (ADB codes); typed characters go through the shared
        // table so both front ends bind the same shortcuts.
        std::optional<Command> command;
        if (code == 0x7b)
            command = Command::Left;
        else if (code == 0x7c)
            command = Command::Right;
        else if (code == 0x7d)
            command = Command::Down;
        else if (code == 0x7e)
            command = Command::Up;
        else if (code == 0x74) // Page Up: wheel drivers (USB Overdrive and the like) send it
            command = Command::ZoomIn;
        else if (code == 0x79) // Page Down
            command = Command::ZoomOut;
        const auto binding = binding_for_char(ch);
        if (!command)
            command = binding.command;
        if (command)
            return apply(CommandIssued{time, *command});
        if (binding.overlay == KeyBinding::Overlay::Night) {
            overlays_.toggle_night();
            reshow();
        } else if (binding.overlay == KeyBinding::Overlay::Measure) {
            overlays_.toggle_measure();
            hover_valid_ = false; // measuring makes the whole globe clickable
            reshow();
        }
        return false;
    }
    WindowPtr window_ = nullptr;
    GWorldPtr world_ = nullptr;
    RgnHandle mouse_region_ = nullptr;
    Model model_ = initial_model(640, 480);
    std::optional<Scene> scene_;
    std::optional<LandClasses> classes_;
    std::optional<TerrainScene> terrain_;
    std::optional<Countries> countries_;
    std::optional<CountryScene> country_scene_;
    std::optional<Places> places_;
    std::optional<PlaceScene> place_scene_;
    Overlays overlays_;
    InfoCardWindow info_;
    std::optional<Framebuffer> fb_;
    RefineRunner runner_; // the refinement job, the zoom preview and the markers
    std::uint64_t wake_ = 0;
    std::uint64_t refine_t0_ = 0, copy_ms_ = 0;
    bool titled_ = true, dirty_ = false, shown_quick_ = false;
    ShownFrame shown_; // overlays and the bar, built on a copy of the frame for each conversion
    bool bar_shown_ = false;
    unsigned last_unknown_ = 0, unknown_count_ = 0;
    std::uint64_t settled_ms_ = 0;
    ClassicPresentation presentation_;
    unsigned present_bands_ = 0;
    bool foreground_ = true;
    std::uint64_t max_turn_ms_ = 0, max_step_ms_ = 0;
    bool redraw_ = true;
    CursorIntent cursor_ = CursorIntent::Arrow;
    CursorTarget hover_;
    ScreenPoint hover_at_{};
    std::uint64_t generation_seen_ = 0;
    bool hover_valid_ = false;
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
