// Mmc.h - builders for the MMC commands used by the burner (pure functions, no I/O).
#pragma once
#include <cstdint>
#include <cstring>
#include <vector>

namespace dis {
namespace mmc {

struct Cdb {
    uint8_t b[16];
    uint8_t len;
    Cdb() : len(0) { std::memset(b, 0, sizeof b); }
};

struct WriteParams {
    bool     testWrite   = false;   // simulate only (not supported by every drive)
    bool     bufe        = true;    // drive-side buffer-underrun-free recording
    bool     dataTrack   = false;   // track mode: audio (0) or data (4)
    uint16_t pauseSectors = 150;
};

Cdb testUnitReady();
Cdb inquiry(uint8_t allocLen);
Cdb getConfiguration();                               // header only (current profile)
Cdb readDiscInformation(uint16_t allocLen);
Cdb readTrackInformation(uint32_t track, uint16_t allocLen);
Cdb setCdSpeed(uint16_t readKBps, uint16_t writeKBps);   // 0xFFFF = max
Cdb modeSelect10(uint16_t paramListLen);
Cdb sendCueSheet(uint32_t cueSheetBytes);
Cdb write10(int32_t lba, uint16_t blocks);            // lba may be negative (pre-gap)
Cdb readBufferCapacity();
Cdb synchronizeCache();
Cdb startStopUnit(bool eject);

// MODE SELECT(10) parameter list: 8-byte header + Write Parameters page (0x05, 52 bytes).
// Write type = SAO/DAO (02h), data block type 0 = raw 2352 bytes.
std::vector<uint8_t> writeParametersList(const WriteParams& p);

// READ BUFFER CAPACITY response: total buffer length and free ("blank") length in bytes.
bool parseBufferCapacity(const uint8_t* data, size_t len, uint32_t& total, uint32_t& blank);

inline uint16_t be16(const uint8_t* p) { return (uint16_t)((p[0] << 8) | p[1]); }
inline uint32_t be32(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

} // namespace mmc
} // namespace dis
