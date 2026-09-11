#include "Hitmarker.h"
#include "../Vars.h"
#include "../../SDK/DrawManager/DrawManager.h"
#include "../../Util/Logger/Logger.h"

#include <cctype>
#include <cmath>
#include <cstring>
#include <thread>

namespace
{
	// Бумера нет в дампе ID: опознаём по имени класса, как Aimbot/ESP.
	bool IsBoomerByName(const char* szNet)
	{
		if (!szNet || !szNet[0])
			return false;
		char szLower[64] = { };
		int i = 0;
		for (; i < 63 && szNet[i]; i++)
			szLower[i] = (char)tolower((unsigned char)szNet[i]);
		szLower[i] = '\0';
		return strstr(szLower, "boomer") != nullptr;
	}
	// Best-effort опознание shootable-врага, зеркалит классификацию Aimbot:
	// игроки чужой команды, особые, обычные + ведьма. Возвращает HP и якорь.
	bool GetEnemyHp(IClientEntity* pEntity, C_TerrorPlayer* pLocal, int& nHpOut, Vector& vAnchorOut)
	{
		ClientClass* pCC = pEntity->GetClientClass();
		if (!pCC)
			return false;

		const int nID = pCC->m_ClassID;

		// Выжившие/боты чужой команды (+танк за игроков).
		if (nID == CTerrorPlayer || nID == SurvivorBot || nID == Tank)
		{
			C_TerrorPlayer* pPlayer = pEntity->As<C_TerrorPlayer*>();
			if (!G::Util.IsValidTarget(pLocal, pPlayer, false))
				return false;
			nHpOut = pPlayer->GetHealth();
			vAnchorOut = pPlayer->m_vecOrigin() + Vector(0.0f, 0.0f, pPlayer->m_vecMaxs().z * 0.7f);
			return nHpOut >= 0;
		}

		// Особые заражённые (+бумер по имени: его ID нет в дампе).
		if (nID == Hunter || nID == Smoker || nID == Jockey || nID == Spitter || nID == Charger || IsBoomerByName(pCC->m_pNetworkName))
		{
			C_BaseEntity* pEnt = pEntity->As<C_BaseEntity*>();
			C_BasePlayer* pPl = pEntity->As<C_BasePlayer*>();
			if (!pEnt || !pPl || pPl->m_lifeState() != 0)
				return false;
			const int nTeam = pEnt->m_iTeamNum();
			if ((nTeam != TEAM_SURVIVOR && nTeam != TEAM_INFECTED) || nTeam == pLocal->GetTeamNumber())
				return false;
			nHpOut = pEnt->GetHealth();
			vAnchorOut = pEnt->m_vecOrigin() + Vector(0.0f, 0.0f, pEnt->m_vecMaxs().z * 0.7f);
			return nHpOut > 0;
		}

		// Обычные + ведьма.
		if (nID == Infected || nID == Witch)
		{
			C_BaseEntity* pEnt = pEntity->As<C_BaseEntity*>();
			C_Infected* pInf = pEntity->As<C_Infected*>();
			if (!pEnt || !pInf)
				return false;
			if (!G::Util.IsInfectedAlive(pInf->m_usSolidFlags(), pInf->m_nSequence()))
				return false;
			nHpOut = pEnt->GetHealth();
			vAnchorOut = pEnt->m_vecOrigin() + Vector(0.0f, 0.0f, pEnt->m_vecMaxs().z * 0.7f);
			return nHpOut > 0;
		}

		return false;
	}

	bool IsVisibleTo(C_TerrorPlayer* pLocal, const Vector& vEye, const Vector& vPoint)
	{
		trace_t tr{};
		CTraceFilterHitAll filter(static_cast<IHandleEntity*>(pLocal));
		G::Util.Trace(vEye, vPoint, MASK_SHOT, &filter, &tr);
		return !tr.DidHit();
	}
}

void CFeatures_Hitmarker::PlayHit(bool bKill)
{
	if (!Vars::Hitmarker::bSound)
		return;

	// Антиспам: дробовики/ очередь бьют каждый кадр.
	const unsigned long long ullNow = GetTickCount64();
	if (ullNow - m_ullLastSnd < 40)
		return;
	m_ullLastSnd = ullNow;

	const int nPitch = U::Math.Clamp(Vars::Hitmarker::nPitch, 200, 2000);
	const int nFreq = bKill ? (nPitch * 3 / 2) : nPitch;
	// Beep синхронный — уводим в фон, кадр не ждёт.
	std::thread([nFreq, bKill]() { Beep(nFreq, bKill ? 120 : 60); }).detach();
}

