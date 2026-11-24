// End-to-end: BMP -> polar dither engine -> sectors -> RawDaoWriter -> mock drive.
#include "../src/burn/RawDaoWriter.h"
#include "MockDevice.h"
#include <chrono>
#include <cstdio>
using namespace dis;

int main(int argc, char** argv) {
    if (argc < 2) { std::printf("usage: %s image.bmp\n", argv[0]); return 1; }
    GrayRaster img; std::string err;
    if (!loadBmpAsGrayRaster(argv[1], 1024, img, err)) { std::printf("load: %s\n", err.c_str()); return 2; }

    DitherParams dp; dp.levels = 2;
    PolarDitherEngine eng(DiscGeometry(), img, dp);
    ComposerConfig cc; cc.layout = kLayoutAudio; cc.levels = dp.levels; cc.cellBytes = dp.cellBytes;
    SectorComposer comp(cc);
    ImageSectorSource src(eng, comp);

    MockDevice dev; dev.drainPerWrite = 0.6 * 27 * 2352;
    RawDaoWriter w(dev, comp);
    BurnOptions o; o.speedX = 8;
    uint32_t lastPct = 0;
    auto t0 = std::chrono::steady_clock::now();
    BurnResult r = w.burn(src, o, [&](const BurnProgress& p) {
        uint32_t pct = p.sectorsDone * 100 / p.sectorsTotal;
        if (pct >= lastPct + 25) { std::printf("  %3u%%  fill=%.2f\n", pct, p.bufferFill); lastPct = pct; }
        return true; });
    double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::printf("ok=%d err='%s'\nsectors=%llu (expected %u + 150)  writes=%llu  contiguous=%d  host time=%.2fs\n",
                r.ok, r.error.c_str(), (unsigned long long)dev.sectors, src.totalSectors(),
                (unsigned long long)dev.writes, dev.contiguous, sec);
    std::printf("low=%u crit=%u  host-side rate = %.0f sectors/s (1x needs 75, 52x needs 3900)\n",
                r.lowEvents, r.criticalEvents, dev.sectors / sec);
    return r.ok && dev.contiguous ? 0 : 3;
}
