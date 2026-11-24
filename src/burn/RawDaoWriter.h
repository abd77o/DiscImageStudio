// RawDaoWriter.h - orchestrates a complete Disc-At-Once burn through IScsiDevice.
//   preflight -> SET CD SPEED -> MODE SELECT (write params) -> SEND CUE SHEET
//   -> WRITE(10) from LBA -150 (pre-gap) .. end -> SYNCHRONIZE CACHE (lead-out)
#pragma once
#include "SafeBufferMonitor.h"
#include "ScsiDevice.h"
#include "SectorComposer.h"
#include "SectorSource.h"
#include <functional>
#include <string>

namespace dis {

struct BurnOptions {
    int      speedX        = 4;       // 0 = drive maximum
    bool     testWrite     = false;
    bool     bufe          = true;
    bool     eject         = false;
    uint32_t chunkSectors  = 27;      // 27 * 2352 = 63,504 B  (<= 64 KiB adapter limit)
    bool     experimentalSpeedDrop = false;   // try SET CD SPEED mid-write when Critical (off by default)
    MonitorConfig monitor;
};

struct BurnProgress {
    uint32_t    sectorsDone  = 0;
    uint32_t    sectorsTotal = 0;
    BufferState state        = kBufHealthy;
    double      bufferFill   = 1.0;
};

struct BurnResult {
    bool        ok = false;
    bool        cancelled = false;
    std::string error;
    uint32_t    sectorsWritten = 0;     // including the 150 pre-gap sectors
    uint32_t    lowEvents = 0, criticalEvents = 0;
    double      minFill = 1.0;
    int         recommendedSpeedX = 0;  // for the next burn
};

// Return false from the callback to cancel (the disc will be unusable).
typedef std::function<bool(const BurnProgress&)> ProgressCallback;

class RawDaoWriter {
public:
    RawDaoWriter(IScsiDevice& dev, const SectorComposer& composer)
        : dev_(dev), composer_(composer) {}

    BurnResult burn(SectorSource& src, const BurnOptions& opt, ProgressCallback cb);

private:
    bool run(const mmc::Cdb& cdb, DataDir dir, void* buf, uint32_t len, uint32_t timeoutSec,
             const char* what, BurnResult& res, bool fatal = true);
    IScsiDevice& dev_;
    const SectorComposer& composer_;
};

} // namespace dis
