#include "RawDaoWriter.h"
#include "CueSheet.h"
#include <algorithm>
#include <cstdio>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <time.h>
#endif

namespace dis {
namespace {

void sleepMs(unsigned ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec ts; ts.tv_sec = ms / 1000; ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, 0);
#endif
}

// Raise the feeding thread's priority for the duration of the burn.
struct PriorityBoost {
#ifdef _WIN32
    int old;
    PriorityBoost() { old = GetThreadPriority(GetCurrentThread()); SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL); }
    ~PriorityBoost() { SetThreadPriority(GetCurrentThread(), old); }
#endif
};

std::string describe(const char* what, const ScsiResult& r) {
    char buf[200];
    std::snprintf(buf, sizeof buf, "%s failed: status=0x%02X sense=%X/%02X/%02X os=%u",
                  what, r.status, r.senseKey, r.asc, r.ascq, r.osError);
    return buf;
}

uint16_t speedKBps(int x) { return x <= 0 ? 0xFFFF : (uint16_t)std::min(x * 176, 0xFFFE); }

} // namespace

bool RawDaoWriter::run(const mmc::Cdb& cdb, DataDir dir, void* buf, uint32_t len, uint32_t timeoutSec,
                       const char* what, BurnResult& res, bool fatal) {
    ScsiResult r = dev_.execute(cdb, dir, buf, len, timeoutSec);
    if (r.ok) return true;
    if (fatal) res.error = describe(what, r);
    return false;
}

