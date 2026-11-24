#include "CalibrationSource.h"
#include <algorithm>

namespace dis {

std::vector<uint8_t> CalibrationConfig::defaultLadder() {
    const uint8_t v[16] = { 0x00, 0xFF, 0x55, 0xAA, 0x0F, 0xF0, 0x33, 0xCC,
                            0x11, 0xEE, 0x24, 0x49, 0x92, 0x01, 0x80, 0x7F };
    return std::vector<uint8_t>(v, v + 16);
}

CalibrationConfig CalibrationConfig::atProgramStart(uint32_t region, uint32_t total) {
    CalibrationConfig c;
    c.totalSectors = std::max<uint32_t>(300, total);
    c.regionLength = std::min(region, c.totalSectors);
    c.regionStart  = 0;
    return c;
}

CalibrationConfig CalibrationConfig::atProgramEnd(uint32_t region, uint32_t total) {
    CalibrationConfig c;
    c.totalSectors = std::max<uint32_t>(300, total);
    c.regionLength = std::min(region, c.totalSectors);
    c.regionStart  = c.totalSectors - c.regionLength;
    return c;
}

CalibrationSectorSource::CalibrationSectorSource(const CalibrationConfig& cfg, const SectorComposer& composer)
    : cfg_(cfg), comp_(composer), lba_(0) {
    if (cfg_.blockSectors == 0) cfg_.blockSectors = 1;
    if (cfg_.ladder.empty()) cfg_.ladder = CalibrationConfig::defaultLadder();
}

bool CalibrationSectorSource::next(uint8_t* out) {
    if (lba_ >= cfg_.totalSectors) return false;
    uint8_t pattern = cfg_.lightByte;
    if (lba_ >= cfg_.regionStart && lba_ < cfg_.regionStart + cfg_.regionLength) {
        const uint32_t block = (lba_ - cfg_.regionStart) / cfg_.blockSectors;
        if (cfg_.mode == CalibrationConfig::kAlternate) pattern = (block % 2 == 0) ? cfg_.lightByte : cfg_.darkByte;
        else pattern = cfg_.ladder[block % cfg_.ladder.size()];
    }
    comp_.composeUniform((int32_t)lba_, pattern, out);
    ++lba_;
    return true;
}

void CalibrationSectorSource::regionRadiusMm(const DiscGeometry& g, const CalibrationConfig& c,
                                             double& r0, double& r1) {
    r0 = g.radiusAtLength((double)c.regionStart * g.sectorLengthMm);
    r1 = g.radiusAtLength((double)(c.regionStart + c.regionLength) * g.sectorLengthMm);
}

} // namespace dis
