#pragma once
#include "../../SDK/SDK.h"

struct KillEntry_t {
    std::string killer;
    std::string victim;
    std::string weapon;
    float t = 0.0f;      // ImGui::GetTime() на момент создания
    float curX = 0.0f;   // текущая X для slide-анимации
};

class CFeatures_Killfeed {
public:
    void Push(const char* killer, const char* victim, const char* weapon);
    void Draw(); // вызывать каждый кадр из Paint (вне HUD)
    void Clear() { m_aEntries.clear(); }
private:
    std::vector<KillEntry_t> m_aEntries;
};

namespace F { inline CFeatures_Killfeed Killfeed; }
