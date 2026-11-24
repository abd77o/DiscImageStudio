#include "SectorComposer.h"
#include "Scrambler.h"
#include <cstring>

namespace dis {

static uint8_t bcd(unsigned v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

SectorComposer::SectorComposer(const ComposerConfig& cfg) : cfg_(cfg) {
    if (cfg_.cellBytes <= 0 || kSector % cfg_.cellBytes != 0) cfg_.cellBytes = 196;
    if (cfg_.levels < 2) cfg_.levels = 2;
    if (cfg_.levels > 8) cfg_.levels = 8;
    if (cfg_.layout == kLayoutAudio) cfg_.driveScrambles = false;   // audio is never scrambled
}

void SectorComposer::compose(int32_t lba, const uint8_t* tones, uint8_t* out) const {
    const int cps = cellsPerSector();
    for (int i = 0; i < cps; ++i) {
        int t = tones[i];
        if (t >= cfg_.levels) t = cfg_.levels - 1;
        std::memset(out + (size_t)i * cfg_.cellBytes, cfg_.tonePattern[t], (size_t)cfg_.cellBytes);
    }
    finish(lba, out);
}

void SectorComposer::composeUniform(int32_t lba, uint8_t patternByte, uint8_t* out) const {
    std::memset(out, patternByte, kSector);
    finish(lba, out);
}

// On entry the buffer holds the bytes we WANT on disc (the pattern). For the data layout we
// add sync + a header that looks valid after scrambling, then (if the drive will scramble)
// pre-apply the keystream so that drive-scramble(host bytes) == wanted bytes.
void SectorComposer::finish(int32_t lba, uint8_t* s) const {
    if (cfg_.layout != kLayoutDataMode1) return;

    s[0] = 0x00;
    std::memset(s + 1, 0xFF, 10);
    s[11] = 0x00;

    const unsigned abs = (unsigned)(lba + 150);
    const uint8_t* key = Scrambler::table();
    s[12] = (uint8_t)(bcd(abs / 4500)        ^ key[0]);   // minutes
    s[13] = (uint8_t)(bcd((abs / 75) % 60)   ^ key[1]);   // seconds
    s[14] = (uint8_t)(bcd(abs % 75)          ^ key[2]);   // frames
    s[15] = (uint8_t)(0x01                   ^ key[3]);   // mode 1

    if (cfg_.driveScrambles) Scrambler::apply(s);
}

} // namespace dis
