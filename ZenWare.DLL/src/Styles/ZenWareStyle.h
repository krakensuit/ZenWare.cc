#pragma once
#include <imgui.h>

// Applies the ZenWare palette to the entire ImGui style
void ApplyZenWareStyle();

// Draws the ZenWare logo with a gradient and glow
void DrawZenWareLogo(ImDrawList* drawList, ImVec2 pos, float scale = 1.0f);

namespace ZenWare {
    bool Button(const char* label, ImVec2 size = ImVec2(0, 0));
    bool Toggle(const char* label, bool* v);
    void Tab(const char* label, int idx, int* active);
}
