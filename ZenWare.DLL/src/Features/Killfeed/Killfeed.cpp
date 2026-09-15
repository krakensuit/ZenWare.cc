#include "Killfeed.h"
#include "../../Util/Logger/Logger.h"
#include "../Lang/Lang.h"
#include "../Vars.h"
#include "../../SDK/DrawManager/DrawManager.h"
#include "../../Util/Anim/Anim.h"

void CFeatures_Killfeed::Push(const char* killer, const char* victim, const char* weapon) {
    if (!victim || !victim[0]) return;
    if (!killer) killer = "";
    if (!weapon) weapon = "";
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
        //Negative age (curtime rollback on map change) would otherwise give
        //alpha 0 forever and occupy one of the 6 slots permanently.
        if (age > 3.0f || age < -0.5f) { m_aEntries.erase(m_aEntries.begin() + i); continue; }
        float appear = Anim::EaseOutCubic(std::clamp(age / 0.25f, 0.0f, 1.0f));
        float vanish = age > 2.5f ? 1.0f - std::clamp((age - 2.5f) / 0.5f, 0.0f, 1.0f) : 1.0f;
        float alpha = appear * vanish;
        if (alpha <= 0.01f) continue;
        // Card behind the text: width from measurements, pinned to the right edge.
        const bool bKill = !e.killer.empty();
        const char* szWord = Lang::T(bKill ? "killed" : "died");
        const int wKill = bKill ? G::Draw.GetTextWidth(EFonts::ESP_NAME, e.killer.c_str()) : 0;
        const int wWord = G::Draw.GetTextWidth(EFonts::ESP, szWord);
        const int wVict = G::Draw.GetTextWidth(EFonts::ESP_NAME, e.victim.c_str());
        int w = 12 + wKill + (bKill ? 6 : 0) + wWord + 6 + wVict + 12;
        if (w < 200) w = 200;
        if (w > 560) w = 560;
        const int h = 30;
        const float targetX = (float)G::Draw.m_nScreenW - (float)w - 16.0f;
        e.curX = Anim::Lerp(e.curX, targetX, 1.0f - expf(-10.0f * dt));
        float yOff = (1.0f - appear) * -16.0f;
        int ix = (int)e.curX;
        int iy = (int)(y + yOff);
        const int nA = (int)(255 * alpha);
        // Shadow + gradient body + outline + accent bar on the left.
        G::Draw.Rect(ix + 2, iy + 2, w, h, Color(0, 0, 0, (int)(110 * alpha)));
        G::Draw.GradientRect(ix, iy, ix + w, iy + h, Color(22, 24, 23, (int)(215 * alpha)), Color(10, 11, 10, (int)(215 * alpha)), false);
        G::Draw.OutlinedRect(ix, iy, w, h, Color(0, 0, 0, (int)(200 * alpha)));
        if (bKill)
            G::Draw.Rect(ix + 1, iy + 1, 3, h - 2, Color(0, 255, 171, nA));
        else
            G::Draw.Rect(ix + 1, iy + 1, 3, h - 2, Color(120, 120, 120, nA));
        int tx = ix + 12;
        const int ty = iy + 8;
        if (bKill)
        {
            G::Draw.String(EFonts::ESP_NAME, tx, ty, Color(0, 255, 171, nA), TXT_DEFAULT, "%s", e.killer.c_str());
            tx += wKill + 6;
        }
        G::Draw.String(EFonts::ESP, tx, ty + 1, Color(200, 205, 200, (int)(230 * alpha)), TXT_DEFAULT, "%s", szWord);
        tx += wWord + 6;
        G::Draw.String(EFonts::ESP_NAME, tx, ty, Color(255, 110, 110, nA), TXT_DEFAULT, "%s", e.victim.c_str());
        y += (float)(h + 8);
    }
}

