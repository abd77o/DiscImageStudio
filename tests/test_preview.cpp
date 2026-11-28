#include "../src/gui/DiscPreview.h"
#include <cstdio>
#include <cstdlib>
using namespace dis;
static int g_fail = 0;
#define CHECK(c) do { if (!(c)) { std::printf("  FAIL line %d: %s\n", __LINE__, #c); ++g_fail; } } while (0)

static uint32_t at(const std::vector<uint32_t>& b, double xMm, double yMm) {
    const float c = DiscPreview::kSize * 0.5f, R = c - 2.f, ppm = R / 60.f;
    int x = (int)(c + xMm * ppm), y = (int)(c - yMm * ppm);
    return b[(size_t)y * DiscPreview::kSize + x];
}
static int lum(uint32_t p) { return (int)(((p >> 16) & 255) * 30 + ((p >> 8) & 255) * 59 + (p & 255) * 11) / 100; }
static int alpha(uint32_t p) { return (int)(p >> 24); }

int main() {
    DiscPreview prev; std::vector<uint32_t> out((size_t)DiscPreview::kSize * DiscPreview::kSize);
    GrayRaster img; img.size = 64; img.px.assign(64 * 64, 0);          // left half black, right half white
    for (int y = 0; y < 64; ++y) for (int x = 32; x < 64; ++x) img.px[y * 64 + x] = 255;
    PreviewParams p;
    prev.render(&img, p, out.data());

    CHECK(alpha(at(out, 0, 0)) == 0);                                   // centre hole is transparent
    CHECK(alpha(at(out, 59.5, 0)) > 0 && alpha(at(out, 0, 59.5)) > 0);  // rim present
    CHECK(out[0] == 0 && out.back() == 0);                              // corners outside the disc
    CHECK(alpha(at(out, 40, 0)) == 255);
    int lWhite = lum(at(out, 40, 0)), lBlack = lum(at(out, -40, 0));
    std::printf("  white side lum=%d, black side lum=%d\n", lWhite, lBlack);
    CHECK(lWhite > 150 && lBlack < 90);
    int l1 = lum(at(out, 20, 0)), l2 = lum(at(out, -20, 0));
    std::printf("  inner blank ring lum=%d / %d (picture not burned here)\n", l1, l2);
    CHECK(abs(l1 - l2) < 60);
    p.mirror = true; prev.render(&img, p, out.data());
    CHECK(lum(at(out, 40, 0)) < 90 && lum(at(out, -40, 0)) > 150);
    p.mirror = false; p.angleDeg = 180; prev.render(&img, p, out.data());
    CHECK(lum(at(out, 40, 0)) < 90 && lum(at(out, -40, 0)) > 150);
    p.angleDeg = 0; p.brightness = 0.5f;
    CHECK(DiscPreview::toneCurve(0.2f, p) > DiscPreview::toneCurve(0.2f, PreviewParams()));
    PreviewParams flat; flat.contrast = 0.f;
    CHECK(DiscPreview::toneCurve(0.1f, flat) == DiscPreview::toneCurve(0.9f, flat));
    prev.render(0, PreviewParams(), out.data()); CHECK(alpha(at(out, 40, 0)) == 255);
    PreviewParams small; small.imageRadiusMm = 40; prev.render(&img, small, out.data());
    CHECK(lum(at(out, 50, 0)) < 190 && lum(at(out, 50, 0)) > 100);       // beyond picture radius: flat silver

    std::printf(g_fail ? "\n%d FAILURES\n" : "\nALL TESTS PASSED\n", g_fail);
    return g_fail ? 1 : 0;
}