BurnResult RawDaoWriter::burn(SectorSource& src, const BurnOptions& opt, ProgressCallback cb) {
    BurnResult res;
    const uint32_t total = src.totalSectors();
    if (total < 300) { res.error = "track shorter than 300 sectors (4 s)"; return res; }
    const bool data = composer_.layout() == kLayoutDataMode1;
    PriorityBoost boost; (void)boost;

    // ---- preflight ---------------------------------------------------------------------
    bool ready = false;
    for (int i = 0; i < 20 && !ready; ++i) {
        ready = run(mmc::testUnitReady(), kDataNone, 0, 0, 10, "TEST UNIT READY", res, false);
        if (!ready) sleepMs(500);
    }
    if (!ready) { res.error = "drive not ready (no disc?)"; return res; }

    uint8_t cfg[8] = {0};
    if (run(mmc::getConfiguration(), kDataIn, cfg, sizeof cfg, 10, "GET CONFIGURATION", res, false)) {
        uint16_t profile = mmc::be16(cfg + 6);
        if (profile != 0x0009 && profile != 0x000A) {
            char b[80]; std::snprintf(b, sizeof b, "media is not CD-R/CD-RW (profile 0x%04X)", profile);
            res.error = b; return res;
        }
    }
    uint8_t di[34] = {0};
    if (run(mmc::readDiscInformation(sizeof di), kDataIn, di, sizeof di, 10, "READ DISC INFORMATION", res, false)) {
        if ((di[2] & 0x03) != 0) { res.error = "disc is not blank"; return res; }
    }

    // ---- configure ---------------------------------------------------------------------
    int currentX = opt.speedX;
    if (!run(mmc::setCdSpeed(0xFFFF, speedKBps(currentX)), kDataNone, 0, 0, 10, "SET CD SPEED", res, false)) {
        /* some drives reject it; the drive then picks its own speed */
    }

    mmc::WriteParams wp;
    wp.testWrite = opt.testWrite; wp.bufe = opt.bufe; wp.dataTrack = data;
    std::vector<uint8_t> ms = mmc::writeParametersList(wp);
    if (!run(mmc::modeSelect10((uint16_t)ms.size()), kDataOut, &ms[0], (uint32_t)ms.size(), 30, "MODE SELECT (write parameters)", res))
        return res;

    std::vector<uint8_t> cue = buildDaoCueSheet(data, total);
    if (!run(mmc::sendCueSheet((uint32_t)cue.size()), kDataOut, &cue[0], (uint32_t)cue.size(), 30, "SEND CUE SHEET", res))
        return res;

    // ---- write loop --------------------------------------------------------------------
    const uint32_t maxSec = std::max<uint32_t>(1, dev_.maxTransferBytes() / SectorComposer::kSector);
    const uint32_t chunk  = std::max<uint32_t>(1, std::min(opt.chunkSectors, maxSec));
    const size_t   chunkBytes = (size_t)chunk * SectorComposer::kSector;
    const int      kMaxAhead = 8;
    std::vector<uint8_t> ring(chunkBytes * kMaxAhead);

    SafeBufferMonitor monitor(opt.monitor);
    int32_t  lba = -150;
    const int32_t end = (int32_t)total;
    uint32_t batch = 0, lastReported = 0;
    BufferState lastState = kBufHealthy;
    double lastFill = 1.0;
    bool speedDropped = false;

    while (lba < end) {
        struct Slot { int32_t lba; uint32_t n; } slots[kMaxAhead];
        int ns = 0;
        int k = std::min(monitor.prefetchChunks(), kMaxAhead);
        int32_t cur = lba;
        for (int j = 0; j < k && cur < end; ++j) {
            uint32_t n = (uint32_t)std::min<int64_t>(chunk, (int64_t)end - cur);
            uint8_t* dst = &ring[(size_t)j * chunkBytes];
            for (uint32_t i = 0; i < n; ++i) {
                int32_t L = cur + (int32_t)i;
                uint8_t* sec = dst + (size_t)i * SectorComposer::kSector;
                if (L < 0) composer_.composeUniform(L, composer_.lightestPattern(), sec);
                else if (!src.next(sec)) { res.error = "sector source ended early"; return res; }
            }
            slots[ns].lba = cur; slots[ns].n = n; ++ns;
            cur += (int32_t)n;
        }
        for (int j = 0; j < ns; ++j) {
            mmc::Cdb w = mmc::write10(slots[j].lba, (uint16_t)slots[j].n);
            uint8_t* src_ = &ring[(size_t)j * chunkBytes];
            const uint32_t bytes = slots[j].n * SectorComposer::kSector;
            ScsiResult r;
            for (int attempt = 0; attempt < 6; ++attempt) {
                r = dev_.execute(w, kDataOut, src_, bytes, 60);
                if (r.ok || r.senseKey != 0x02) break;     // retry only "NOT READY" (e.g. busy)
                sleepMs(50);
            }
            if (!r.ok) { res.error = describe("WRITE(10)", r); res.sectorsWritten = (uint32_t)(slots[j].lba + 150); return res; }
        }
        lba = cur;
        ++batch;

        if (monitor.shouldPoll(batch)) {
            uint8_t bc[12] = {0};
            uint32_t tot = 0, blank = 0;
            if (run(mmc::readBufferCapacity(), kDataIn, bc, sizeof bc, 10, "READ BUFFER CAPACITY", res, false) &&
                mmc::parseBufferCapacity(bc, sizeof bc, tot, blank)) {
                BufferSample s; s.total = tot; s.blank = blank;
                monitor.update(s, batch);
                lastFill = s.fill();
                if (opt.experimentalSpeedDrop && monitor.state() == kBufCritical && !speedDropped) {
                    currentX = SafeBufferMonitor::recommendSpeed(currentX > 0 ? currentX : 52, 0, 1);
                    run(mmc::setCdSpeed(0xFFFF, speedKBps(currentX)), kDataNone, 0, 0, 10, "SET CD SPEED (mid-write)", res, false);
                    speedDropped = true;
                }
            }
        }

        const uint32_t done = (uint32_t)(lba + 150);
        const bool stateChanged = monitor.state() != lastState;
        if (cb && (stateChanged || done - lastReported >= (total + 150) / 100 || lba >= end)) {
            BurnProgress p;
            p.sectorsDone = done; p.sectorsTotal = total + 150;
            p.state = monitor.state(); p.bufferFill = lastFill;
            lastReported = done; lastState = monitor.state();
            if (!cb(p)) {
                res.cancelled = true; res.sectorsWritten = done;
                res.error = "cancelled by user (the disc is unusable)";
                return res;
            }
        }
    }
    res.sectorsWritten = total + 150;

    // ---- finish: the drive writes the lead-out ------------------------------------------
    if (!run(mmc::synchronizeCache(), kDataNone, 0, 0, 900, "SYNCHRONIZE CACHE", res)) return res;
    if (opt.eject) run(mmc::startStopUnit(true), kDataNone, 0, 0, 30, "EJECT", res, false);

    res.lowEvents = monitor.lowEvents();
    res.criticalEvents = monitor.criticalEvents();
    res.minFill = monitor.minFill();
    res.recommendedSpeedX = SafeBufferMonitor::recommendSpeed(opt.speedX > 0 ? opt.speedX : 52,
                                                              res.lowEvents, res.criticalEvents);
    res.ok = true;
    return res;
}

} // namespace dis
