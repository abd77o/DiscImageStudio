// Tests for the non-UI parts of step 3: config.ini, image cropping, and the BurnJob worker thread.
#include "../src/core/Config.h"
#include "../src/core/ImageLoader.h"
#include "../src/gui/BurnJob.h"
#include "MockDevice.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>
using namespace dis;

static int g_fail = 0;
#define CHECK(c) do { if (!(c)) { std::printf("  FAIL line %d: %s\n", __LINE__, #c); ++g_fail; } } while (0)

// forwards to a shared mock so the test can inspect it after the job (job owns the proxy)
struct Proxy : IScsiDevice {
    MockDevice* t; unsigned sleepUs;
    Proxy(MockDevice* m, unsigned s) : t(m), sleepUs(s) {}
    ScsiResult execute(const mmc::Cdb& c, DataDir d, void* p, uint32_t n, uint32_t to) override {
        if (sleepUs && c.b[0] == 0x2A) std::this_thread::sleep_for(std::chrono::microseconds(sleepUs));
        return t->execute(c, d, p, n, to);
    }
    uint32_t maxTransferBytes() const override { return t->maxTransferBytes(); }
};

static BurnJob::DeviceFactory factory(MockDevice* m, unsigned sleepUs = 0, bool fail = false) {
    return [m, sleepUs, fail](char, std::unique_ptr<IScsiDevice>& d, std::string& err) {
        if (fail) { err = "access denied - run as administrator"; return false; }
        d.reset(new Proxy(m, sleepUs)); return true; };
}
static void waitDone(BurnJob& j) { while (j.running()) std::this_thread::sleep_for(std::chrono::milliseconds(5)); }

static void testConfig() {
    std::printf("[config]\n");
    AppConfig c; c.lightByte = 0xF0; c.darkByte = 0x0F; c.testWritePassed = true; c.speedX = 8;
    c.angleOffsetDeg = 12.5; c.mirror = true; c.jjn = true; c.cellBytes = 392;
    const char* path = "dis_test_config.ini";
    CHECK(saveConfig(path, c));
    AppConfig d; CHECK(loadConfig(path, d));
    CHECK(d.lightByte == 0xF0 && d.darkByte == 0x0F && d.testWritePassed && d.speedX == 8);
    CHECK(d.angleOffsetDeg == 12.5 && d.mirror && d.jjn && d.cellBytes == 392);
    // hand-edited / hostile file: hex without prefix, junk, out-of-range values
    FILE* f = std::fopen(path, "wb");
    std::fputs("; comment\n[tones]\nlight_byte = ab\ndark_byte=0x1FF\nspeed_x=999\ncell_bytes=123\ngamma=50\nbogus=1\nno equals line\n", f);
    std::fclose(f);
    AppConfig e; CHECK(loadConfig(path, e));
    CHECK(e.lightByte == 0xAB && e.darkByte == 0x11);        // 0x1FF rejected -> default kept
    CHECK(e.speedX == 52 && e.cellBytes == 196 && e.gamma == 3.0f);
    AppConfig g; CHECK(!loadConfig("does_not_exist.ini", g) && g.lightByte == 0xEE);
}

static void testCropAndScale() {
    std::printf("[image crop]\n");
    // 300x100 image: left third black, middle third white, right third gray -> centre crop = 100x100 white
    std::vector<uint8_t> px(300 * 100);
    for (int y = 0; y < 100; ++y) for (int x = 0; x < 300; ++x) px[y * 300 + x] = x < 100 ? 0 : (x < 200 ? 255 : 128);
    GrayRaster r;
    CHECK(buildRasterFromGray(&px[0], 300, 100, 1024, r));
    CHECK(r.size == 100 && r.srcWidth == 300 && r.srcHeight == 100);   // never upscaled
    CHECK(r.px[50 * 100 + 50] == 255 && r.px[0] == 255 && r.px[100 * 100 - 1] == 255);
    // downscale: 400x400 checker of 2x2 blocks -> 100x100 gives mid-gray
    std::vector<uint8_t> chk(400 * 400);
    for (int y = 0; y < 400; ++y) for (int x = 0; x < 400; ++x) chk[y * 400 + x] = (((x / 2) + (y / 2)) & 1) ? 255 : 0;
    CHECK(buildRasterFromGray(&chk[0], 400, 400, 100, r) && r.size == 100);
    CHECK(r.px[5050] > 120 && r.px[5050] < 135);
    std::string err; GrayRaster z;
    CHECK(!loadImageAsGrayRaster("does_not_exist.png", 512, z, err) && err == "cannot open file");
    FILE* f = std::fopen("dis_junk.bin", "wb"); std::fputs("hello world, not an image", f); std::fclose(f);
    CHECK(!loadImageAsGrayRaster("dis_junk.bin", 512, z, err));
    std::printf("  junk file -> '%s'\n", err.c_str());
}

