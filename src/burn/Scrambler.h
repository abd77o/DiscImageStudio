// Scrambler.h - CD-ROM sector scrambler (ECMA-130 Annex B).
// 15-bit LFSR, polynomial x^15 + x + 1, initial state 0x0001, LSB-first output.
// It is applied to bytes 12..2351 of a 2352-byte sector (the 12 sync bytes are not scrambled).
// XOR is its own inverse, so apply() both scrambles and descrambles.
#pragma once
#include <cstdint>

namespace dis {

class Scrambler {
public:
    static const int kSectorBytes    = 2352;
    static const int kSyncBytes      = 12;
    static const int kScrambledBytes = kSectorBytes - kSyncBytes;   // 2340

    static const uint8_t* table();               // 2340-byte keystream
    static void apply(uint8_t* sector2352);      // XOR keystream into bytes 12..2351
};

} // namespace dis
