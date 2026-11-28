#include "Widgets.h"
#include <algorithm>
#include <cmath>

namespace ui {

void drawIcon(ImDrawList* dl, Icon ic, ImVec2 c, float s, ImU32 col) {
    const float r = s * 0.5f, th = s >= 18.f ? 1.8f : 1.5f;
    switch (ic) {
    case kIconDisc:
        dl->AddCircle(c, r, col, 32, th);
        dl->AddCircle(c, r * 0.28f, col, 16, th);
        break;
    case kIconImage:
        dl->AddRect(ImVec2(c.x - r, c.y - r * 0.8f), ImVec2(c.x + r, c.y + r * 0.8f), col, 3.f, 0, th);
        dl->AddCircleFilled(ImVec2(c.x - r * 0.35f, c.y - r * 0.25f), r * 0.17f, col, 12);
        dl->AddTriangleFilled(ImVec2(c.x - r * 0.75f, c.y + r * 0.6f), ImVec2(c.x - r * 0.05f, c.y - r * 0.1f), ImVec2(c.x + r * 0.55f, c.y + r * 0.6f), col);
        break;
    case kIconSliders:
        for (int i = 0; i < 3; ++i) {
            float y = c.y + (i - 1) * r * 0.6f, kx = c.x + (i == 1 ? 0.35f : (i == 0 ? -0.3f : 0.1f)) * r;
            dl->AddLine(ImVec2(c.x - r, y), ImVec2(c.x + r, y), col, th);
            dl->AddCircleFilled(ImVec2(kx, y), r * 0.2f, col, 10);
        }
        break;
    case kIconGear:
        dl->AddCircle(c, r * 0.5f, col, 20, th);
        for (int i = 0; i < 8; ++i) {
            float a = i * 0.785398f;
            dl->AddLine(ImVec2(c.x + r * 0.7f * std::cos(a), c.y + r * 0.7f * std::sin(a)),
                        ImVec2(c.x + r * 0.98f * std::cos(a), c.y + r * 0.98f * std::sin(a)), col, th + 0.8f);
        }
        break;
    case kIconRefresh: {
        dl->PathArcTo(c, r * 0.75f, 0.7f, 5.4f, 24);
        dl->PathStroke(col, 0, th);
        ImVec2 e(c.x + r * 0.75f * std::cos(0.7f), c.y + r * 0.75f * std::sin(0.7f));
        dl->AddTriangleFilled(ImVec2(e.x + r * 0.5f, e.y - r * 0.1f), ImVec2(e.x - r * 0.35f, e.y - r * 0.25f), ImVec2(e.x + r * 0.05f, e.y + r * 0.5f), col);
        break; }
    case kIconInfo:
        dl->AddCircle(c, r, col, 24, th);
        dl->AddCircleFilled(ImVec2(c.x, c.y - r * 0.42f), r * 0.11f, col, 8);
        dl->AddLine(ImVec2(c.x, c.y - r * 0.1f), ImVec2(c.x, c.y + r * 0.5f), col, th);
        break;
    case kIconPlay:
        dl->AddTriangleFilled(ImVec2(c.x - r * 0.5f, c.y - r * 0.7f), ImVec2(c.x - r * 0.5f, c.y + r * 0.7f), ImVec2(c.x + r * 0.7f, c.y), col);
        break;
    case kIconFlask:
        dl->AddLine(ImVec2(c.x - r * 0.25f, c.y - r), ImVec2(c.x - r * 0.25f, c.y - r * 0.2f), col, th);
        dl->AddLine(ImVec2(c.x + r * 0.25f, c.y - r), ImVec2(c.x + r * 0.25f, c.y - r * 0.2f), col, th);
        dl->AddLine(ImVec2(c.x - r * 0.25f, c.y - r * 0.2f), ImVec2(c.x - r * 0.85f, c.y + r * 0.8f), col, th);
        dl->AddLine(ImVec2(c.x + r * 0.25f, c.y - r * 0.2f), ImVec2(c.x + r * 0.85f, c.y + r * 0.8f), col, th);
        dl->AddLine(ImVec2(c.x - r * 0.85f, c.y + r * 0.8f), ImVec2(c.x + r * 0.85f, c.y + r * 0.8f), col, th);
        break;
    case kIconLock:
        dl->AddRect(ImVec2(c.x - r * 0.7f, c.y - r * 0.05f), ImVec2(c.x + r * 0.7f, c.y + r * 0.85f), col, 2.f, 0, th);
        dl->PathArcTo(ImVec2(c.x, c.y - r * 0.05f), r * 0.45f, 3.14159f, 6.28318f, 12);
        dl->PathStroke(col, 0, th);
        break;
    case kIconCheck:
        dl->AddLine(ImVec2(c.x - r * 0.7f, c.y), ImVec2(c.x - r * 0.15f, c.y + r * 0.6f), col, th + 0.4f);
        dl->AddLine(ImVec2(c.x - r * 0.15f, c.y + r * 0.6f), ImVec2(c.x + r * 0.75f, c.y - r * 0.55f), col, th + 0.4f);
        break;
    case kIconX:
        dl->AddLine(ImVec2(c.x - r * 0.6f, c.y - r * 0.6f), ImVec2(c.x + r * 0.6f, c.y + r * 0.6f), col, th);
        dl->AddLine(ImVec2(c.x - r * 0.6f, c.y + r * 0.6f), ImVec2(c.x + r * 0.6f, c.y - r * 0.6f), col, th);
        break;
    default: break;
    }
}

bool beginCard(const char* id, ImVec2 size, const char* title, Icon icon) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, cCard());
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 10));
    // border=true only so the window padding is honoured (ChildBorderSize is 0 -> nothing drawn)
    bool open = ImGui::BeginChild(id, size, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    drawIcon(dl, icon, ImVec2(p.x + 8, p.y + 10), 16, cAccent());
    ImGui::SetCursorScreenPos(ImVec2(p.x + 24, p.y + 1));
    ImGui::PushFont(fonts.bold);
    ImGui::TextUnformatted(title);
    ImGui::PopFont();
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 30));
    return open;
}

