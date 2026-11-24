// PlatformFile.h - file access that handles non-ASCII (e.g. Arabic) paths on Windows.
// All paths in the core/GUI layers are UTF-8 std::string.
#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace dis {

FILE* openFileUtf8(const std::string& pathUtf8, const char* mode);   // mode: "rb", "wb"
bool  readFileUtf8(const std::string& pathUtf8, std::vector<uint8_t>& out, size_t maxBytes);

} // namespace dis
