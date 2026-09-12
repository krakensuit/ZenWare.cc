#include "Killfeed.h"
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
        //Отрицательный возраст (отмотка curtime при смене карты) иначе даёт
        //alpha 0 навсегда и вечно занимает слот из 6.
        if (age > 3.0f || age < -0.5f) { m_aEntries.erase(m_aEntries.begin() + i); continue; }
        float appear = Anim::EaseOutCubic(std::clamp(age / 0.25f, 0.0f, 1.0f));
        float vanish = age > 2.5f ? 1.0f - std::clamp((age - 2.5f) / 0.5f, 0.0f, 1.0f) : 1.0f;
        float alpha = appear * vanish;
        if (alpha <= 0.01f) continue;
        // Карточка под текст: ширина по замерам, прижата к правому краю.
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
        // Тень + градиентное тело + рамка + акцентная полоса слева.
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
    // Кто держит жертву пином — тот почти наверняка и добил.
    const EHANDLE hPins[] = {
        pVictim->m_tongueOwner(), pVictim->m_pounceAttacker(),
        pVictim->m_jockeyAttacker(), pVictim->m_carryAttacker(), pVictim->m_pummelAttacker()
    };
    for (const EHANDLE& h : hPins)
    {
        if (!h.IsValid()) continue;
        IClientEntity* pEnt = I::ClientEntityList->GetClientEntityFromHandle(h);
        //Хендл из нетвара при nOff=0 читает vtable и может стать «валидным»
        //мусором на оружие/проп — без ClassID-гейта дальше виртуалки по чужому типу.
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
        // Сначала дешёвый фильтр по классу: GetPlayerInfo по всем 2k слотам
        // каждый кадр — лишний шторм вызовов движка. Имя нужно только в
        // момент смерти (для строки киллфида), живые храним по индексу.
        ClientClass* pCC = pEntity->GetClientClass();
        if (!pCC) continue;
        const int nID = pCC->m_ClassID;
        //Смерти СИ тоже в ленту: классы особых + бумер по имени (ID нет в дампе).
        //GetPlayerInfo для ботов-СИ вернёт false — такие строки тихо пропускаются.
        if (nID != CTerrorPlayer && nID != SurvivorBot && nID != Tank
            && nID != Hunter && nID != Smoker && nID != Jockey && nID != Spitter
            && nID != Charger && nID != Witch && !G::Util.IsSpecialByName(pCC->m_pNetworkName))
            continue;
        //Ведьма — C_Infected, а не игрок: deadflag/lifeState из DT_BasePlayer
        //на ней читают чужие поля. Живость — через IsInfectedAlive как в ESP.
        C_TerrorPlayer* pPlayer = nullptr;
        bool bAlive = false;
        if (nID == Witch)
        {
            C_Infected* pWitch = pEntity->As<C_Infected*>();
            if (!pWitch) continue;
            bAlive = G::Util.IsInfectedAlive(pWitch->m_usSolidFlags(), pWitch->m_nSequence());
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
            //GetPlayerInfo ждёт клиент-слот: ботовые СИ живут на индексах
            //выше maxclients. Имя — только для игроков, остальным имя класса.
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
