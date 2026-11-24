// PolarDitherEngine.h - streaming error-diffusion dithering along the disc spiral.
//
// The disc is divided into "cells" of cellBytes raw bytes each (sector-aligned).
// Cell n sits at a known (radius, angle) on the spiral. For every cell we sample the
// image at that position, quantise to `levels` tones and push the error to the cell's
// neighbours IN THE DISC'S OWN GEOMETRY: the next cell on the track, and the cells at
// the same angle on the next revolution(s). Memory is a ring buffer covering ~2
// revolutions (< 1 MB) regardless of image or disc size.
#pragma once
#include "DiscGeometry.h"
#include "ImageSource.h"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dis {

enum class Kernel { FloydSteinberg, JarvisJudiceNinke };

struct DitherParams {
    int    cellBytes      = 196;   // must divide 2352 (12 cells per sector)
    int    levels         = 2;     // 2..8 tones (0 = darkest, levels-1 = lightest)
    Kernel kernel         = Kernel::FloydSteinberg;
    double imageRadiusMm  = 58.0;  // image circle radius on the disc
    float  brightness     = 0.0f;  // -1..1
    float  contrast       = 1.0f;
    float  gamma          = 1.0f;
    double angleOffsetDeg = 0.0;   // calibration: rotate whole image
    bool   mirror         = false; // calibration: flip if viewed from the wrong side
};

class PolarDitherEngine {
public:
    PolarDitherEngine(const DiscGeometry& g, const GrayRaster& img, const DitherParams& p);

    uint64_t totalCells() const { return total_; }
    uint64_t cursor()     const { return cursor_; }
    bool     done()       const { return cursor_ >= total_; }
    void     reset();

    // Produce up to maxCells tone values (one byte each, 0..levels-1). Returns count.
    // Call repeatedly with a small buffer (e.g. 64K cells) and feed the burner.
    size_t produce(uint8_t* out, size_t maxCells);

    // Geometry helpers (also used by calibration and the preview tool).
    double cellLengthMm() const { return cellLenMm_; }
    uint64_t sectorOfCell(uint64_t n) const { return n * p_.cellBytes / DiscGeometry::kSectorBytes; }
    void positionMm(double sMm, double& xMm, double& yMm) const;
    const DitherParams& params() const { return p_; }

private:
    struct Tap { int dRev; int dCol; float w; };

    uint8_t step();
    void buildKernel();

    DiscGeometry g_;
    const GrayRaster& img_;
    DitherParams p_;
    double cellLenMm_ = 0, angleOffsetRad_ = 0;
    uint64_t total_ = 0, cursor_ = 0;
    std::vector<Tap> taps_;
    std::vector<float> err_;     // ring buffer
    size_t mask_ = 0;
};

} // namespace dis
