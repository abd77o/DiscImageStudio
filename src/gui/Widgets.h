// Widgets.h - small custom-drawn widgets (cards, toggle, buttons, icons, progress bar).
// Icons are vector-drawn with ImDrawList primitives, so no icon font / image files are needed.
#pragma once
#include "Theme.h"
#include <string>

namespace ui {

enum Icon { kIconNone = 0, kIconDisc, kIconImage, kIconSliders, kIconGear, kIconRefresh,
            kIconInfo, kIconPlay, kIconFlask, kIconLock, kIconCheck, kIconX };

void drawIcon(ImDrawList* dl, Icon ic, ImVec2 center, float size, ImU32 col);

bool beginCard(const char* id, ImVec2 size, const char* title, Icon icon);   // always call endCard()
void endCard();

bool toggle(const char* id, bool* value, bool disabled);
bool primaryButton(const char* label, ImVec2 size, Icon icon);
bool secondaryButton(const char* label, ImVec2 size);
bool iconButton(const char* id, Icon icon, float size, const char* tooltip);
void infoTip(const char* id, const char* text);          // (i) icon, tooltip on hover
void tooltipFor(const char* text);                       // tooltip for the previous item
void progressBar(float fraction, ImVec2 size, const char* label);
void dot(ImU32 color, float radius);                      // inline status dot
std::string ellipsize(const std::string& s, float maxWidth);

} // namespace ui
