# Pull request

## What this changes

<!-- One or two sentences. Link the issue it closes: "Closes #12". -->

## Why

<!-- The problem, the hardware quirk, or the measurement behind it. -->

## Verification

- [ ] `test_burn_logic` passes
- [ ] `test_gui_logic` passes
- [ ] `test_image_burn <image.bmp>` passes
- [ ] New or changed behaviour is covered by a test that runs without hardware
- [ ] Tested on real hardware (say what: drive, firmware, media, speed) — **or** explicitly "mock drive only"

## Checklist

- [ ] No `#ifdef _WIN32` added above `IScsiDevice`
- [ ] No new mandatory runtime dependency
- [ ] Comments explain the *why* (protocol constant, ECMA/MMC section) where the code is non-obvious
- [ ] `CHANGELOG.md` updated under `[Unreleased]` for user-visible changes
- [ ] If a burn-path behaviour changed: the safety rails (test-write lockout, cancel warning) still hold
