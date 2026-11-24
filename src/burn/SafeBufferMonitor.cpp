#include "SafeBufferMonitor.h"

namespace dis {

SafeBufferMonitor::SafeBufferMonitor(const MonitorConfig& cfg)
    : cfg_(cfg), state_(kBufHealthy), primed_(false), lowEvents_(0), criticalEvents_(0), minFill_(1.0) {}

bool SafeBufferMonitor::shouldPoll(uint32_t batchIndex) const {
    if (state_ != kBufHealthy || !primed_) return true;
    return cfg_.pollEveryBatches == 0 || (batchIndex % cfg_.pollEveryBatches) == 0;
}

BufferState SafeBufferMonitor::update(const BufferSample& s, uint32_t batchIndex) {
    const double f = s.fill();
    if (!primed_) {
        if (f >= cfg_.recoverFill || batchIndex >= cfg_.primeBatchLimit) primed_ = true;
        else return state_;                       // still filling up after start: not an event
    }
    if (f < minFill_) minFill_ = f;

    const BufferState prev = state_;
    if (f < cfg_.criticalFill)                             state_ = kBufCritical;
    else if (f < cfg_.lowFill)                             state_ = kBufLow;
    else if (prev != kBufHealthy && f < cfg_.recoverFill)  state_ = kBufLow;   // hysteresis band
    else                                                   state_ = kBufHealthy;

    if (prev == kBufHealthy && state_ == kBufLow)       ++lowEvents_;
    if (prev != kBufCritical && state_ == kBufCritical) ++criticalEvents_;
    return state_;
}

int SafeBufferMonitor::prefetchChunks() const {
    switch (state_) {
        case kBufCritical: return 8;
        case kBufLow:      return 4;
        default:           return primed_ ? 1 : 4;
    }
}

int SafeBufferMonitor::recommendSpeed(int x, uint32_t low, uint32_t crit) {
    static const int steps[] = { 1, 2, 4, 8, 10, 12, 16, 24, 32, 40, 48, 52 };
    const int n = (int)(sizeof(steps) / sizeof(steps[0]));
    int idx = 0;
    for (int i = 0; i < n; ++i) if (steps[i] <= x) idx = i;
    int drop = crit > 0 ? 2 : (low > 2 ? 1 : 0);
    idx -= drop;
    if (idx < 0) idx = 0;
    return steps[idx];
}

} // namespace dis
