// ScsiDevice.h - tiny abstraction over "send one SCSI command". SptiDevice implements it on
// Windows; tests implement a mock. Everything above this layer is platform-independent.
#pragma once
#include "Mmc.h"
#include <cstdint>
#include <string>

namespace dis {

enum DataDir { kDataNone = 0, kDataIn = 1, kDataOut = 2 };

struct ScsiResult {
    bool     ok;
    uint8_t  status;       // SCSI status (0 = GOOD, 2 = CHECK CONDITION)
    uint8_t  senseKey, asc, ascq;
    uint32_t osError;      // GetLastError() if the pass-through call itself failed
    ScsiResult() : ok(false), status(0), senseKey(0), asc(0), ascq(0), osError(0) {}
};

class IScsiDevice {
public:
    virtual ~IScsiDevice() {}
    virtual ScsiResult execute(const mmc::Cdb& cdb, DataDir dir, void* data,
                               uint32_t length, uint32_t timeoutSec) = 0;
    virtual uint32_t maxTransferBytes() const { return 65536; }
};

} // namespace dis
