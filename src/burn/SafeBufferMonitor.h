// SafeBufferMonitor.h - watches the drive's internal buffer fill level and decides how
// aggressively the writer should feed it. Pure logic (no I/O) so it can be unit-tested.
//
// IMPORTANT: the drive's write speed is fixed for the session once the burn starts, and
// MMC has no standard way to change it mid-write. What the host CAN do when the buffer
// runs low is (a) send data in larger back-to-back batches, (b) stop doing any other work
// (GUI redraws, progress), and (c) remember the event so the NEXT burn starts slower.
#pragma once
#include <cstdint>

namespace dis {

enum BufferState { kBufHealthy = 0, kBufLow = 1, kBufCritical = 2 };

struct BufferSample {
    uint32_t total = 0;     // drive buffer size, bytes
    uint32_t blank = 0;     // free space, bytes
    double fill() const { return total ? 1.0 - (double)blank / (double)total : 1.0; }
};

struct MonitorConfig {
    double   lowFill        = 0.30;   // below this fill level -> Low
    double   criticalFill   = 0.10;   // below this -> Critical
    double   recoverFill    = 0.50;   // hysteresis: back to Healthy only above this
    uint32_t pollEveryBatches = 4;    // poll rate while healthy (every batch when not)
    uint32_t primeBatchLimit  = 32;   // buffer is empty at start: ignore until primed or this many batches
};

class SafeBufferMonitor {
public:
    explicit SafeBufferMonitor(const MonitorConfig& cfg = MonitorConfig());

    bool        shouldPoll(uint32_t batchIndex) const;
    BufferState update(const BufferSample& s, uint32_t batchIndex);
    BufferState state() const { return state_; }
    bool        primed() const { return primed_; }

    // How many chunks to prepare and send back-to-back before looking at anything else.
    int prefetchChunks() const;

    uint32_t lowEvents()      const { return lowEvents_; }
    uint32_t criticalEvents() const { return criticalEvents_; }
    double   minFill()        const { return minFill_; }

    // Suggested speed (x) for the NEXT burn after this one's events.
    static int recommendSpeed(int currentX, uint32_t lowEvents, uint32_t criticalEvents);

private:
    MonitorConfig cfg_;
    BufferState   state_;
    bool          primed_;
    uint32_t      lowEvents_, criticalEvents_;
    double        minFill_;
};

} // namespace dis
