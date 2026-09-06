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
	if (!Vars::Alerts::bEnabled || (!Vars::Alerts::bTank && !Vars::Alerts::bWitch))
		return;
	if (!I::EngineClient || !I::EngineClient->IsInGame())
		return;

	const int nLocalIdx = I::EngineClient->GetLocalPlayer();
	C_TerrorPlayer* pLocal = nullptr;
	if (nLocalIdx >= 0)
	{
		IClientEntity* pEnt = I::ClientEntityList->GetClientEntity(nLocalIdx);
		if (pEnt) pLocal = pEnt->As<C_TerrorPlayer*>();
	}
	if (!pLocal)
		return;
	const Vector vEye = G::Util.GetEyePosition(pLocal);

	bool bTank = false, bWitch = false;
	float flTankD = 0.0f, flWitchD = 0.0f;

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
		if (bIsTank && (!bTank || flD < flTankD)) { bTank = true; flTankD = flD; }
		if (bIsWitch && !bIsTank && (!bWitch || flD < flWitchD)) { bWitch = true; flWitchD = flD; }
	}

	const int nCX = G::Draw.m_nScreenW / 2;
	int nY = 96;
	if (bTank)
	{
		const float flPulse = 0.6f + 0.4f * sinf((float)(GetTickCount64() % 6283) / 1000.0f * 3.0f);
		Color clr(255, (int)(60 + 40 * (1.0f - flPulse)), (int)(60 + 40 * (1.0f - flPulse)), 255);
		G::Draw.String(EFonts::MENU_TAB, nCX, nY, clr, TXT_CENTERXY, "%s %.0fm", Lang::T("TANK"), flTankD);
		nY += G::Draw.GetFontHeight(EFonts::MENU_TAB) + 6;
	}
	if (bWitch)
	{
		G::Draw.String(EFonts::MENU_TAB, nCX, nY, Color(200, 0, 255, 255), TXT_CENTERXY, "%s %.0fm", Lang::T("WITCH"), flWitchD);
	}
}
