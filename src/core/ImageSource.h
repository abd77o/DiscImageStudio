// ImageSource.h - streaming image loader. Reads the file ROW BY ROW and box-filters
// it into a small square 8-bit working raster (default <= 1024x1024 = 1 MB), so the
// full-size source image is never held in memory.
#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace dis {

struct GrayRaster {
    int size = 0;                    // square: size x size
    int srcWidth = 0, srcHeight = 0; // original image size (for display)
    std::vector<uint8_t> px;         // 0 = black, 255 = white

    // x,y in [-1,1] (unit disc, y up). Outside the disc -> 1.0 (white/background).
    float sampleDisc(float x, float y) const;
};

// Supports uncompressed BMP (8/24/32 bpp). Centre-crops to a square.
// (PNG/JPEG: add a decoder later behind the same interface.)
bool loadBmpAsGrayRaster(const std::string& pathUtf8, int maxSize,
                         GrayRaster& out, std::string& error);
// Same, from an already-open file (caller closes it).
bool loadBmpFromFile(FILE* f, int maxSize, GrayRaster& out, std::string& error);

} // namespace dis