void CFeatures_Hitmarker::OnTick()
{
	U::Log.Crumb("Hitmarker::OnTick");
	if (!Vars::Hitmarker::bEnabled || !I::EngineClient || !I::EngineClient->IsInGame() || !I::ClientEntityList || !I::GlobalVars)
	{
		if (!m_mHp.empty() || !m_aNums.empty() || m_nShots || m_nHits) { Clear(); }
		return;
	}

	C_TerrorPlayer* pLocal = nullptr;
	{
		const int nLocalIdx = I::EngineClient->GetLocalPlayer();
		if (nLocalIdx > 0)
		{
			IClientEntity* pEnt = I::ClientEntityList->GetClientEntity(nLocalIdx);
			if (G::Util.IsPlayerEntity(pEnt)) pLocal = pEnt->As<C_TerrorPlayer*>();
		}
	}
	if (!pLocal || pLocal->deadflag() || pLocal->m_lifeState() != 0)
		return;

	// Урон по нам: просадка HP локального между кадрами.
	bool bJustDamaged = false;
	{
		const int nHp = pLocal->GetHealth();
		if (m_nLocalHp >= 0 && nHp < m_nLocalHp)
		{
			m_flLastDmgT = I::GlobalVars->curtime;
			bJustDamaged = true;
		}
		m_nLocalHp = nHp;
	}

	const Vector vEye = G::Util.GetEyePosition(pLocal);

	// Зачёт только пока жмём огонь (мышь или клавиша аима): иначе это урон союзников.
	const bool bFiring = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0
		|| (Vars::Aimbot::nKey && (GetAsyncKeyState(Vars::Aimbot::nKey) & 0x8000) != 0);

	const float flNow = I::GlobalVars->curtime;
	std::map<int, int> mSeen;

	const int nMax = I::ClientEntityList->GetMaxEntities();
	for (int n = 1; n <= nMax; n++)
	{
		IClientEntity* pEntity = I::ClientEntityList->GetClientEntity(n);
		if (!pEntity || pEntity->IsDormant())
			continue;

		int nHp = 0;
		Vector vAnchor;
		if (!GetEnemyHp(pEntity, pLocal, nHp, vAnchor))
			continue;

		mSeen[n] = nHp;

		auto it = m_mHp.find(n);
		const int nPrev = (it == m_mHp.end()) ? nHp : it->second;
		const int nDmg = nPrev - nHp;

		// Скачок вверх = хил/респавн, огромный скачок вниз = телепорт сущности.
		if (nDmg <= 0 || nDmg > 2000)
			continue;
		// Порог: мелкий урон не засчитываем (добивание — всегда).
		if (nHp > 0 && nDmg < Vars::Hitmarker::nMinDmg)
			continue;
		if (!bFiring)
			continue;
		if (!IsVisibleTo(pLocal, vEye, vAnchor))
			continue;

		const bool bKill = (nHp <= 0);
		m_flLastHitT = flNow;
		m_nHits++;
		if (bKill)
			m_nKills++;
		PlayHit(bKill);

		if (Vars::Hitmarker::bNumbers && m_aNums.size() < 12)
		{
			HitNum_t m;
			m.vWorld = vAnchor;
			m.nDamage = nDmg;
			m.flT = flNow;
			m.bKill = bKill;
			m_aNums.push_back(m);
		}
	}

	m_mHp.swap(mSeen);

	// Откуда прилетело: в кадр урона ищем ближайшего видимого врага.
	// Дорогой скан (трейсы) — только по факту просадки HP, не каждый кадр.
	if (bJustDamaged && Vars::Hitmarker::bDmgArrow)
	{
		float flBest = 2000.0f * 2000.0f;
		Vector vBest;
		bool bFound = false;

		for (int n = 1; n <= nMax; n++)
		{
			IClientEntity* pEntity = I::ClientEntityList->GetClientEntity(n);
			if (!pEntity || pEntity->IsDormant())
				continue;

			int nHp = 0;
			Vector vAnchor;
			if (!GetEnemyHp(pEntity, pLocal, nHp, vAnchor))
				continue;

			const float flD2 = (vAnchor - vEye).LenghtSqr();
			if (flD2 >= flBest)
				continue;
			if (!IsVisibleTo(pLocal, vEye, vAnchor))
				continue;

			flBest = flD2;
			vBest = vAnchor;
			bFound = true;
		}

		if (bFound)
		{
			m_vAttacker = vBest;
			m_flAttackerT = flNow;
			m_bHasAttacker = true;
		}
	}
}

