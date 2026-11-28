# Hardware notes and known limits

**Status: experimental. The burn path has been tested only against a mock drive, never on real hardware.**
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
