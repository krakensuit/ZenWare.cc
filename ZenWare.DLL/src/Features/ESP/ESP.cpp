#include "ESP.h"

#include "../Vars.h"

void CFeatures_ESP::Render()
{
	if (!Vars::ESP::bEnabled || !I::EngineClient->IsInGame() || I::EngineVGui->IsGameUIVisible())
		return;

	const int nLocalIndex = I::EngineClient->GetLocalPlayer();

	C_TerrorPlayer* pLocal = I::ClientEntityList->GetClientEntity(nLocalIndex)->As<C_TerrorPlayer*>();

	if (!pLocal)
		return;

	for (int n = 1; n < (I::ClientEntityList->GetMaxEntities() + 1); n++)
	{
		if (n == nLocalIndex)
			continue;

		IClientEntity* pEntity = I::ClientEntityList->GetClientEntity(n);

		if (!pEntity || pEntity->IsDormant())
			continue;

		ClientClass* pCC = pEntity->GetClientClass();

		if (!pCC)
			continue;

		switch (pCC->m_ClassID)
		{
			case CTerrorPlayer:
			case SurvivorBot:
			{
				C_TerrorPlayer* pPlayer = pEntity->As<C_TerrorPlayer*>();

				if (G::Util.IsValidTarget(pLocal, pPlayer, false))
					DrawPlayer(pLocal, pPlayer, n);

				break;
			}
			case CWeaponSpawn:
			case CPipeBomb:
			case CMolotov:
			case CPainPills:
			case CItem_Adrenaline:
			case CFirstAidKit:
			case CItemDefibrillator:
			case CPropMinigun:
			case CPropMountedGun:
			{
				if (Vars::ESP::bItems)
					DrawItem(pLocal, pEntity->As<C_BaseEntity*>());

				break;
			}
			case Infected:
			{
				if (Vars::ESP::bCommon)
					DrawCommon(pEntity->As<C_BaseEntity*>());

				break;
			}
			default:
				break;
		}
	}
}

