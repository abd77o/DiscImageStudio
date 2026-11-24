#!/bin/sh
# Same as fetch_deps.bat, for Linux/macOS/Git-Bash.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEPS="$(mktemp -d)"
mkdir -p "$ROOT/third_party/imgui" "$ROOT/third_party/stb"
git clone --depth 1 --branch v1.90.9 https://github.com/ocornut/imgui.git "$DEPS/imgui"
git clone --depth 1 https://github.com/nothings/stb.git "$DEPS/stb"
for f in imgui.h imgui.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp imgui_internal.h imconfig.h imstb_rectpack.h imstb_textedit.h imstb_truetype.h; do cp "$DEPS/imgui/$f" "$ROOT/third_party/imgui/"; done
for f in imgui_impl_dx9.h imgui_impl_dx9.cpp imgui_impl_win32.h imgui_impl_win32.cpp; do cp "$DEPS/imgui/backends/$f" "$ROOT/third_party/imgui/"; done
cp "$DEPS/stb/stb_image.h" "$ROOT/third_party/stb/"
rm -rf "$DEPS"
echo "Dependencies installed in third_party/"
