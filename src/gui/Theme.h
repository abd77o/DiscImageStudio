// Theme.h - palette, fonts and ImGui style for the dark UI.
#pragma once
#include "imgui.h"

namespace ui {

struct Fonts { ImFont* regular = 0; ImFont* bold = 0; ImFont* smallFont = 0; };
extern Fonts fonts;

void loadFonts();      // Segoe UI from C:\Windows\Fonts (falls back to ImGui's built-in font)
void applyTheme();     // call after ImGui::CreateContext()

inline ImU32 cBg()        { return IM_COL32(0x18, 0x1A, 0x20, 255); }
inline ImU32 cCard()      { return IM_COL32(0x22, 0x25, 0x2E, 255); }
inline ImU32 cField()     { return IM_COL32(0x2B, 0x2F, 0x3A, 255); }
inline ImU32 cFieldHot()  { return IM_COL32(0x35, 0x3A, 0x48, 255); }
inline ImU32 cAccent()    { return IM_COL32(0x3B, 0x82, 0xF6, 255); }
inline ImU32 cAccentHot() { return IM_COL32(0x5B, 0x98, 0xF8, 255); }
inline ImU32 cAccentDn()  { return IM_COL32(0x2A, 0x6C, 0xDC, 255); }
inline ImU32 cText()      { return IM_COL32(0xE6, 0xE8, 0xEE, 255); }
inline ImU32 cDim()       { return IM_COL32(0x8D, 0x93, 0xA3, 255); }
inline ImU32 cGood()      { return IM_COL32(0x34, 0xD3, 0x99, 255); }
inline ImU32 cWarn()      { return IM_COL32(0xFB, 0xBF, 0x24, 255); }
inline ImU32 cBad()       { return IM_COL32(0xF8, 0x71, 0x71, 255); }

inline ImVec4 vec(ImU32 c) { return ImGui::ColorConvertU32ToFloat4(c); }

} // namespace ui
