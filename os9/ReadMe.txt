Poodle Earth 0.1 - public preview for Mac OS 9 (PowerPC)

An offline, software-rendered globe: drag to turn it, zoom in, and click a
country or city for its information card. Nothing is downloaded or sent;
there is no network use.

WHAT IT NEEDS
 - A PowerPC Mac running Mac OS 9 (Color QuickDraw, Appearance Manager).
 - Application memory: Preferred 24 MB, Minimum 12 MB (already set in the
   application). If memory is short Poodle Earth opens a smaller window;
   with a big monitor, raise the Preferred size (Finder: select the
   application, File > Get Info > Memory). Quit it before changing that.
 - Tested so far: a Power Mac G4 (AGP, 467 MHz, 256 MB) on bare Mac OS 9.2.2.
   Other Macs and system versions have not been tried. Not for Mac OS X
   (Classic mode on Tiger worked in earlier builds, but is not supported).

INSTALL
 Copy the folder (the Poodle Earth application, this ReadMe, the two
 License files - the GPL in halves small enough for SimpleText - and
 DataCredit) to your hard disk and double-click the application. It writes no
 preferences or other files. If you downloaded the .sit, expand it with
 StuffIt Expander. The .bin is the same application as a MacBinary file, and
 the .dsk is the same application on a 1.44 MB HFS disk image.

USE
 Drag the globe to turn it (arrow keys also turn it). Double-click to
 center and zoom in. Click a country or city for its card; Copy in the Edit
 menu copies the card. The Map and Relief looks are in the list at the right
 and the View menu.
 Single keys: + and - zoom, R reset, N align north, G graticule,
 B borders, T drag mode, D night side, M measure two points. The menus
 show the Command-key versions (Command-= also zooms in). Command-W
 closes About or Preferences; Command-Q quits.
 Edit > Preferences sets metric or imperial units.

KNOWN LIMITS (version 0.1)
 - After drawing or resizing, the edge of the globe may occasionally be
   left unfinished; turn the globe a little or zoom to repaint it.
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
 The complete corresponding source code of this exact version, including
 the scripts that build and package it, is the file
 PoodleEarth-0.1.0-XXXXXXX-source.tar.gz, published with the same download
 page as this program; XXXXXXX is the same revision code as in this
 download's file name. The repository is https://github.com/jackwillis/globe
 and this version is tagged v0.1.0.