void CFeatures_Hitmarker::Draw()
{
	if (m_aNums.empty() || !G::Draw.m_nScreenW || !I::GlobalVars)
		return;

	const float flNow = I::GlobalVars->curtime;
	const float flDur = U::Math.Clamp(Vars::Hitmarker::nDurationMs, 400, 3000) / 1000.0f;

	for (int i = (int)m_aNums.size() - 1; i >= 0; --i)
	{
		HitNum_t& m = m_aNums[i];
		const float flAge = flNow - m.flT;
		if (flAge > flDur) { m_aNums.erase(m_aNums.begin() + i); continue; }
		if (flAge < 0.0f)
			continue;

		Vector vS;
		if (!G::Util.W2S(m.vWorld, vS))
			continue;

		const int nA = (int)(255.0f * (1.0f - flAge / flDur));
		const int nY = (int)vS.y - (int)(flAge * 46.0f);
		char sz[32] = { };
		sprintf_s(sz, m.bKill ? "KILL -%d" : "-%d", m.nDamage);
		const Color clr = m.bKill ? Color(255, 70, 70, nA) : Color(0, 255, 171, nA);
		G::Draw.String(EFonts::ESP_NAME, (int)vS.x, nY, clr, TXT_CENTERXY, "%s", sz);
	}

	// Красная вспышка по краям при уроне по нам (направления нет: ивентов в движке нет).
	if (Vars::Hitmarker::bDmgFlash)
	{
		const float flDmgAge = flNow - m_flLastDmgT;
		if (flDmgAge >= 0.0f && flDmgAge < 0.6f)
		{
			const int nA = (int)(110.0f * (1.0f - flDmgAge / 0.6f));
			const Color clrF(255, 40, 40, nA);
			const int W = G::Draw.m_nScreenW, H = G::Draw.m_nScreenH;
			const int nT = 26; // толщина рамки
			G::Draw.Rect(0, 0, W, nT, clrF);
			G::Draw.Rect(0, H - nT, W, nT, clrF);
			G::Draw.Rect(0, 0, nT, H, clrF);
			G::Draw.Rect(W - nT, 0, nT, H, clrF);
		}
	}

	// Стрелка на последнего атакующего: направление из углов обзора,
	// не W2S — враг часто за спиной, проекции там нет.
	if (Vars::Hitmarker::bDmgArrow && m_bHasAttacker && I::EngineClient && I::ClientEntityList)
	{
		const float flAtkAge = flNow - m_flAttackerT;
		if (flAtkAge >= 0.0f && flAtkAge < 3.0f)
		{
			const int nLocalIdx = I::EngineClient->GetLocalPlayer();
			IClientEntity* pEnt = (nLocalIdx > 0) ? I::ClientEntityList->GetClientEntity(nLocalIdx) : nullptr;
			C_TerrorPlayer* pLocal = G::Util.IsPlayerEntity(pEnt) ? pEnt->As<C_TerrorPlayer*>() : nullptr;
			if (pLocal && !pLocal->deadflag() && pLocal->m_lifeState() == 0)
			{
				const Vector vTo = m_vAttacker - G::Util.GetEyePosition(pLocal);
				if (vTo.LenghtSqr() > 1.0f)
				{
					Vector vAng;
					I::EngineClient->GetViewAngles(vAng);
					const float flRel = (atan2f(vTo.y, vTo.x) * 180.0f / 3.14159265f) - vAng.y;
					const float flRad = flRel * 3.14159265f / 180.0f;
					float dx = -sinf(flRad), dy = -cosf(flRad);

					const float cx = G::Draw.m_nScreenW * 0.5f, cy = G::Draw.m_nScreenH * 0.5f;
					const float px = cx + dx * 110.0f, py = cy + dy * 110.0f;

					const int nA = (int)(255.0f * (1.0f - flAtkAge / 3.0f));
					const Color clrA(255, 70, 70, nA);
					G::Draw.Line((int)cx, (int)cy, (int)px, (int)py, clrA);

					Vector2D tri[3] = {
						Vector2D(px + dx * 10.0f, py + dy * 10.0f),
						Vector2D(px - dy * 7.0f - dx * 4.0f, py + dx * 7.0f - dy * 4.0f),
						Vector2D(px + dy * 7.0f - dx * 4.0f, py - dx * 7.0f - dy * 4.0f)
					};
					G::Draw.Triangle(tri, clrA);

					char szD[16] = { };
					sprintf_s(szD, "%.0fm", sqrtf(vTo.LenghtSqr()) / 52.5f);
					G::Draw.String(EFonts::ESP_NAME, (int)px, (int)py + 16, clrA, TXT_CENTERXY, "%s", szD);
				}
			}
		}
	}

	// Стата сессии: попадания / выстрелы / точность / добивания.
	// Справа внизу, над вотермарком: с киллфидом сверху не пересекается.
	if (Vars::Hitmarker::bStats)
	{
		const int nAcc = (m_nShots > 0) ? (m_nHits * 100 / m_nShots) : 0;
		const int nX = G::Draw.m_nScreenW - 228;
		const int nY0 = G::Draw.m_nScreenH - 130;
		G::Draw.String(EFonts::MENU_CONSOLAS, nX, nY0, Color(140, 160, 152, 255), TXT_DEFAULT, "session");
		G::Draw.String(EFonts::MENU_CONSOLAS, nX, nY0 + 16, Color(235, 245, 240, 255), TXT_DEFAULT, "hits %d / shots %d", m_nHits, m_nShots);
		G::Draw.String(EFonts::MENU_CONSOLAS, nX, nY0 + 32, Color(0, 255, 171, 255), TXT_DEFAULT, "acc %d%%  kills %d", nAcc, m_nKills);
	}
}

float CFeatures_Hitmarker::SecondsSinceHit() const
{
	if (m_flLastHitT < -999.0f || !I::GlobalVars)
		return -1.0f;
	return I::GlobalVars->curtime - m_flLastHitT;
}

void CFeatures_Hitmarker::OnShot()
{
	if (Vars::Hitmarker::bEnabled && I::EngineClient && I::EngineClient->IsInGame())
		m_nShots++;
}
