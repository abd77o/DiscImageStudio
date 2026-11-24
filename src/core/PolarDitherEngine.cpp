#include "PolarDitherEngine.h"
#include <algorithm>
#include <cmath>

namespace dis {

static size_t nextPow2(size_t v) { size_t p = 1; while (p < v) p <<= 1; return p; }

PolarDitherEngine::PolarDitherEngine(const DiscGeometry& g, const GrayRaster& img,
                                     const DitherParams& p)
    : g_(g), img_(img), p_(p) {
    if (p_.cellBytes <= 0 || DiscGeometry::kSectorBytes % p_.cellBytes != 0) p_.cellBytes = 196;
    p_.levels = std::max(2, std::min(8, p_.levels));
    cellLenMm_ = p_.cellBytes * g_.mmPerByte();
    double rEnd = std::min(p_.imageRadiusMm, g_.programEndMm);
    total_ = (uint64_t)(g_.lengthAtRadius(rEnd) / cellLenMm_);
    angleOffsetRad_ = p_.angleOffsetDeg * kPi / 180.0;
    buildKernel();
    size_t nMax = (size_t)std::ceil(2.0 * kPi * g_.programEndMm / cellLenMm_) + 2;
    err_.assign(nextPow2(2 * nMax + 8), 0.0f);   // covers 2 revolutions + margin
    mask_ = err_.size() - 1;
}

void PolarDitherEngine::buildKernel() {
    taps_.clear();
    if (p_.kernel == Kernel::FloydSteinberg) {
        Tap t[] = { {0, 1, 7/16.f}, {1, -1, 3/16.f}, {1, 0, 5/16.f}, {1, 1, 1/16.f} };
        taps_.assign(t, t + 4);
    } else { // Jarvis-Judice-Ninke, /48
        const float d = 48.f;
        Tap t[] = { {0, 1, 7/d}, {0, 2, 5/d},
                    {1,-2, 3/d}, {1,-1, 5/d}, {1,0, 7/d}, {1,1, 5/d}, {1,2, 3/d},
                    {2,-2, 1/d}, {2,-1, 3/d}, {2,0, 5/d}, {2,1, 3/d}, {2,2, 1/d} };
        taps_.assign(t, t + 12);
    }
}

void PolarDitherEngine::reset() {
    cursor_ = 0;
    std::fill(err_.begin(), err_.end(), 0.0f);
}

void PolarDitherEngine::positionMm(double sMm, double& xMm, double& yMm) const {
    double r = g_.radiusAtLength(sMm);
    double th = std::fmod(g_.angleAtRadius(r), 2.0 * kPi) + angleOffsetRad_;
    xMm = r * std::cos(th);
    yMm = r * std::sin(th);
    if (p_.mirror) xMm = -xMm;
}

uint8_t PolarDitherEngine::step() {
    const uint64_t n = cursor_++;
    const double s = ((double)n + 0.5) * cellLenMm_;
    double xMm, yMm;
    positionMm(s, xMm, yMm);
    const double r = g_.radiusAtLength(s);

    // 1) sample + tone curve
    float v = img_.sampleDisc((float)(xMm / p_.imageRadiusMm), (float)(yMm / p_.imageRadiusMm));
    v = (v - 0.5f) * p_.contrast + 0.5f + p_.brightness;
    v = std::min(1.0f, std::max(0.0f, v));
    if (p_.gamma != 1.0f) v = std::pow(v, p_.gamma);

    // 2) add diffused error, quantise
    float& slot = err_[(size_t)n & mask_];
    float t = v + slot;
    slot = 0.0f;                                   // slot is reused by cell n + capacity
    const int L = p_.levels;
    int q = (int)std::floor(t * (L - 1) + 0.5f);
    q = std::max(0, std::min(L - 1, q));
    float e = t - (float)q / (float)(L - 1);
    e = std::max(-1.0f, std::min(1.0f, e));        // anti-runaway

    // 3) diffuse along the spiral: same track (dRev=0) / next revolutions at the same angle
    const int N = (int)(2.0 * kPi * r / cellLenMm_ + 0.5);   // cells per revolution here
    for (size_t i = 0; i < taps_.size(); ++i) {
        int64_t idx = (int64_t)n + (int64_t)taps_[i].dRev * N + taps_[i].dCol;
        err_[(size_t)idx & mask_] += e * taps_[i].w;
    }
    return (uint8_t)q;
}

size_t PolarDitherEngine::produce(uint8_t* out, size_t maxCells) {
    size_t k = 0;
    while (k < maxCells && cursor_ < total_) out[k++] = step();
    return k;
}

} // namespace dis
