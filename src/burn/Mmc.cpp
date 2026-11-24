#include "Mmc.h"

namespace dis {
namespace mmc {

static void put16(uint8_t* p, uint32_t v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static void put32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}

Cdb testUnitReady() { Cdb c; c.b[0] = 0x00; c.len = 6; return c; }

Cdb inquiry(uint8_t allocLen) { Cdb c; c.b[0] = 0x12; c.b[4] = allocLen; c.len = 6; return c; }

Cdb getConfiguration() { Cdb c; c.b[0] = 0x46; put16(&c.b[7], 8); c.len = 10; return c; }

Cdb readDiscInformation(uint16_t n) { Cdb c; c.b[0] = 0x51; put16(&c.b[7], n); c.len = 10; return c; }

Cdb readTrackInformation(uint32_t track, uint16_t n) {
    Cdb c; c.b[0] = 0x52; c.b[1] = 0x01; put32(&c.b[2], track); put16(&c.b[7], n); c.len = 10; return c;
}

Cdb setCdSpeed(uint16_t r, uint16_t w) {
    Cdb c; c.b[0] = 0xBB; put16(&c.b[2], r); put16(&c.b[4], w); c.len = 12; return c;
}

Cdb modeSelect10(uint16_t n) { Cdb c; c.b[0] = 0x55; c.b[1] = 0x10; put16(&c.b[7], n); c.len = 10; return c; }

Cdb sendCueSheet(uint32_t n) {
    Cdb c; c.b[0] = 0x5D; c.b[6] = (uint8_t)(n >> 16); c.b[7] = (uint8_t)(n >> 8); c.b[8] = (uint8_t)n;
    c.len = 10; return c;
}

Cdb write10(int32_t lba, uint16_t blocks) {
    Cdb c; c.b[0] = 0x2A; put32(&c.b[2], (uint32_t)lba); put16(&c.b[7], blocks); c.len = 10; return c;
}

Cdb readBufferCapacity() { Cdb c; c.b[0] = 0x5C; put16(&c.b[7], 12); c.len = 10; return c; }

Cdb synchronizeCache() { Cdb c; c.b[0] = 0x35; c.len = 10; return c; }

Cdb startStopUnit(bool eject) { Cdb c; c.b[0] = 0x1B; c.b[4] = eject ? 0x02 : 0x00; c.len = 6; return c; }

std::vector<uint8_t> writeParametersList(const WriteParams& p) {
    std::vector<uint8_t> v(8 + 52, 0);          // mode parameter header (10-byte form) + page
    uint8_t* pg = &v[8];
    pg[0] = 0x05;                               // page code: Write Parameters
    pg[1] = 0x32;                               // page length (50)
    pg[2] = (uint8_t)((p.bufe ? 0x40 : 0) | (p.testWrite ? 0x10 : 0) | 0x02);   // write type 2 = SAO/DAO
    pg[3] = (uint8_t)(p.dataTrack ? 0x04 : 0x00);   // multi-session 00b (closed), track mode
    pg[4] = 0x00;                               // data block type 0 = raw, 2352 bytes
    pg[8] = 0x00;                               // session format: CD-DA / CD-ROM
    put16(&pg[14], p.pauseSectors);             // audio pause length
    return v;
}

bool parseBufferCapacity(const uint8_t* d, size_t len, uint32_t& total, uint32_t& blank) {
    if (len < 12) return false;
    total = be32(d + 4);
    blank = be32(d + 8);
    return total != 0;
}

} // namespace mmc
} // namespace dis
