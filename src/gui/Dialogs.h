#pragma once
#include <string>
namespace dis {
// Native "open file" dialog for images. Returns a UTF-8 path, or "" if cancelled / unsupported.
std::string openImageDialog(void* ownerHwnd);
// Opens a file with its default program (the burn log opens in Notepad). No-op if unsupported.
void openInDefaultApp(void* ownerHwnd, const std::string& pathUtf8);
}
