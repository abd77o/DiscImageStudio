# Hardware notes and known limits

Part of [DiscImageStudio](../README.md). If you test on real hardware, please send a
[hardware report](../.github/ISSUE_TEMPLATE/hardware_report.yml) — that data is the main thing this project lacks.

**Status: experimental.** The automated test suite exercises the whole burn path against a *mock* drive; real burns
are only documented anecdotally (the disc photos in the README were burned by the author, [@abd77o](https://github.com/abd77o)).
There is no systematic drive x media x speed matrix yet — that is what the hardware reports are for.
Use a CD-RW or a spare CD-R first, with *Test write* ticked.

* **CD only.** DVD has no standard raw-write mode on most burners. The preflight rejects non-CD media.
* **Audio DAO layout (default).** Audio tracks are not scrambled and carry no EDC/ECC, so all 2352 bytes of a
  sector go straight into the drive's CIRC/EFM encoder. A disc burned this way contains noise-like audio -
  do not play it on speakers. A Mode-1 layout (with the ECMA-130 scrambler) exists in code but its CUE-sheet
  data-form constants (`kFormDataRaw`, `kFormLeadData` in `CueSheet.h`) must be verified against MMC first.
* **Buffer strategy.** The drive's write speed is fixed for the session; MMC has no standard mid-write speed change.
  `SafeBufferMonitor` polls READ BUFFER CAPACITY, sends larger back-to-back batches when the buffer is low,
  and recommends a lower speed for the next burn. `experimentalSpeedDrop` (off by default) tries SET CD SPEED mid-write.
* **Resolution.** Cells are ~1.3 mm along the track and 1.6 um across it; CIRC smears data over roughly one sector.
  Expect smooth radial gradation and limited tangential detail. Contrast is moderate and angle-dependent.
* **Calibration.** The lead-in is written by the drive and cannot be controlled. The ladder burns 16 candidate
  bytes as concentric rings near the disc centre; pick the lightest/darkest and save them to `config.ini`.
* **Test write.** Many newer drives no longer support it, and a few ignore the bit. If yours rejects it, verify on
  a CD-RW, then set `test_write_passed=1` in `config.ini`.
* **Cancelling a real burn ruins the disc** (Disc-At-Once cannot be resumed).
* **Administrator rights** are required for SPTI writes (the exe embeds a `requireAdministrator` manifest).

## Reporting your results

A useful report contains the drive model and firmware, the media (brand, type, speed rating), the sector layout,
the write speed, the exact SCSI error if there was one, the calibration pair you saved in `config.ini`, and — most
valuable of all — a photograph of the burned disc taken in daylight at a shallow angle.

Use the *Hardware report* issue template so nothing gets forgotten.
