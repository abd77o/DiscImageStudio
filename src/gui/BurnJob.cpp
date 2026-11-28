#include "BurnJob.h"
#include "../burn/LoggingDevice.h"
#include "../burn/SenseText.h"
#include "../core/PlatformFile.h"
#include "../core/PolarDitherEngine.h"
#include <cstdio>
#include <exception>

namespace dis {

BurnJob::BurnJob(DeviceFactory f)
    : factory_(f), running_(false), cancel_(false), finished_(false),
      done_(0), total_(0), state_(0), fillPermille_(1000) {}

BurnJob::~BurnJob() {
    cancel_ = true;
    if (th_.joinable()) th_.join();
}

BurnJob::Status BurnJob::status() const {
    Status s;
    s.done = done_; s.total = total_; s.state = state_; s.fill = fillPermille_ / 1000.0;
    return s;
}

bool BurnJob::start(const BurnRequest& r, std::string& error) {
    if (running_) { error = "a burn is already running"; return false; }
    if (th_.joinable()) th_.join();
    if (r.kind == BurnRequest::kImage && (!r.image || r.image->size < 2)) { error = "no image loaded"; return false; }
    req_ = r;
    res_ = BurnResult();
    done_ = 0; total_ = 0; state_ = 0; fillPermille_ = 1000;
    cancel_ = false; finished_ = false; running_ = true;
    th_ = std::thread(&BurnJob::run, this);
    return true;
}

static void logLine(FILE* f, const std::string& s) {
    if (!f) return;
    std::fputs(s.c_str(), f); std::fputc('\n', f); std::fflush(f);
}

void BurnJob::run() {
    FILE* lf = 0;
    try {
        if (!req_.logPath.empty()) {
            lf = openFileUtf8(req_.logPath, "ab");
            if (lf) {                                   // keep the log small: restart it when it passes 512 KB
                std::fseek(lf, 0, SEEK_END);
                if (std::ftell(lf) > 512L * 1024) { std::fclose(lf); lf = openFileUtf8(req_.logPath, "wb"); }
            }
        }
        char hdr[200];
        std::snprintf(hdr, sizeof hdr, "\n==== burn session (build %s %s) ====\ndrive %c:  kind=%d  speed=%dx  testWrite=%d  cell=%d  light=0x%02X dark=0x%02X",
                      __DATE__, __TIME__, req_.driveLetter ? req_.driveLetter : '?', (int)req_.kind, req_.cfg.speedX,
                      req_.testWrite ? 1 : 0, req_.cfg.cellBytes, req_.cfg.lightByte, req_.cfg.darkByte);
        logLine(lf, hdr);

        std::unique_ptr<IScsiDevice> dev;
        std::string err;
        if (!factory_(req_.driveLetter, dev, err)) {
            res_.error = err.empty() ? "cannot open the drive" : err;
            logLine(lf, "open drive FAILED: " + res_.error);
        } else {
            std::unique_ptr<LoggingDevice> logdev;
            if (lf) logdev.reset(new LoggingDevice(dev.get(), [lf](const std::string& s) { logLine(lf, s); }));
            IScsiDevice& useDev = logdev ? static_cast<IScsiDevice&>(*logdev) : *dev;

            const AppConfig& c = req_.cfg;
            ComposerConfig cc;
            cc.layout = kLayoutAudio;                 // full control of all 2352 bytes, no scrambler
            cc.levels = 2;
            cc.cellBytes = c.cellBytes;
            cc.tonePattern[0] = c.darkByte;
            cc.tonePattern[1] = c.lightByte;
            SectorComposer comp(cc);

            std::unique_ptr<PolarDitherEngine> eng;
            std::unique_ptr<SectorSource> src;
            if (req_.kind == BurnRequest::kImage) {
                DitherParams dp;
                dp.cellBytes = c.cellBytes; dp.levels = 2;
                dp.kernel = c.jjn ? Kernel::JarvisJudiceNinke : Kernel::FloydSteinberg;
                dp.imageRadiusMm = c.imageRadiusMm;
                dp.brightness = c.brightness; dp.contrast = c.contrast; dp.gamma = c.gamma;
                dp.angleOffsetDeg = c.angleOffsetDeg; dp.mirror = c.mirror;
                eng.reset(new PolarDitherEngine(DiscGeometry(), *req_.image, dp));
                src.reset(new ImageSectorSource(*eng, comp));
            } else {
                CalibrationConfig cal = req_.calib;
                cal.lightByte = c.lightByte; cal.darkByte = c.darkByte;
                src.reset(new CalibrationSectorSource(cal, comp));
            }
            total_ = src->totalSectors() + 150;
            { char b[80]; std::snprintf(b, sizeof b, "program length %u sectors (+150 pre-gap)", src->totalSectors()); logLine(lf, b); }

            BurnOptions bo;
            bo.speedX = c.speedX;
            bo.testWrite = req_.testWrite;
            bo.eject = !req_.testWrite;
            RawDaoWriter writer(useDev, comp);
            res_ = writer.burn(*src, bo, [this](const BurnProgress& p) {
                done_ = p.sectorsDone; total_ = p.sectorsTotal;
                state_ = (int)p.state; fillPermille_ = (int)(p.bufferFill * 1000.0);
                return !cancel_.load();
            });
            char b[200];
            std::snprintf(b, sizeof b, "RESULT: %s  written=%u  low=%u critical=%u minFill=%.0f%%",
                          res_.ok ? "OK" : (res_.cancelled ? "CANCELLED" : "FAILED"), res_.sectorsWritten,
                          res_.lowEvents, res_.criticalEvents, res_.minFill * 100.0);
            logLine(lf, b);
            if (!res_.ok) logLine(lf, "ERROR: " + res_.error);
        }
    } catch (const std::exception& e) {
        res_.ok = false; res_.error = std::string("internal error: ") + e.what();
        logLine(lf, "EXCEPTION: " + res_.error);
    }
    if (lf) std::fclose(lf);
    running_ = false;
    finished_ = true;
}

} // namespace dis
