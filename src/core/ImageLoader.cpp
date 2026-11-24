#include "ImageLoader.h"
#include "PlatformFile.h"
#include <algorithm>
#include <cstdio>
#include <vector>

#ifndef DIS_NO_STB
#include "stb_image.h"          // declarations only; the implementation lives in stb_impl.cpp
#endif

namespace dis {

bool buildRasterFromGray(const uint8_t* gray, int w, int h, int maxSize, GrayRaster& out) {
    if (!gray || w < 2 || h < 2) return false;
    const int crop = std::min(w, h);
    const int cx = (w - crop) / 2, cy = (h - crop) / 2;
    const int S = std::max(2, std::min(maxSize, crop));   // never upscale
    out.size = S;
    out.px.assign((size_t)S * S, 255);
    out.srcWidth = w; out.srcHeight = h;

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
        const uint8_t* row = gray + (size_t)y * w;
        for (int x = cx; x < cx + crop; ++x) {
            int dx = (int)((int64_t)(x - cx) * S / crop);
            sum[dx] += row[x]; cnt[dx] += 1.f;
        }
    }
    flush();
    return true;
}

bool loadImageAsGrayRaster(const std::string& path, int maxSize, GrayRaster& out, std::string& error) {
    FILE* f = openFileUtf8(path, "rb");
    if (!f) { error = "cannot open file"; return false; }
    uint8_t magic[8] = {0};
    size_t got = std::fread(magic, 1, sizeof magic, f);
    std::fseek(f, 0, SEEK_SET);

    if (got >= 2 && magic[0] == 'B' && magic[1] == 'M') {          // BMP: streamed row by row
        bool ok = loadBmpFromFile(f, maxSize, out, error);
        std::fclose(f);
        return ok;
    }
    std::fclose(f);

#ifdef DIS_NO_STB
    error = "this build has no PNG/JPEG support (stb_image.h was not found at build time)";
    return false;
#else
    const bool isPng = got >= 8 && magic[0] == 0x89 && magic[1] == 'P' && magic[2] == 'N' && magic[3] == 'G';
    const bool isJpg = got >= 3 && magic[0] == 0xFF && magic[1] == 0xD8 && magic[2] == 0xFF;
    if (!isPng && !isJpg) { error = "unsupported format (use PNG, JPEG or BMP)"; return false; }

    std::vector<uint8_t> file;
    if (!readFileUtf8(path, file, 128u * 1024 * 1024)) { error = "cannot read file (empty or over 128 MB)"; return false; }

    int w = 0, h = 0, comp = 0;
    if (!stbi_info_from_memory(&file[0], (int)file.size(), &w, &h, &comp)) {
        error = std::string("decode error: ") + stbi_failure_reason(); return false;
    }
    if ((int64_t)w * h > kMaxPixels) {
        char b[96]; std::snprintf(b, sizeof b, "image too large (%.1f MP, limit 40 MP)", (double)w * h / 1e6);
        error = b; return false;
    }
    // Ask for gray+alpha (2 bytes/pixel) so transparent PNG areas can be composited over white.
    unsigned char* px = stbi_load_from_memory(&file[0], (int)file.size(), &w, &h, &comp, 2);
    if (!px) { error = std::string("decode error: ") + stbi_failure_reason(); return false; }
    std::vector<uint8_t>().swap(file);                       // free compressed data early

    const size_t n = (size_t)w * h;
    for (size_t i = 0; i < n; ++i) {                          // in place: index i <= 2i is safe
        unsigned g = px[2 * i], a = px[2 * i + 1];
        px[i] = (uint8_t)((g * a + 255u * (255u - a) + 127u) / 255u);
    }
    bool ok = buildRasterFromGray(px, w, h, maxSize, out);
    stbi_image_free(px);
    if (!ok) error = "image too small";
    return ok;
#endif
}

} // namespace dis
