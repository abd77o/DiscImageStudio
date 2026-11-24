// MockDevice - a fake burner with a leaky buffer, used to test the whole pipeline on any OS.
#pragma once
#include "../src/burn/ScsiDevice.h"
#include <algorithm>
#include <vector>

class MockDevice : public dis::IScsiDevice {
public:
    uint32_t bufTotal = 2u * 1024 * 1024;
    double   fill = 0;                 // bytes currently in the drive buffer
    double   drainPerWrite = 0;        // bytes the "laser" consumes between two WRITE commands
    uint32_t maxTransfer = 65536;

    std::vector<uint8_t> cueSheet, modeList;
    bool     cueSeen = false, writeBeforeCue = false, contiguous = true;
    int32_t  firstLba = 0, nextLba = 0;
    uint64_t writes = 0, sectors = 0;
    int      speedCmds = 0, syncCmds = 0;
    std::vector<uint8_t> lastWrite;    // payload of the most recent WRITE(10)
    std::vector<uint8_t> firstWriteCdb;

    dis::ScsiResult execute(const dis::mmc::Cdb& c, dis::DataDir, void* d, uint32_t len, uint32_t) override {
        dis::ScsiResult r; r.ok = true;
        uint8_t* p = static_cast<uint8_t*>(d);
        switch (c.b[0]) {
        case 0x2A: {
            int32_t lba = (int32_t)dis::mmc::be32(&c.b[2]);
            uint32_t n = dis::mmc::be16(&c.b[7]);
            if (!cueSeen) writeBeforeCue = true;
            if (writes == 0) { firstLba = lba; nextLba = lba; firstWriteCdb.assign(c.b, c.b + c.len); }
            if (lba != nextLba) contiguous = false;
            if (len != n * 2352) contiguous = false;
            nextLba = lba + (int32_t)n;
            ++writes; sectors += n;
            fill = std::max(0.0, fill - drainPerWrite);
            fill = std::min<double>(bufTotal, fill + len);
            lastWrite.assign(p, p + len);
            break; }
        case 0x5C:
            std::fill(p, p + len, 0);
            p[1] = 10;
            p[4] = (uint8_t)(bufTotal >> 24); p[5] = (uint8_t)(bufTotal >> 16); p[6] = (uint8_t)(bufTotal >> 8); p[7] = (uint8_t)bufTotal;
            { uint32_t blank = bufTotal - (uint32_t)fill;
              p[8] = (uint8_t)(blank >> 24); p[9] = (uint8_t)(blank >> 16); p[10] = (uint8_t)(blank >> 8); p[11] = (uint8_t)blank; }
            break;
        case 0x46: std::fill(p, p + len, 0); p[7] = 0x09; break;      // profile CD-R
        case 0x51: std::fill(p, p + len, 0); break;                   // empty disc
        case 0x5D: cueSheet.assign(p, p + len); cueSeen = true; break;
        case 0x55: modeList.assign(p, p + len); break;
        case 0xBB: ++speedCmds; break;
        case 0x35: ++syncCmds; break;
        default: break;
        }
        return r;
    }
    uint32_t maxTransferBytes() const override { return maxTransfer; }
};