void endCard() {
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

bool toggle(const char* id, bool* v, bool disabled) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = 38.f, h = 20.f;
    ImGui::InvisibleButton(id, ImVec2(w, h));
    const bool clicked = ImGui::IsItemClicked(0) && !disabled;
    if (clicked) *v = !*v;
    const bool hot = ImGui::IsItemHovered(0) && !disabled;
    ImU32 track = *v ? (hot ? cAccentHot() : cAccent()) : (hot ? cFieldHot() : cField());
    if (disabled) track = *v ? IM_COL32(0x2A, 0x4F, 0x8E, 255) : cField();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), track, h * 0.5f);
    dl->AddCircleFilled(ImVec2(*v ? p.x + w - h * 0.5f : p.x + h * 0.5f, p.y + h * 0.5f), h * 0.5f - 3.f,
                        disabled ? IM_COL32(190, 196, 210, 255) : IM_COL32(255, 255, 255, 255), 16);
    return clicked;
}

static bool coloredButton(const char* label, ImVec2 size, ImU32 base, ImU32 hot, ImU32 down, ImFont* font) {
    ImGui::PushStyleColor(ImGuiCol_Button, base);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hot);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, down);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);
    ImGui::PushFont(font);
    bool r = ImGui::Button(label, size);
    ImGui::PopFont();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
    return r;
}

bool primaryButton(const char* label, ImVec2 size, Icon icon) {
    bool r = coloredButton(label, size, cAccent(), cAccentHot(), cAccentDn(), fonts.bold);
    if (icon != kIconNone) {
        ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
        drawIcon(ImGui::GetWindowDrawList(), icon, ImVec2(a.x + 22, (a.y + b.y) * 0.5f), 16, IM_COL32(255, 255, 255, 255));
    }
    return r;
}

bool secondaryButton(const char* label, ImVec2 size) {
    return coloredButton(label, size, cField(), cFieldHot(), cAccentDn(), fonts.regular);
}

bool iconButton(const char* id, Icon icon, float size, const char* tooltip) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, ImVec2(size, size));
    const bool hot = ImGui::IsItemHovered(0);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (hot) dl->AddRectFilled(p, ImVec2(p.x + size, p.y + size), cField(), 8.f);
    drawIcon(dl, icon, ImVec2(p.x + size * 0.5f, p.y + size * 0.5f), size * 0.55f, hot ? cText() : cDim());
    if (hot && tooltip) tooltipFor(tooltip);
    return ImGui::IsItemClicked(0);
}

void tooltipFor(const char* text) {
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 22.f);
    ImGui::TextUnformatted(text);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

void infoTip(const char* id, const char* text) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, ImVec2(18, 18));
    const bool hot = ImGui::IsItemHovered(0);
    drawIcon(ImGui::GetWindowDrawList(), kIconInfo, ImVec2(p.x + 9, p.y + 9), 15, hot ? cAccent() : cDim());
    if (hot) tooltipFor(text);
}

void progressBar(float f, ImVec2 size, const char* label) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::Dummy(size);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float h = size.y;
    dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + h), cField(), h * 0.5f);
    f = f < 0.f ? 0.f : (f > 1.f ? 1.f : f);
    if (f > 0.f) {
        float w = std::max(h, size.x * f);
        dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), cAccent(), h * 0.5f);
    }
    if (label && label[0]) {
        ImGui::PushFont(fonts.smallFont);
        ImVec2 ts = ImGui::CalcTextSize(label);
        dl->AddText(ImVec2(p.x + (size.x - ts.x) * 0.5f, p.y + (h - ts.y) * 0.5f), IM_COL32(255, 255, 255, 255), label);
        ImGui::PopFont();
    }
}

void dot(ImU32 color, float r) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(r * 2 + 2, r * 2 + 2));
    ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(p.x + r + 1, p.y + 9), r, color, 12);
}

std::string ellipsize(const std::string& s, float maxW) {
    if (ImGui::CalcTextSize(s.c_str()).x <= maxW) return s;
    size_t lo = 0, hi = s.size();
    while (lo < hi) {                                  // longest prefix that fits with "..."
        size_t mid = (lo + hi + 1) / 2;
        if (ImGui::CalcTextSize((s.substr(0, mid) + "...").c_str()).x <= maxW) lo = mid; else hi = mid - 1;
    }
    return s.substr(0, lo) + "...";
}

} // namespace ui
