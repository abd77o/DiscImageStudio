// SptiDevice.h - Windows SCSI Pass-Through Interface (IOCTL_SCSI_PASS_THROUGH_DIRECT).
// Needs administrator rights to write: embed a manifest with requestedExecutionLevel
// "requireAdministrator". Uses only kernel32 / ntddscsi - no other dependencies.
#pragma once
#ifdef _WIN32
#include "ScsiDevice.h"
#include <string>
#include <vector>

namespace dis {

class SptiDevice : public IScsiDevice {
public:
    SptiDevice();
    ~SptiDevice();

    static std::vector<char> listCdDrives();           // drive letters of optical drives
    bool open(char driveLetter, std::string& error);   // opens \\.\X:
    void close();
    bool isOpen() const;

    ScsiResult execute(const mmc::Cdb& cdb, DataDir dir, void* data,
                       uint32_t length, uint32_t timeoutSec) override;
    uint32_t maxTransferBytes() const override { return maxTransfer_; }
    std::string inquiryString();                       // "VENDOR PRODUCT REV"

private:
    void* handle_;
    void* buffer_;            // page-aligned bounce buffer (satisfies any adapter alignment mask)
    uint32_t bufferSize_;
    uint32_t maxTransfer_;
};

} // namespace dis
#endif
