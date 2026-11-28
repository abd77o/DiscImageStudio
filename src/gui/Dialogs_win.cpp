#ifdef _WIN32
#include "Dialogs.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>

namespace dis {

std::string openImageDialog(void* owner) {
    wchar_t file[MAX_PATH * 2] = {0};
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof ofn);
    ofn.lStructSize = sizeof ofn;
    ofn.hwndOwner   = (HWND)owner;
    ofn.lpstrFilter = L"Images (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0All files\0*.*\0";
    ofn.lpstrFile   = file;
    ofn.nMaxFile    = (DWORD)(sizeof file / sizeof file[0]);
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
    if (!GetOpenFileNameW(&ofn)) return std::string();
    int n = WideCharToMultiByte(CP_UTF8, 0, file, -1, NULL, 0, NULL, NULL);
    if (n <= 1) return std::string();
    std::string out((size_t)n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, file, -1, &out[0], n, NULL, NULL);
    out.resize((size_t)n - 1);
    return out;
}

void openInDefaultApp(void* owner, const std::string& path) {
    int n = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, NULL, 0);
    if (n <= 1) return;
    std::wstring w((size_t)n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &w[0], n);
    ShellExecuteW((HWND)owner, L"open", w.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

} // namespace dis
#endif
