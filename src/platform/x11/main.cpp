// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// X11 front end: one window showing the globe. Sleeps until input or a one-shot wake, turns
// X events into Msgs, runs update(), and renders only when update() asks for a redraw.
//
//   globe-x11 [--atlas FILE] [--elevation FILE] [--size WxH] [--script TRACE [--exit-when-done]]
//             [--idle-seconds N] [--verify-pixels] [--stats] [--no-zoom-preview]
//             [--finish-refinement]
//
// Mouse: drag the left button to rotate, wheel to zoom. Keys: arrows rotate, + / - zoom, R reset,
// G / B toggle graticule / borders, N aligns north up, T switches between
// turntable and trackball dragging, S switches the terrain style, P opens the looks palette, D
// shades the night side, Esc or Q quits.
//
// --script feeds a recorded trace (see globe/trace.hpp) after the first exposure, rendering once
// per redraw the way interactive use would. --idle-seconds waits that long afterwards and reports
// CPU time used and frames drawn while idle. --stats prints a JSON timing summary.
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <sys/resource.h>
#include <sys/select.h>

#include <array>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

#include "cli.hpp"
#include "clock.hpp"
#include "elevation_packs.hpp"
#include "frame_stats.hpp"
#include "globe/app.hpp"
#include "globe/atlas.hpp"
#include "globe/embedded_atlas.hpp"
#include "globe/embedded_countries.hpp"
#include "globe/embedded_elevation.hpp"
#include "globe/embedded_fineelevation.hpp"
#include "globe/embedded_places.hpp"
#include "globe/overlays.hpp"
#include "globe/places.hpp"
#include "globe/refine_runner.hpp"
#include "globe/shown_frame.hpp"
#include "globe/terrain.hpp"
#include "globe/trace.hpp"
#include "globe/trackball.hpp"
#include "globe/zoom.hpp"
#include "info_window.hpp"
#include "pixel_format.hpp"
#include "style_window.hpp"

using namespace globe;

namespace {

struct Options {
    const char* atlas_path = nullptr;
    std::array<const char*, 3> elevation_paths{};
    unsigned elevation_count = 0;
    const char* script_path = nullptr;
    int width = 640;
    int height = 480;
    bool exit_when_done = false;
    bool stats = false;
    bool verify_pixels = false;
    int idle_seconds = 0;
    bool no_zoom_preview = false;
    bool finish_refinement =
        false; // script: run each refinement to its end before the next message
};

struct Timings {
    std::vector<double> render_ms;
    std::vector<double> pack_ms;
    std::vector<double> present_ms;
};

bool read_file(const char* path, std::vector<std::uint8_t>& out) {
    std::FILE* f = std::fopen(path, "rb");
    if (f == nullptr) {
        return false;
    }
    std::uint8_t buf[8192];
    std::size_t n = 0;
    out.clear();
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) {
        out.insert(out.end(), buf, buf + n);
    }
    std::fclose(f);
    return true;
}

double ms_since(std::uint64_t t0) {
    return static_cast<double>(monotonic_ns() - t0) / 1.0e6;
}

class App {
  public:
    App(Display* dpy, const Scene& scene, ElevationPacks& packs, const Options& opt)
        : dpy_(dpy), scene_(scene), packs_(packs), opt_(opt),
          model_(initial_model(opt.width, opt.height)), info_(dpy), styles_(dpy) {
        packs_.register_levels(model_);
        places_ = Places::parse(kEmbeddedPlacesData, kEmbeddedPlacesSize);
        if (places_)
            place_scene_ = PlaceScene::create(*places_);
        countries_ = Countries::parse(kEmbeddedCountriesData, kEmbeddedCountriesSize);
        if (countries_)
            country_scene_ = CountryScene::create(*countries_);
        overlays_.attach(countries_ ? &*countries_ : nullptr,
                         country_scene_ ? &*country_scene_ : nullptr, places_ ? &*places_ : nullptr,
                         place_scene_ ? &*place_scene_ : nullptr);
        runner_.set_places(place_scene_ ? &*place_scene_ : nullptr);
        runner_.enable_preview(!opt.no_zoom_preview);
    }

    ~App() {
        for (auto cursor : cursors_)
            if (cursor)
                XFreeCursor(dpy_, cursor);
        if (toolbar_)
            XDestroyWindow(dpy_, toolbar_);
        if (image_ != nullptr) {
            XDestroyImage(image_);
        }
        if (gc_ != nullptr) {
            XFreeGC(dpy_, gc_);
        }
        if (win_ != 0) {
            XDestroyWindow(dpy_, win_);
        }
    }

