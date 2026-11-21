Poodle Earth - Classic PowerPC ReadMe

Poodle Earth v0.1.2

An offline, software-rendered globe: drag to turn it, zoom in, and click a
country or city for its information card. Nothing is downloaded or sent;
there is no network use.

WHAT IT NEEDS
 - A PowerPC Mac; the published v0.1.0 was tested on Mac OS 9.2.2.
 - Application memory: Preferred 24 MB, Minimum 12 MB (already set in the
   application). If memory is short Poodle Earth opens a smaller window;
   with a big monitor, raise the Preferred size (Finder: select the
   application, File > Get Info > Memory). Quit it before changing that.
 - Tested so far: a Power Mac G4 (AGP, 467 MHz, 256 MB) on bare Mac OS 9.2.2.
   Later QuickDraw-cursor previews also passed owner-reported smoke tests
   in Infinite Mac on PowerPC System 7.1.2, 7.5, 7.5.3 and Mac OS 7.6,
   8.0, 8.1 and 8.5 (#174). These are build-specific tests, not exhaustive
   support promises. No 68k application. Not a native Mac OS X program;
   earlier Tiger Classic tests are separate from bare Mac OS 9.

INSTALL
 StuffIt path: expand the downloaded .sit on the Mac with StuffIt Expander.
 Keep the Poodle Earth application, this ReadMe, both License files (the
 GPL in halves small enough for SimpleText) and DataCredit together in a
 folder on your writable hard disk. Double-click the application.

 ISO/CD path: attach the ISO as a CD-ROM
 in a compatible PowerPC emulator, open the volume in Finder, and drag the
 application and all four text files into a folder on the guest hard disk.
 Launch the copy; StuffIt is not needed. The CD is not bootable and contains
 no system software. Use Finder to preserve the resource fork and Finder
 information. Ordinary copies through non-Mac filesystems can lose them.

 On CD, Get Info memory settings are read-only: quit and use the disk copy.
 The app writes no preferences or other files.

USE
 Drag the globe to turn it (arrow keys also turn it). Double-click to
 center and zoom in. Click a country or city for its card; Copy in the Edit
 menu copies the card. The Map and Relief looks are in the list at the right
 and the View menu.
 Single keys: + and - zoom, R reset, N align north, G graticule,
 B borders, T drag mode, D night side, M measure two points. The menus
 show the Command-key versions (Command-= also zooms in). Command-Q quits.
 Edit > Preferences sets metric or imperial units for this session only.
 This version has a center-distance scale, a north-following compass and
 country names on settled views. View > Country Names toggles the names;
 labels disappear during drag and zoom. Classic uses installed Helvetica,
 then Palatino, and Poodle Earth distributes no fonts.
 Controls help: H or ? or Apple menu > Controls; Escape hides help and does
 not quit. S and H are plain keys only; use the menu Command shortcuts for
 Copy, Preferences, Close and Quit. Other listed globe keys also have Command
 alternatives in the Controls list. Command-W closes the front window.
 Classic has no native mouse-wheel input; use Page Up/Down or + and -.

KNOWN LIMITS
 - The original v0.1.0 could leave an unfinished globe edge; turn or zoom
   to repaint.
 - The data is small: 1:110 million-scale coastlines and borders,
   one-degree terrain, 304 large cities. Small islands, small countries
   and many cities are missing. It is a globe, not a street atlas.
 - Place names are plain ASCII. The window has no icon of its own yet.
 - The window size is limited by the memory available to the application.

LICENSE AND CREDITS
 Copyright (c) 2026 Jack Willis. Free software under the GNU General Public
 License, version 3 or later, with NO WARRANTY: see the two License files.
 Data credits (Natural Earth, NOAA ETOPO 2022, Wikidata, and RESOLVE
 Ecoregions 2017 under CC BY 4.0, adapted): see the DataCredit file and the
 About Poodle Earth window (Apple menu).

SOURCE CODE
 Matching source is published beside each released application at:
 https://github.com/jackwillis/poodle-earth/releases
 Use the release manifest for the matching source revision and checksums.
 Repository and user guide:
 https://github.com/jackwillis/poodle-earth
 https://github.com/jackwillis/poodle-earth/blob/main/docs/user-guide.md
