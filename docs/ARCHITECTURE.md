# Architecture

A tour of the code for people who want to change it. Line-level detail lives in the headers — this document
explains the boundaries, the invariants, and the reasoning behind the odd-looking choices.

## The one seam that matters

```
        ┌─────────────────────────────────────────────────────────────┐
        │  platform-neutral core + burn orchestration + GUI logic     │
        └───────────────────────────┬─────────────────────────────────┘
                                    │  IScsiDevice::execute(cdb, dir, buf, len, timeout)
        ┌───────────────────────────┴─────────────────────────────────┐
        │  SptiDevice (Windows)          MockDevice (tests)           │
        └─────────────────────────────────────────────────────────────┘
```

`src/burn/ScsiDevice.h` declares a single method: send one SCSI command, get one `ScsiResult`
(status, sense key, ASC/ASCQ, OS error). Everything above it — dithering, sector composition, MMC command
building, CUE sheets, the buffer monitor, the whole `RawDaoWriter` state machine, the GUI's job flow — is plain
C++11 with no Windows, no threads of its own and no I/O. That is what lets the test suite exercise a complete
Disc-At-Once burn on Linux with no burner attached.

Corollary for contributors: **do not** add `#ifdef _WIN32` above this line. If a new feature needs the platform,
give it a small abstract interface next to `IScsiDevice` and implement it twice.

## Pipeline, module by module

### 1. `src/core` — geometry, image, dither

- **`DiscGeometry.h`** — closed-form spiral math for a CLV disc (1.2 m/s, 75 sectors/s, 1.6 µm pitch,
  2352 bytes per sector ⇒ ~16 mm of track per sector). Given a distance along the track it returns radius and
  angle: `r² = r₀² + p·s/π`, `θ = 2π(r − r₀)/p`. Both directions are used; the inverse (`lengthAtRadius`) sizes
  the burn, the forward form places cells.
- **`ImageSource.h/.cpp`, `ImageLoader.*`** — streaming loaders. BMP is decoded row by row; PNG/JPEG go through
  `stb_image` from memory (`stb_impl.cpp` is the only translation unit that compiles stb, with only PNG and JPEG
  enabled). The result is a square `GrayRaster` of at most 1024 × 1024 bytes, built by centre-cropping and
  box-filtering. `kMaxPixels` refuses absurd source images so a 512 MB Windows 7 box cannot be made to swap.
- **`ImageSource` sampling contract** — `sampleDisc(x, y)` takes coordinates in the unit disc with `y` up and
  returns 1.0 (white) outside the disc. Every consumer obeys this, which is why the calibration sources and the
  preview tool never agree to disagree about orientation.
- **`PolarDitherEngine.*`** — the interesting part. The disc is a sequence of *cells* (`cellBytes`, default 196;
  it must divide 2352, giving 12 cells per sector, ≈1.33 mm of track each). Cell *n* has a known position, so the
  engine samples the raster there, quantises to `levels` tones, and diffuses the error to neighbours **in disc
  geometry**: the next cell along the track, and the cell at the same angle one or more revolutions later.
  The error store is a ring buffer spanning ~2 revolutions (well under 1 MB), so memory does not depend on disc
  length. `produce()` hands out tones in chunks; the burner pulls, the engine never allocates during a burn.
- **`Config.*`** — a deliberately tiny `key=value` reader/writer for `config.ini`. Missing files and unknown keys
  are not errors. Values are clamped on load, because a hand-edited config must not be able to produce a
  nonsensical burn.
- **`PlatformFile.*`** — UTF-8 paths everywhere; on Windows this wraps the wide-character API so non-ASCII
  (for example Arabic) directories work.

### 2. `src/burn` — from tones to a drive

- **`Scrambler.*`** — ECMA-130 Annex B scrambler: 15-bit LFSR (`x¹⁵ + x + 1`), state `0x0001`, LSB-first,
  applied to bytes 12…2351 (sync is left alone). XOR is involutive, so the same function scrambles and
  descrambles.
- **`SectorComposer.*`** — maps a tone index to one of eight byte patterns and fills a raw 2352-byte sector.
  - `kLayoutAudio` (default): audio tracks are neither scrambled nor protected by EDC/ECC, so all 2352 bytes are
    under host control and reach the drive's CIRC/EFM encoder verbatim. This is the layout with the fewest
    unknowns, hence the default.
  - `kLayoutDataMode1`: sync + a valid MSF header + pattern body, no EDC/ECC (readers will complain — fine for
    an art disc). If the drive scrambles the track itself, the composer pre-XORs the keystream so the pattern
    still arrives at the encoder unchanged.
  - The `tonePattern` table ships with placeholders: the real light/dark pair depends on drive and media and is
    chosen with the calibration ladder.