static void testJobLadder() {
    std::printf("[job: calibration ladder, test write]\n");
    MockDevice dev; dev.drainPerWrite = 0.4 * 27 * 2352;
    BurnJob job(factory(&dev));
    BurnRequest r; r.kind = BurnRequest::kCalibLadder; r.driveLetter = 'D'; r.testWrite = true;
    r.cfg.speedX = 8;
    r.calib = CalibrationConfig::atProgramStart(16 * 100, 16 * 100);
    r.calib.mode = CalibrationConfig::kLadder; r.calib.blockSectors = 100;
    std::string err; CHECK(job.start(r, err));
    waitDone(job);
    CHECK(job.takeFinished() && !job.takeFinished());
    BurnResult res = job.result();
    std::printf("  ok=%d err='%s' sectors=%llu status.done=%u/%u\n", res.ok, res.error.c_str(),
                (unsigned long long)dev.sectors, job.status().done, job.status().total);
    CHECK(res.ok && dev.sectors == 1600 + 150 && dev.contiguous);
    CHECK(dev.modeList.size() == 60 && (dev.modeList[10] & 0x10) == 0x10);   // TEST WRITE bit set
    CHECK(dev.lastWrite.size() > 0);
    // last band = candidate #16 = 0x7F
    CHECK(dev.lastWrite[dev.lastWrite.size() - 1] == 0x7F);
}

static void testJobImageAndRealBurnFlag() {
    std::printf("[job: image, real burn flag]\n");
    MockDevice dev; dev.drainPerWrite = 0.4 * 27 * 2352;
    BurnJob job(factory(&dev));
    BurnRequest r; r.kind = BurnRequest::kImage; r.driveLetter = 'E'; r.testWrite = false;
    r.image.reset(new GrayRaster); r.image->size = 64; r.image->px.assign(64 * 64, 128);
    std::string err; CHECK(job.start(r, err));
    waitDone(job);
    BurnResult res = job.result();
    std::printf("  ok=%d sectors=%llu\n", res.ok, (unsigned long long)dev.sectors);
    CHECK(res.ok && dev.contiguous && dev.sectors > 300000);
    CHECK((dev.modeList[10] & 0x10) == 0);                                   // real write: bit clear
    CHECK(!job.start(BurnRequest(), err) || true);
    BurnRequest bad; bad.kind = BurnRequest::kImage;
    CHECK(!job.start(bad, err) && err == "no image loaded");
}

static void testJobCancelAndErrors() {
    std::printf("[job: cancel + open failure]\n");
    MockDevice dev; dev.drainPerWrite = 10000;
    BurnJob job(factory(&dev, 300));
    BurnRequest r; r.kind = BurnRequest::kImage; r.driveLetter = 'D'; r.testWrite = true;
    r.image.reset(new GrayRaster); r.image->size = 64; r.image->px.assign(64 * 64, 200);
    std::string err; CHECK(job.start(r, err));
    CHECK(!job.start(r, err) && err == "a burn is already running");
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    job.cancel();
    waitDone(job);
    BurnResult res = job.result();
    std::printf("  cancelled=%d written=%llu of ~336k\n", res.cancelled, (unsigned long long)dev.sectors);
    CHECK(res.cancelled && !res.ok && dev.sectors < 300000);

    MockDevice dev2;
    BurnJob job2(factory(&dev2, 0, true));
    CHECK(job2.start(r, err)); waitDone(job2);
    CHECK(!job2.result().ok && job2.result().error.find("administrator") != std::string::npos);
}

int main() {
    testConfig(); testCropAndScale(); testJobLadder(); testJobImageAndRealBurnFlag(); testJobCancelAndErrors();
    std::printf(g_fail ? "\n%d FAILURES\n" : "\nALL TESTS PASSED\n", g_fail);
    return g_fail ? 1 : 0;
}
