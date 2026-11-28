#include "DiscPreview.h"
#include <algorithm>
#include <cmath>

namespace dis {

namespace {
const float kOuterMm = 60.0f, kHoleMm = 7.5f, kHubMm = 17.5f, kProgStartMm = 25.0f, kProgEndMm = 58.0f;
inline float clamp01(float v) { return v < 0.f ? 0.f : (v > 1.f ? 1.f : v); }
inline uint8_t toByte(float v) { return (uint8_t)(v < 0.f ? 0.f : (v > 255.f ? 255.f : v + 0.5f)); }
}

DiscPreview::DiscPreview() : sheen_((size_t)kSize * kSize) {
    const float c = kSize * 0.5f;
    for (int y = 0; y < kSize; ++y)
        for (int x = 0; x < kSize; ++x) {
            float ang = std::atan2(c - (y + 0.5f), (x + 0.5f) - c);
            float a = std::max(0.f, std::cos(2.f * (ang - 0.75f)));
            float b = std::max(0.f, std::cos(2.f * (ang + 2.20f)));
            float s = 0.75f * std::pow(a, 5.f) + 0.45f * std::pow(b, 5.f);
            sheen_[(size_t)y * kSize + x] = clamp01(s);
        }
}

float DiscPreview::toneCurve(float v, const PreviewParams& p) {
    v = (v - 0.5f) * p.contrast + 0.5f + p.brightness;
    v = clamp01(v);
    return p.gamma != 1.0f ? std::pow(v, p.gamma) : v;
}

void DiscPreview::render(const GrayRaster* img, const PreviewParams& p, uint32_t* out) const {
    const int N = kSize;
    const float c = N * 0.5f, R = c - 2.f, mmPerPx = kOuterMm / R;
    const float ang = (float)(p.angleDeg * 3.14159265358979 / 180.0);
    const float ca = std::cos(ang), sa = std::sin(ang);
    const float imgR = (float)std::min(p.imageRadiusMm, (double)kProgEndMm);

    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            const float dx = ((x + 0.5f) - c) * mmPerPx, dy = (c - (y + 0.5f)) * mmPerPx;
            const float r = std::sqrt(dx * dx + dy * dy);
            const float alpha = clamp01((kOuterMm - r) / mmPerPx + 0.5f) * clamp01((r - kHoleMm) / mmPerPx + 0.5f);
            uint32_t* o = &out[(size_t)y * N + x];
            if (alpha <= 0.f) { *o = 0; continue; }

            float cr, cg, cb;
            float shine = 0.80f + 0.40f * sheen_[(size_t)y * N + x];
            if (r < kHubMm) {                                    // clear hub ring
                cr = 70; cg = 78; cb = 96; shine = 0.9f + 0.2f * sheen_[(size_t)y * N + x];
            } else if (r >= kProgStartMm && r <= kProgEndMm && img && r <= imgR) {   // burned picture
                float px = dx * ca + dy * sa, py = -dx * sa + dy * ca;
                if (p.mirror) px = -px;
                float t = toneCurve(img->sampleDisc(px / imgR, py / imgR), p);
                cr = 28 + t * (222 - 28); cg = 34 + t * (228 - 34); cb = 46 + t * (238 - 46);
            } else if (r >= kProgStartMm && r <= kProgEndMm) {   // burnable area, no image yet
                cr = 128; cg = 140; cb = 156;
            } else {                                              // unwritten silver
                cr = 152; cg = 162; cb = 174;
            }
            *o = ((uint32_t)toByte(alpha * 255.f) << 24) | ((uint32_t)toByte(cr * shine) << 16) |
                 ((uint32_t)toByte(cg * shine) << 8) | (uint32_t)toByte(cb * shine);
        }
    }
}

} // namespace dis
