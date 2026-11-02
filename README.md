# Planet lab

Portable C99 CPU-rendered globe for Linux and NetBSD/macppc.
Phase 1 currently renders synthetic latitude/longitude grids headlessly.

```
make
make test
./planet grid.ppm 31 22
```

The optional arguments are yaw and pitch in degrees. Output is a 320×240
binary RGB PPM. No X11 libraries are required for this phase.

See docs/architecture.md and docs/validation.md for conventions and evidence.

## Real coastlines

```
./planet earth.ppm 0 -15 data/coastline.plnt
make check-map
```

## Interactive X11 window

On Linux:

```
make planet-x11
./planet-x11
```

On the G4, in a terminal in the logged-in desktop session:

```
cd /home/claude/planet-lab
./planet-x11 data/coastline.plnt
```

Drag with the left mouse button to rotate; scroll the wheel to zoom.
Arrows also rotate, + / - zoom, R resets, Esc quits. The remote SSH account cannot
currently access the desktop display. Run from an already authorized session;
do not change display permissions. The project directory may need to be copied
into the desktop user's own workspace if that user cannot traverse this path.

The globe stays still until you interact. The application blocks until input or
exposure requires a redraw.
It uses XPutImage and XPutPixel's image-layout handling for TrueColor visuals.
Oversized windows use a centered render capped at 2048 pixels per axis.

## Headless benchmark

```
make planet-bench
./planet-bench data/coastline.plnt > result.json
```

See docs/benchmarks/README.md for the native G4 baseline and timing limitations.

## Themes and map layers

Click the top toolbar: the first button cycles Ocean / Atlas / Terminal colors;
Grid, Borders and Coasts toggle their respective layers; Reset restores the camera.
Keyboard alternatives: T = theme, G = grid, B = borders, C = coastlines.
Mouse drag and wheel zoom work as before. Borders load from data/borders.plnt;
if unavailable, the toolbar displays Borders n/a.

The colored globe background is a flat ocean disc. Land fill and actual terrain
or elevation colors are not implemented yet; palettes do not imply elevation.
The plain headless renderer and its benchmark keep their original output.

## Performance comparisons and presentation timing

```
make planet-view-bench planet-x11-bench
./planet-view-bench 640 > view-comparison.json
./planet-x11-bench --headless 640 > conversion-comparison.json
```

The first command checks every optimized render against the reference. The second
compares conversion against XPutPixel using an explicitly modeled RGB565 image;
it needs Xlib but no display and measures no presentation.

From an already authorized desktop terminal, measure real submission and pacing:

```
./planet-x11-bench 320 > presentation-320.json
./planet-x11-bench 640 > presentation-640.json
```

Each run is capped at 120 samples, stops starting samples after 45 seconds, and
has a 55-second process alarm as a hard backstop.
The presentation benchmark opens a temporary window and targets 30 FPS. It reports
conversion, colored rendering, submission/flush, XSync proxy, and paced frame times
separately. It closes its window on completion. XSync is not physical scanout.
The interactive application remains mouse-driven and does not auto-rotate.

On NetBSD, add these arguments to make when building X11 targets:

```
X11_CFLAGS=-I/usr/X11R7/include X11_LIBS='-L/usr/X11R7/lib -Wl,-rpath,/usr/X11R7/lib -lX11'
```

The latest native before/after measurements are in
[the optimization report](docs/benchmarks/portable-optimization.md).

The controls occupy a separate 28-pixel strip above the globe. The initial window
is 640×508, preserving a full 640×480 globe viewport. The toolbar has its own child
window and offscreen backing pixmap, so globe updates never overwrite it. Mouse
rotation and wheel zoom apply within the globe area.

## Wikidata facts

Click a diamond city marker to open its popover. Thirteen cities, including
Chicago, are included in an offline Wikidata snapshot. Facts include a description,
dated population, coordinates and source revision. Toggle markers with Places
or P. Close a popover with x, Escape, or by clicking/dragging the globe.

Copy Wikipedia and Copy Wikidata place their URLs on the clipboard; paste into a
browser while the globe stays open. The Wikidata link points to the saved source
revision. No network connection is needed to view the saved facts.

See [places and data format](docs/places.md) and [source provenance](data/SOURCES.md).
