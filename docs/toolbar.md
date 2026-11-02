# X11 tools and cursor feedback

The separate utility window draws the core's visible, enabled and checked command states.
Click a button to issue the same command as a key. Checked buttons are filled; disabled buttons
have no outline. Closing the toolbar hides it; F2 shows it again. It follows the bottom edge on
main-window resize. The hand cursor uses the X core font: hand2 over the globe, fleur while
actively dragging, arrow elsewhere. These are fallbacks, not theme open/closed-hand artwork.

Button geometry and hit testing are pure, allocation-free core functions, as is cursor intent.
No timer or redraw is requested just for hovering. The UI redraws on exposure or changed command
state. Tests cover gaps, half-open edges, nonfinite coordinates, both drag modes, native clicks,
disabled North up and actual cursor transition calls. Cursor-disabled and inclusive-edge mutants
fail; the X11 interaction test initially failed because no toolbar window existed.

GCC ASan/UBSan 24/24 passed, including both pixel depths and quiet idle. The first clang X11 run
was sandboxed and could not open private displays; rerun outside the sandbox is recorded separately.
Retro68 compile probes include the portable core; no real G4 desktop acceptance was performed.
