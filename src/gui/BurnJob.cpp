#include "BurnJob.h"
#include "../core/PolarDitherEngine.h"
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

void BurnJob::run() {
    try {
        std::unique_ptr<IScsiDevice> dev;
        std::string err;
        if (!factory_(req_.driveLetter, dev, err)) {
            res_.error = err.empty() ? "cannot open the drive" : err;
        } else {
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

            BurnOptions bo;
            bo.speedX = c.speedX;
            bo.testWrite = req_.testWrite;
            bo.eject = !req_.testWrite;
            RawDaoWriter writer(*dev, comp);
            res_ = writer.burn(*src, bo, [this](const BurnProgress& p) {
                done_ = p.sectorsDone; total_ = p.sectorsTotal;
                state_ = (int)p.state; fillPermille_ = (int)(p.bufferFill * 1000.0);
                return !cancel_.load();
            });
        }
    } catch (const std::exception& e) {
        res_.ok = false; res_.error = std::string("internal error: ") + e.what();
    }
    running_ = false;
    finished_ = true;
}

} // namespace dis
