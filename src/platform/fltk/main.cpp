// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>

#include "cli.hpp"
#include "clock.hpp"
#include "elevation_packs.hpp"
#include "globe/card_text.hpp"
#include "globe/embedded_atlas.hpp"
#include "globe/embedded_countries.hpp"
#include "globe/embedded_elevation.hpp"
#include "globe/embedded_fineelevation.hpp"
#include "globe/embedded_places.hpp"
#include "globe/refine_runner.hpp"
#include "globe/shown_frame.hpp"
#include "pixel_format.hpp"

#include <array>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iterator>
#include <memory>
#include <new>
#include <string_view>
#include <vector>

using namespace globe;

namespace {
constexpr int kChromeHeight = 32;
constexpr PixelFormat kRgb{24, 0xff0000, 0x00ff00, 0x0000ff, true};

std::uint64_t now_ms() {
    return monotonic_ns() / 1000000;
}

struct Timing {
    std::uint64_t ns = 0;
    unsigned calls = 0;

    void add(std::uint64_t start) {
        ns += monotonic_ns() - start;
        ++calls;
    }

    void print(const char* name) const {
        std::printf("%s: %u calls, %.3f ms total\n", name, calls, ns / 1.0e6);
    }
};

class InformationWindow : public Fl_Window {
  public:
    InformationWindow() : Fl_Window(360, 240, "Poodle Earth — Information") {
        resizable(text_);
        end();
        set_card(nothing_selected_card());
    }

    void set_card(const InfoCard& card) {
        char buffer[1024];
        const auto count = card_text(card, buffer, sizeof buffer, '\n');
        text_.value(buffer, static_cast<int>(count));
    }

  private:
    Fl_Multiline_Output text_{8, 8, 344, 224};
};

class Workbench;

class Canvas : public Fl_Widget {
  public:
    Canvas(int width, int height, Workbench& app)
        : Fl_Widget(0, kChromeHeight, width, height), app_(app) {}

    void draw() override;
    int handle(int event) override;
    Timing presentation;

  private:
    Workbench& app_;
};

class Workbench {
  public:
    Workbench(const Scene& scene, ElevationPacks& packs, int width, int height)
        : model_(initial_model(width, height)), packs_(packs), runner_(scene),
          window_(width, height + kChromeHeight, "Poodle Earth — FLTK"),
          canvas_(width, height, *this) {
        window_.resizable(canvas_);
        window_.size_range(1, kChromeHeight + 1, kMaxViewportDimension,
                           kMaxViewportDimension + kChromeHeight);
        window_.end();
        window_.callback(
            [](Fl_Widget*, void* p) {
                static_cast<Workbench*>(p)->apply(CommandIssued{now_ms(), Command::Quit});
            },
            this);
        relief_.callback(
            [](Fl_Widget*, void* p) {
                static_cast<Workbench*>(p)->apply(CommandIssued{now_ms(), Command::CycleStyle});
            },
            this);
        info_button_.callback([](Fl_Widget*, void* p) { static_cast<Workbench*>(p)->info_.show(); },
                              this);

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
    }

    int run(bool stats) {
        window_.show();
        apply(Tick{now_ms()});
        while (window_.shown()) {
            sync_size();
            if (!window_.shown())
                break;
            const auto now = now_ms();
            if (wake_ && now >= wake_) {
                wake_ = 0;
                apply(Tick{now});
            }
            if (redraw_) {
                const auto start = monotonic_ns();
                runner_.render_moving(*frame_, model_, packs_.for_model(model_));
                moving_.add(start);
                redraw_ = false;
                publish();
            }
            step();
            if (runner_.active()) {
                Fl::check();
            } else if (wake_) {
                const auto after = now_ms();
                Fl::wait(wake_ > after ? (wake_ - after) / 1000.0 : 0.0);
            } else {
                Fl::wait();
            }
        }
        if (stats) {
            moving_.print("moving render");
            refinement_.print("refinement steps");
            packing_.print("shown frame + RGB conversion");
            canvas_.presentation.print("FLTK draw submission");
        }
        return failed_ ? 1 : 0;
    }

    void quit() {
        info_.hide();
        window_.hide();
    }

    void apply(const Msg& msg) {
        const auto generation = model_.generation;
        const auto stage = model_.stage;
        const auto fx = update(model_, msg);
        if (fx.quit) {
            quit();
            return;
        }
        if (model_.generation != generation) {
            runner_.cancel(frame_ ? &*frame_ : nullptr, model_.camera);
            redraw_ |= stage == QualityStage::Refine;
        }
        if (fx.refine)
            runner_.start(model_, frame_ ? &*frame_ : nullptr, packs_.for_model(model_));
        redraw_ |= fx.redraw;
        if (fx.wake_after_ms)
            wake_ = now_ms() + fx.wake_after_ms;
        if (fx.pick) {
            const bool changed = overlays_.click(model_.camera, fx.pick_at);
            if (const auto card = overlays_.take_card())
                info_.set_card(*card);
            if (changed && !redraw_)
                publish();
        }
        // A completed refinement may have been held while a click was pending.
        if (dirty_ && !redraw_ && !runner_.active() && !presentation_held(model_))
            publish();
    }

    void draw(int x, int y, int width, int height) const {
        if (rgb_ && frame_->width() == width && frame_->height() == height)
            fl_draw_image(rgb_.get(), x, y, width, height);
        else
            fl_rectf(x, y, width, height, FL_BLACK);
    }

