// DiscPreview.h - software renderer for the "live disc" picture (no ImGui / Win32 here, so it
// is unit-testable). Produces a 384x384 straight-alpha ARGB image of a CD-R:
//   * 60 mm outer radius, 7.5 mm centre hole (transparent), clear hub ring up to 17.5 mm
//   * the real burnable ring is only 25..58 mm - the picture is cropped to that annulus,
//     exactly as it will be burned, with the inner part left blank
//   * the user's brightness / contrast / gamma / rotation / mirror are applied live
//   * a conic "CD sheen" is baked in for a realistic look
#pragma once
#include "../core/ImageSource.h"
#include <cstdint>
#include <vector>

namespace dis {

struct PreviewParams {
    float  brightness = 0.0f, contrast = 1.0f, gamma = 1.0f;
    double angleDeg = 0.0;
    bool   mirror = false;
    double imageRadiusMm = 58.0;
};

class DiscPreview {
public:
    static const int kSize = 384;
    DiscPreview();
    // img may be null (blank disc). out must hold kSize*kSize pixels (0xAARRGGBB).
    void render(const GrayRaster* img, const PreviewParams& p, uint32_t* out) const;
    static float toneCurve(float v, const PreviewParams& p);
private:
    std::vector<float> sheen_;
};

} // namespace dis
