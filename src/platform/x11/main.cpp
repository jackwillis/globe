// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// X11 front end: one window showing the globe. Blocks in XNextEvent when nothing changes, turns
// X events into Msgs, runs update(), and renders only when update() asks for a redraw.
//
//   globe-x11 [--atlas FILE] [--size WxH] [--script TRACE [--exit-when-done]]
//             [--idle-seconds N] [--stats]
//
// Mouse: drag the left button to rotate, wheel to zoom. Keys: arrows rotate, + / - zoom, R reset,
// G / B / C toggle graticule / borders / coastlines, N aligns north up, T switches between
// turntable and trackball dragging, Esc or Q quits.
//
// --script feeds a recorded trace (see globe/trace.hpp) after the first exposure, rendering once
// per redraw the way interactive use would. --idle-seconds waits that long afterwards and reports
// CPU time used and frames drawn while idle. --stats prints a JSON timing summary.
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <sys/resource.h>
#include <sys/select.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "cli.hpp"
#include "clock.hpp"
#include "frame_stats.hpp"
#include "globe/app.hpp"
#include "globe/atlas.hpp"
#include "globe/embedded_atlas.hpp"
#include "globe/trace.hpp"
#include "pixel_format.hpp"

using namespace globe;

namespace {

struct Options {
    const char* atlas_path = nullptr;
    const char* script_path = nullptr;
    int width = 640;
    int height = 480;
    bool exit_when_done = false;
    bool stats = false;
    int idle_seconds = 0;
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
    App(Display* dpy, const Scene& scene, const Options& opt)
        : dpy_(dpy), scene_(scene), opt_(opt), model_(initial_model(opt.width, opt.height)) {}

    ~App() {
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
                         PointerMotionMask | KeyPressMask);
        wm_delete_ = XInternAtom(dpy_, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(dpy_, win_, &wm_delete_, 1);
        gc_ = XCreateGC(dpy_, win_, 0, nullptr);
        if (!make_buffers(opt_.width, opt_.height)) {
            return false;
        }
        XMapWindow(dpy_, win_);
        return true;
    }

    int run() {
        if (script_active_ && !load_script()) {
            return 2;
        }
        bool quit = false;
        while (!quit) {
            XEvent ev;
            XNextEvent(dpy_, &ev); // blocks: no timer, no polling, no idle redraw
            quit = handle(ev);
            while (!quit && XPending(dpy_) > 0) {
                XNextEvent(dpy_, &ev);
                quit = handle(ev);
            }
            if (!quit && need_redraw_) {
                present();
                need_redraw_ = false;
                if (!script_started_ && first_expose_ && script_active_) {
                    script_started_ = true;
                    quit = play_script();
                    if (!quit && opt_.exit_when_done) {
                        quit = true;
                    }
                    if (!quit && opt_.idle_seconds > 0) {
                        wait_idle();
                        quit = true; // measurement mode: report and leave
                    }
                }
            }
        }
        return 0;
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

    bool apply(const Msg& msg) {
        const Effects fx = update(model_, msg);
        need_redraw_ |= fx.redraw;
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
        }
        return false;
    }

    void wait_idle() {
        rusage a{};
        getrusage(RUSAGE_SELF, &a);
        const std::size_t frames0 = t_.render_ms.size();
        const std::uint64_t t0 = monotonic_ns();
        const int fd = ConnectionNumber(dpy_);
        while (ms_since(t0) < opt_.idle_seconds * 1000.0) {
            fd_set set;
            FD_ZERO(&set);
            FD_SET(fd, &set);
            timeval tv{0, 100000};
            if (select(fd + 1, &set, nullptr, nullptr, &tv) > 0) {
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

    void present() {
        if (!fb_ || image_ == nullptr) {
            return;
        }
        Framebuffer& fb = *fb_;
        std::uint64_t t = monotonic_ns();
        render_globe(fb, model_.camera, scene_, kDefaultPalette, effective_layers(model_));
        t_.render_ms.push_back(ms_since(t));
        t = monotonic_ns();
        convert_pixels(fb, format_, reinterpret_cast<std::uint8_t*>(image_->data),
                       static_cast<std::size_t>(image_->bytes_per_line));
        t_.pack_ms.push_back(ms_since(t));
        t = monotonic_ns();
        XPutImage(dpy_, win_, gc_, image_, 0, 0, 0, 0, static_cast<unsigned>(fb.width()),
                  static_cast<unsigned>(fb.height()));
        XSync(dpy_, False);
        t_.present_ms.push_back(ms_since(t));
    }

    // Returns true when the application should quit.
    bool handle(XEvent& ev) {
        const std::uint64_t now = monotonic_ns() / 1000000ULL;
        switch (ev.type) {
        case Expose:
            if (ev.xexpose.count == 0) {
                first_expose_ = true;
                need_redraw_ = true;
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
            return ev.xbutton.button == Button1 ? apply(PointerUp{now}) : false;
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
        switch (sym) {
        case XK_Left:
            return apply(CommandIssued{now, Command::Left});
        case XK_Right:
            return apply(CommandIssued{now, Command::Right});
        case XK_Up:
            return apply(CommandIssued{now, Command::Up});
        case XK_Down:
            return apply(CommandIssued{now, Command::Down});
        case XK_plus:
        case XK_equal:
        case XK_KP_Add:
            return apply(CommandIssued{now, Command::ZoomIn});
        case XK_minus:
        case XK_KP_Subtract:
            return apply(CommandIssued{now, Command::ZoomOut});
        case XK_r:
            return apply(CommandIssued{now, Command::Reset});
        case XK_g:
            return apply(CommandIssued{now, Command::ToggleGraticule});
        case XK_b:
            return apply(CommandIssued{now, Command::ToggleBorders});
        case XK_c:
            return apply(CommandIssued{now, Command::ToggleCoastlines});
        case XK_n:
            return apply(CommandIssued{now, Command::AlignNorth});
        case XK_t:
            return apply(CommandIssued{now, Command::ToggleDragMode});
        case XK_Escape:
        case XK_q:
            return apply(CommandIssued{now, Command::Quit});
        default:
            return false;
        }
    }

    Display* dpy_;
    const Scene& scene_;
    Options opt_;
    Model model_;
    Window win_ = 0;
    GC gc_ = nullptr;
    Visual* visual_ = nullptr;
    int depth_ = 0;
    Atom wm_delete_ = 0;
    XImage* image_ = nullptr;
    PixelFormat format_{};
    std::optional<Framebuffer> fb_;
    bool need_redraw_ = false;
    bool first_expose_ = false;
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
        } else if (a == "--script" && i + 1 < argc) {
            opt.script_path = argv[++i];
        } else if (a == "--exit-when-done") {
            opt.exit_when_done = true;
        } else if (a == "--idle-seconds" && i + 1 < argc) {
            if (!parse_int_arg(argv[++i], 0, 3600, opt.idle_seconds)) {
                std::fprintf(stderr, "bad --idle-seconds (0..3600)\n");
                return 2;
            }
        } else if (a == "--stats") {
            opt.stats = true;
        } else {
            std::fprintf(stderr, "usage: globe-x11 [--atlas FILE] [--size WxH] [--script TRACE "
                                 "[--exit-when-done]] [--idle-seconds N] [--stats]\n");
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

    Display* dpy = XOpenDisplay(nullptr);
    if (dpy == nullptr) {
        std::fprintf(stderr, "globe-x11: cannot open display (is DISPLAY set and authorized?)\n");
        return 1;
    }
    int rc = 0;
    {
        App app(dpy, *scene, opt);
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
