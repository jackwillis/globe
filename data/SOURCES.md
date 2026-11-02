# Atlas data provenance

`atlas.gatl` is generated; the format is described in `docs/atlas-format.md`. The same text is
embedded in the file itself.

| Layer | Dataset | Version | Source ZIP SHA-256 |
| --- | --- | --- | --- |
| Coastline | Natural Earth 1:110m coastline | 4.1.0 | `664449b39070027e882abb295974d182afec18ca21107273d17e9e8bf6f64817` |
| Borders | Natural Earth 1:110m admin-0 boundary lines (land) | 5.1.0 | `bb2954981570498a4b4344f8e83fa54caa5858e15e800094112407d2bd2f940d` |

- Coastline ZIP: https://naturalearth.s3.amazonaws.com/110m_physical/ne_110m_coastline.zip
- Borders ZIP: https://naturalearth.s3.amazonaws.com/110m_cultural/ne_110m_admin_0_boundary_lines_land.zip
- Terms (public domain): https://www.naturalearthdata.com/about/terms-of-use/
- Retrieved 2026-11-01. The hashes match those recorded for the v1 prototype, retrieved the same
  day. The ZIPs are not committed (about 140 KB); the builder verifies them.
- Natural Earth draws de facto boundaries. The border layer is a cartographic reference, not a
  statement about sovereignty.

