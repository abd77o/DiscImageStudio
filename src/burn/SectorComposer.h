// SectorComposer.h - turns dither tones into raw 2352-byte sectors.
//
// Goal: control exactly which bytes enter the drive's CIRC/EFM encoder, because that
// determines the physical pit pattern (and therefore how light/dark the area looks).
//
//  kLayoutAudio     : all 2352 bytes are pattern. Audio tracks are NOT scrambled and carry
//                     no EDC/ECC, so the host has full control. (Safest default.)
//  kLayoutDataMode1 : 12-byte sync + 4-byte header (valid MSF, mode 1) + pattern body.
//                     There is no EDC/ECC, so readers will report errors - fine for art discs.
//                     If the drive scrambles this track itself (driveScrambles = true) the
//                     composer pre-XORs the keystream so the pattern still reaches the CIRC
//                     encoder unchanged. If the drive does not scramble (raw write), set false.
#pragma once
#include <cstddef>
#include <cstdint>

namespace dis {

enum SectorLayout { kLayoutAudio = 0, kLayoutDataMode1 = 1 };

struct ComposerConfig {
    SectorLayout layout        = kLayoutAudio;
    bool         driveScrambles = false;
    int          cellBytes     = 196;       // must match DitherParams::cellBytes
    int          levels        = 2;         // number of tones in use (2..8)
    // Byte value that represents each tone (0 = darkest). PLACEHOLDERS: the real
    // light/dark byte pair depends on the drive/media - pick it with the calibration ladder.
    uint8_t      tonePattern[8] = { 0x11, 0xEE, 0x33, 0xCC, 0x55, 0xAA, 0x0F, 0xF0 };
};

class SectorComposer {
public:
    static const int kSector = 2352;

    explicit SectorComposer(const ComposerConfig& cfg);

    int cellsPerSector() const { return kSector / cfg_.cellBytes; }
    SectorLayout layout() const { return cfg_.layout; }
    const ComposerConfig& config() const { return cfg_; }
    uint8_t lightestPattern() const { return cfg_.tonePattern[cfg_.levels - 1]; }

    // tones: cellsPerSector() values. lba may be negative (pre-gap): header uses lba + 150.
    void compose(int32_t lba, const uint8_t* tones, uint8_t* out2352) const;
    void composeUniform(int32_t lba, uint8_t patternByte, uint8_t* out2352) const;

private:
    void finish(int32_t lba, uint8_t* sector) const;
    ComposerConfig cfg_;
};

} // namespace dis
