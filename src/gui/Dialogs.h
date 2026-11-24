#pragma once
#include <string>
namespace dis {
// Native "open file" dialog for images. Returns a UTF-8 path, or "" if cancelled / unsupported.
std::string openImageDialog(void* ownerHwnd);
}
