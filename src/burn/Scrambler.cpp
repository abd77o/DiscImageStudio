#include "Scrambler.h"

namespace dis {
namespace {

struct Table {
    uint8_t t[Scrambler::kScrambledBytes];
    Table() {
        uint16_t lfsr = 1;
        for (int i = 0; i < Scrambler::kScrambledBytes; ++i) {
            uint8_t b = 0;
            for (int bit = 0; bit < 8; ++bit) {
                b |= (uint8_t)((lfsr & 1u) << bit);
                unsigned fb = (lfsr ^ (lfsr >> 1)) & 1u;          // x^15 + x + 1
                lfsr = (uint16_t)((lfsr >> 1) | (fb << 14));
            }
            t[i] = b;
        }
    }
};

} // namespace

const uint8_t* Scrambler::table() {
    static const Table tbl;
    return tbl.t;
}

void Scrambler::apply(uint8_t* s) {
    const uint8_t* t = table();
    for (int i = 0; i < kScrambledBytes; ++i) s[kSyncBytes + i] ^= t[i];
}

} // namespace dis
