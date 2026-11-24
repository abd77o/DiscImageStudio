// Platform-independent tests of the burn pipeline (no hardware needed).
#include "../src/burn/CueSheet.h"
#include "../src/burn/RawDaoWriter.h"
#include "../src/burn/Scrambler.h"
#include "../src/burn/SectorComposer.h"
#include "../src/calib/CalibrationSource.h"
#include "MockDevice.h"
#include <cstdio>
#include <cstring>
using namespace dis;

static int g_fail = 0;
#define CHECK(c) do { if (!(c)) { std::printf("  FAIL line %d: %s\n", __LINE__, #c); ++g_fail; } } while (0)

static void testScrambler() {
    std::printf("[scrambler]\n");
    const uint8_t* t = Scrambler::table();
    std::printf("  first bytes: "); for (int i = 0; i < 8; ++i) std::printf("%02X ", t[i]); std::printf("\n");
    const uint8_t known[8] = { 0x01, 0x80, 0x00, 0x60, 0x00, 0x28, 0x00, 0x1E };
    CHECK(std::memcmp(t, known, 8) == 0);
    uint8_t s[2352]; for (int i = 0; i < 2352; ++i) s[i] = (uint8_t)(i * 7);
    uint8_t o[2352]; std::memcpy(o, s, 2352);
    Scrambler::apply(s);
    CHECK(std::memcmp(s, o, 12) == 0);          // sync untouched
    CHECK(std::memcmp(s, o, 2352) != 0);
    Scrambler::apply(s);
    CHECK(std::memcmp(s, o, 2352) == 0);        // involution
}

static void testComposer() {
    std::printf("[composer]\n");
    ComposerConfig a; a.layout = kLayoutDataMode1; a.driveScrambles = false;
    ComposerConfig b = a; b.driveScrambles = true;
    SectorComposer ca(a), cb(b);
    uint8_t tones[12]; for (int i = 0; i < 12; ++i) tones[i] = (uint8_t)(i & 1);
    uint8_t x[2352], y[2352];
    ca.compose(0, tones, x);
    cb.compose(0, tones, y);
    CHECK(x[0] == 0x00 && x[1] == 0xFF && x[10] == 0xFF && x[11] == 0x00);       // sync
    uint8_t h[4]; for (int k = 0; k < 4; ++k) h[k] = x[12 + k] ^ Scrambler::table()[k];
    CHECK(h[0] == 0x00 && h[1] == 0x02 && h[2] == 0x00 && h[3] == 0x01);          // 00:02:00 mode 1
    // what the drive would put on disc = scramble(host bytes) must equal the "raw" layout
    uint8_t z[2352]; std::memcpy(z, y, 2352); Scrambler::apply(z);
    CHECK(std::memcmp(z, x, 2352) == 0);
    // body of the raw layout is exactly the tone patterns
    CHECK(x[100] == a.tonePattern[0] && x[16 + 196] == a.tonePattern[1] && x[2351] == a.tonePattern[1]);
    // pre-gap LBA -150 -> 00:00:00
    ca.composeUniform(-150, 0xEE, x);
    CHECK((x[12] ^ Scrambler::table()[0]) == 0 && (x[13] ^ Scrambler::table()[1]) == 0);
    // audio layout: pure pattern, driveScrambles forced off
    ComposerConfig au; au.layout = kLayoutAudio; au.driveScrambles = true;
    SectorComposer cau(au);
    cau.compose(5, tones, x);
    CHECK(x[0] == au.tonePattern[0] && x[2351] == au.tonePattern[1]);
}

static void testCommands() {
    std::printf("[commands]\n");
    mmc::Cdb w = mmc::write10(-150, 27);
    CHECK(w.len == 10 && w.b[0] == 0x2A && w.b[2] == 0xFF && w.b[3] == 0xFF && w.b[4] == 0xFF && w.b[5] == 0x6A);
    CHECK(w.b[7] == 0 && w.b[8] == 27);
    std::vector<uint8_t> cs = buildDaoCueSheet(false, 1000);
    CHECK(cs.size() == 32);
    CHECK(cs[0] == 0x01 && cs[3] == 0x01);                       // lead-in, audio lead form
    CHECK(cs[8 + 1] == 1 && cs[8 + 2] == 0 && cs[8 + 5] == 0);    // track 1 index 0 at 00:00:00
    CHECK(cs[16 + 2] == 1 && cs[16 + 6] == 2);                    // index 1 at 00:02:00
    CHECK(cs[24 + 1] == 0xAA && cs[24 + 5] == 0 && cs[24 + 6] == 15 && cs[24 + 7] == 25);  // 1150 frames
    std::vector<uint8_t> mp = mmc::writeParametersList(mmc::WriteParams());
    CHECK(mp.size() == 60 && mp[8] == 0x05 && mp[9] == 0x32 && mp[10] == 0x42 && mp[11] == 0x00);
    mmc::Cdb bc = mmc::readBufferCapacity(); CHECK(bc.b[0] == 0x5C && bc.b[8] == 12);
    mmc::Cdb ss = mmc::setCdSpeed(0xFFFF, 706); CHECK(ss.len == 12 && ss.b[4] == 0x02 && ss.b[5] == 0xC2);
}

