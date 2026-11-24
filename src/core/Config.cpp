#include "Config.h"
#include "PlatformFile.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace dis {

void AppConfig::clampValues() {
    speedX = std::max(1, std::min(52, speedX));
    static const int ok[] = { 49, 98, 147, 196, 294, 392, 588 };
    bool valid = false; for (size_t i = 0; i < sizeof ok / sizeof ok[0]; ++i) valid = valid || ok[i] == cellBytes;
    if (!valid) cellBytes = 196;
    brightness = std::max(-0.5f, std::min(0.5f, brightness));
    contrast   = std::max(0.2f, std::min(3.0f, contrast));
    gamma      = std::max(0.3f, std::min(3.0f, gamma));
    if (angleOffsetDeg < -360.0 || angleOffsetDeg > 360.0) angleOffsetDeg = 0.0;
    imageRadiusMm = std::max(30.0, std::min(58.0, imageRadiusMm));
    bandSectors = std::max(100, std::min(20000, bandSectors));
    altBlockSectors = std::max(1, std::min(500, altBlockSectors));
    altMinutes = std::max(1, std::min(20, altMinutes));
}

static std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r' || s[a] == '\n')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r' || s[b - 1] == '\n')) --b;
    return s.substr(a, b - a);
}

static bool parseByte(const std::string& v, uint8_t& out) {
    char* end = 0;
    long x = std::strtol(v.c_str(), &end, 16);       // accepts "EE" and "0xEE"
    if (end == v.c_str() || x < 0 || x > 255) return false;
    out = (uint8_t)x; return true;
}

bool loadConfig(const std::string& path, AppConfig& cfg) {
    std::vector<uint8_t> data;
    if (!readFileUtf8(path, data, 1 << 20)) return false;
    std::string text(data.begin(), data.end());
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t nl = text.find('\n', pos);
        std::string line = trim(text.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos));
        pos = (nl == std::string::npos) ? text.size() + 1 : nl + 1;
        if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = trim(line.substr(0, eq)), v = trim(line.substr(eq + 1));
        uint8_t b;
        if      (k == "light_byte")       { if (parseByte(v, b)) cfg.lightByte = b; }
        else if (k == "dark_byte")        { if (parseByte(v, b)) cfg.darkByte = b; }
        else if (k == "test_write_passed"){ cfg.testWritePassed = std::atoi(v.c_str()) != 0; }
        else if (k == "speed_x")          { cfg.speedX = std::atoi(v.c_str()); }
        else if (k == "cell_bytes")       { cfg.cellBytes = std::atoi(v.c_str()); }
        else if (k == "kernel")           { cfg.jjn = (v == "jjn"); }
        else if (k == "brightness")       { cfg.brightness = (float)std::atof(v.c_str()); }
        else if (k == "contrast")         { cfg.contrast = (float)std::atof(v.c_str()); }
        else if (k == "gamma")            { cfg.gamma = (float)std::atof(v.c_str()); }
        else if (k == "angle_offset_deg") { cfg.angleOffsetDeg = std::atof(v.c_str()); }
        else if (k == "mirror")           { cfg.mirror = std::atoi(v.c_str()) != 0; }
        else if (k == "image_radius_mm")  { cfg.imageRadiusMm = std::atof(v.c_str()); }
        else if (k == "band_sectors")     { cfg.bandSectors = std::atoi(v.c_str()); }
        else if (k == "alt_block_sectors"){ cfg.altBlockSectors = std::atoi(v.c_str()); }
        else if (k == "alt_minutes")      { cfg.altMinutes = std::atoi(v.c_str()); }
    }
    cfg.clampValues();
    return true;
}

bool saveConfig(const std::string& path, const AppConfig& c) {
    FILE* f = openFileUtf8(path, "wb");
    if (!f) return false;
    std::fprintf(f, "# DiscImageStudio settings\r\n");
    std::fprintf(f, "[tones]\r\nlight_byte=0x%02X\r\ndark_byte=0x%02X\r\n\r\n", c.lightByte, c.darkByte);
    std::fprintf(f, "[burn]\r\nspeed_x=%d\r\ntest_write_passed=%d\r\n\r\n", c.speedX, c.testWritePassed ? 1 : 0);
    std::fprintf(f, "[image]\r\ncell_bytes=%d\r\nkernel=%s\r\nbrightness=%.3f\r\ncontrast=%.3f\r\ngamma=%.3f\r\n"
                    "angle_offset_deg=%.3f\r\nmirror=%d\r\nimage_radius_mm=%.2f\r\n\r\n",
                 c.cellBytes, c.jjn ? "jjn" : "fs", c.brightness, c.contrast, c.gamma,
                 c.angleOffsetDeg, c.mirror ? 1 : 0, c.imageRadiusMm);
    std::fprintf(f, "[calibration]\r\nband_sectors=%d\r\nalt_block_sectors=%d\r\nalt_minutes=%d\r\n",
                 c.bandSectors, c.altBlockSectors, c.altMinutes);
    return std::fclose(f) == 0;
}

} // namespace dis
