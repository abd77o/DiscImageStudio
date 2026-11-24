// ImageLoader.h - loads PNG / JPEG (stb_image) or BMP (streaming) into the square 8-bit
// working raster used by the dither engine, centre-cropped to a square.
#pragma once
#include "ImageSource.h"
#include <cstdint>
#include <string>

namespace dis {

// Format is detected from the file's first bytes, not its extension.
bool loadImageAsGrayRaster(const std::string& pathUtf8, int maxSize,
                           GrayRaster& out, std::string& error);

// Centre-crop to a square and box-filter down to <= maxSize. `gray` is w*h bytes.
bool buildRasterFromGray(const uint8_t* gray, int w, int h, int maxSize, GrayRaster& out);

const int64_t kMaxPixels = 40LL * 1000 * 1000;   // refuse larger images (RAM on weak PCs)

} // namespace dis
