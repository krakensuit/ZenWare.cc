#pragma once
#include "../../SDK/SDK.h"
#include <set>
#include <string>
#include <vector>

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
    void OnTick(); // опрос смертей (ивентов в движке нет) — тоже из Paint
    void Clear() { m_aEntries.clear(); m_alive.clear(); }
private:
    // Имя пин-атакующего (язык/прыгун/жокей/толстяк-носильщик/громила),
    // если именно он держит жертву в момент смерти.
    bool PinKillerName(C_TerrorPlayer* pVictim, char* szOut, size_t nOut);
    std::vector<KillEntry_t> m_aEntries;
    std::set<int> m_alive; // entindex живых именованных игроков
};

namespace F { inline CFeatures_Killfeed Killfeed; }
