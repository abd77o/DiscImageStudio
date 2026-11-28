// LoggingDevice.h - decorator that logs every SCSI command (CDB, small payloads, result and
// decoded sense) so a failed burn on real hardware can be diagnosed from burn_log.txt.
// WRITE(10) and READ BUFFER CAPACITY are throttled (first few + every Nth + all failures).
#pragma once
#include "ScsiDevice.h"
#include <chrono>
#include <functional>
#include <string>

namespace dis {

class LoggingDevice : public IScsiDevice {
public:
    typedef std::function<void(const std::string&)> Sink;
    LoggingDevice(IScsiDevice* inner, const Sink& sink);   // inner is not owned

    ScsiResult execute(const mmc::Cdb& cdb, DataDir dir, void* data, uint32_t length, uint32_t timeoutSec) override;
    uint32_t maxTransferBytes() const override { return inner_->maxTransferBytes(); }
    void note(const std::string& text);

private:
    IScsiDevice* inner_;
    Sink sink_;
    uint64_t writes_, polls_;
    std::chrono::steady_clock::time_point t0_;
};

} // namespace dis
