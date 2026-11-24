// Config.h - tiny key=value settings file (config.ini).
#pragma once
#include <cstdint>
#include <string>

namespace dis {

struct AppConfig {
    // tone bytes found with the calibration ladder
    uint8_t lightByte = 0xEE;
    uint8_t darkByte  = 0x11;
    // safety: real burning stays locked until one test write has succeeded
    bool    testWritePassed = false;
    int     speedX    = 4;
    // image / dither
    int     cellBytes = 196;
    bool    jjn       = false;          // false = Floyd-Steinberg
    float   brightness = 0.0f, contrast = 1.0f, gamma = 1.0f;
    double  angleOffsetDeg = 0.0;
    bool    mirror    = false;
    double  imageRadiusMm = 58.0;
    // calibration
    int     bandSectors = 5000;         // ladder: sectors per candidate byte
    int     altBlockSectors = 10;       // alternating test: sectors per light/dark block
    int     altMinutes = 3;

    void clampValues();
};

// Missing file -> returns false and leaves cfg untouched (defaults). Unknown keys are ignored.
bool loadConfig(const std::string& pathUtf8, AppConfig& cfg);
bool saveConfig(const std::string& pathUtf8, const AppConfig& cfg);

} // namespace dis
