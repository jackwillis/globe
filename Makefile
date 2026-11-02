CC = cc
CFLAGS = -O2 -std=c99 -Wall -Wextra -Wpedantic
CPPFLAGS = -Isrc
LDLIBS = -lm

all: planet

planet: src/main.c src/planet.c src/map.c src/planet.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ src/main.c src/planet.c src/map.c $(LDLIBS)

test_planet: tests/test_planet.c src/planet.c src/map.c src/planet.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ tests/test_planet.c src/planet.c src/map.c $(LDLIBS)

test: test_planet
	./test_planet

clean:
	rm -f planet test_planet planet-x11 planet-bench planet-view-bench planet-x11-bench test_pixels test_ximage test_places test_x11_input planet-places-bench

# NetBSD: make planet-x11 X11_CFLAGS=-I/usr/X11R7/include X11_LIBS='-L/usr/X11R7/lib -Wl,-rpath,/usr/X11R7/lib -lX11'
X11_CFLAGS =
X11_LIBS = -lX11
planet-x11: src/places.c src/places.h src/popover.c src/popover.h src/x11.c src/planet.c src/map.c src/planet.h src/ximage.c src/ximage.h src/pixels.c src/pixels.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(X11_CFLAGS) $(LDFLAGS) -o $@ src/x11.c src/places.c src/popover.c src/planet.c src/map.c src/ximage.c src/pixels.c $(LDLIBS) $(X11_LIBS)

check-map: planet
	python3 tests/test_map.py

planet-bench: src/bench.c src/planet.c src/map.c src/planet.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ src/bench.c src/planet.c src/map.c $(LDLIBS)

BENCH_FLAGS = -DPLANET_BUILD_FLAGS='"$(CFLAGS)"'

planet-view-bench: src/bench_meta.h src/bench_view.c src/planet.c src/map.c src/planet.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(BENCH_FLAGS) -o $@ src/bench_view.c src/planet.c src/map.c $(LDLIBS)

test_pixels: tests/test_pixels.c src/pixels.c src/pixels.h src/planet.c src/map.c src/planet.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ tests/test_pixels.c src/pixels.c src/planet.c src/map.c $(LDLIBS)

check-pixels: test_pixels
	./test_pixels

test_ximage: tests/test_ximage.c src/ximage.c src/ximage.h src/pixels.c src/pixels.h src/planet.c src/map.c src/planet.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(X11_CFLAGS) $(LDFLAGS) -o $@ tests/test_ximage.c src/ximage.c src/pixels.c src/planet.c src/map.c $(LDLIBS) $(X11_LIBS)

check-ximage: test_ximage
	./test_ximage

planet-x11-bench: src/bench_meta.h src/bench_x11.c src/planet.c src/map.c src/planet.h src/ximage.c src/ximage.h src/pixels.c src/pixels.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(X11_CFLAGS) $(LDFLAGS) $(BENCH_FLAGS) -o $@ src/bench_x11.c src/planet.c src/map.c src/ximage.c src/pixels.c $(LDLIBS) $(X11_LIBS)

test_places: tests/test_places.c src/places.c src/places.h src/planet.c src/map.c src/planet.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ tests/test_places.c src/places.c src/planet.c src/map.c $(LDLIBS)

check-places: test_places
	./test_places
	python3 tests/test_places.py

test_x11_input: tests/test_x11_input.c src/places.c src/places.h src/planet.c src/map.c src/planet.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(X11_CFLAGS) $(LDFLAGS) -o $@ tests/test_x11_input.c src/places.c src/planet.c src/map.c $(LDLIBS) $(X11_LIBS)

# Linux/Xvfb integration checks; native G4 interaction uses its existing desktop.
check-x11: planet-x11 test_x11_input
	X11_INPUT_TEST=./test_x11_input python3 scripts/smoke_x11.py
	X11_DEPTH=24 X11_INPUT_TEST=./test_x11_input python3 scripts/smoke_x11.py

planet-places-bench: src/bench_places.c src/bench_meta.h src/places.c src/places.h src/planet.c src/map.c src/planet.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(BENCH_FLAGS) -o $@ src/bench_places.c src/places.c src/planet.c src/map.c $(LDLIBS)
