// CalibrationSource.h - visible test patterns burned as an ordinary SectorSource.
//
//  kAlternate : blockSectors light, blockSectors dark, repeat. On the disc this shows up as
//               spiral arms / radial wedges; their count and slope reveal the drive's real
//               sector->angle mapping (use it to set angleOffsetDeg / mirror).
//  kLadder    : a list of candidate pattern bytes, one band of blockSectors each. Look at the
//               disc and note which bands look lightest and darkest - those become the
//               light/dark bytes in ComposerConfig. Run it once with driveScrambles=false and
//               once with true: the setting that gives clearly different bands is the right one.
//
// Placement: LBA 0 is at ~25 mm radius. The lead-in is written by the drive and cannot be
// controlled, so the test region is placed either at the programme start (cheap: a short
// disc) or at the end of a full-length burn (the real outer edge, but costs a whole disc).
#pragma once
#include "../burn/SectorSource.h"
#include "../core/DiscGeometry.h"
#include <vector>

namespace dis {

struct CalibrationConfig {
    enum Mode { kAlternate = 0, kLadder = 1 };
    Mode     mode          = kAlternate;
    uint32_t totalSectors  = 22500;     // 5 minutes
    uint32_t regionStart   = 0;
    uint32_t regionLength  = 22500;
    uint32_t blockSectors  = 10;
    uint8_t  lightByte     = 0xEE;      // outside the region and for "light" blocks
    uint8_t  darkByte      = 0x11;
    std::vector<uint8_t> ladder;        // used by kLadder

    static std::vector<uint8_t> defaultLadder();   // 16 distinct candidate bytes
    static CalibrationConfig atProgramStart(uint32_t regionSectors, uint32_t totalSectors);
    static CalibrationConfig atProgramEnd(uint32_t regionSectors, uint32_t totalSectors);
};

class CalibrationSectorSource : public SectorSource {
public:
    CalibrationSectorSource(const CalibrationConfig& cfg, const SectorComposer& composer);
    uint32_t totalSectors() const override { return cfg_.totalSectors; }
    bool next(uint8_t* out2352) override;
    void rewind() override { lba_ = 0; }

    // Radius range (mm) covered by the test region, for showing the user where to look.
    static void regionRadiusMm(const DiscGeometry& g, const CalibrationConfig& c, double& r0, double& r1);

private:
    CalibrationConfig cfg_;
    const SectorComposer& comp_;
    uint32_t lba_;
};

} // namespace dis
