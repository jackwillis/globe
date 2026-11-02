# Moving-frame scaling, 2026-11-01

Frozen terrain-era benchmark (9c62abc), `--convert --repeat 2`, standard drag trace. One run per
size per machine; no simultaneous agent hardware job during the recorded G4 series. Host release
compiler and G4 cross compiler are recorded in each JSON. These are moving outline frames, not
settled terrain, X11 submission or end-to-end interaction latency. Owner system activity is uncontrolled.

| Size | Host render / pack ms | G4 render / pack ms | Checksum (both) |
| --- | --- | --- | --- |
| 320x240 | 0.2155 / 0.0666 | 12.0303 / 1.4811 | 766242cde85247e3 |
| 640x480 | 0.2877 / 0.1886 | 18.2821 / 6.8071 | 41c7adb508763323 |
| 800x600 | 0.3706 / 0.2973 | 21.9879 / 11.5227 | a7a7de843b3e3b43 |
| 1024x768 | 0.4510 / 0.4494 | 28.4881 / 19.4551 | c44ecfa7baedbf7b |

Packing grows roughly with framebuffer area. Rendering also grows with projected geometry and
clipping, so these four measurements are not a fitted complexity model. Raw `scaling-{host,g4}-*.json`
files accompany this report. A preliminary G4 series overlapped a unit test and was replaced.
