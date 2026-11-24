#include "SectorSource.h"
#include <algorithm>

namespace dis {

ImageSectorSource::ImageSectorSource(PolarDitherEngine& engine, const SectorComposer& composer)
    : eng_(engine), comp_(composer), tones_(composer.cellsPerSector()), lba_(0) {
    uint64_t cps = (uint64_t)comp_.cellsPerSector();
    uint64_t sectors = (eng_.totalCells() + cps - 1) / cps;
    total_ = (uint32_t)std::max<uint64_t>(300, sectors);
}

bool ImageSectorSource::next(uint8_t* out) {
    if (lba_ >= total_) return false;
    size_t got = eng_.produce(tones_.data(), tones_.size());
    // past the end of the image circle: lightest tone (neutral background)
    uint8_t light = (uint8_t)(comp_.config().levels - 1);
    for (size_t i = got; i < tones_.size(); ++i) tones_[i] = light;
    comp_.compose((int32_t)lba_, tones_.data(), out);
    ++lba_;
    return true;
}

} // namespace dis
