# Building DiscImageStudio

Two audiences here: people who just want the `.exe`, and people who need to reproduce CI or debug the toolset.

## Fastest path (Windows)

```bat
git clone https://github.com/abd77o/DiscImageStudio.git
cd DiscImageStudio
scripts\fetch_deps.bat
build_vs2017.bat
```

Result: `build\Release\DiscImageStudio.exe` — 32-bit, static CRT (`/MT`), elevation manifest, runs on Windows 7
x86 and x64.

## What `build_vs2017.bat` does

1. Calls `scripts\fetch_deps.bat` (Dear ImGui v1.90.9 + `stb_image.h` into `third_party\`).
2. Configures CMake with `-A Win32` and the `v141_xp` toolset, whose platform toolset keeps the binary loadable
   on Windows 7.
3. Builds `Release`.

If you do not have the *Windows XP support for C++* component of the VS installer, either install it or configure
without `-T v141_xp` — the result runs on Windows 8.1/10/11 only.

## Manual CMake

```bat
:: Developer Command Prompt for VS 2017+
cmake -S . -B build -A Win32 -T v141_xp
cmake --build build --config Release -j
```

Useful options:

| Option / variable | Effect |
|---|---|
| `-A Win32` | 32-bit build (flag omitted ⇒ x64) |
| `-T v141_xp` | Windows 7-compatible toolset |
| `-DCMAKE_TOOLCHAIN_FILE=...` | Cross-compiling from Linux (untested, expect to fight `d3d9`) |
| absence of `third_party/stb` | BMP-only build, warning at configure time |
| absence of `third_party/imgui` | GUI target skipped (tests and `dither_preview` still build) |

## Linux and macOS

There is no GUI and no burner, but the entire core and the test suite build with any C++11 compiler:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/test_burn_logic
./build/test_gui_logic
./build/dither_preview artwork.bmp disc.pgm 196 2 fs
```

`scripts/fetch_deps.sh` installs `stb_image` (enables PNG/JPEG) and the ImGui sources, but the ImGui GUI itself is
Windows-only.

## Dependencies

| Component | Version | License | How it arrives |
|---|---|---|---|
| Dear ImGui | v1.90.9 (≥1.84 required for `BeginDisabled`) | MIT | `scripts/fetch_deps.*` — never committed |
| stb_image | `nothings/stb` HEAD | public domain / MIT | same |
| Direct3D 9 | — | OS component | Windows SDK |

## Troubleshooting

| Symptom | Cause and fix |
|---|---|
| `d3d9.h` not found with `v141_xp` | Install the DirectX SDK (June 2010) and add its `Include` and `Lib\x86` folders, or build without `-T v141_xp` |
| `imgui_impl_dx9.cpp` unresolved symbols | `third_party\imgui` is incomplete — rerun the fetch script |
| `libpng`/`libjpeg` link errors | `stb_impl.cpp` was not compiled: `third_party/stb/stb_image.h` is missing |
| `LNK4098` / mixed CRT | Something added `/MD`; the project forces `/MT` for every configuration |
| Exe fails to start with `0xc0000135` | Not the static build, or a missing VC++ redistributable |
| GUI runs but the burn button stays disabled | Expected: a successful *Test write* is required first |
| Burn fails with `CHECK CONDITION`, sense `ILLEGAL REQUEST` | The drive does not accept raw DAO writes at this block size — see [HARDWARE_NOTES.md](HARDWARE_NOTES.md) |

## Reproducing CI locally

CI runs exactly this on Linux, plus the Windows job:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
./build/test_burn_logic
./build/test_gui_logic
```

The Windows job additionally builds `DiscImageStudio.exe` with `-A Win32` and uploads it as a workflow artifact.