Regenerate (needs only Python 3's standard library):

```
python3 tools/atlas-builder/build_atlas.py ne_110m_coastline.zip \
    ne_110m_admin_0_boundary_lines_land.zip data/atlas.gatl
```

The output is byte-identical for the same inputs (SHA-256 of `atlas.gatl` as built on
2026-11-01: `ec2f5bde911fc131610786520a36482a98ad7312f29b953abc7ba6ccda8df773`).

## Elevation source check — 2026-11-01 (stop before builder)

Selected source for the future coarse pack: NOAA NCEI **ETOPO 2022 v1, 60 arc-second global
Ice Surface elevation**. This includes ocean bathymetry and the top of the Greenland/Antarctic
ice sheets, rather than the bedrock beneath them. The dataset is identified by
[DOI 10.25921/fd45-gt74](https://doi.org/10.25921/fd45-gt74).
The [official product page](https://www.ncei.noaa.gov/products/etopo-global-relief-model)
links the 60-second surface NetCDF below. The
[dataset metadata](https://www.ncei.noaa.gov/metadata/geoportal/rest/metadata/item/gov.noaa.ngdc.mgg.dem%3Aetopo_2022/html)
identifies these NOAA data as public domain in the United States and **CC0-1.0 worldwide**.

| Observed source | Value |
| --- | --- |
| File | `ETOPO_2022_v1_60s_N90W180_surface.nc` |
| Downloaded bytes | **478,290,125** |
| SHA-256, computed from complete download | `74bbc5c23e85188d93f0beba8116babf69e1a8f160e8dc9f053bf8df01b197b6` |
| Format | NetCDF-4 classic model in HDF5; gzip-compressed datasets |
| Elevation | `z(lat, lon)`, float32, **10,800 × 21,600** cells |
| Height reference | Meters, positive up; EGM2008, EPSG:3855 |
| Cell registration | Area/cell registered (`GDAL_AREA_OR_POINT=Area`, `node_offset=1`) |
| No-data value | `-99999.0` |

- [Exact HTTP source](https://www.ngdc.noaa.gov/thredds/fileServer/global/ETOPO2022/60s/60s_surface_elev_netcdf/ETOPO_2022_v1_60s_N90W180_surface.nc)
- [NOAA catalog entry](https://www.ngdc.noaa.gov/thredds/catalog/global/ETOPO2022/60s/60s_surface_elev_netcdf/catalog.html?dataset=globalDatasetScan/ETOPO2022/60s/60s_surface_elev_netcdf/ETOPO_2022_v1_60s_N90W180_surface.nc)

Retrieved 2026-11-01, America/Chicago. HTTP 200, `application/x-netcdf`; Last-Modified
2022-10-04 21:55:19 UTC. The catalog's advertised byte count matched the full download.
The SHA-256 was streamed during download and independently recomputed with `sha256sum`;
this is our observed digest, not a claimed publisher checksum. The source is not committed. It
was moved on 2026-11-01 from `/tmp` to `~/scratch/globe-data/etopo/` on the development laptop
so a reboot does not lose it; any copy of the file is valid if its SHA-256 matches the digest in
the table above (re-verified after the move).

Inspection of the actual file with h5py confirmed HDF5 magic `89 48 44 46 0d 0a 1a 0a`,
`_nc3_strict=1`, CF-1.5, the shape/type/units above, and coordinate arrays. Longitude increases
from -179.99166666666667 to 179.99166666666667 degrees; latitude increases from
-89.99166666666666 to 89.99166666666667 degrees. **Row zero of `z` is south**, even though the
stored GDAL GeoTransform describes a north-origin raster. A future builder must follow the
coordinate variables rather than assume north-to-south row order. Corner samples were read
successfully (southwest 2832.339599609375 m, northwest -4153.25 m); the whole elevation array
was not scanned for extrema or missing values.

The THREDDS catalog contains an apparently stale ETOPO1 description/citation. Source identity
here follows the ETOPO 2022 product link, exact filename and dataset metadata, not that catalog
summary. Attribution for a future generated pack should cite NOAA NCEI, the DOI above and its
access date. No GELV format, decoder, builder, quantization table or elevation pack was created
in this source-check pass.

## Generated elevation pack — 2026-11-01

The subsequent all-open-items pass implemented GELV v1 and built `elevation-1deg.gelv` from the
verified persisted source above. The source SHA-256 was rechecked before conversion. The pack is
360x180, 65,352 bytes, SHA-256
`2ceb255dcdad98fe28beaf85f1a64122956af4a8388e95f094858d3162b085be`.
Two independent conversions produced identical bytes; a later equivalent scalar quantizer
reproduced the same digest. Build environment: h5py 3.16.0, numpy 2.5.4, modern host Python.
The data retains the source's CC0-1.0 status; conversion tools and runtime code are GPL-3.0-or-later.
See `docs/elevation-format.md` for the mean-cell aggregation and quantization table decisions.

## Glaciated-areas source check — 2026-11-01 (not yet used by any builder)

Candidate source for an ice flag in the elevation pack (Antarctica and Greenland white, coastal
rock not): Natural Earth **glaciated areas** polygons, physical theme, version **4.1.0** at all
three detail levels. Public domain (same terms as the other Natural Earth layers). Nothing reads
these files yet; they are recorded so the later builder can verify them.

| Level | ZIP bytes | Source ZIP SHA-256 | Records | Parts | Points |
| --- | --- | --- | --- | --- | --- |
| 1:110m | 19,590 | `7247fc9dc9b7d66b0bd8970fad81c46cb18d392ed52be348a313bbd8c1f4b990` | 11 | 12 | 827 |
| 1:50m | 216,484 | `f48644b4523482501bcd380c4b7345ba574ab79b87df753447d772ea239b7daf` | 377 | 395 | 18,868 |
| 1:10m | 1,644,604 | `de6ff396ab7eee8cbe2146030cc29278ef67c468e5675c748b1372e241ecbbff` | 1,886 | 2,558 | 239,371 |

- URLs: `https://naturalearth.s3.amazonaws.com/{110m,50m,10m}_physical/ne_{110m,50m,10m}_glaciated_areas.zip`
- Counts come from reading the `.shp` headers and records (shape type 5, polygon); byte counts and
  hashes are from the complete downloads. Not measured: how the polygons rasterize onto the 1-degree grid.
- Antarctica is the largest polygon at every level (bounding box to -90 latitude, north edge -63);
  Greenland is the second (about 60 to 83 N). At 110m the layer is only 11 records, so it
  covers the two ice sheets and a few caps; 50m adds many smaller glaciers. 50m at about 217 KB
  is small enough to use as a build-time input; it never ships to the target.
- The ZIPs are not committed. They are kept at `~/scratch/globe-data/naturalearth/` on the
  development laptop.

## Lakes source check — 2026-11-01 (not yet used by any builder)

Candidate source for water cells in the elevation pack (the Great Lakes are missing from the 110m
coastline and are one cell or less at 1 degree): Natural Earth **lakes** polygons, physical theme,
version **5.0.0** at all three detail levels. Public domain (same Natural Earth terms as the other
layers). Nothing reads these files yet.

| Level | ZIP bytes | Source ZIP SHA-256 | Records | Parts | Points |
| --- | --- | --- | --- | --- | --- |
| 1:110m | 23,622 | `f2eed3c738a93010770acb0ba44273ea6a83b053641588bc902d9d6fd1cdafcb` | 24 | 24 | 465 |
| 1:50m | 252,615 | `f28d42c286d96b57a17aac2cbeb432f8c65532c20063495711fbc64e24666df3` | 412 | 465 | 19,778 |
| 1:10m | 2,349,685 | `0803a06f9c3cb4671d89b68c48b142aad9366ba40f665245e12a913fbc61722a` | 1,355 | 1,597 | 162,852 |

- URLs: `https://naturalearth.s3.amazonaws.com/{110m,50m,10m}_physical/ne_{110m,50m,10m}_lakes.zip`
- Measured from the complete downloads: byte counts, SHA-256, shape type 5 (polygon) at every level,
  and record, part and point counts read from the `.shp`. Names come from the `.dbf` `name` field.
- The five Great Lakes are present at every level, one record each (a part is a ring; islands are
  holes, so use even-odd fill). Bounding boxes agree to about 0.1 degree across levels. Points per
  lake at 50m: Superior 446, Michigan 313, Huron 583, Erie 172, Ontario 229 (110m: 31, 32, 32, 19,
  17; 10m: 1311, 785, 1540, 404, 548). Lake Victoria and Lake Baikal are also present at every level.
  **No record named Caspian exists** at any level; the Caspian Sea is not in this layer.
- **Rasterization estimate (measured, one-off Python, not a builder):** the five Great Lakes at 50m
  against 1-degree cells with edges on whole degrees, sampling 16 x 16 points per cell and filling by
  even-odd. 67 cells touch a Great Lake polygon. **29 cells are more than 50% water** by lon/lat
  area (not cos-latitude weighted); 30 cells have their center inside a polygon. The two rules
  disagree on 5 cells (lon/lat of the cell's west-south corner: -89/48, -88/44, -80/42, -80/43,
  -77/43), so the choice of rule changes the picture slightly. Largest cells are nearly all water:
  Superior at -87/47 is 100%, -88/47 is 98%, -89/47 is 95%.
- Not measured: how lakes beyond the Great Lakes rasterize, the effect on the other lakes at 1
  degree, whether the elevation file already shows the Great Lakes as water, and any builder cost.
- The ZIPs are not committed; they are kept at `~/scratch/globe-data/naturalearth/` on the
  development laptop. 50m (about 253 KB) is the likely build-time input; it never ships to the target.
