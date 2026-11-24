// dither_preview - runs the engine headless and renders what the disc would look like
// (each cell painted along its real spiral segment, averaged per pixel) to a PGM file.
// Usage: dither_preview in.bmp out.pgm [cellBytes=196] [levels=2] [fs|jjn]
#include "../src/core/PolarDitherEngine.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace dis;

int main(int argc, char** argv) {
    if (argc < 3) { std::printf("usage: %s in.bmp out.pgm [cellBytes] [levels] [fs|jjn]\n", argv[0]); return 1; }
    DitherParams p;
    if (argc > 3) p.cellBytes = std::atoi(argv[3]);
    if (argc > 4) p.levels = std::atoi(argv[4]);
    if (argc > 5 && !std::strcmp(argv[5], "jjn")) p.kernel = Kernel::JarvisJudiceNinke;

    GrayRaster img; std::string err;
    if (!loadBmpAsGrayRaster(argv[1], 1024, img, err)) { std::printf("load error: %s\n", err.c_str()); return 2; }

    DiscGeometry g;
    PolarDitherEngine eng(g, img, p);

    const int P = 1024;
    const double pxMm = 2.0 * g.programEndMm / P;
    std::vector<float> acc((size_t)P * P, 0.f), cnt((size_t)P * P, 0.f);
    const int sub = std::max(1, (int)std::ceil(eng.cellLengthMm() / pxMm));

    std::vector<uint8_t> buf(65536);
    uint64_t n = 0; size_t got;
    while ((got = eng.produce(buf.data(), buf.size())) > 0) {
        for (size_t i = 0; i < got; ++i, ++n) {
            float gray = buf[i] / (float)(eng.params().levels - 1);
            for (int k = 0; k < sub; ++k) {
                double s = ((double)n + (k + 0.5) / sub) * eng.cellLengthMm();
                double x, y; eng.positionMm(s, x, y);
                int ix = (int)((x + g.programEndMm) / pxMm), iy = (int)((g.programEndMm - y) / pxMm);
                if (ix < 0 || iy < 0 || ix >= P || iy >= P) continue;
                acc[(size_t)iy * P + ix] += gray; cnt[(size_t)iy * P + ix] += 1.f;
            }
        }
    }
    FILE* f = std::fopen(argv[2], "wb");
    std::fprintf(f, "P5\n%d %d\n255\n", P, P);
    for (size_t i = 0; i < acc.size(); ++i) {
        unsigned char c = cnt[i] > 0 ? (unsigned char)(acc[i] / cnt[i] * 255.f + 0.5f) : 255;
        std::fputc(c, f);
    }
    std::fclose(f);
    std::printf("cells=%llu sectors=%llu cellLen=%.3fmm\n", (unsigned long long)eng.totalCells(),
                (unsigned long long)eng.sectorOfCell(eng.totalCells()), eng.cellLengthMm());
    return 0;
}