  private:
    void sync_size() {
        const int width = canvas_.w(), height = canvas_.h();
        if (frame_ && width == frame_->width() && height == frame_->height())
            return;
        auto frame = Framebuffer::create(width, height);
        std::unique_ptr<unsigned char[]> rgb;
        if (frame)
            rgb.reset(
                new (std::nothrow) unsigned char[static_cast<std::size_t>(width) * height * 3]);
        if (!frame || !rgb) {
            std::fprintf(stderr, "globe-fltk: cannot allocate %dx%d viewport\n", width, height);
            failed_ = true;
            quit();
            return;
        }
        apply(Resize{width, height});
        apply(Tick{now_ms()}); // anchor the resize settle deadline
        frame_ = std::move(frame);
        rgb_ = std::move(rgb);
        redraw_ = true;
        dirty_ = false;
    }

    void step() {
        if (!runner_.active())
            return;
        const auto start = monotonic_ns();
        const auto progress = runner_.step(*frame_, model_);
        refinement_.add(start);
        dirty_ |= progress.band_ready;
        if (!runner_.active()) {
            dirty_ = true;
            apply(RefinementFinished{model_.generation, model_.refine_level});
        }
        if (dirty_ && !presentation_held(model_) && now_ms() >= next_publish_)
            publish();
    }

    void publish() {
        const auto start = monotonic_ns();
        const auto& shown = shown_.begin(
            overlays_, *frame_, model_.camera, static_cast<std::int64_t>(std::time(nullptr)),
            frame_settled(model_, runner_.active()), bar_progress(model_, runner_.job()));
        convert_pixels(shown, kRgb, rgb_.get(), static_cast<std::size_t>(shown.width()) * 3);
        shown_.end();
        packing_.add(start);
        dirty_ = false;
        next_publish_ = now_ms() + 33;
        canvas_.redraw();
    }

    Model model_;
    ElevationPacks& packs_;
    RefineRunner runner_;
    std::optional<Places> places_;
    std::optional<PlaceScene> place_scene_;
    std::optional<Countries> countries_;
    std::optional<CountryScene> country_scene_;
    Overlays overlays_;
    ShownFrame shown_;
    std::optional<Framebuffer> frame_;
    std::unique_ptr<unsigned char[]> rgb_;
    bool redraw_ = true;
    bool dirty_ = false;
    bool failed_ = false;
    std::uint64_t wake_ = 0;
    std::uint64_t next_publish_ = 0;
    Timing moving_, refinement_, packing_;

    InformationWindow info_;
    Fl_Window window_;
    Canvas canvas_;
    Fl_Check_Button relief_{6, 3, 65, 26, "Relief"};
    Fl_Button info_button_{72, 3, 66, 26, "Info"};
};

void Canvas::draw() {
    const auto start = monotonic_ns();
    app_.draw(x(), y(), w(), h());
    presentation.add(start);
}

int Canvas::handle(int event) {
    const auto now = now_ms();
    const double px = Fl::event_x() - x(), py = Fl::event_y() - y();
    switch (event) {
    case FL_PUSH:
        if (Fl::event_button() != FL_LEFT_MOUSE)
            return 0;
        app_.apply(PointerDown{now, px, py});
        return 1;
    case FL_DRAG:
        app_.apply(PointerMove{now, px, py});
        return 1;
    case FL_RELEASE:
        app_.apply(PointerUp{now});
        return 1;
    case FL_MOUSEWHEEL:
        app_.apply(Wheel{now, -Fl::event_dy()});
        return 1;
    default:
        return Fl_Widget::handle(event);
    }
}
} // namespace

int main(int argc, char** argv) {
    int width = 420, height = 440;
    const char* atlas_path = nullptr;
    std::array<const char*, 3> elevations{};
    unsigned elevation_count = 0;
    bool stats = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--stats") {
            stats = true;
        } else if (arg == "--atlas" && i + 1 < argc) {
            atlas_path = argv[++i];
        } else if (arg == "--elevation" && i + 1 < argc && elevation_count < elevations.size()) {
            elevations[elevation_count++] = argv[++i];
        } else if (arg == "--size" && i + 1 < argc &&
                   parse_size_arg(argv[++i], kMaxViewportDimension, width, height)) {
            continue;
        } else {
            std::fprintf(stderr, "usage: globe-fltk [--size WxH] [--atlas FILE] "
                                 "[--elevation FILE]... [--stats]\n");
            return 2;
        }
    }
    std::vector<std::uint8_t> external;
    if (atlas_path) {
        std::ifstream input(atlas_path, std::ios::binary);
        if (!input) {
            std::fprintf(stderr, "globe-fltk: cannot open %s\n", atlas_path);
            return 2;
        }
        external.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    }
    AtlasError error = AtlasError::Ok;
    const auto atlas = Atlas::parse(atlas_path ? external.data() : kEmbeddedAtlasData,
                                    atlas_path ? external.size() : kEmbeddedAtlasSize, &error);
    if (!atlas) {
        std::fprintf(stderr, "globe-fltk: atlas: %s\n", atlas_error_name(error));
        return 2;
    }
    const auto scene = Scene::create(*atlas);
    if (!scene) {
        std::fprintf(stderr, "globe-fltk: cannot build atlas scene\n");
        return 2;
    }
    ElevationPacks packs;
    if (!packs.initialize(kEmbeddedElevationData, kEmbeddedElevationSize))
        std::fprintf(stderr, "globe-fltk: %s; showing outlines\n", packs.error());
    if (elevation_count == 0 &&
        !packs.add_embedded(kEmbeddedFineElevationData, kEmbeddedFineElevationSize))
        std::fprintf(stderr, "globe-fltk: optional half-degree terrain unavailable: %s\n",
                     packs.error());
    for (unsigned i = 0; i < elevation_count; ++i) {
        if (!packs.import_file(elevations[i])) {
            std::fprintf(stderr, "globe-fltk: %s\n", packs.error());
            return 2;
        }
    }

    Fl::visual(FL_RGB);
    Workbench app(*scene, packs, width, height);
    return app.run(stats);
}
