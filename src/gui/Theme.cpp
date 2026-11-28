#include "Theme.h"
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace ui {

Fonts fonts;

static std::string winFontPath(const char* file) {
#ifdef _WIN32
    wchar_t dir[MAX_PATH] = {0};
    UINT n = GetWindowsDirectoryW(dir, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return std::string();
    std::wstring w = std::wstring(dir, n) + L"\\Fonts\\";
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, NULL, 0, NULL, NULL);
    std::string out((size_t)len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &out[0], len, NULL, NULL);
    out.resize((size_t)len - 1);
    return out + file;
#else
    (void)file; return std::string();
#endif
}

void loadFonts() {
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig fc;
    fc.OversampleH = 2; fc.OversampleV = 1; fc.PixelSnapH = true;   // small atlas = little RAM
    const std::string reg = winFontPath("segoeui.ttf"), bold = winFontPath("segoeuib.ttf");
    fonts.regular = reg.empty() ? 0 : io.Fonts->AddFontFromFileTTF(reg.c_str(), 15.0f, &fc);
    if (!fonts.regular) fonts.regular = io.Fonts->AddFontDefault();
    fonts.bold  = bold.empty() ? 0 : io.Fonts->AddFontFromFileTTF(bold.c_str(), 16.0f, &fc);
    if (!fonts.bold) fonts.bold = fonts.regular;
    fonts.smallFont = reg.empty() ? 0 : io.Fonts->AddFontFromFileTTF(reg.c_str(), 12.5f, &fc);
    if (!fonts.smallFont) fonts.smallFont = fonts.regular;
}

void applyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 0; s.ChildRounding = 12; s.FrameRounding = 8; s.PopupRounding = 12;
    s.GrabRounding = 8; s.ScrollbarRounding = 8;
    s.WindowPadding = ImVec2(14, 12); s.FramePadding = ImVec2(10, 6); s.ItemSpacing = ImVec2(8, 8);
    s.FrameBorderSize = 0; s.ChildBorderSize = 0; s.PopupBorderSize = 1; s.WindowBorderSize = 0;
    s.ScrollbarSize = 10; s.GrabMinSize = 14;

    ImVec4* c = s.Colors;
    c[ImGuiCol_Text]                 = vec(cText());
    c[ImGuiCol_TextDisabled]         = vec(cDim());
    c[ImGuiCol_WindowBg]             = vec(cBg());
    c[ImGuiCol_ChildBg]              = vec(cCard());
    c[ImGuiCol_PopupBg]              = vec(cCard());
    c[ImGuiCol_Border]               = vec(IM_COL32(0x35, 0x3A, 0x48, 255));
    c[ImGuiCol_FrameBg]              = vec(cField());
    c[ImGuiCol_FrameBgHovered]       = vec(cFieldHot());
    c[ImGuiCol_FrameBgActive]        = vec(cFieldHot());
    c[ImGuiCol_ScrollbarBg]          = vec(IM_COL32(0, 0, 0, 0));
    c[ImGuiCol_ScrollbarGrab]        = vec(cField());
    c[ImGuiCol_ScrollbarGrabHovered] = vec(cFieldHot());
    c[ImGuiCol_ScrollbarGrabActive]  = vec(cAccent());
    c[ImGuiCol_CheckMark]            = vec(cAccent());
    c[ImGuiCol_SliderGrab]           = vec(cAccent());
    c[ImGuiCol_SliderGrabActive]     = vec(cAccentHot());
    c[ImGuiCol_Button]               = vec(cField());
    c[ImGuiCol_ButtonHovered]        = vec(cFieldHot());
    c[ImGuiCol_ButtonActive]         = vec(cAccentDn());
    c[ImGuiCol_Header]               = vec(cField());
    c[ImGuiCol_HeaderHovered]        = vec(cFieldHot());
    c[ImGuiCol_HeaderActive]         = vec(cAccentDn());
    c[ImGuiCol_Separator]            = vec(IM_COL32(0x35, 0x3A, 0x48, 255));
    c[ImGuiCol_ModalWindowDimBg]     = vec(IM_COL32(0, 0, 0, 150));
}

} // namespace ui