bool CFeatures_Killfeed::PinKillerName(C_TerrorPlayer* pVictim, char* szOut, size_t nOut)
{
    if (!pVictim || !szOut || !nOut) return false;
    szOut[0] = '\0';
    // Whoever holds the victim in a pin almost certainly landed the kill.
    const EHANDLE hPins[] = {
        pVictim->m_tongueOwner(), pVictim->m_pounceAttacker(),
        pVictim->m_jockeyAttacker(), pVictim->m_carryAttacker(), pVictim->m_pummelAttacker()
    };
    for (const EHANDLE& h : hPins)
    {
        if (!h.IsValid()) continue;
        IClientEntity* pEnt = I::ClientEntityList->GetClientEntityFromHandle(h);
        //A handle from a netvar with nOff=0 reads the vtable and can become
        //"valid" garbage pointing at a weapon/prop — without the ClassID gate
        //this goes on to virtual calls on a foreign type.
        if (!pEnt || !G::Util.IsPlayerEntity(pEnt)) continue;
        player_info_t pi = {};
        if (I::EngineClient->GetPlayerInfo(h.GetEntryIndex(), &pi) && pi.name[0])
        {
            pi.name[31] = '\0';
            strcpy_s(szOut, nOut, pi.name);
            return true;
        }
    }
    return false;
}

void CFeatures_Killfeed::OnTick()
{
	U::Log.Crumb("Killfeed::OnTick");
    if (!Vars::Killfeed::bEnabled || !I::EngineClient || !I::EngineClient->IsInGame())
    {
        if (!m_alive.empty() || !m_aEntries.empty()) { m_alive.clear(); m_aEntries.clear(); }
        return;
    }

    std::set<int> aliveNow;
    const int nMax = I::ClientEntityList ? I::ClientEntityList->GetMaxEntities() : 0;
    for (int n = 1; n <= nMax; n++)
    {
        IClientEntity* pEntity = I::ClientEntityList->GetClientEntity(n);
        if (!pEntity || pEntity->IsDormant()) continue;
        // Cheap class filter first: GetPlayerInfo over all 2k slots every frame
        // is an unnecessary storm of engine calls. The name is only needed at
        // the moment of death (for the killfeed line); the living are tracked by index.
        ClientClass* pCC = pEntity->GetClientClass();
        if (!pCC) continue;
        const int nID = pCC->m_ClassID;
        //SI deaths go into the feed too: special classes + boomer by name (no ID in the dump).
        //GetPlayerInfo returns false for bot SIs — such lines are silently skipped.
        if (nID != CTerrorPlayer && nID != SurvivorBot && nID != Tank
            && nID != Hunter && nID != Smoker && nID != Jockey && nID != Spitter
            && nID != Charger && nID != Witch && !G::Util.IsSpecialByName(pCC->m_pNetworkName))
            continue;
        //SIs are C_TerrorPlayer siblings: only C_BasePlayer/C_BaseEntity netvars,
        //no GetHealth virtual calls on foreign objects.
        const bool bIsSI = (nID == Hunter || nID == Smoker || nID == Jockey
            || nID == Spitter || nID == Charger || nID == Witch
            || G::Util.IsSpecialByName(pCC->m_pNetworkName));
        C_TerrorPlayer* pPlayer = nullptr;
        bool bAlive = false;
        if (bIsSI)
        {
            C_BasePlayer* pBase = pEntity->As<C_BasePlayer*>();
            if (!pBase) continue;
            bAlive = !pBase->deadflag() && pBase->m_lifeState() == 0;
        }
        else
        {
            pPlayer = pEntity->As<C_TerrorPlayer*>();
            if (!pPlayer) continue;
            bAlive = !pPlayer->deadflag() && pPlayer->m_lifeState() == 0 && pPlayer->GetHealth() > 0;
        }
        if (bAlive)
        {
            aliveNow.insert(n);
            continue;
        }
        if (m_alive.count(n))
        {
            //GetPlayerInfo expects a client slot: bot SIs live on indices above
            //maxclients. Name for players only, class name for everyone else.
            char szVictim[64] = { };
            const int nMaxClients = I::EngineClient->GetMaxClients();
            player_info_t pi = {};
            if (n >= 1 && n <= nMaxClients && I::EngineClient->GetPlayerInfo(n, &pi) && pi.name[0])
            {
                pi.name[31] = '\0';
                strcpy_s(szVictim, pi.name);
            }
            else if (pCC->m_pNetworkName)
            {
                strncpy_s(szVictim, pCC->m_pNetworkName, _TRUNCATE);
            }
            else continue;
            char szKiller[32] = { };
            PinKillerName(pPlayer, szKiller, sizeof(szKiller));
            Push(szKiller, szVictim, "");
        }
    }
    m_alive.swap(aliveNow);
}
