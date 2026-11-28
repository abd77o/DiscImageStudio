#include "LoggingDevice.h"
#include "SenseText.h"
#include <cstdio>

namespace dis {

static std::string hex(const uint8_t* p, size_t n) {
    std::string s; char b[4];
    for (size_t i = 0; i < n; ++i) { std::snprintf(b, sizeof b, "%02X", p[i]); s += b; if (i + 1 < n) s += ' '; }
    return s;
}

LoggingDevice::LoggingDevice(IScsiDevice* inner, const Sink& sink)
    : inner_(inner), sink_(sink), writes_(0), polls_(0), t0_(std::chrono::steady_clock::now()) {}

void LoggingDevice::note(const std::string& text) { if (sink_) sink_(text); }

ScsiResult LoggingDevice::execute(const mmc::Cdb& cdb, DataDir dir, void* data, uint32_t len, uint32_t timeoutSec) {
    ScsiResult r = inner_->execute(cdb, dir, data, len, timeoutSec);
    const uint8_t op = cdb.b[0];
    bool quiet = false;
    if (op == 0x2A) { ++writes_; quiet = r.ok && !(writes_ <= 3 || writes_ % 2000 == 0); }
    if (op == 0x5C) { ++polls_;  quiet = r.ok && !(polls_ <= 3 || polls_ % 100 == 0); }
    if (quiet || !sink_) return r;

    long ms = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0_).count();
    char head[96];
    std::snprintf(head, sizeof head, "+%6ldms  %-22s", ms, mmcOpcodeName(op));
    std::string line = head;
    line += "CDB " + hex(cdb.b, cdb.len);
    if (len) {
        char b[48]; std::snprintf(b, sizeof b, "  %s %u B", dir == kDataOut ? "OUT" : "IN", len);
        line += b;
        if (len <= 64 && data && (dir == kDataOut || r.ok)) line += " [" + hex((const uint8_t*)data, len) + "]";
    }
    if (r.ok) line += "  -> OK";
    else {
        char b[64]; std::snprintf(b, sizeof b, "  -> FAILED status=%02X os=%u : ", r.status, r.osError);
        line += b + senseDescription(r.senseKey, r.asc, r.ascq);
    }
    sink_(line);
    return r;
}

} // namespace dis