void CFeatures_ESP::DrawPlayer(C_TerrorPlayer* pLocal, C_TerrorPlayer* pPlayer, const int nEntityIndex)
{
	int x, y, w, h;

	if (!GetBounds(pPlayer, x, y, w, h) || w <= 0 || h <= 0)
		return;

	const bool bIsEnemy = (pPlayer->GetTeamNumber() != pLocal->GetTeamNumber());
	const Color clrTeam = bIsEnemy ? Vars::Chams::clrEnemy : Vars::Chams::clrAlly;

	const int nHealth = U::Math.Clamp(pPlayer->GetHealth(), 0, 100);
	const Color clrHP(255 - (255 * nHealth) / 100, (255 * nHealth) / 100, 0, 255);

	wchar_t wszDist[16] = { };

	if (Vars::ESP::bDistance)
		swprintf_s(wszDist, L" [%.0fm]", G::Util.GetEyePosition(pLocal).DistTo(G::Util.GetEyePosition(pPlayer)) / 52.5f);

	player_info_t pi;

	//Box outline around the model bounds.
	if (Vars::ESP::bBox)
	{
		G::Draw.OutlinedRect(x - 1, y - 1, w + 2, h + 2, { 10, 10, 12, 200 });
		G::Draw.OutlinedRect(x, y, w, h, clrTeam);
	}

	//Vertical health bar on the left side of the box (red -> green).
	if (Vars::ESP::bHealthBar)
	{
		constexpr int nBarW = 4;
		const int nBarH = static_cast<int>((h * nHealth) / 100.0f);

		G::Draw.Rect(x - nBarW - 3, y - 1, nBarW + 2, h + 2, { 10, 10, 12, 200 });
		G::Draw.Rect(x - nBarW - 2, (y + h) - nBarH, nBarW, nBarH, clrHP);
	}

	//Nickname (+distance) and HP text above the box.
	if (Vars::ESP::bName && I::EngineClient->GetPlayerInfo(nEntityIndex, &pi))
	{
		const int nCenterX = x + (w / 2);

		char szDistNarrow[16] = { };
		if (wszDist[0])
			sprintf_s(szDistNarrow, " [%.0fm]", G::Util.GetEyePosition(pLocal).DistTo(G::Util.GetEyePosition(pPlayer)) / 52.5f);

		int nTextY = y - G::Draw.GetFontHeight(EFonts::ESP_NAME);
		G::Draw.String(EFonts::ESP_NAME, nCenterX, nTextY, clrTeam, TXT_CENTERXY, "%s%s", pi.name, szDistNarrow);

		nTextY -= G::Draw.GetFontHeight(EFonts::ESP);
		G::Draw.String(EFonts::ESP, nCenterX, nTextY, clrHP, TXT_CENTERXY, "%ihp", nHealth);

		// Held weapon - через netvar m_hActiveWeapon (надёжнее виртуалки)
		EHANDLE hActive = pPlayer->m_hActiveWeapon();
		C_BaseEntity* pEntActive = nullptr;
		if (hActive.IsValid())
		{
			IClientEntity* pViaHandle = I::ClientEntityList->GetClientEntityFromHandle(hActive);
			if (pViaHandle) pEntActive = pViaHandle->As<C_BaseEntity*>();
		}
		C_BaseCombatWeapon* pActive = pEntActive ? pEntActive->As<C_BaseCombatWeapon*>() : nullptr;

		if (pActive)
		{
			wchar_t wszDisplay[64] = { };
			bool bHasName = false;
			C_TerrorWeapon* pTW = pActive->As<C_TerrorWeapon*>();
			if (pTW)
			{
				int id = pTW->GetWeaponID();
				if (id > 0 && id < 38 && wcscmp(g_aSpawnInfo[id].m_szName, L"unknown") != 0)
				{
					wcscpy_s(wszDisplay, g_aSpawnInfo[id].m_szName);
					bHasName = true;
				}
			}
			if (!bHasName)
			{
				ClientClass* pWCC = pActive->GetClientClass();
				const char* szName = (pWCC && pWCC->m_pNetworkName) ? pWCC->m_pNetworkName : pActive->GetPrintName();
				if (szName && szName[0])
				{
					if (szName[0] == 'C') szName++;
					if (szName[0] == '#') szName++;
					if (!strncmp(szName, "Weapon", 6)) szName += 6;
					wsprintfW(wszDisplay, L"%hs", szName);
					if (wcsstr(wszDisplay, L"unknown")) bHasName = false;
					else bHasName = wszDisplay[0] != L'\0';
				}
			}
			if (bHasName)
			{
				// Иконка из игры: пробуем найти материал оружия (если есть - рисуем иконку рядом)
				// Для простоты пока текст, иконку можно включить если материал найден
				nTextY -= G::Draw.GetFontHeight(EFonts::ESP_WEAPON);
				G::Draw.String(EFonts::ESP_WEAPON, nCenterX, nTextY, Color(220,220,220,255), TXT_CENTERXY, "%ls", wszDisplay);
			}
		}
	}
}