static void testMonitor2() {
    std::printf("[monitor]\n");
    SafeBufferMonitor m;
    BufferSample s; s.total = 1000;
    s.blank = 300; m.update(s, 1);                                   // 70% -> primed, healthy
    s.blank = 750; m.update(s, 2); CHECK(m.state() == kBufLow);       // 25% full
    s.blank = 950; m.update(s, 3); CHECK(m.state() == kBufCritical);  // 5% full
    s.blank = 600; m.update(s, 4); CHECK(m.state() == kBufLow);       // 40%: inside hysteresis band
    s.blank = 400; m.update(s, 5); CHECK(m.state() == kBufHealthy);   // 60%
    CHECK(m.lowEvents() == 1 && m.criticalEvents() == 1);
    CHECK(m.prefetchChunks() == 1);
    CHECK(SafeBufferMonitor::recommendSpeed(8, 0, 1) == 2);
    CHECK(SafeBufferMonitor::recommendSpeed(8, 5, 0) == 4);
    CHECK(SafeBufferMonitor::recommendSpeed(8, 0, 0) == 8);
}

static void testCalibration() {
    std::printf("[calibration]\n");
    ComposerConfig cc; cc.layout = kLayoutAudio;
    SectorComposer comp(cc);
    CalibrationConfig cfg = CalibrationConfig::atProgramStart(100, 1000);
    cfg.blockSectors = 10; cfg.lightByte = 0xEE; cfg.darkByte = 0x11;
    CalibrationSectorSource src(cfg, comp);
    uint8_t s[2352]; int light = 0, dark = 0, other = 0;
    for (uint32_t i = 0; i < 1000; ++i) {
        src.next(s);
        if (i < 100) { uint8_t expect = ((i / 10) % 2 == 0) ? 0xEE : 0x11; CHECK(s[0] == expect && s[2351] == expect); }
        if (s[0] == 0xEE) ++light; else if (s[0] == 0x11) ++dark; else ++other;
    }
    CHECK(!src.next(s) && other == 0 && dark == 50);
    DiscGeometry g; double r0, r1;
    CalibrationSectorSource::regionRadiusMm(g, CalibrationConfig::atProgramEnd(22500, 336000), r0, r1);
    std::printf("  edge region radius %.1f..%.1f mm\n", r0, r1);
    CHECK(r0 > 56.0 && r1 > 57.5 && r1 <= 58.2);
}

static dis::BurnResult burnCalib(MockDevice& dev, uint32_t total, int speed, uint32_t* lowOut = 0) {
    ComposerConfig cc; cc.layout = kLayoutAudio;
    SectorComposer comp(cc);
    CalibrationConfig cfg = CalibrationConfig::atProgramStart(total, total);
    CalibrationSectorSource src(cfg, comp);
    RawDaoWriter w(dev, comp);
    BurnOptions o; o.speedX = speed;
    BurnResult r = w.burn(src, o, 0);
    if (lowOut) *lowOut = r.lowEvents;
    return r;
}

static void testBurnHealthy() {
    std::printf("[burn: healthy drive]\n");
    MockDevice dev; dev.drainPerWrite = 0.5 * 27 * 2352;
    BurnResult r = burnCalib(dev, 5000, 8);
    std::printf("  ok=%d err='%s' writes=%llu sectors=%llu low=%u crit=%u minFill=%.2f recSpeed=%dx\n", r.ok, r.error.c_str(),
                (unsigned long long)dev.writes, (unsigned long long)dev.sectors, r.lowEvents, r.criticalEvents, r.minFill, r.recommendedSpeedX);
    CHECK(r.ok && dev.contiguous && !dev.writeBeforeCue && dev.firstLba == -150);
    CHECK(dev.sectors == 5150 && dev.syncCmds == 1 && dev.speedCmds == 1);
    CHECK(r.lowEvents == 0 && r.criticalEvents == 0 && r.recommendedSpeedX == 8);
    CHECK(dev.cueSheet.size() == 32 && dev.modeList.size() == 60);
}

static void testBurnStarved() {
    std::printf("[burn: starved drive]\n");
    MockDevice dev; dev.drainPerWrite = 1.3 * 27 * 2352;
    BurnResult r = burnCalib(dev, 5000, 16);
    std::printf("  ok=%d low=%u crit=%u minFill=%.3f recSpeed=%dx\n", r.ok, r.lowEvents, r.criticalEvents, r.minFill, r.recommendedSpeedX);
    CHECK(r.ok && r.criticalEvents >= 1 && r.recommendedSpeedX < 16 && dev.contiguous);
}

static void testBurnChunkClamp() {
    std::printf("[burn: transfer limit]\n");
    MockDevice dev; dev.maxTransfer = 32768; dev.drainPerWrite = 10000;
    BurnResult r = burnCalib(dev, 600, 4);
    CHECK(r.ok && dev.contiguous && dev.writes == (750 + 12) / 13);   // 13 sectors per 32 KiB
}

static void testBurnFailure() {
    std::printf("[burn: too short]\n");
    MockDevice dev;
    ComposerConfig cc; SectorComposer comp(cc);
    CalibrationConfig cfg = CalibrationConfig::atProgramStart(100, 300);
    cfg.totalSectors = 100;                       // below the 300-sector minimum
    CalibrationSectorSource src(cfg, comp);
    RawDaoWriter w(dev, comp);
    BurnResult r = w.burn(src, BurnOptions(), 0);
    CHECK(!r.ok && dev.writes == 0);
}

int main() {
    testScrambler(); testComposer(); testCommands();
    testMonitor2(); testCalibration();
    testBurnHealthy(); testBurnStarved(); testBurnChunkClamp(); testBurnFailure();
    std::printf(g_fail ? "\n%d FAILURES\n" : "\nALL TESTS PASSED\n", g_fail);
    return g_fail ? 1 : 0;
}
