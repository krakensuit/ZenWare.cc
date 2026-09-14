#pragma once
#include "../../SDK/SDK.h"
#include <set>
#include <string>
#include <vector>

struct KillEntry_t {
    std::string killer;
    std::string victim;
    std::string weapon;
    float t = 0.0f;      // ImGui::GetTime() at creation time
    float curX = 0.0f;   // current X for the slide animation
};

class CFeatures_Killfeed {
public:
    void Push(const char* killer, const char* victim, const char* weapon);
    void Draw(); // call every frame from Paint (outside the HUD)
    void OnTick(); // death polling (no engine events) — also from Paint
    void Clear() { m_aEntries.clear(); m_alive.clear(); }
private:
    // Name of the pin attacker (tongue/pounce/jockey/carry/pummel holder),
    // if it is the one holding the victim at the moment of death.
    bool PinKillerName(C_TerrorPlayer* pVictim, char* szOut, size_t nOut);
    std::vector<KillEntry_t> m_aEntries;
    std::set<int> m_alive; // entindexes of living named players
};

namespace F { inline CFeatures_Killfeed Killfeed; }
