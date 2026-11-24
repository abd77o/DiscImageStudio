# Changelog

All notable changes to this project are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html). While the major version is `0`, anything may change.

## [Unreleased]

### Planned
- Real-hardware validation matrix and a bug-report template for burn results
- Automatic calibration (read the ladder back instead of choosing bands by eye)
- Radial/tangential pre-compensation for CIRC smearing

## [0.1.0] — 2026-11-24

First public release. Experimental: the burn path is validated against a mock drive only.

### Added
- Polar dithering engine: image sampled along the CD spiral, error diffusion in disc geometry
  (next cell on the track and the same angle on following revolutions), Floyd–Steinberg and
  Jarvis–Judice–Ninke kernels, 2–8 tones, constant memory via a ring buffer.
- Streaming image loader: BMP natively, PNG/JPEG through `stb_image`, box-filtered into a
  square 8-bit raster of at most 1024 × 1024.
- Sector composer for the Audio DAO layout (unscrambled, no EDC/ECC) and a Mode-1 layout with an
  ECMA-130 scrambler.
- MMC/SPTI burner: `SET CD SPEED`, `MODE SELECT` write parameters, `SEND CUE SHEET`, `WRITE(10)` from
  LBA −150, `SYNCHRONIZE CACHE`, buffer-underrun-free recording, optional test write.
- `SafeBufferMonitor`: `READ BUFFER CAPACITY` polling, adaptive batch sizing, next-burn speed advice.
- Calibration sources: candidate-byte ladder and alternating light/dark bands, plus `config.ini` persistence.
- Dear ImGui + Direct3D 9 GUI for Windows 7, statically linked, elevation manifest, single worker thread for
  burning, test-write lockout before the first real burn.
- Hardware-free test suite: CDB builders, scrambler vectors, composer layout, buffer monitor, burn scenarios,
  GUI job flow, and an end-to-end image burn against a mock device.
- `tools/dither_preview`: headless renderer that paints each cell along its real spiral segment.
- GitHub Actions CI (Linux tests on GCC and Clang, Windows GUI build and artifact) and dependency fetchers.

[Unreleased]: https://github.com/abd77o/DiscImageStudio/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/abd77o/DiscImageStudio/releases/tag/v0.1.0
