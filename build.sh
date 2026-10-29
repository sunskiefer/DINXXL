#!/usr/bin/env bash
# Build DINXXL for MPC OS Gen1 / Force (armv7-a hard-float, glibc <= 2.31 so it also loads on MPC OS 2.x).
#   ./build.sh                     (MPC_VST=../mpc-vst-plugins to use another framework checkout)
# Needs: python3 + Pillow + Playwright Chromium (skin art) and Zig (pip install ziglang). No Docker.
# Output: build/package/L'Cronx - VST - DINXXL/  (skin + dinxxl.so) and build/pluginlist-entry.xml
set -euo pipefail
cd "$(dirname "$0")"
MPC_VST="${MPC_VST:-$PWD/third_party/mpc-vst-plugins}"
ZIG="${ZIG:-python3 -m ziglang}"
NAME="L'Cronx - VST - DINXXL"

python3 tools/gen_params.py
python3 tools/make_art.py
python3 tools/make_tile.py
mkdir -p build/arm
rm -rf build/skin
env -u SHADOW_ART python3 "$MPC_VST/tools/gen_vst.py" vst.json

TGT="-target arm-linux-gnueabihf.2.31 -mcpu=generic+v7a+vfp3d16-d32-neon+thumb2 -ffp-contract=off"
COMMON="-O2 -fPIC -fvisibility=hidden -ffunction-sections -fdata-sections -DNDEBUG -w"
# the vendored firmware and its shims see the firmware's own headers ...
FW="-std=gnu++17 -fno-exceptions -fno-rtti -Isrc/shim -Isrc/fw/include -Isrc/fw/src -Isrc/fw/lib/miniz/src -Isrc"
# ... the plugin glue sees the framework's engine.h, not the firmware's
GLUE="-std=gnu++17 -fno-exceptions -fno-rtti -Isrc -Ibuild -I$MPC_VST/wrapper"

OBJS=""
for f in src/machine.cpp src/hw.cpp src/fw_unit.cpp src/fw_unit2.cpp \
         src/dsp/ymfm_opl.cpp src/dsp/ymfm_adpcm.cpp src/dsp/ymfm_pcm.cpp src/dsp/ymfm_ssg.cpp; do
  o="build/arm/$(echo "$f" | tr / _).o"
  $ZIG c++ $TGT $COMMON $FW -c "$f" -o "$o"
  OBJS="$OBJS $o"
done
$ZIG cc $TGT $COMMON -std=gnu11 -Isrc/fw/lib/miniz/src -c src/fw/lib/miniz/src/miniz_tinfl.c -o build/arm/miniz_tinfl.o
$ZIG c++ $TGT $COMMON $GLUE -c src/engine.cc -o build/arm/engine.o
$ZIG cc $TGT $COMMON -std=gnu11 -Ibuild -I"$MPC_VST/wrapper" -c "$MPC_VST/wrapper/vst2_wrap.c" -o build/arm/vst2_wrap.o

printf '{\n  global: VSTPluginMain;\n  local: *;\n};\n' > build/arm/exports.map
$ZIG c++ $TGT -shared -fPIC -Wl,--no-undefined -Wl,--version-script=build/arm/exports.map -Wl,--gc-sections -Wl,-s \
  $OBJS build/arm/miniz_tinfl.o build/arm/engine.o build/arm/vst2_wrap.o -lm -ldl -lpthread -o build/arm/dinxxl.so

PKG="build/package/$NAME"
rm -rf build/package && mkdir -p "$PKG"
cp -r "build/skin/$NAME/." "$PKG/"
cp build/arm/dinxxl.so "$PKG/"
mkdir -p "$PKG/card" && cp card/factory.bin "$PKG/card/"      # the BlastBeats factory card (tools/pack_card.py)
echo "-> $PKG"
