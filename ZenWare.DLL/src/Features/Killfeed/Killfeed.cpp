#include "Killfeed.h"
#include "../Lang/Lang.h"
#include "../../SDK/DrawManager/DrawManager.h"
#include "../../Util/Anim/Anim.h"

void CFeatures_Killfeed::Push(const char* killer, const char* victim, const char* weapon) {
    if (!killer || !victim || !weapon) return;
    if (!killer[0] || !victim[0]) return;
    if (!I::GlobalVars) return;
    KillEntry_t e;
    e.killer = killer;
    e.victim = victim;
    e.weapon = weapon;
    e.t = I::GlobalVars->curtime;
    e.curX = (float)G::Draw.m_nScreenW + 40.0f;
    m_aEntries.push_back(std::move(e));
    if (m_aEntries.size() > 6) m_aEntries.erase(m_aEntries.begin());
}

void CFeatures_Killfeed::Draw() {
    if (m_aEntries.empty() || !G::Draw.m_nScreenW || !I::GlobalVars) return;
    float y = 60.0f;
    float now = I::GlobalVars->curtime;
    float dt = I::GlobalVars->frametime;
    if (dt <= 0.0f || dt > 0.1f) dt = 0.016f;

    for (int i = (int)m_aEntries.size() - 1; i >= 0; --i) {
        auto &e = m_aEntries[i];
        float age = now - e.t;
        if (age > 3.0f) { m_aEntries.erase(m_aEntries.begin() + i); continue; }
        float appear = Anim::EaseOutCubic(std::clamp(age / 0.25f, 0.0f, 1.0f));
        float vanish = age > 2.5f ? 1.0f - std::clamp((age - 2.5f) / 0.5f, 0.0f, 1.0f) : 1.0f;
        float alpha = appear * vanish;
        if (alpha <= 0.01f) continue;
        float targetX = (float)G::Draw.m_nScreenW - 320.0f;
        e.curX = Anim::Lerp(e.curX, targetX, 1.0f - expf(-10.0f * dt));
        float yOff = (1.0f - appear) * -16.0f;
        int ix = (int)e.curX;
        int iy = (int)(y + yOff);
        int w = 300, h = 26;
        Color bg(14, 16, 15, (int)(210 * alpha));
        Color border(0, 255, 171, (int)(255 * alpha));
        G::Draw.Rect(ix, iy, w, h, bg);
        G::Draw.Rect(ix, iy, 3, h, border);
        // Name rendering via G::Draw
        G::Draw.String(EFonts::ESP_NAME, ix + 10, iy + 6, Color(0,255,171,(int)(255*alpha)), TXT_DEFAULT, "%s", e.killer.c_str());
        G::Draw.String(EFonts::ESP, ix + 110, iy + 7, Color(255,255,255,(int)(230*alpha)), TXT_DEFAULT, "%s", Lang::T("killed"));
        G::Draw.String(EFonts::ESP, ix + 160, iy + 7, Color(255,80,80,(int)(255*alpha)), TXT_DEFAULT, "%s", e.victim.c_str());
        y += 34.0f;
    }
}
