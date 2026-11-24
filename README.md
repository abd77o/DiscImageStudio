<div align="center">
  <img src="docs/images/hero-7disc.jpg" alt="7Disc Image Studio — disc authoring and etching suite" width="100%">
</div>

# 7Disc Image Studio 💿

**Draw a picture into the data side of a CD-R using the disc's own pit pattern — no label, no printer, no LightScribe drive. The laser is the pen.**

<div align="center">

**🇬🇧 English** &nbsp;·&nbsp; [**🇸🇦 العربية — اقرأ هذا الملف بالعربية**](README.ar.md)

</div>

<div align="center">
  <img src="docs/images/real-burn-result.jpg" alt="A CD-R burned by 7Disc Image Studio, photographed on a desk" width="72%">
  <br>
  <sub><b>A real CD-R, burned by this program — shot with a camera, not rendered.</b><br>
  The entire picture is made of the pit pattern on the <i>data</i> side: no label, no printing, no ink. (Photo: <a href="https://github.com/abd77o">@abd77o</a>)</sub>
</div>

[![Latest release](https://img.shields.io/github/v/release/abd77o/DiscImageStudio?label=release&color=2ea44f)](https://github.com/abd77o/DiscImageStudio/releases/latest)
[![Downloads](https://img.shields.io/github/downloads/abd77o/DiscImageStudio/total?label=downloads&color=0969da)](https://github.com/abd77o/DiscImageStudio/releases)
[![CI](https://github.com/abd77o/DiscImageStudio/actions/workflows/ci.yml/badge.svg)](https://github.com/abd77o/DiscImageStudio/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows%207%2F8%2F10%2F11%20(x86%2Fx64)-0078D4.svg)](#download)
[![No runtime dependencies](https://img.shields.io/badge/dependencies-zero-3fb950.svg)](#features)

<div align="center">

<a href="https://github.com/abd77o/DiscImageStudio/releases/latest/download/DiscImageStudio-win32.exe">
  <img src="https://img.shields.io/badge/%E2%AC%87%EF%B8%8F%20Download%20for%20Windows-DiscImageStudio.exe-2ea44f?style=for-the-badge" alt="Download for Windows" height="46">
</a>
&nbsp;
<a href="https://github.com/abd77o/DiscImageStudio/releases">
  <img src="https://img.shields.io/badge/All%20releases-Releases-444?style=for-the-badge" alt="All releases" height="46">
</a>

</div>

**One 610 KB executable · no installer · no runtime dependencies · no network needed after download**

| 1 | 2 | 3 |
|:--:|:--:|:--:|
| **Download** the `exe` | **Run it as administrator** | **Pick your image** and start burning |

> ### ⚠️ Read this first
> This software is **experimental**, and it writes to the disc at the drive's raw command level, which means **the disc is permanently consumed** once you burn.
> Start with a **CD-RW**, keep **Test write** enabled, and read [docs/HARDWARE_NOTES.md](docs/HARDWARE_NOTES.md) before using a disc you care about.
> **Experimental software that permanently destroys the disc it writes to.**

---

## 🎯 Why 7Disc?

- **An etching studio, not just a disc burner.** Burners write files; this program writes an **image**. Every detail you see on the disc surface is the result of pit decisions made at the single-sector level.
- **As light as a disc studio can be.** Zero runtime dependencies and one statically linked executable: no .NET, no extra packages, no installation.
- **Built for the machines people actually use.** It runs on Windows 7 and on a 512 MB machine; the dithering engine's whole error-diffusion memory is under 1 MB, and the internal raster never exceeds 1 MB, whatever the disc size.
- **It speaks the drive's own language.** MMC commands at the SCSI level: `SEND CUE SHEET` and `WRITE(10)` in Disc-At-Once mode, with buffer-underrun handling, and no intermediary layer hiding anything.
- **A clear philosophy: don't ruin your disc.** A buffer watchdog reads the drive's buffer fill level live, widens its batches, and suggests a slower speed for the next attempt; a real burn also stays locked until *Test write* has passed once.
- **Calibrated on your hardware.** A 16-step ladder of byte patterns is burned as rings; you pick the lightest and darkest by eye, and those values are saved and used for your drive and your media — not for theoretical assumptions.
- **Proof, not renders.** The image at the top of this page is a photograph of a real burned disc, and the burn path is rehearsed end to end on a mock drive with no hardware at all.
- **No claims without evidence.** Three test suites exercise the complete burn path against a mock drive, with no hardware required, and they run on Linux on every change.

> **Verifiable numbers:** the whole program is **610 KB** — smaller than a phone photo — with **44 C++ files** out of 76 files in the repository, **3 test suites**, and **zero** dependencies.
>
> **Honestly:** `0.1.x` is a strong beginning, not a finish line. The burn path is verified automatically against a mock drive, and real-world results are documented with the author's own photographs. The (drive × media × speed) matrix is still being built — and it is the single most valuable thing you can contribute.

---

## 🖥️ Interface

<div align="center">
  <img src="docs/images/ui-concept.jpg" alt="Proposed interface design for 7Disc Image Studio" width="94%">
  <br>
  <sub><b>The proposed modern interface</b> — Start / Design / Preview / Etch / Manage.<br>
  The interface shipped in <code>0.1.x</code> is simpler than that: a single ImGui window with an image tab, a calibration tab and a progress bar.</sub>
</div>

---

<a id="features"></a>

## 🛠️ Features

- **Polar dithering engine:** the image is sampled along the real spiral path (`r² = r₀² + p·s/π`), and the error is diffused **in the geometry of the disc itself**: to the next cell along the track, and to the cell at the same angle after one or more revolutions. **Floyd–Steinberg** and **Jarvis–Judice–Ninke** kernels, 2 to 8 tones.
- **Constant memory:** the image is read line by line and downsampled to a small square raster, and the error lives in a ring buffer sized for roughly two revolutions — so burning an 80-minute disc needs no more memory than burning a 5-minute one.
- **Full sector control:** every one of the **2352 bytes** in a sector is composed inside the program (Audio DAO layout by default, with an ECMA-130 scrambler for the Mode-1 layout), so exactly what the dithering engine chose reaches the CIRC/EFM encoder.
- **Complete MMC burn path:** `SET CD SPEED` → `MODE SELECT` → `SEND CUE SHEET` → `WRITE(10)` from LBA −150 → `SYNCHRONIZE CACHE`, over SPTI, with buffer-underrun-free recording enabled.
- **Buffer watchdog:** reads `READ BUFFER CAPACITY` during the burn, widens batches as the buffer drains, and suggests a slower speed for next time instead of aborting the burn halfway through.
- **16-step calibration:** a ladder of candidate bytes burned as successive rings; you pick the lightest and darkest for your drive and media, and the value is stored in `config.ini`.
- **Safety interlocks:** a real burn stays locked until *Test write* passes once, and cancelling warns that DAO cannot be resumed.
- **Hardware-free tests:** the engine, sector composition, every MMC command, the buffer watchdog and the whole burn orchestration are tested against a mock drive.

---

## ⚙️ How it works

<div align="center">
  <img src="docs/images/architecture.svg" alt="Data path from image to disc" width="100%">
</div>

1. **Image → 8-bit raster.** `ImageSource` reads the file line by line (PNG/JPEG through `stb_image`, BMP natively) and downsamples it to a square 8-bit raster no larger than 1024 × 1024.
2. **Raster → tone levels.** `PolarDitherEngine` walks the spiral cell by cell (196 bytes per cell, 12 cells per sector — about 1.33 mm of track). Each cell is sampled at its true (radius, angle), then quantised, and the error moves to its neighbours **on the disc** — which is why the grain follows the track instead of the screen's edges.
3. **Tones → sectors.** `SectorComposer` turns each tone into a raw 2352-byte sector. In Audio DAO there is no scrambling and no EDC/ECC, so the bytes reach the encoder untouched.
4. **Sectors → drive.** `RawDaoWriter` issues the MMC sequence above, fed by `SafeBufferMonitor`, and reports progress, buffer state and the suggested speed back to the interface.

### Disc geometry, by the numbers

| Quantity | Value |
|---|---|
| Cell size | 196 bytes of payload |
| Cells per sector | 12 |
| Sector | 2352 bytes, raw |
| Cell pitch along the track | ≈ 1.333 mm |
| Track pitch across the radius | 1.6 µm |
| Spiral mapping | `r² = r₀² + p·s/π` |
| Total cells / sectors for an 80-minute disc | 3,403,510 / 336,125 |

Deeper detail lives in [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

---

<a id="download"></a>

## ⬇️ Download and install

### The easy way: a ready-made executable

<div align="center">

**[⬇️ Download `DiscImageStudio-win32.exe` from the releases page](https://github.com/abd77o/DiscImageStudio/releases/latest)**

</div>

| Step | What you do |
|:--:|---|
| 1 | Download `DiscImageStudio-win32.exe`, plus the `.sha256` file so you can verify the download |
| 2 | Put it in any folder — **no installation, no setup, no registry changes** |
| 3 | Right-click it and choose **Run as administrator** (mandatory: writing to the drive needs administrator rights) |
| 4 | If Windows warns you: **More info** → **Run anyway** (the binary is not signed with a paid certificate) |

### Verifying the download (optional)

```powershell
Get-FileHash .\DiscImageStudio-win32.exe -Algorithm SHA256
# then compare the output with DiscImageStudio-win32.exe.sha256 from the same release page
```

### Or build it yourself from source

```bat
git clone https://github.com/abd77o/DiscImageStudio.git
cd DiscImageStudio
scripts\fetch_deps.bat       :: Dear ImGui + stb_image
build_vs2017.bat             :: -> build\Release\DiscImageStudio.exe
```

Or directly with CMake: `cmake -S . -B build -A Win32 -T v141_xp`, then `cmake --build build --config Release`.

<details>
<summary><strong>Linux / macOS — tests only (no GUI, no drive)</strong></summary>

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

</details>

---

## 💻 Requirements

| Item | Details |
|---|---|
| **Operating system** | Windows 7 SP1 or newer; the static x86 build also runs on 64-bit Windows (the code is designed and engineered for 7 through 11) |
| **Memory** | Any amount; the program works within a few megabytes and is designed for 512 MB machines |
| **Hardware** | A **CD-R / CD-RW** burner that supports raw Disc-At-Once writing. **CD media only** — most DVD burners do not support raw DAO |
| **Images** | PNG / JPEG / BMP |
| **Privileges** | Administrator (mandatory for writing to the drive) |
| **Network** | Not required after download |

---

## 🚀 Usage

1. Run the program **as administrator**, pick your drive and a low speed (**4×** is a safe start).
2. **Calibration tab:** burn the ladder onto a CD-RW, look at the disc, and record the lightest and darkest ranges; the values are saved to `config.ini`.
3. **Image tab:** load your picture, adjust brightness, contrast, gamma, rotation and image radius, and check the preview. Keep **Test write** enabled until it passes once.
4. **Burn:** progress, buffer state and the suggested speed appear live. **Cancelling a real burn destroys the disc**, because Disc-At-Once cannot be resumed.

---

## 🛡️ Safety and hardware limits

- **CD media only.** The pre-burn check rejects any non-CD media.
- **Burning destroys the disc.** Start with a CD-RW.
- **Resolution is limited by the medium itself:** cells are about 1.3 mm along the track and 1.6 µm across it, and the encoder spreads data across a whole sector — expect a smooth gradient radially and less detail tangentially.
- **Test write** is unsupported on many modern burners; if your drive refuses it, try a CD-RW first, then set `test_write_passed=1` in `config.ini`.
- The full list: [docs/HARDWARE_NOTES.md](docs/HARDWARE_NOTES.md).

---

## 📁 Repository layout

| Path | Contents |
|---|---|
| `src/core` | Disc geometry, streaming image loading, polar dithering engine, `config.ini` |
| `src/burn` | Scrambler, sector composition, MMC commands, CUE sheet, buffer watchdog, DAO writer, SPTI device |
| `src/calib` | Calibration sources (ladder and interleaved ranges) |
| `src/gui` | Burn thread, ImGui interface logic, Win32 and Direct3D 9 host |
| `tests` | Mock-drive tests (logic, interface, full burn) |
| `tools` | `dither_preview` — headless preview |
| `scripts` · `docs` | Dependency fetching · architecture, build and hardware notes |

---

## 🧪 Tests and continuous integration

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
ctest --test-dir build --output-on-failure
```

Three suites: `test_burn_logic` (scrambler, sector composition, every CDB, buffer watchdog, burn scenarios), `test_gui_logic` (settings, clipping, task lifecycle, cancellation, burn lock), and `test_image_burn` (from BMP all the way to the writer on a mock drive). None of them needs a burner, a disc or administrator rights.

GitHub Actions runs the tests on Linux (GCC and Clang) and builds the interface on Windows; every tag produces a **published release** carrying `DiscImageStudio-win32.exe` and its checksum file.

---

## 🗺️ Roadmap

- [ ] **The modern interface** from the proposed design (Start / Design / Preview / Etch / Manage, with a dark mode)
- [ ] A validation matrix on real hardware (drive × media × speed) with a ready-made report template
- [ ] Automatic calibration (reading the ladder by inspection instead of by eye)
- [ ] Pre-compensation for CIRC scrambling to sharpen the image
- [ ] A command-line mode for batch-burning a set of designs
- [ ] Verifying the Mode-1 constants in the CUE sheet (`kFormDataRaw`, `kFormLeadData`) against MMC

---

## 🤝 Contributing

The most valuable contribution today is a **hardware report**: drive model, firmware version, media type, speed, result, and a photo of the disc. Read [CONTRIBUTING.md](CONTRIBUTING.md) and [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).

- 🐞 [Report an issue](../../issues/new/choose) · 🔐 [Security policy](SECURITY.md) · 📋 [Changelog](CHANGELOG.md)

---

## 📄 License

[MIT](LICENSE) © 2026 **abd77o**.

Third-party components: **Dear ImGui** (MIT) and **stb_image** (public domain / MIT), fetched at build time and never committed to the repository.

## 📬 Contact

- GitHub: [@abd77o](https://github.com/abd77o) · Email: [poi99poin@gmail.com](mailto:poi99poin@gmail.com)
- Releases: [github.com/abd77o/DiscImageStudio/releases](https://github.com/abd77o/DiscImageStudio/releases)

---

<div dir="rtl" align="right">

**🇸🇦 النسخة العربية الكاملة من هذا الملف متاحة في [README.ar.md](README.ar.md).**

</div>
