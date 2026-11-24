#include "ImageSource.h"
#include "PlatformFile.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace dis {

float GrayRaster::sampleDisc(float x, float y) const {
    if (size < 2 || x * x + y * y > 1.0f) return 1.0f;
    float fx = (x * 0.5f + 0.5f) * (size - 1);
    float fy = (0.5f - y * 0.5f) * (size - 1);
    int x0 = (int)fx, y0 = (int)fy;
    if (x0 > size - 2) x0 = size - 2;
    if (y0 > size - 2) y0 = size - 2;
    float tx = fx - x0, ty = fy - y0;
    const uint8_t* p = &px[(size_t)y0 * size + x0];
    float a = p[0] * (1.0f - tx) + p[1] * tx;
    float b = p[size] * (1.0f - tx) + p[size + 1] * tx;
    return (a * (1.0f - ty) + b * ty) / 255.0f;
}

#pragma pack(push, 1)
struct BmpFileHeader { uint16_t type; uint32_t size; uint16_t r1, r2; uint32_t offBits; };
struct BmpInfoHeader {
    uint32_t size; int32_t w, h; uint16_t planes, bpp;
    uint32_t comp, sizeImg; int32_t xppm, yppm; uint32_t clrUsed, clrImp;
};
#pragma pack(pop)

bool loadBmpFromFile(FILE* f, int maxSize, GrayRaster& out, std::string& error) {

    BmpFileHeader fh; BmpInfoHeader ih;
    if (std::fread(&fh, sizeof fh, 1, f) != 1 || std::fread(&ih, sizeof ih, 1, f) != 1 ||
        fh.type != 0x4D42) { error = "not a BMP file"; return false; }
    if (ih.comp != 0 || (ih.bpp != 8 && ih.bpp != 24 && ih.bpp != 32) || ih.w <= 0 || ih.h == 0) {
        error = "unsupported BMP (need uncompressed 8/24/32 bpp)"; return false;
    }
    const int w = ih.w, h = std::abs(ih.h);
    const bool topDown = ih.h < 0;

    uint8_t pal[256];                                    // palette pre-converted to luma
    for (int i = 0; i < 256; ++i) pal[i] = (uint8_t)i;
    if (ih.bpp == 8) {
        int n = ih.clrUsed ? (int)ih.clrUsed : 256; if (n > 256) n = 256;
        std::fseek(f, 14 + (long)ih.size, SEEK_SET);
        for (int i = 0; i < n; ++i) {
            uint8_t q[4]; if (std::fread(q, 1, 4, f) != 4) break;
            pal[i] = (uint8_t)((q[2] * 299 + q[1] * 587 + q[0] * 114) / 1000);
        }
    }

    const int crop = std::min(w, h);
    const int cx = (w - crop) / 2, cy = (h - crop) / 2;
    const int S = std::max(2, std::min(maxSize, crop));  // never upscale -> no empty cells
    const size_t stride = (size_t)(((ih.bpp * w + 31) / 32) * 4);

    out.size = S;
    out.px.assign((size_t)S * S, 255);

    std::vector<uint8_t> row(stride);
    std::vector<float> sum(S, 0.f), cnt(S, 0.f);
    int curDy = -1;
    auto flush = [&]() {
        if (curDy < 0) return;
        for (int dx = 0; dx < S; ++dx) {
            float v = cnt[dx] > 0 ? sum[dx] / cnt[dx] : 255.f;
            out.px[(size_t)curDy * S + dx] = (uint8_t)(v + 0.5f);
            sum[dx] = 0.f; cnt[dx] = 0.f;
        }
    };

    for (int y = cy; y < cy + crop; ++y) {
        int dy = (int)((int64_t)(y - cy) * S / crop);
        if (dy != curDy) { flush(); curDy = dy; }
        int fy = topDown ? y : (h - 1 - y);
        std::fseek(f, (long)(fh.offBits + (uint64_t)fy * stride), SEEK_SET);
        if (std::fread(row.data(), 1, stride, f) != stride) { error = "truncated BMP"; return false; }
        for (int x = cx; x < cx + crop; ++x) {
            int v;
            if (ih.bpp == 8)       v = pal[row[x]];
            else if (ih.bpp == 24) { const uint8_t* q = &row[(size_t)x * 3]; v = (q[2] * 299 + q[1] * 587 + q[0] * 114) / 1000; }
            else                   { const uint8_t* q = &row[(size_t)x * 4]; v = (q[2] * 299 + q[1] * 587 + q[0] * 114) / 1000; }
            int dx = (int)((int64_t)(x - cx) * S / crop);
            sum[dx] += (float)v; cnt[dx] += 1.f;
        }
    }
    flush();
    out.srcWidth = w; out.srcHeight = h;
    return true;
}

bool loadBmpAsGrayRaster(const std::string& path, int maxSize, GrayRaster& out, std::string& error) {
    FILE* f = openFileUtf8(path, "rb");
    if (!f) { error = "cannot open file"; return false; }
    bool ok = loadBmpFromFile(f, maxSize, out, error);
    std::fclose(f);
    return ok;
}

} // namespace dis