void CFeatures_ESP::DrawItem(C_TerrorPlayer* pLocal, C_BaseEntity* pEntity)
{
	if (!pEntity)
		return;

	int x, y, w, h;

	if (!GetBounds(pEntity, x, y, w, h) || w <= 0 || h <= 0)
		return;

	ClientClass* pCC = pEntity->GetClientClass();

	//Mounted guns show their heat instead of a weapon name.
	if (U::Math.CompareGroup(pCC->m_ClassID, CPropMinigun, CPropMountedGun))
	{
		C_BaseMountedWeapon* pMounted = pEntity->As<C_BaseMountedWeapon*>();

		if (!pMounted)
			return;

		G::Draw.String(EFonts::ESP, x + (w / 2), y + (h / 2), { 204, 204, 204, 255 }, TXT_CENTERXY,
			L"gun | heat %.0f%%", U::Math.Clamp(pMounted->m_heat() * 100.0f, 0.0f, 100.0f));
		return;
	}

	C_WeaponSpawn* pSpawn = pEntity->As<C_WeaponSpawn*>();

	if (!pSpawn)
		return;

	// Try netvar first (more reliable for spawns), fallback to virtual
	int nID = pSpawn->m_weaponID();
	if (nID <= 0 || nID >= 38 || wcscmp(g_aSpawnInfo[nID].m_szName, L"unknown") == 0)
		nID = pSpawn->GetWeaponID();
	nID = U::Math.Clamp(nID, 0, 38);

	// If still unknown, use class name as fallback instead of showing "unknown"
	const wchar_t* pName = g_aSpawnInfo[nID].m_szName;
	wchar_t wszFallback[64] = { };
	if (wcscmp(pName, L"unknown") == 0)
	{
		const char* szClass = pCC ? pCC->m_pNetworkName : nullptr;
		if (szClass)
		{
			// e.g., "CWeaponSpawn" -> show as "weapon"
			wsprintfW(wszFallback, L"%hs", szClass);
			pName = wszFallback;
		}
	}

	Color clrItem(g_aSpawnInfo[nID].m_Color.r(), g_aSpawnInfo[nID].m_Color.g(), g_aSpawnInfo[nID].m_Color.b(), 255);
	// Fix invisible dark text: ensure at least 180 brightness for item names
	if (clrItem.r() < 60 && clrItem.g() < 60 && clrItem.b() < 60)
		clrItem = Color(200,200,200,255);

	wchar_t wszLine[96] = { };
	wcscpy_s(wszLine, pName);

	if (Vars::ESP::bDistance)
	{
		wchar_t wszDist[16] = { };
		swprintf_s(wszDist, L" [%.0fm]", G::Util.GetEyePosition(pLocal).DistTo(pEntity->m_vecOrigin()) / 52.5f);
		wcscat_s(wszLine, wszDist);
	}

	G::Draw.String(EFonts::ESP, x + (w / 2), y + (h / 2), clrItem, TXT_CENTERXY, wszLine);
}

void CFeatures_ESP::DrawCommon(C_BaseEntity* pEntity)
{
	C_Infected* pInfected = pEntity ? pEntity->As<C_Infected*>() : nullptr;

	if (!pInfected)
		return;

	//Alive check from the base's own utility: solid flags + animation sequence.
	if (!G::Util.IsInfectedAlive(pInfected->m_usSolidFlags(), pInfected->m_nSequence()))
		return;

	int x, y, w, h;

	if (!GetBounds(pInfected, x, y, w, h) || w <= 4 || h <= 4)
		return;

	const Color clrCommon(170, 60, 60, 220);
	G::Draw.OutlinedRect(x, y, w, h, clrCommon);
}

bool CFeatures_ESP::GetBounds(C_BaseEntity* pBaseEntity, int& x, int& y, int& w, int& h)
{
	Vector vPoints[8];
	U::Math.BuildTransformedBox(vPoints, pBaseEntity->m_vecMins(), pBaseEntity->m_vecMaxs(), pBaseEntity->RenderableToWorldTransform());

	Vector flb, brt, blb, frt, frb, brb, blt, flt;
	if (G::Util.W2S(vPoints[3], flb) && G::Util.W2S(vPoints[5], brt)
		&& G::Util.W2S(vPoints[0], blb) && G::Util.W2S(vPoints[4], frt)
		&& G::Util.W2S(vPoints[2], frb) && G::Util.W2S(vPoints[1], brb)
		&& G::Util.W2S(vPoints[6], blt) && G::Util.W2S(vPoints[7], flt))
	{
		const Vector vTransformed[8] = { flb, brt, blb, frt, frb, brb, blt, flt };

		float left = flb.x;
		float top = flb.y;
		float righ = flb.x;
		float bottom = flb.y;

		for (int n = 1; n < 8; n++)
		{
			if (left > vTransformed[n].x)
				left = vTransformed[n].x;

			if (top < vTransformed[n].y)
				top = vTransformed[n].y;

			if (righ < vTransformed[n].x)
				righ = vTransformed[n].x;

			if (bottom > vTransformed[n].y)
				bottom = vTransformed[n].y;
		}

		x = static_cast<int>(left);
		y = static_cast<int>(bottom);
		w = static_cast<int>(righ - left);
		h = static_cast<int>(top - bottom);

		return !(x > G::Draw.m_nScreenW || (x + w) < 0 || y > G::Draw.m_nScreenH || (y + h) < 0);
	}

	return false;
}