    bool init() {
        const int screen = DefaultScreen(dpy_);
        Visual* visual = DefaultVisual(dpy_, screen);
        if (visual->c_class != TrueColor && visual->c_class != DirectColor) {
            std::fprintf(stderr, "globe-x11: default visual is not TrueColor/DirectColor\n");
            return false;
        }
        depth_ = DefaultDepth(dpy_, screen);
        visual_ = visual;
        win_ = XCreateSimpleWindow(
            dpy_, RootWindow(dpy_, screen), 0, 0, static_cast<unsigned>(opt_.width),
            static_cast<unsigned>(opt_.height), 0, 0, BlackPixel(dpy_, screen));
        XStoreName(dpy_, win_, "Globe");
        XSelectInput(dpy_, win_,
                     ExposureMask | StructureNotifyMask | ButtonPressMask | ButtonReleaseMask |
                         PointerMotionMask | KeyPressMask | EnterWindowMask | LeaveWindowMask);
        wm_delete_ = XInternAtom(dpy_, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(dpy_, win_, &wm_delete_, 1);
        gc_ = XCreateGC(dpy_, win_, 0, nullptr);
        if (!make_buffers(opt_.width, opt_.height)) {
            return false;
        }
        toolbar_ = XCreateSimpleWindow(dpy_, RootWindow(dpy_, screen), 0, opt_.height + 20,
                                       kToolbarWidth, kToolbarHeight, 1, BlackPixel(dpy_, screen),
                                       WhitePixel(dpy_, screen));
        XStoreName(dpy_, toolbar_, "Globe tools");
        XSetTransientForHint(dpy_, toolbar_, win_);
        const Atom kind = XInternAtom(dpy_, "_NET_WM_WINDOW_TYPE_UTILITY", False);
        XChangeProperty(dpy_, toolbar_, XInternAtom(dpy_, "_NET_WM_WINDOW_TYPE", False), XA_ATOM,
                        32, PropModeReplace, reinterpret_cast<const unsigned char*>(&kind), 1);
        XSetWMProtocols(dpy_, toolbar_, &wm_delete_, 1);
        XSelectInput(dpy_, toolbar_, ExposureMask | ButtonPressMask | KeyPressMask);
        cursors_ = {XCreateFontCursor(dpy_, XC_left_ptr), XCreateFontCursor(dpy_, XC_hand2),
                    XCreateFontCursor(dpy_, XC_fleur)};
        XDefineCursor(dpy_, win_, cursors_[0]);
        XMapWindow(dpy_, win_);
        XMapWindow(dpy_, toolbar_);
        return true;
    }

    int run() {
        if (script_active_ && !load_script()) {
            return 2;
        }
        apply(Tick{monotonic_ns() / 1000000ULL}); // establish the initial settle clock
        bool quit = false;
        while (!quit) {
            if (!wait_events()) {
                return 2;
            }
            // How long this turn keeps the loop from looking at input: the worst one bounds the
            // latency of any click or wheel step, refining or not.
            const std::uint64_t turn_start = monotonic_ns();
            const bool refining = runner_.active();
            XEvent ev;
            while (!quit && XPending(dpy_) > 0) {
                XNextEvent(dpy_, &ev);
                quit = handle(ev);
            }
            if (!quit && need_redraw_) {
                present();
                need_redraw_ = false;
            }
            // Mapping/configure and exposure can arrive on separate event-loop turns.
            // Dispatch scripts independently of whether this turn needed a new render.
            if (!quit && !script_started_ && first_expose_) {
                script_started_ = true;
                if (script_active_)
                    quit = play_script();
                if (!quit && opt_.exit_when_done)
                    quit = true;
                if (!quit && opt_.idle_seconds > 0) {
                    wait_idle();
                    quit = true;
                }
            }
            if (!quit)
                refine_step();
            const double turn = ms_since(turn_start);
            max_turn_ms_ = std::max(max_turn_ms_, turn);
            if (refining)
                max_refine_turn_ms_ = std::max(max_refine_turn_ms_, turn);
        }
        return pixels_ok_ ? 0 : 3;
    }

    void set_script(bool active) { script_active_ = active; }

    void print_stats() {
        const Summary r = summarize(t_.render_ms.data(), t_.render_ms.size());
        const Summary p = summarize(t_.pack_ms.data(), t_.pack_ms.size());
        const Summary x = summarize(t_.present_ms.data(), t_.present_ms.size());
        std::printf("{\n  \"tool\": \"globe-x11\",\n  \"viewport\": \"%dx%d\",\n",
                    model_.camera.width, model_.camera.height);
        std::printf("  \"visual_depth\": %d,\n  \"bits_per_pixel\": %d,\n", depth_,
                    format_.bits_per_pixel);
        std::printf("  \"masks\": \"%06x/%06x/%06x\",\n  \"msb_first\": %s,\n",
                    static_cast<unsigned>(format_.red_mask),
                    static_cast<unsigned>(format_.green_mask),
                    static_cast<unsigned>(format_.blue_mask), format_.msb_first ? "true" : "false");
        std::printf("  \"frames\": %zu,\n", t_.render_ms.size());
        std::printf("  \"wake_ticks\": %zu,\n  \"quality_stage\": %d,\n", wake_ticks_,
                    static_cast<int>(model_.stage));
        std::printf("  \"verified_frames\": %zu,\n", verified_frames_);
        std::printf("  \"refine_bands\": %zu,\n  \"refine_work_ms\": %.4f,\n", refine_bands_,
                    refine_work_ms_);
        std::printf(
            "  \"refinement_jobs\": %u,\n  \"zoom_cache_valid\": %d,\n  \"zoom_previews\": %u,\n",
            refinement_jobs_, runner_.zoom().valid() ? 1 : 0, zoom_previews_);
        std::printf("  \"max_turn_ms\": %.3f,\n  \"max_refine_turn_ms\": %.3f,\n", max_turn_ms_,
                    max_refine_turn_ms_);
        std::printf("  \"presentations\": %zu,\n", t_.present_ms.size());
        std::printf("  \"elevation_pack_count\": %u,\n  \"terrain_cache_bytes\": %zu,\n",
                    packs_.count(), packs_.bytes());
        std::printf("  \"toolbar_commands\": %zu,\n  \"cursor_changes\": %zu,\n", toolbar_commands_,
                    cursor_changes_);
        std::printf("  \"info_cards\": %zu,\n  \"info_scrolls\": %zu,\n", info_cards_,
                    info_.scrolls());
        std::printf("  \"terrain_style\": %u,\n", static_cast<unsigned>(model_.terrain_style));
        print_summary("render_ms", r, false);
        print_summary("pack_ms", p, false);
        print_summary("present_ms", x, false);
        if (opt_.idle_seconds > 0) {
            std::printf(
                "  \"idle_seconds\": %d,\n  \"idle_cpu_ms\": %.3f,\n  \"idle_frames\": %zu,\n",
                opt_.idle_seconds, idle_cpu_ms_, idle_frames_);
        }
        std::printf(
            "  \"limits\": \"present_ms is XPutImage plus an XSync round trip, which bounds "
            "submission to the X server but is not scanout or vsync; with Xvfb it times a "
            "software server\"\n}\n");
    }

  private:
    static void print_summary(const char* name, const Summary& s, bool last) {
        std::printf("  \"%s\": {\"count\": %zu, \"min\": %.4f, \"mean\": %.4f, \"p50\": %.4f, "
                    "\"p95\": %.4f, \"max\": %.4f}%s\n",
                    name, s.count, s.min, s.mean, s.p50, s.p95, s.max, last ? "" : ",");
    }

    bool make_buffers(int w, int h) {
        auto fb = Framebuffer::create(w, h);
        if (!fb) {
            return false;
        }
        XImage* img =
            XCreateImage(dpy_, visual_, static_cast<unsigned>(depth_), ZPixmap, 0, nullptr,
                         static_cast<unsigned>(w), static_cast<unsigned>(h), 32, 0);
        if (img == nullptr) {
            return false;
        }
        img->data = static_cast<char*>(std::malloc(static_cast<std::size_t>(img->bytes_per_line) *
                                                   static_cast<std::size_t>(h)));
        if (img->data == nullptr) {
            XDestroyImage(img);
            return false;
        }
        // The image is authoritative for the layout the server wants.
        const PixelFormat format = {img->bits_per_pixel, static_cast<std::uint32_t>(img->red_mask),
                                    static_cast<std::uint32_t>(img->green_mask),
                                    static_cast<std::uint32_t>(img->blue_mask),
                                    img->byte_order == MSBFirst};
        if (!pixel_format_supported(format)) {
            std::fprintf(stderr,
                         "globe-x11: unsupported pixel layout (%d bpp, masks %06lx/%06lx/%06lx)\n",
                         img->bits_per_pixel, img->red_mask, img->green_mask, img->blue_mask);
            XDestroyImage(img);
            return false;
        }
        if (image_ != nullptr) {
            XDestroyImage(image_);
        }
        image_ = img;
        fb_.emplace(std::move(*fb));
        format_ = format;
        shown_frame_ = nullptr;
        frame_valid_ = false; // the new buffers are blank until the next render
        return true;
    }

    bool load_script() {
        std::vector<std::uint8_t> bytes;
        if (!read_file(opt_.script_path, bytes)) {
            std::fprintf(stderr, "globe-x11: cannot read %s\n", opt_.script_path);
            return false;
        }
        const std::string text(bytes.begin(), bytes.end());
        std::size_t pos = 0;
        while (pos < text.size()) {
            std::size_t end = text.find('\n', pos);
            if (end == std::string::npos) {
                end = text.size();
            }
            if (const auto msg = parse_message(std::string_view(text).substr(pos, end - pos))) {
                script_.push_back(*msg);
            }
            pos = end + 1;
        }
        return true;
    }

    // The core decides when (a plain click that was not the first half of a double click).
    void pick_at(ScreenPoint at) {
        const bool changed = overlays_.click(model_.camera, at);
        if (const auto card = overlays_.take_card()) {
            info_.show(win_, *card, model_.camera.width + 20);
            ++info_cards_;
        }
        if (changed)
            reshow();
    }
    // Shows the current frame again (an overlay changed); no render, no refine.
    void reshow() {
        if (frame_valid_ && !need_redraw_ && fb_)
            upload(0, fb_->height());
    }
    bool apply(const Msg& msg) {
        if (std::holds_alternative<Tick>(msg) && refine_waiting_) {
            refine_waiting_ = false;
            wake_at_ms_ = 0;
        }
        // Repaint is a presentation request. Keep the cached frame and any in-flight job.
        if (std::holds_alternative<Repaint>(msg)) {
            if (!need_redraw_ && fb_)
                upload(0, fb_->height());
            return false;
        }
        const auto old_generation = model_.generation;
        const auto old_stage = model_.stage;
        Effects fx = update(model_, msg);
        if (model_.generation != old_generation) {
            refine_waiting_ = false;
            runner_.cancel(fb_ ? &*fb_ : nullptr, model_.camera);
            if (old_stage == QualityStage::Refine)
                fx.redraw = true;
        }
        if (fx.refine) {
            ++refinement_jobs_;
            next_upload_ms_ = 0;
            upload_dirty_ = false;
            runner_.start(model_, fb_ ? &*fb_ : nullptr, packs_.for_model(model_));
        }
        if (fx.redraw && (std::holds_alternative<Resize>(msg) ||
                          std::holds_alternative<RegisterElevation>(msg))) {
            fx.wake_after_ms = update(model_, Tick{monotonic_ns() / 1000000ULL}).wake_after_ms;
        }
        need_redraw_ |= fx.redraw;
        if (fx.present_only) {
            // Expose or update: show the pixels we already have; render only if there are none.
            if (frame_valid_)
                resend();
            else
                need_redraw_ = true;
        }
        if (fx.pick)
            pick_at(fx.pick_at);
        if (fx.wake_after_ms != 0) {
            wake_at_ms_ = monotonic_ns() / 1000000ULL + fx.wake_after_ms;
        }
        styles_.set_current(model_.terrain_style == TerrainStyle::Map ? 0 : 1);
        if (std::holds_alternative<Resize>(msg))
            info_.move(model_.camera.width + 20);
        if (toolbar_ && std::holds_alternative<Resize>(msg))
            XMoveWindow(dpy_, toolbar_, 0, model_.camera.height + 20);
        if (toolbar_ && toolbar_state(model_) != toolbar_cache_)
            draw_toolbar();
        update_cursor();
        return fx.quit;
    }

    // One frame per redraw request, exactly as interactive use would draw them.
    bool play_script() {
        for (const Msg& msg : script_) {
            if (apply(msg)) {
                return true;
            }
            if (need_redraw_) {
                if (const auto* r = std::get_if<Resize>(&msg)) {
                    (void)r;
                    XResizeWindow(dpy_, win_, static_cast<unsigned>(model_.camera.width),
                                  static_cast<unsigned>(model_.camera.height));
                    if (!make_buffers(model_.camera.width, model_.camera.height)) {
                        return true;
                    }
                }
                present();
                need_redraw_ = false;
            }
            while (opt_.finish_refinement && runner_.active() && !refine_waiting_)
                refine_step();
        }
        return false;
    }

    // Sleep until input, one requested wake, or a measurement deadline. No periodic polling.
    bool wait_events(std::uint64_t end_ms = 0) {
        if (XPending(dpy_) > 0 || (runner_.active() && !refine_waiting_)) {
            return true;
        }
        const std::uint64_t now = monotonic_ns() / 1000000ULL;
        std::uint64_t deadline = wake_at_ms_;
        if (end_ms != 0 && (deadline == 0 || end_ms < deadline)) {
            deadline = end_ms;
        }
        timeval tv{};
        if (deadline != 0 && deadline > now) {
            const std::uint64_t delay = deadline - now;
            tv.tv_sec = static_cast<long>(delay / 1000);
            tv.tv_usec = static_cast<long>((delay % 1000) * 1000);
        }
        fd_set set;
        FD_ZERO(&set);
        const int fd = ConnectionNumber(dpy_);
        FD_SET(fd, &set);
        const int ready = select(fd + 1, &set, nullptr, nullptr, deadline == 0 ? nullptr : &tv);
        if (ready < 0 && errno != EINTR) {
            std::perror("globe-x11: select");
            return false;
        }
        const std::uint64_t after = monotonic_ns() / 1000000ULL;
        if (ready == 0 && wake_at_ms_ != 0 && after >= wake_at_ms_) {
            wake_at_ms_ = 0; // consume before update requests the next one-shot wake
            ++wake_ticks_;
            apply(Tick{after});
        }
        return true;
    }

    void wait_idle() {
        // Finish scripted resize/expose events before starting the quiet-idle measurement.
        // XPending includes events already buffered by XSync, which select alone cannot see.
        XSync(dpy_, False);
        while (XPending(dpy_) > 0) {
            XEvent ev;
            XNextEvent(dpy_, &ev);
            if (handle(ev)) {
                return;
            }
        }
        if (need_redraw_) {
            present();
            need_redraw_ = false;
        }
        finish_settling();
        rusage a{};
        getrusage(RUSAGE_SELF, &a);
        const std::size_t frames0 = t_.render_ms.size();
        const std::uint64_t t0 = monotonic_ns();
        const std::uint64_t end_ms =
            t0 / 1000000ULL + static_cast<std::uint64_t>(opt_.idle_seconds) * 1000;
        while (monotonic_ns() / 1000000ULL < end_ms) {
            if (!wait_events(end_ms)) {
                break;
            }
            while (XPending(dpy_) > 0) {
                XEvent ev;
                XNextEvent(dpy_, &ev);
                if (handle(ev)) {
                    return;
                }
            }
            if (need_redraw_) {
                present();
                need_redraw_ = false;
            }
        }
        rusage b{};
        getrusage(RUSAGE_SELF, &b);
        const auto us = [](const rusage& r) {
            return static_cast<double>(r.ru_utime.tv_sec + r.ru_stime.tv_sec) * 1.0e6 +
                   static_cast<double>(r.ru_utime.tv_usec + r.ru_stime.tv_usec);
        };
        idle_cpu_ms_ = (us(b) - us(a)) / 1000.0;
        idle_frames_ = t_.render_ms.size() - frames0;
    }

    // Test mode only: read the actual window, not the XImage we submitted. The generic
    // converter is the oracle so an error in an optimized packing path cannot cancel out.
    bool verify_window() {
        if (!fb_) {
            return false;
        }
        // What the window should hold: the frame as last shown, overlays and progress bar included.
        const Framebuffer& fb = shown_frame_ != nullptr ? *shown_frame_ : *fb_;
        XImage* actual = XGetImage(dpy_, win_, 0, 0, static_cast<unsigned>(fb.width()),
                                   static_cast<unsigned>(fb.height()), AllPlanes, ZPixmap);
        if (actual == nullptr) {
            std::fprintf(stderr, "globe-x11: XGetImage failed\n");
            return false;
        }
        const std::size_t stride = pixel_row_bytes(format_, fb.width());
        std::vector<std::uint8_t> expected(stride * static_cast<std::size_t>(fb.height()));
        if (!convert_pixels_generic(fb, format_, expected.data(), stride)) {
            XDestroyImage(actual);
            return false;
        }
        const unsigned long mask = format_.red_mask | format_.green_mask | format_.blue_mask;
        const int bytes = format_.bits_per_pixel / 8;
        bool ok = true;
        for (int y = 0; y < fb.height() && ok; ++y) {
            for (int x = 0; x < fb.width(); ++x) {
                const std::uint8_t* p = expected.data() + static_cast<std::size_t>(y) * stride +
                                        static_cast<std::size_t>(x) * bytes;
                unsigned long want = 0;
                for (int k = 0; k < bytes; ++k) {
                    const int shift = 8 * (format_.msb_first ? bytes - 1 - k : k);
                    want |= static_cast<unsigned long>(p[k]) << shift;
                }
                const unsigned long got = XGetPixel(actual, x, y) & mask;
                if (got != want) {
                    std::fprintf(stderr,
                                 "globe-x11: pixel mismatch at %d,%d: got %lx expected %lx\n", x, y,
                                 got, want);
                    ok = false;
                    break;
                }
            }
        }
        XDestroyImage(actual);
        if (ok) {
            ++verified_frames_;
        }
        return ok;
    }

    void refine_step() {
        if (!fb_ || !runner_.active() || refine_waiting_)
            return;
        const std::uint64_t t = monotonic_ns();
        const auto step = runner_.step(*fb_, model_);
        refine_work_ms_ += ms_since(t);
        if (step.band_ready) {
            ++refine_bands_;
            present_refine(step.first_row, step.end_row);
        }
        if (runner_.job() != nullptr && !runner_.active() &&
            (upload_dirty_ || shown_quick_ || bar_shown_) && fb_ && !presentation_held(model_)) {
            upload(0, fb_->height()); // the last throttled frame is always shown
            upload_dirty_ = false;
        }
        if (runner_.job() != nullptr && !runner_.active() && model_.refine_active) {
            apply(RefinementFinished{model_.generation, model_.refine_level});
            return; // a successor owns a fresh continuation and its own pacing
        }
        if (step.wake_after_ms) {
            refine_waiting_ = true;
            wake_at_ms_ = monotonic_ns() / 1000000ULL + step.wake_after_ms;
        }
    }

    // Showing a frame costs time (pack plus upload, a lot on a slow machine or a remote display),
    // so a refinement frame is shown only when the last upload took under a quarter of the time
    // since it started. Skipped frames are caught up by the next shown one.
    void present_refine(int first, int end) {
        const std::uint64_t start = monotonic_ns() / 1000000ULL;
        if (presentation_held(model_) || start < next_upload_ms_) {
            upload_dirty_ = true;
            return;
        }
        upload(first, end);
        upload_dirty_ = false;
        next_upload_ms_ = start + 4 * (monotonic_ns() / 1000000ULL - start);
    }

    void finish_settling() {
        // Measurement begins only after requested settling/refinement completes. While drawing,
        // return to X event handling after every bounded work step, just like the main loop.
        while (wake_at_ms_ != 0 || runner_.active()) {
            if (!wait_events())
                return;
            const std::uint64_t turn_start = monotonic_ns();
            const bool refining = runner_.active();
            while (XPending(dpy_) > 0) {
                XEvent ev;
                XNextEvent(dpy_, &ev);
                if (handle(ev))
                    return;
            }
            if (need_redraw_) {
                present();
                need_redraw_ = false;
            }
            refine_step();
            const double turn = ms_since(turn_start);
            max_turn_ms_ = std::max(max_turn_ms_, turn);
            if (refining)
                max_refine_turn_ms_ = std::max(max_refine_turn_ms_, turn);
        }
    }

    void present() {
        if (!fb_ || image_ == nullptr) {
            return;
        }
        Framebuffer& fb = *fb_;
        std::uint64_t t = monotonic_ns();
        if (runner_.render_moving(fb, model_, packs_.for_model(model_)))
            ++zoom_previews_;
        t_.render_ms.push_back(ms_since(t));
        frame_valid_ = true;
        upload(0, fb.height());
    }

    // Shows the already packed image again, without rendering or converting.
    void resend() {
        if (!fb_ || image_ == nullptr)
            return;
        const std::uint64_t t = monotonic_ns();
        XPutImage(dpy_, win_, gc_, image_, 0, 0, 0, 0, static_cast<unsigned>(fb_->width()),
                  static_cast<unsigned>(fb_->height()));
        XSync(dpy_, False);
        t_.present_ms.push_back(ms_since(t));
        if (opt_.verify_pixels) {
            pixels_ok_ &= verify_window();
        }
    }

    void upload(int first_row, int end_row) {
        if (!fb_ || image_ == nullptr)
            return;
        Framebuffer& fb = *fb_;
        upload_dirty_ = false;
        std::uint64_t t = monotonic_ns();
        // Overlays and the progress bar go on a copy (or the bar on the frame for this upload), so
        // the rendered frame stays exactly as it was and clearing an overlay needs no re-render.
        const bool settled = frame_settled(model_, runner_.active());
        const double progress = bar_progress(model_, runner_.job());
        if (progress >= 0.0 || bar_shown_) { // a bar to draw, or one to wipe: cover its rows
            first_row = std::min(first_row, fb.height() - kProgressBarRows);
            end_row = fb.height();
        }
        const Framebuffer* shown =
            &shown_.begin(overlays_, fb, model_.camera,
                          static_cast<std::int64_t>(std::time(nullptr)), settled, progress);
        shown_quick_ = shown_.quick();
        bar_shown_ = shown_.bar();
        shown_frame_ = shown;
        convert_pixels(*shown, format_, reinterpret_cast<std::uint8_t*>(image_->data),
                       static_cast<std::size_t>(image_->bytes_per_line));
        t_.pack_ms.push_back(ms_since(t));
        t = monotonic_ns();
        XPutImage(dpy_, win_, gc_, image_, 0, first_row, 0, first_row,
                  static_cast<unsigned>(fb.width()), static_cast<unsigned>(end_row - first_row));
        XSync(dpy_, False);
        t_.present_ms.push_back(ms_since(t));
        if (opt_.verify_pixels) {
            pixels_ok_ &= verify_window();
        }
        shown_.end(); // the frame is as it was rendered
    }

    // Returns true when the application should quit.
    void update_cursor() {
        const auto intent = cursor_for(model_, pointer_);
        if (intent != cursor_) {
            cursor_ = intent;
            XDefineCursor(dpy_, win_, cursors_[static_cast<unsigned>(intent)]);
            ++cursor_changes_;
        }
    }
    void draw_toolbar() {
        toolbar_cache_ = toolbar_state(model_);
        static constexpr std::array<const char*, kCommandCount> labels = {
            "Left",      "Right",   "Up",       "Down",      "Zoom in", "Zoom out", "Reset",
            "Graticule", "Borders", "North up", "Trackball", "Quit",    "Style"};
        const int screen = DefaultScreen(dpy_);
        const auto white = WhitePixel(dpy_, screen), black = BlackPixel(dpy_, screen);
        XSetForeground(dpy_, gc_, white);
        XFillRectangle(dpy_, toolbar_, gc_, 0, 0, kToolbarWidth, kToolbarHeight);
        for (std::size_t i = 0; i < kCommandCount; ++i) {
            const auto state = toolbar_cache_[i];
            if (!state.visible)
                continue;
            const auto r = toolbar_button(static_cast<Command>(i));
            XSetForeground(dpy_, gc_, state.checked ? black : white);
            XFillRectangle(dpy_, toolbar_, gc_, r.x, r.y, r.width, r.height);
            XSetForeground(dpy_, gc_, black);
            if (state.enabled)
                XDrawRectangle(dpy_, toolbar_, gc_, r.x, r.y, r.width - 1, r.height - 1);
            XSetForeground(dpy_, gc_, state.checked ? white : black);
            XDrawString(dpy_, toolbar_, gc_, r.x + 6, r.y + 18, labels[i],
                        static_cast<int>(std::strlen(labels[i])));
        }
    }
    bool handle(XEvent& ev) {
        const std::uint64_t now = monotonic_ns() / 1000000ULL;
        if (info_.handle(ev))
            return false;
        if (const auto r = styles_.handle(ev); r.handled) {
            const int current = model_.terrain_style == TerrainStyle::Map ? 0 : 1;
            // Two looks: clicking the other row is a switch.
            if (r.picked >= 0 && r.picked != current)
                return apply(CommandIssued{now, Command::CycleStyle});
            return false;
        }
        if (ev.xany.window == toolbar_) {
            if (ev.type == Expose && ev.xexpose.count == 0)
                draw_toolbar();
            if (ev.type == ClientMessage && static_cast<Atom>(ev.xclient.data.l[0]) == wm_delete_)
                XUnmapWindow(dpy_, toolbar_);
            if (ev.type == KeyPress)
                return on_key(XLookupKeysym(&ev.xkey, 0), now);
            if (ev.type == ButtonPress && ev.xbutton.button == Button1) {
                const auto hit = toolbar_hit({double(ev.xbutton.x), double(ev.xbutton.y)});
                if (hit) {
                    const auto state = toolbar_state(model_)[static_cast<std::size_t>(*hit)];
                    if (state.visible && state.enabled) {
                        ++toolbar_commands_;
                        return apply(CommandIssued{now, *hit});
                    }
                }
            }
            return false;
        }
        if (ev.type == MotionNotify || ev.type == ButtonPress || ev.type == ButtonRelease) {
            pointer_ = {double(ev.xmotion.x), double(ev.xmotion.y)};
            update_cursor();
        }
        switch (ev.type) {
        case EnterNotify:
            pointer_ = {double(ev.xcrossing.x), double(ev.xcrossing.y)};
            update_cursor();
            return false;
        case LeaveNotify:
            pointer_ = {-1e20, -1e20};
            update_cursor();
            return false;
        case Expose:
            if (ev.xexpose.count == 0) {
                first_expose_ = true;
                return apply(Repaint{});
            }
            return false;
        case ConfigureNotify: {
            const int w = ev.xconfigure.width;
            const int h = ev.xconfigure.height;
            if (w != model_.camera.width || h != model_.camera.height) {
                const bool q = apply(Resize{w, h});
                if (model_.camera.width == w && model_.camera.height == h) {
                    make_buffers(w, h);
                }
                return q;
            }
            return false;
        }
        case ButtonPress:
            if (ev.xbutton.button == Button1) {
                return apply(PointerDown{now, static_cast<double>(ev.xbutton.x),
                                         static_cast<double>(ev.xbutton.y)});
            }
            if (ev.xbutton.button == Button4) {
                return apply(Wheel{now, 1});
            }
            if (ev.xbutton.button == Button5) {
                return apply(Wheel{now, -1});
            }
            return false;
        case ButtonRelease:
            if (ev.xbutton.button == Button1) {
                const bool quit = apply(PointerUp{now});
                // A refinement finished (or advanced) while the button was down: show it now.
                if (upload_dirty_ && fb_ && !need_redraw_ && model_.stage != QualityStage::Moving)
                    upload(0, fb_->height());
                return quit;
            }
            return false;
        case MotionNotify: {
            XEvent next;
            while (XCheckTypedWindowEvent(dpy_, win_, MotionNotify, &next)) {
                ev = next; // only the latest pointer position matters
            }
            if ((ev.xmotion.state & Button1Mask) != 0) {
                return apply(PointerMove{now, static_cast<double>(ev.xmotion.x),
                                         static_cast<double>(ev.xmotion.y)});
            }
            return false;
        }
        case KeyPress: {
            const KeySym sym = XLookupKeysym(&ev.xkey, 0);
            return on_key(sym, now);
        }
        case ClientMessage:
            return static_cast<Atom>(ev.xclient.data.l[0]) == wm_delete_;
        default:
            return false;
        }
    }

    bool on_key(KeySym sym, std::uint64_t now) {
        // Arrow, Page and function keys are decoded here; typed characters go through the shared
        // table so both front ends bind the same shortcuts.
        std::optional<Command> command;
        int ch = 0;
        switch (sym) {
        case XK_F2:
            XMapRaised(dpy_, toolbar_);
            return false;
        case XK_Left:
            command = Command::Left;
            break;
        case XK_Right:
            command = Command::Right;
            break;
        case XK_Up:
            command = Command::Up;
            break;
        case XK_Down:
            command = Command::Down;
            break;
        case XK_Prior: // Page Up: wheel drivers send it
            command = Command::ZoomIn;
            break;
        case XK_Next:
            command = Command::ZoomOut;
            break;
        case XK_KP_Add:
            ch = '+';
            break;
        case XK_KP_Subtract:
            ch = '-';
            break;
        case XK_Escape:
            ch = 27;
            break;
        default:
            ch = sym < 0x80 ? static_cast<int>(sym) : 0;
            break;
        }
        const auto binding = binding_for_char(ch);
        if (!command)
            command = binding.command;
        if (command)
            return apply(CommandIssued{now, *command});
        switch (binding.overlay) {
        case KeyBinding::Overlay::Night:
            overlays_.toggle_night();
            reshow();
            break;
        case KeyBinding::Overlay::Measure:
            overlays_.toggle_measure();
            reshow();
            break;
        case KeyBinding::Overlay::Palette:
            styles_.show(win_, model_.camera.width + 20,
                         model_.terrain_style == TerrainStyle::Map ? 0 : 1);
            break;
        case KeyBinding::Overlay::Nothing:
            break;
        }
        return false;
    }

    Display* dpy_;
    const Scene& scene_;
    ElevationPacks& packs_;
    Options opt_;
    Model model_;
    InfoWindow info_;
    StyleWindow styles_;
    std::optional<Places> places_;
    std::optional<PlaceScene> place_scene_;
    std::optional<Countries> countries_;
    std::optional<CountryScene> country_scene_;
    Overlays overlays_;
    std::size_t info_cards_ = 0;
    Window win_ = 0, toolbar_ = 0;
    std::array<Cursor, 3> cursors_{};
    CursorIntent cursor_ = CursorIntent::Arrow;
    ScreenPoint pointer_{-1e20, -1e20};
    ToolbarState toolbar_cache_{};
    std::size_t toolbar_commands_ = 0, cursor_changes_ = 0;
    GC gc_ = nullptr;
    Visual* visual_ = nullptr;
    int depth_ = 0;
    Atom wm_delete_ = 0;
    XImage* image_ = nullptr;
    PixelFormat format_{};
    std::optional<Framebuffer> fb_;
    std::size_t wake_ticks_ = 0;
    bool refine_waiting_ = false;
    std::uint64_t wake_at_ms_ = 0;
    RefineRunner runner_{scene_}; // the refinement job, the zoom preview and the markers
    ShownFrame shown_; // overlays and the bar, built on a copy of the frame for each upload
    const Framebuffer* shown_frame_ = nullptr; // what the window last got (fb_ or an overlay copy)
    bool bar_shown_ = false;                   // the last upload drew the progress bar
    unsigned zoom_previews_ = 0;
    double max_turn_ms_ = 0;
    double max_refine_turn_ms_ = 0;
    unsigned refinement_jobs_ = 0;
    std::size_t refine_bands_ = 0;
    std::uint64_t next_upload_ms_ = 0;
    bool upload_dirty_ = false;
    bool shown_quick_ = false; // the frame on screen has the cheap overlays; show the full ones
    double refine_work_ms_ = 0;
    bool pixels_ok_ = true;
    std::size_t verified_frames_ = 0;
    bool need_redraw_ = true;
    bool first_expose_ = false;
    bool frame_valid_ = false; // fb_ and image_ hold a rendered frame
    bool script_active_ = false;
    bool script_started_ = false;
    std::vector<Msg> script_;
    Timings t_;
    double idle_cpu_ms_ = 0.0;
    std::size_t idle_frames_ = 0;
};

} // namespace

