#include "Alerts.h"

#include "../Lang/Lang.h"
#include "../Vars.h"

#include <cctype>
#include <cstring>

namespace
{
	// Имя класса в нижний регистр, проверка подстроки.
	bool NameHas(const char* szNet, const char* sub)
	{
		if (!szNet || !szNet[0] || !sub)
			return false;
		char szLower[64] = { };
		int i = 0;
		for (; i < 63 && szNet[i]; i++)
			szLower[i] = (char)tolower((unsigned char)szNet[i]);
		szLower[i] = '\0';
		return strstr(szLower, sub) != nullptr;
	}
}

void CFeatures_Alerts::Render()
{
	//bPinned/bRevive равноправные: без них в условии только они включённые
	//давали ранний return и никогда не рисовались.
	if (!Vars::Alerts::bEnabled || (!Vars::Alerts::bTank && !Vars::Alerts::bWitch
		&& !Vars::Alerts::bSIList && !Vars::Alerts::bPinned && !Vars::Alerts::bRevive
		&& !Vars::Alerts::bSpitAlert))
		return;
	if (!I::EngineClient || !I::EngineClient->IsInGame())
		return;

	const int nLocalIdx = I::EngineClient->GetLocalPlayer();
	C_TerrorPlayer* pLocal = nullptr;
	if (nLocalIdx >= 0)
	{
		IClientEntity* pEnt = I::ClientEntityList->GetClientEntity(nLocalIdx);
		if (G::Util.IsPlayerEntity(pEnt)) pLocal = pEnt->As<C_TerrorPlayer*>();
	}
	if (!pLocal)
		return;
	const Vector vEye = G::Util.GetEyePosition(pLocal);

	bool bTank = false, bWitch = false;
	float flTankD = 0.0f, flWitchD = 0.0f;
	C_BaseEntity* pTankEnt = nullptr;

	const int nMax = I::ClientEntityList ? I::ClientEntityList->GetMaxEntities() : 0;
	for (int n = 1; n <= nMax; n++)
	{
		if (n == nLocalIdx)
			continue;
		IClientEntity* pEntity = I::ClientEntityList->GetClientEntity(n);
		if (!pEntity || pEntity->IsDormant())
			continue;
		ClientClass* pCC = pEntity->GetClientClass();
		if (!pCC)
			continue;
		const int nID = pCC->m_ClassID;
		const bool bIsTank = (nID == Tank) || NameHas(pCC->m_pNetworkName, "tank");
		const bool bIsWitch = (nID == Witch) || NameHas(pCC->m_pNetworkName, "witch");
		if (!bIsTank && !bIsWitch)
			continue;
		if ((bIsTank && !Vars::Alerts::bTank) || (bIsWitch && !Vars::Alerts::bWitch))
			continue;

		// Живой? (лёгкая проверка, без доверия чужой раскладке)
		C_BasePlayer* pPl = pEntity->As<C_BasePlayer*>();
		if (!pPl || pPl->m_lifeState() != 0)
			continue;
		C_BaseEntity* pBase = pEntity->As<C_BaseEntity*>();
		if (!pBase)
			continue;
		const float flD = (pBase->m_vecOrigin() - vEye).Lenght() / 52.5f;
		if (bIsTank && (!bTank || flD < flTankD)) { bTank = true; flTankD = flD; pTankEnt = pBase; }
		if (bIsWitch && !bIsTank && (!bWitch || flD < flWitchD)) { bWitch = true; flWitchD = flD; }
	}

	const int nCX = G::Draw.m_nScreenW / 2;
	int nY = 96;

	// Центрированная «пилюля»: тень + градиент + рамка + акцент сверху.
	// Возвращает высоту, caller двигает nY сам.
	auto Pill = [&](EFonts eFont, const char* szText, Color clrText, Color clrAccent, int nCX_, int nY_) -> int
	{
		if (!szText || !szText[0]) return 0;
		const int nW = G::Draw.GetTextWidth(eFont, szText) + 30;
		const int nH = G::Draw.GetFontHeight(eFont) + 12;
		const int nX = nCX_ - nW / 2;
		G::Draw.Rect(nX + 2, nY_ + 2, nW, nH, Color(0, 0, 0, 110));
		G::Draw.GradientRect(nX, nY_, nX + nW, nY_ + nH, Color(20, 22, 21, 215), Color(10, 11, 10, 215), false);
		G::Draw.OutlinedRect(nX, nY_, nW, nH, Color(0, 0, 0, 200));
		G::Draw.Rect(nX, nY_, nW, 2, clrAccent);
		G::Draw.String(eFont, nCX_, nY_ + 6, clrText, TXT_CENTERX, "%s", szText);
		return nH;
	};

	// Пин на нас — самое срочное, поверх всего.
	if (Vars::Alerts::bPinned && !pLocal->deadflag() && pLocal->m_lifeState() == 0)
	{
		const char* szPin = nullptr;
		if (pLocal->m_jockeyAttacker().IsValid()) szPin = "JOCKEY";
		else if (pLocal->m_pounceAttacker().IsValid()) szPin = "HUNTER";
		else if (pLocal->m_tongueOwner().IsValid()) szPin = "SMOKER";
		else if (pLocal->m_carryAttacker().IsValid() || pLocal->m_pummelAttacker().IsValid()) szPin = "CHARGER";
		if (szPin)
		{
			char szPinTitle[64] = { };
			sprintf_s(szPinTitle, "%s: %s", Lang::T("PINNED"), szPin);
			const float flPulse = 0.6f + 0.4f * sinf((float)(GetTickCount64() % 6283) / 1000.0f * 6.0f);
			Color clr(255, (int)(60 + 40 * (1.0f - flPulse)), (int)(60 + 40 * (1.0f - flPulse)), 255);
			nY += Pill(EFonts::MENU_TAB, szPinTitle, clr, Color(255, 70, 70, 255), nCX, nY) + 4;
			nY += Pill(EFonts::MENU_CONSOLAS, Lang::T("wriggle WASD+mouse"), Color(235, 245, 240, 255), Color(0, 255, 171, 255), nCX, nY) + 6;
		}
	}
	if (bTank)
	{
		const float flPulse = 0.6f + 0.4f * sinf((float)(GetTickCount64() % 6283) / 1000.0f * 3.0f);
		Color clr(255, (int)(60 + 40 * (1.0f - flPulse)), (int)(60 + 40 * (1.0f - flPulse)), 255);
		char szTank[64] = { };
		sprintf_s(szTank, "%s %.0fm", Lang::T("TANK"), (double)flTankD);
		nY += Pill(EFonts::MENU_TAB, szTank, clr, Color(255, 70, 70, 255), nCX, nY) + 4;
		if (Vars::Alerts::bTankHp && pTankEnt)
		{
			//pTankEnt может быть name-совпадением (камень танка): клампим оба.
			const int nHp = U::Math.Clamp(pTankEnt->GetHealth(), 0, 20000);
			const int nMaxHp = U::Math.Clamp(pTankEnt->As<C_TerrorPlayer*>()->m_iMaxHealth(), 1, 20000);
			const int nPct = U::Math.Clamp(nHp * 100 / nMaxHp, 0, 100);
			const int nBW = 320, nBX = nCX - nBW / 2;
			G::Draw.Rect(nBX, nY, nBW, 10, Color(10, 12, 11, 200));
			G::Draw.Rect(nBX, nY, nBW * nPct / 100, 10, Color(255, nPct > 50 ? 170 : 70, 40, 255));
			G::Draw.OutlinedRect(nBX, nY, nBW, 10, Color(0, 0, 0, 180));
			char szHp[32] = { };
			sprintf_s(szHp, "%d", nHp > 0 ? nHp : 0);
			G::Draw.String(EFonts::MENU_CONSOLAS, nCX, nY - 1, Color(255, 255, 255, 255), TXT_CENTERXY, "%s", szHp);
			nY += 16;
		}
	}
	if (bWitch)
	{
		char szWitch[64] = { };
		sprintf_s(szWitch, "%s %.0fm", Lang::T("WITCH"), (double)flWitchD);
		nY += Pill(EFonts::MENU_TAB, szWitch, Color(200, 0, 255, 255), Color(200, 0, 255, 255), nCX, nY) + 4;
	}

	// Блевотина под ногами: плевок плевальщицы бьёт по площади, радиус ~4м.
	// Стоишь внутри — красным капсом поверх остального (после пина).
	if (Vars::Alerts::bSpitAlert && !pLocal->deadflag() && pLocal->m_lifeState() == 0)
	{
		const Vector vFeet = pLocal->m_vecOrigin();
		bool bInSpit = false;
		for (int n = 1; n <= nMax && !bInSpit; n++)
		{
			if (n == nLocalIdx)
				continue;
			IClientEntity* pEntity = I::ClientEntityList->GetClientEntity(n);
			if (!pEntity || pEntity->IsDormant())
				continue;
			ClientClass* pCC = pEntity->GetClientClass();
			if (!pCC)
				continue;
			if (pCC->m_ClassID != CSpitterProjectile && !NameHas(pCC->m_pNetworkName, "spitter"))
				continue;
			C_BaseEntity* pBase = pEntity->As<C_BaseEntity*>();
			if (!pBase)
				continue;
			Vector vD = pBase->m_vecOrigin() - vFeet;
			vD.z = 0.0f;
			if (vD.Lenght() / 52.5f < 4.5f)
				bInSpit = true;
		}
		if (bInSpit)
		{
			const float flPulse = 0.6f + 0.4f * sinf((float)(GetTickCount64() % 6283) / 1000.0f * 6.0f);
			Color clr(255, (int)(60 + 40 * (1.0f - flPulse)), (int)(60 + 40 * (1.0f - flPulse)), 255);
			nY += Pill(EFonts::MENU_TAB, Lang::T("SPIT! MOVE"), clr, Color(255, 70, 70, 255), nCX, nY) + 4;
		}
	}

	// Союзник в инкапе — зовём реанимировать (ближайший).
	if (Vars::Alerts::bRevive)
	{
		const int nLocalTeam = pLocal->GetTeamNumber();
		const C_TerrorPlayer* pBest = nullptr;
		const char* szBestName = nullptr;
		char szNameBuf[32] = { };
		float flBestD = 1e30f;
		for (int n = 1; n <= nMax; n++)
		{
			if (n == nLocalIdx)
				continue;
			IClientEntity* pEntity = I::ClientEntityList->GetClientEntity(n);
			if (!pEntity || pEntity->IsDormant())
				continue;
			ClientClass* pCC = pEntity->GetClientClass();
			if (!pCC)
				continue;
			const int nID = pCC->m_ClassID;
			if (nID != CTerrorPlayer && nID != SurvivorBot)
				continue;
			C_TerrorPlayer* pT = pEntity->As<C_TerrorPlayer*>();
			if (!pT || pT->GetTeamNumber() != nLocalTeam)
				continue;
			if (pT->deadflag() || pT->m_lifeState() != 0 || pT->GetHealth() <= 0)
				continue;
			if (!pT->m_isIncapacitated())
				continue;
			player_info_t pi = {};
			if (!I::EngineClient->GetPlayerInfo(n, &pi) || !pi.name[0])
				continue;
			pi.name[31] = '\0';
			const float flD = (pT->m_vecOrigin() - vEye).Lenght() / 52.5f;
			if (flD < flBestD)
			{
				flBestD = flD;
				pBest = pT;
				strcpy_s(szNameBuf, pi.name);
				szBestName = szNameBuf;
			}
		}
		if (pBest && szBestName)
		{
			char szRevive[96] = { };
			sprintf_s(szRevive, "%s: %s %.0fm", Lang::T("REVIVE"), szBestName, (double)flBestD);
			nY += Pill(EFonts::MENU_TAB, szRevive, Color(0, 255, 171, 255), Color(0, 255, 171, 255), nCX, nY) + 4;
		}
	}

	// Список живых особых рядом: имя + дистанция, правая колонка.
	if (Vars::Alerts::bSIList)
	{
		struct SI_t { const char* szName; float flD; };
		SI_t aSI[12] = { };
		int nSI = 0;
		const int nLocalTeam = pLocal->GetTeamNumber();

		for (int n = 1; n <= nMax && nSI < 12; n++)
		{
			if (n == nLocalIdx)
				continue;
			IClientEntity* pEntity = I::ClientEntityList->GetClientEntity(n);
			if (!pEntity || pEntity->IsDormant())
				continue;
			ClientClass* pCC = pEntity->GetClientClass();
			if (!pCC)
				continue;

			const char* szName = nullptr;
			switch (pCC->m_ClassID)
			{
				case Hunter: szName = "Hunter"; break;
				case Smoker: szName = "Smoker"; break;
				case Jockey: szName = "Jockey"; break;
				case Spitter: szName = "Spitter"; break;
				case Charger: szName = "Charger"; break;
				case Tank: szName = "Tank"; break;
				default:
					if (NameHas(pCC->m_pNetworkName, "boomer"))
						szName = "Boomer";
					break;
			}
			if (!szName)
				continue;

			C_BasePlayer* pPl = pEntity->As<C_BasePlayer*>();
		if (!pPl || pPl->m_lifeState() != 0)
			continue;
		C_BaseEntity* pBase = pEntity->As<C_BaseEntity*>();
		if (!pBase || !G::Util.IsValidTeam(pBase->m_iTeamNum()) || pBase->m_iTeamNum() == nLocalTeam)
			continue;

			const float flD = (pBase->m_vecOrigin() - vEye).Lenght() / 52.5f;
			// Вставка по возрастанию дистанции.
			int nPos = nSI;
			while (nPos > 0 && aSI[nPos - 1].flD > flD) { aSI[nPos] = aSI[nPos - 1]; nPos--; }
			aSI[nPos].szName = szName;
			aSI[nPos].flD = flD;
			nSI++;
		}

		if (nSI > 0)
		{
			// Панелька под список: тень + градиент + рамка + акцент слева.
			const int nRows = nSI < 8 ? nSI : 8;
			const int nLH = 16;
			const int nPW = 190, nPH = 26 + nRows * nLH + 6;
			const int nX = 16, nSY0 = 336;
			G::Draw.Rect(nX + 2, nSY0 + 2, nPW, nPH, Color(0, 0, 0, 110));
			G::Draw.GradientRect(nX, nSY0, nX + nPW, nSY0 + nPH, Color(20, 22, 21, 215), Color(10, 11, 10, 215), false);
			G::Draw.OutlinedRect(nX, nSY0, nPW, nPH, Color(0, 0, 0, 200));
			G::Draw.Rect(nX + 1, nSY0 + 1, 3, nPH - 2, Color(0, 255, 171, 255));
			G::Draw.String(EFonts::MENU_CONSOLAS, nX + 12, nSY0 + 6, Color(140, 160, 152, 255), TXT_DEFAULT, "%s", "SI NEARBY");
			for (int i = 0; i < nRows; i++)
				G::Draw.String(EFonts::MENU_CONSOLAS, nX + 12, nSY0 + 24 + i * nLH, Color(255, 120, 80, 255), TXT_DEFAULT, "%s %.0fm", aSI[i].szName, (double)aSI[i].flD);
		}
	}
}
