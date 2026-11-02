# Next useful increments

1. **Sparse labels and picking.** Start with continents and oceans, then major
   countries/cities. Clip hidden-hemisphere labels, rank candidates and prevent
   overlaps. Add hover or click coordinates to make the globe useful for exploring.
   Retain offline-prepared source attribution; do not place text across the limb.
2. **Zoom-dependent vector detail.** Prepare several simplification levels
   offline in the existing bounded binary format. Select by projected error, not
   just a fixed zoom threshold, and use hysteresis so detail does not flicker
   between levels. Recheck dateline and horizon clipping at each level. Measure
   memory use and native frame times before adding more detailed source geometry.
3. **Land fill, then real elevation.** Add tested spherical polygon handling,
   including holes, Antarctica and horizon clipping. An elevation layer needs a
   separate attributed dataset, compact preprocessing and an explicit color key.
   Keep vector overlays and the portable reference path.
4. **Measure native presentation.** Run the supplied bounded X11 benchmark from
   the authorized desktop. Choose any further transport or AltiVec work based on
   measured conversion, submission and pacing costs.

The toolbar separation is implemented. This list is a roadmap, not a claim that
labels, level-of-detail switching or terrain are already available.
