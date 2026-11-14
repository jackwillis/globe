// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Classic PowerPC proof of life: Toolbox window, RGB555 GWorld and wake-driven input loop.
#include "about_window.hpp"
#include "classic_present.hpp"
#include "classic_wait.hpp"
#include "classic_window_fit.hpp"
#include "clock.hpp"
#include "globe/app.hpp"
#include "globe/card_text.hpp"
#include "globe/embedded_atlas.hpp"
#include "globe/embedded_countries.hpp"
#include "globe/embedded_elevation.hpp"
#include "globe/embedded_landclasses.hpp"
#include "globe/embedded_places.hpp"
#include "globe/looks.hpp"
#include "globe/menus.hpp"
#include "globe/overlays.hpp"
#include "globe/refine_runner.hpp"
#include "globe/shown_frame.hpp"
#include "globe/terrain.hpp"
#include "globe/zoom.hpp"
#include "menu_bar.hpp"
#include "pixel_format.hpp"
#include "prefs_window.hpp"
#include "scale_label.hpp"
#include "sidebar.hpp"
#include "wall_clock.hpp"
#include <Dialogs.h>
#include <Events.h>
#include <Memory.h>
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
// 'ALRT' id in os9/globe.r: shown when the application cannot start.
constexpr short kStartupAlert = 128;
std::uint64_t now_ms() {
    return monotonic_ns() / 1000000;
}
// The Globe window's content rectangle in global coordinates: the screen below the menu bar and
// title bar, less a margin. The sidebar takes its right `Sidebar::kWidth` columns and the globe the
// rest; both stay inside the sizes the grow box allows.
Rect window_content() {
    constexpr short kGap = 6, kTitleBar = 20;
    const Rect screen = (*GetMainDevice())->gdRect; // the menu bar's screen
    const short top = static_cast<short>(screen.top + GetMBarHeight() + kTitleBar + kGap);
    const short left = static_cast<short>(screen.left + kGap);
    const short width = std::clamp<short>(static_cast<short>(screen.right - screen.left - 2 * kGap),
                                          Sidebar::kWidth + kClassicMinGlobeWidth, 1600);
    const short height =
        std::clamp<short>(static_cast<short>(screen.bottom - kGap - top), kClassicMinHeight, 1200);
    return Rect{top, left, static_cast<short>(top + height), static_cast<short>(left + width)};
}
// Keep integer sizing testable without Toolbox; every actual allocation remains fallible.
void fit_to_memory(Rect& r) {
    // The heap zone starts small and grows on demand; until it spans the partition FreeMem sees
    // only a few KB (5,344 bytes at startup on the G4, which shrank the window to its minimum).
    MaxApplZone();
    const auto size =
        fit_classic_window_to_heap(r.right - r.left, r.bottom - r.top, Sidebar::kWidth, FreeMem());
    r.right = static_cast<short>(r.left + size.width);
    r.bottom = static_cast<short>(r.top + size.height);
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
        Rect bounds = window_content();
        fit_to_memory(bounds);
        const unsigned char title[] = {12,  'P', 'o', 'o', 'd', 'l', 'e',
                                       ' ', 'E', 'a', 'r', 't', 'h'};
        window_ = NewCWindow(nullptr, &bounds, title, true, documentProc,
                             reinterpret_cast<WindowPtr>(-1), true, 0);
        mouse_region_ = NewRgn();
        if (!window_ || !mouse_region_)
            return false;
        SetPortWindowPort(window_);
        const int width = bounds.right - bounds.left, height = bounds.bottom - bounds.top;
        const int globe_width = width - Sidebar::kWidth;
        if (!sidebar_.attach(window_) || !buffers(globe_width, height))
            return false;
        sidebar_.layout(static_cast<short>(width), static_cast<short>(height));
        sidebar_.sync(model_.terrain_style);
        sidebar_.show(nothing_selected_card());
        if (globe_width != model_.camera.width || height != model_.camera.height)
            apply(Resize{globe_width, height});
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
                if (part == inMenuBar) {
                    refresh_menus();
                    quit = menu_selected(MenuSelect(event.where), time);
                    // The held menu is not an event-loop turn: do not count it.
                    turn_start = now_ms();
                } else if (hit == window_) {
                    if (part == inContent) {
                        if (FrontWindow() != window_)
                            SelectWindow(window_);
                        else if (sidebar_.contains(local)) {
                            const int row = sidebar_.mouse_down(local);
                            SetPortWindowPort(window_);
                            if (const auto pick = looks_pick(model_.terrain_style, row))
                                quit = apply(CommandIssued{time, *pick});
                        } else
                            quit = apply(PointerDown{time, double(local.h), double(local.v)});
                    } else if (part == inDrag) {
                        Rect limit{20, -32000, 32000, 32000};
                        DragWindow(window_, event.where, &limit);
                        SetPortWindowPort(window_);
                    } else if (part == inGoAway)
                        quit = TrackGoAway(window_, event.where);
                    else if (part == inGrow) {
                        Rect limit{120, static_cast<short>(Sidebar::kWidth + 160), 1201, 1601};
                        const auto size = GrowWindow(window_, event.where, &limit);
                        if (size) {
                            const int width = static_cast<unsigned long>(size) & 0xffffU;
                            const int height = (static_cast<unsigned long>(size) >> 16) & 0xffffU;
                            const int globe_width = width - Sidebar::kWidth;
                            if (buffers(globe_width, height)) {
                                SizeWindow(window_, width, height, true);
                                sidebar_.layout(static_cast<short>(width),
                                                static_cast<short>(height));
                                apply(Resize{globe_width, height});
                                InvalRect(&window_->portRect); // the sidebar moved: redraw it
                            } else
                                SysBeep(10); // no memory for the larger buffers: keep the old size
                        }
                    }
                } else if (hit && hit == prefs_window_.window()) {
                    if (part == inGoAway) {
                        if (TrackGoAway(hit, event.where))
                            prefs_window_.close();
                    } else if (part == inDrag) {
                        Rect limit{20, -32000, 32000, 32000};
                        DragWindow(hit, event.where, &limit);
                    } else if (part == inContent) {
                        if (FrontWindow() != hit)
                            SelectWindow(hit);
                        else {
                            SetPortWindowPort(hit);
                            Point inside = event.where;
                            GlobalToLocal(&inside);
                            if (const auto next = prefs_window_.mouse_down(inside))
                                set_preferences(*next, time);
                        }
                    }
                    SetPortWindowPort(window_);
                } else if (hit && hit == about_.window()) {
                    if (part == inGoAway) {
                        if (TrackGoAway(hit, event.where))
                            about_.close();
                    } else if (part == inDrag) {
                        Rect limit{20, -32000, 32000, 32000};
                        DragWindow(hit, event.where, &limit);
                    } else if (part == inContent && FrontWindow() != hit)
                        SelectWindow(hit);
                    SetPortWindowPort(window_);
                } else if (part == inSysWindow)
                    SystemClick(&event, hit);
            } else if (event.what == mouseUp) {
                quit = apply(PointerUp{time});
                // A refinement finished (or advanced) while the button was down: show it now.
                if (dirty_ && model_.stage != QualityStage::Moving && !redraw_)
                    presentation_.request(model_.generation);
            } else if ((event.what == keyDown || event.what == autoKey) &&
                       (event.modifiers & cmdKey)) {
                refresh_menus();
                // Command-= is Zoom In (the "+" key without Shift on a US keyboard).
                auto key_char = static_cast<CharParameter>(event.message & 0xff);
                if (key_char == '=')
                    key_char = '+';
                const long result = MenuKey(key_char);
                if (result != 0)
                    quit = menu_selected(result, time);
                turn_start = now_ms();
            } else if (event.what == keyDown || event.what == autoKey)
                quit = key(event.message, time);
            else if (event.what == updateEvt) {
                if (prefs_window_.window() &&
                    reinterpret_cast<WindowPtr>(event.message) == prefs_window_.window()) {
                    BeginUpdate(prefs_window_.window());
                    prefs_window_.draw();
                    EndUpdate(prefs_window_.window());
                    SetPortWindowPort(window_);
                } else if (about_.window() &&
                           reinterpret_cast<WindowPtr>(event.message) == about_.window()) {
                    BeginUpdate(about_.window());
                    about_.draw();
                    EndUpdate(about_.window());
                    SetPortWindowPort(window_);
                } else if (reinterpret_cast<WindowPtr>(event.message) == window_) {
                    BeginUpdate(window_);
                    // Background update events repaint the cached GWorld, never convert terrain.
                    copy(0, fb_->height(),
                         foreground_ && !presentation_.pending(model_.generation));
                    sidebar_.draw();
                    EndUpdate(window_);
                }
            } else if (event.what == activateEvt) {
                if (reinterpret_cast<WindowPtr>(event.message) == window_) {
                    sidebar_.activate(event.modifiers & activeFlag);
                    sidebar_.draw();
                }
                if (!(event.modifiers & activeFlag))
                    apply(PointerUp{time});
            } else if (event.what != nullEvent && event.what != activateEvt &&
                       event.what != diskEvt && event.what != osEvt) {
                // Not an event this program knows (a scroll wheel, if the system sends one to a
                // non-Carbon program, would show up here): remember the code for the title.
                last_unknown_ = static_cast<unsigned>(event.what);
                ++unknown_count_;
                show_timing(); // refresh the title so the code is visible
            }
            sidebar_.sync(model_.terrain_style); // the check follows the look, whoever changed it
            SetPortWindowPort(window_);
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
            const bool over_globe = position.x < sidebar_.left();
            const auto intent = FrontWindow() == window_ && over_globe
                                    ? cursor_for(model_, position, target)
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
        scale_label_.sync(model_.camera, fb_->width(), model_.prefs.units);
        shown_.set_scale_label(scale_label_.bitmap());
        shown_.set_scale_units(model_.prefs.units);
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
    // With GLOBE_TIMING_TITLE (cmake -DGLOBE_TIMING_TITLE=ON, for measurement builds) the window
    // title reports the last settle: first refine step to last, and time spent copying. Release
    // builds keep the plain title "Poodle Earth".
    void show_timing() {
        if (!titled_)
            settled_ms_ = now_ms() - refine_t0_;
        titled_ = true;
#ifdef GLOBE_TIMING_TITLE
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
        put("Poodle Earth ");
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
#endif
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
            scale_label_.sync(model_.camera, fb_->width(), model_.prefs.units);
            shown_.set_scale_label(scale_label_.bitmap());
            shown_.set_scale_units(model_.prefs.units);
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
            sidebar_.show(*card);
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
    // An overlay or window the front end handles itself, from a shortcut key or a menu item.
    void overlay(KeyBinding::Overlay which) {
        if (which == KeyBinding::Overlay::Night) {
            overlays_.toggle_night();
            reshow();
        } else if (which == KeyBinding::Overlay::Measure) {
            overlays_.toggle_measure();
            hover_valid_ = false; // measuring makes the whole globe clickable
            reshow();
        }
    }
    // Check marks and dimming for the menu about to be shown; the model is the only state.
    void refresh_menus() {
        if (const MenuHandle view = GetMenuHandle(kViewMenu)) {
            for (short item = 1; item <= kViewItemCount; ++item) {
                const auto s =
                    view_item_state(model_, overlays_.night(), overlays_.measuring(), item);
                if (s.enabled)
                    EnableItem(view, item);
                else
                    DisableItem(view, item);
                CheckItem(view, item, s.checked);
            }
        }
        // Close is for the About and Preferences windows (or a desk accessory) in front; the globe
        // window leaves the program through Quit.
        const WindowPtr front = FrontWindow();
        if (const MenuHandle file = GetMenuHandle(kFileMenu)) {
            if (front != nullptr && front != window_)
                EnableItem(file, kCloseItem);
            else
                DisableItem(file, kCloseItem);
        }
        // Edit is for desk accessories, except Copy, which copies the information card.
        const bool accessory = front != nullptr && GetWindowKind(front) < 0;
        if (const MenuHandle edit = GetMenuHandle(kEditMenu)) {
            for (short item = 1; item <= 6; ++item) {
                if (accessory || item == kCopyItem)
                    EnableItem(edit, item);
                else
                    DisableItem(edit, item);
            }
        }
    }
    // A MenuSelect/MenuKey result: the menu id in the high word, the item in the low. Returns true
    // to quit.
    bool menu_selected(long result, std::uint64_t time) {
        const short menu = static_cast<short>(result >> 16);
        const short item = static_cast<short>(result & 0xffff);
        bool quit = false;
        if (menu == kAppleMenu && item > kAboutItem + 1) {
            unsigned char name[256];
            GetMenuItemText(GetMenuHandle(kAppleMenu), item, name);
            OpenDeskAcc(name);
            SetPortWindowPort(window_);
        } else if (menu == kEditMenu && item == kPreferencesItem) {
            prefs_window_.open(model_.prefs);
            SetPortWindowPort(window_);
        } else if (menu == kEditMenu && item == kCopyItem && !accessory_in_front()) {
            copy_card();
        } else if (menu == kEditMenu) {
            SystemEdit(item - 1);
        } else {
            const auto choice = menu_choice(menu, item, model_.terrain_style);
            if (choice.about)
                show_about();
            else if (choice.close)
                close_front();
            else if (choice.command)
                quit = apply(CommandIssued{time, *choice.command});
            else
                overlay(choice.overlay);
        }
        HiliteMenu(0);
        return quit;
    }
    // File > Close: hides the About or Preferences window in front, or closes a desk accessory.
    void close_front() {
        const WindowPtr front = FrontWindow();
        if (front == nullptr || front == window_)
            return;
        if (front == about_.window())
            about_.close();
        else if (front == prefs_window_.window())
            prefs_window_.close();
        else if (GetWindowKind(front) < 0)
            CloseDeskAcc(GetWindowKind(front));
        SetPortWindowPort(window_);
    }
    bool accessory_in_front() const {
        const WindowPtr front = FrontWindow();
        return front != nullptr && GetWindowKind(front) < 0;
    }
    // Copy Information: the whole card shown, as TEXT on the scrap (lines separated by CR).
    void copy_card() {
        char text[1024];
        const auto length = card_text(sidebar_.card(), text, sizeof text, '\r');
        ZeroScrap();
        PutScrap(static_cast<long>(length), 'TEXT', text);
    }
    // Preferences go through update() like any other state; the text and windows follow the Model.
    void set_preferences(const Preferences& next, std::uint64_t) {
        apply(SetPreferences{next});
        overlays_.set_preferences(model_.prefs);
        if (const auto card = overlays_.take_card())
            sidebar_.show(*card); // a finished measurement follows the units
        reshow();                 // and so does the scale on the map
        prefs_window_.sync(model_.prefs);
        SetPortWindowPort(window_);
    }
    void show_about() {
        about_.open();
        SetPortWindowPort(window_);
    }
    bool key(unsigned long message, std::uint64_t time) {
        // The About and Preferences windows take no globe shortcuts, and a bare Q or Escape must
        // not quit: Quit is Command-Q.
        if (FrontWindow() != window_)
            return false;
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
        if (command == Command::Quit)
            return false;
        if (command)
            return apply(CommandIssued{time, *command});
        overlay(binding.overlay);
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
    Sidebar sidebar_;
    AboutWindow about_;
    ScaleLabel scale_label_; // the scale's label in Monaco
    PrefsWindow prefs_window_;
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
    if (const Handle bar = GetNewMBar(128)) {
        SetMenuBar(bar);
        AppendResMenu(GetMenuHandle(kAppleMenu), 'DRVR');
        DrawMenuBar();
    }
    TEInit();
    InitDialogs(nullptr);
    InitCursor();
    bool started = false;
    {
        Application app;
        started = app.init();
        if (started)
            app.run();
    } // the partly built application releases its memory before the alert needs some
    if (!started) {
        SysBeep(10);
        StopAlert(kStartupAlert, nullptr);
        return 1;
    }
    return 0;
}
