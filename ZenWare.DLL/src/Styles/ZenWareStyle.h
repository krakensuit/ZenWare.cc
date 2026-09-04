#pragma once
#include <imgui.h>

// Применяет палитру ZenWare ко всему ImGui стилю
void ApplyZenWareStyle();

// Рисует логотип ZenWare с градиентом и свечением
void DrawZenWareLogo(ImDrawList* drawList, ImVec2 pos, float scale = 1.0f);

namespace ZenWare {
    bool Button(const char* label, ImVec2 size = ImVec2(0, 0));
    bool Toggle(const char* label, bool* v);
    void Tab(const char* label, int idx, int* active);
}
