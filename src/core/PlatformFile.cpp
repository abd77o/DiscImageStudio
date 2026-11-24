#include "PlatformFile.h"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace dis {

FILE* openFileUtf8(const std::string& path, const char* mode) {
#ifdef _WIN32
    int n = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, NULL, 0);
    if (n <= 0) return NULL;
    std::vector<wchar_t> w((size_t)n);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &w[0], n);
    wchar_t m[8] = {0};
    for (int i = 0; i < 7 && mode[i]; ++i) m[i] = (wchar_t)mode[i];
    return _wfopen(&w[0], m);
#else
    return std::fopen(path.c_str(), mode);
#endif
}

bool readFileUtf8(const std::string& path, std::vector<uint8_t>& out, size_t maxBytes) {
    FILE* f = openFileUtf8(path, "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (sz <= 0 || (size_t)sz > maxBytes) { std::fclose(f); return false; }
    out.resize((size_t)sz);
    bool ok = std::fread(&out[0], 1, (size_t)sz, f) == (size_t)sz;
    std::fclose(f);
    return ok;
}

} // namespace dis