- **`Mmc.*`** — pure functions that build CDBs (`WRITE(10)`, `READ BUFFER CAPACITY`, `SEND CUE SHEET`,
  `MODE SELECT(10)` + the 52-byte Write Parameters page with SAO/DAO and raw-2352 block type, `SET CD SPEED`,
  …) plus parsers for their responses. No I/O, so every byte can be asserted in a unit test.
- **`CueSheet.*`** — MSF conversion and the single-track DAO cue sheet. The Audio data forms are the well-trodden
  values; the two data-track constants are marked `VERIFY` in the header because they must be checked against the
  MMC "CUE SHEET Data Form" table before anyone relies on the Mode-1 layout.
- **`SafeBufferMonitor.*`** — pure policy. It polls `READ BUFFER CAPACITY` every few batches while healthy and
  every batch once the drive is starving, with hysteresis (`lowFill` / `criticalFill` / `recoverFill`) and a
  priming window because the buffer legitimately reads empty at the start of a burn. It cannot change the write
  speed mid-session (MMC offers no standard way), so instead it widens batches, quiets the UI, and recommends a
  lower speed for the next burn.
- **`RawDaoWriter.*`** — the state machine: preflight (`TEST UNIT READY`, `INQUIRY`, `GET CONFIGURATION`,
  `READ DISC INFORMATION`), `SET CD SPEED`, `MODE SELECT`, `SEND CUE SHEET`, `WRITE(10)` from LBA −150 in
  ≤64 KiB chunks (27 sectors), `SYNCHRONIZE CACHE` for the lead-out. It owns cancellation, retries and the
  `BurnResult` (sector counts, buffer events, minimum fill, recommended speed). Progress callbacks return
  `false` to cancel — and cancelling a real DAO burn is fatal to the disc, which the UI states plainly.
- **`SptiDevice.*`** — the Windows implementation over `DeviceIoControl(IOCTL_SCSI_PASS_THROUGH_DIRECT)`, opened
  on `\\.\X:`; `maxTransferBytes()` reflects the adapter limit that sizes the chunks.

### 3. `src/calib` — making the abstract visible

`CalibrationSectorSource` is another `SectorSource`, so a ladder or alternating pattern travels the exact same
path as an image: same composer, same writer, same buffer policy. Two modes:

- **Ladder** — 16 candidate bytes, each burned as a band. Pick the lightest and darkest.
- **Alternate** — light/dark blocks whose spiral arms reveal the drive's real sector→angle mapping, which is how
  `angleOffsetDeg` and `mirror` are found.

The lead-in is written by the drive and cannot be influenced, so the region is placed either at the programme
start (cheap) or at the end of a full-length burn (the true outer edge).

### 4. `src/gui` — UI logic without Win32

- **`App.*`** holds all Dear ImGui drawing and state. It has no D3D or HWND code beyond an opaque `ownerHwnd`,
  so `test_gui_logic` can drive it headlessly.
- **`BurnJob.*`** runs one burn on a worker thread and publishes atomics (progress, buffer state, fill) that the
  UI polls. No locks, no callbacks into the UI, `takeFinished()` returns true exactly once. A
  `DeviceFactory` creates the device, which is how the tests substitute `MockDevice`.
- **`main_win.cpp`** is the only file that creates a window, a D3D9 device and the ImGui backends. **`Dialogs_win.cpp`**
  is the native file picker; `Dialogs_stub.cpp` keeps the Linux test build linkable.
- **Safety rails live here** — `effectiveTestWrite()` keeps real burns locked until `testWritePassed` is set, and
  the confirm popup spells out that the disc is about to be destroyed.

## Testing strategy

| Suite | Covers |
|---|---|
| `test_burn_logic` | scrambler vectors, composer layouts, every CDB and response parser, buffer-monitor state machine, burn scenarios on a mock drive (healthy / starving / transfer-limited / too short) |
| `test_gui_logic` | `config.ini` round-trip and clamping, centre-crop, the job state machine, cancel, test-write lockout |
| `test_image_burn` | end-to-end: BMP → dither → sectors → `RawDaoWriter` → mock device, asserting sector count, write count and contiguity |

`MockDevice` models a drive that drains its buffer at a configurable rate, so buffer-underrun logic is exercised
deterministically instead of hoping a real drive misbehaves on cue.

## Deliberate non-goals

- **DVD/Blu-ray.** No standard raw DAO mode on most drives; the preflight rejects non-CD media.
- **Burning files.** This writes an image, not a file system. There is no ISO layer by design.
- **Rendering the picture as a label.** Every visible pixel is a pit-pattern decision made by the ditherer.
