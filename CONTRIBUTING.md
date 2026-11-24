# Contributing to DiscImageStudio

Thanks for taking a look. This project talks to real optical drives, so the most valuable thing you can send is
**information**: a hardware report, a disc photo, or a failing log line.

## Ways to contribute

| Kind of contribution | Start here |
|---|---|
| Hardware report (drive, firmware, media, speed, result) | Open a *Hardware report* issue — please include a photo if the burn produced something |
| Bug | Bug report template; include the exact error string from the status line and your `config.ini` |
| Feature / design discussion | Feature request template, or a draft PR for small changes |
| Documentation | Just open a PR; small, focused doc PRs are merged quickly |

## Ground rules

1. **Be careful with real hardware.** Never test a change to the burn path on something you are not prepared to
   destroy. Use CD-RW.
2. **Keep the core platform-neutral.** Everything above `IScsiDevice` (dithering, composition, MMC command
   building, burn orchestration) must build and be testable without Windows, a burner or administrator rights.
   If your change needs hardware to verify, add a mock-driven test that covers the logic anyway.
3. **Add a test with the fix.** The suite is fast and runs on Linux; a PR that breaks `test_burn_logic`,
   `test_gui_logic` or `test_image_burn` will not be merged.
4. **No new runtime dependencies.** The shipped binary is a single static `.exe` for Windows 7. Build-time
   vendoring through `scripts/fetch_deps.*` is the only accepted third-party path, and it must stay optional.
5. **Match the surrounding style.** C++11, 4-space indent, `dis::` namespace, comments that explain *why*
   (protocol constants, magic numbers, hardware quirks) rather than restating the code.
6. **Cite your protocol references.** If you touch `Mmc`, `CueSheet`, `Scrambler`, `SectorComposer` or
   `SafeBufferMonitor`, name the MMC/ECMA section in the commit message or PR description.

## Development setup

```bash
git clone https://github.com/abd77o/DiscImageStudio.git
cd DiscImageStudio
scripts/fetch_deps.sh                 # optional: stb_image enables PNG/JPEG input
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/test_burn_logic && ./build/test_gui_logic
./build/dither_preview artwork.bmp disc.pgm 196 2 fs     # eyeball your change
```

On Windows, use `scripts\fetch_deps.bat` and `build_vs2017.bat`.

## Commit messages

Conventional Commits style, please — it keeps the changelog honest:

```
fix(burn): keep WRITE(10) chunks under the 64 KiB adapter limit
feat(dither): add Atkinson kernel
docs(hardware): note that test write is rejected by some drives
```

## Pull requests

- One logical change per PR, with a short description of *what you saw* and *how you verified it*.
- Say explicitly whether you tested on hardware. "Mock drive only" is a perfectly acceptable answer.
- By submitting a PR you agree to license your contribution under the project's MIT license.

## Code of conduct

Participation is covered by [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).

## Security

Please do not open public issues for anything that could write to a device it should not touch — see
[SECURITY.md](SECURITY.md).
