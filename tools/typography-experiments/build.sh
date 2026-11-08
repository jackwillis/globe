#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
# Builds the host-side typography experiments in a scratch directory. Experimental, not part of CMake.
# Usage: build.sh WORKDIR   (downloads libschrift and FreeType sources into WORKDIR; nothing is committed)
set -eu
W=${1:?workdir}; HERE=$(cd "$(dirname "$0")" && pwd)
mkdir -p "$W"; cd "$W"
[ -d schrift ] || git clone -q --depth 1 https://github.com/tomolt/libschrift.git schrift
[ -d freetype ] || git clone -q --depth 1 https://github.com/freetype/freetype.git freetype
FT=$W/freetype; OPT="-Os -ffunction-sections -fdata-sections -w"
mkcfg() { # $1 = dir, $2 = extra module lines
  mkdir -p "$1"
  { echo "FT_USE_MODULE( FT_Driver_ClassRec, tt_driver_class )"; echo "FT_USE_MODULE( FT_Module_Class, sfnt_module_class )";
    echo "FT_USE_MODULE( FT_Renderer_Class, ft_smooth_renderer_class )";
    echo "FT_USE_MODULE( FT_Renderer_Class, ft_raster1_renderer_class )"; echo "FT_USE_MODULE( FT_Module_Class, psnames_module_class )"; echo "$2"; } > "$1/ftmodule_min.h"
}
mkcfg cfg_min ""
# Trim options this experiment does not need (WOFF/SVG gzip, colour layers, variable fonts); keep the bytecode hinter.
for d in cfg_min cfg_af; do
  sed -e 's|^#define \(FT_CONFIG_OPTION_USE_ZLIB\)|#undef \1|' -e 's|^#define \(FT_CONFIG_OPTION_SVG\)|#undef \1|' \
      -e 's|^#define \(TT_CONFIG_OPTION_COLOR_LAYERS\)|#undef \1|' -e 's|^#define \(TT_CONFIG_OPTION_GX_VAR_SUPPORT\)|#undef \1|' \
      "$FT/include/freetype/config/ftoption.h" > "$d/ftoption_min.h"
done
mkcfg cfg_af "FT_USE_MODULE( FT_Module_Class, autofit_module_class )"
SRC="$FT/src/base/ftsystem.c $FT/src/base/ftinit.c $FT/src/base/ftdebug.c $FT/src/base/ftbase.c $FT/src/base/ftbitmap.c $FT/src/base/ftglyph.c $FT/src/sfnt/sfnt.c $FT/src/truetype/truetype.c $FT/src/smooth/smooth.c $FT/src/raster/raster.c $FT/src/psnames/psnames.c"
build_ft() { # $1 = cfg dir, $2 = output prefix, $3 = extra sources
  for t in ft_label ft_pack; do
    gcc $OPT -DFT2_BUILD_LIBRARY -DFT_CONFIG_MODULES_H="<ftmodule_min.h>" -DFT_CONFIG_OPTIONS_H="<ftoption_min.h>" -I"$1" -I"$FT/include" -I"$HERE" \
        "$HERE/$t.c" $SRC $3 -Wl,--gc-sections -s -o "$2_$t"
  done
}
build_ft cfg_min ftmin ""
build_ft cfg_af ftaf "$FT/src/autofit/autofit.c"
gcc $OPT -c "$HERE/countalloc.c" -o countalloc.o
gcc $OPT -std=c99 -D_POSIX_C_SOURCE=200809L -I"$W/schrift" -I"$HERE" -include "$HERE/countalloc.h" "$HERE/schrift_label.c" "$W/schrift/schrift.c" countalloc.o -lm -Wl,--gc-sections -s -o schrift_label
printf 'int main(void){return 0;}\n' > base.c; gcc $OPT base.c -s -o base_empty
echo built
