// The single translation unit that compiles stb_image (PNG + JPEG only, decode from memory).
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_NO_STDIO
#include "stb_image.h"
