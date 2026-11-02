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