int main(int argc, char** argv) {
    Options opt;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--atlas" && i + 1 < argc) {
            opt.atlas_path = argv[++i];
        } else if (a == "--size" && i + 1 < argc) {
            if (!parse_size_arg(argv[++i], kMaxViewportDimension, opt.width, opt.height)) {
                std::fprintf(stderr, "bad --size\n");
                return 2;
            }
        } else if (a == "--elevation" && i + 1 < argc) {
            if (opt.elevation_count == opt.elevation_paths.size()) {
                std::fprintf(stderr, "too many elevation packs\n");
                return 2;
            }
            opt.elevation_paths[opt.elevation_count++] = argv[++i];
        } else if (a == "--script" && i + 1 < argc) {
            opt.script_path = argv[++i];
        } else if (a == "--exit-when-done") {
            opt.exit_when_done = true;
        } else if (a == "--finish-refinement") {
            opt.finish_refinement = true;
        } else if (a == "--no-zoom-preview") {
            opt.no_zoom_preview = true;
        } else if (a == "--idle-seconds" && i + 1 < argc) {
            if (!parse_int_arg(argv[++i], 0, 3600, opt.idle_seconds)) {
                std::fprintf(stderr, "bad --idle-seconds (0..3600)\n");
                return 2;
            }
        } else if (a == "--verify-pixels") {
            opt.verify_pixels = true;
        } else if (a == "--stats") {
            opt.stats = true;
        } else {
            std::fprintf(
                stderr,
                "usage: globe-x11 [--atlas FILE] [--elevation FILE] [--size WxH] [--script TRACE "
                "[--exit-when-done]] [--idle-seconds N] [--verify-pixels] [--stats] "
                "[--no-zoom-preview] [--finish-refinement]\n");
            return 2;
        }
    }
    if (opt.width < 1 || opt.height < 1 || opt.width > kMaxViewportDimension ||
        opt.height > kMaxViewportDimension || opt.idle_seconds < 0 || opt.idle_seconds > 3600) {
        std::fprintf(stderr, "bad arguments\n");
        return 2;
    }

    std::vector<std::uint8_t> atlas_file;
    const std::uint8_t* atlas_data = kEmbeddedAtlasData;
    std::size_t atlas_size = kEmbeddedAtlasSize;
    if (opt.atlas_path != nullptr) {
        if (!read_file(opt.atlas_path, atlas_file)) {
            std::fprintf(stderr, "globe-x11: cannot read %s\n", opt.atlas_path);
            return 2;
        }
        atlas_data = atlas_file.data();
        atlas_size = atlas_file.size();
    }
    AtlasError err = AtlasError::Ok;
    const auto atlas = Atlas::parse(atlas_data, atlas_size, &err);
    if (!atlas) {
        std::fprintf(stderr, "globe-x11: atlas: %s\n", atlas_error_name(err));
        return 2;
    }

    const auto scene = Scene::create(*atlas);
    if (!scene) {
        std::fprintf(stderr, "globe-x11: out of memory building the scene\n");
        return 2;
    }

    ElevationPacks packs;
    if (!packs.initialize(kEmbeddedElevationData, kEmbeddedElevationSize))
        std::fprintf(stderr, "globe-x11: %s; showing outlines\n", packs.error());
    // Explicit imports retain their original capacity/precedence. With none, ship a finer
    // built-in level; an allocation failure retains the validated coarse fallback.
    if (opt.elevation_count == 0 &&
        !packs.add_embedded(kEmbeddedFineElevationData, kEmbeddedFineElevationSize))
        std::fprintf(stderr, "globe-x11: optional half-degree terrain unavailable: %s\n",
                     packs.error());
    for (unsigned i = 0; i < opt.elevation_count; ++i)
        if (!packs.import_file(opt.elevation_paths[i])) {
            std::fprintf(stderr, "globe-x11: %s\n", packs.error());
            return 2;
        }
    Display* dpy = XOpenDisplay(nullptr);
    if (dpy == nullptr) {
        std::fprintf(stderr, "globe-x11: cannot open display (is DISPLAY set and authorized?)\n");
        return 1;
    }
    int rc = 0;
    {
        App app(dpy, *scene, packs, opt);
        app.set_script(opt.script_path != nullptr);
        if (!app.init()) {
            rc = 1;
        } else {
            rc = app.run();
            if (opt.stats) {
                app.print_stats();
            }
        }
    }
    XCloseDisplay(dpy);
    return rc;
}
