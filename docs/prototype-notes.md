# Prototype (v1) notes

The first-draft globe is not part of this tree. It remains available as the
`v1` git tag (commit `53d230d`) and on the `main` branch. It is a reference
only: no code is copied without review for design and licensing.

Concepts worth carrying forward (re-derived and tested, not ported):

- Projection convention: longitude 0, latitude 0 maps to (0, 0, 1); positive
  longitude toward +x, north toward +y; camera-visible geometry has z >= 0.
- Chords that cross z = 0 are clipped by interpolation before projection.
- Line clipping before integer rasterization; non-finite coordinates rejected.
- Pixel packing driven by the display's detected masks and byte order.
- Event-driven redraw: the application blocks when nothing has changed.

Known limits of v1 that v2 addresses: straight chords between 2-degree samples
(no geodesic subdivision), a single detail level, fixed-size full redraws, and
no cache or refinement stages.
