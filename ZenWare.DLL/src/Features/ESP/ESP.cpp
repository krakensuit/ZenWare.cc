#include "ESP.h"

#include "../Vars.h"
#include "../../Util/Logger/Logger.h"

#include <cctype>
#include <cmath>
#include <cstring>
#include <map>

namespace
{
	// Уголки как у боксов игроков: единый стиль ESP.
	void DrawCorners(int x, int y, int w, int h, const Color& clr)
	{
		const int nTick = U::Math.Clamp(w / 4, 6, 16);
		G::Draw.Line(x - 3, y - 3, x - 3 + nTick, y - 3, clr);
		G::Draw.Line(x - 3, y - 3, x - 3, y - 3 + nTick, clr);
		G::Draw.Line(x + w + 3 - nTick, y - 3, x + w + 3, y - 3, clr);
		G::Draw.Line(x + w + 3, y - 3, x + w + 3, y - 3 + nTick, clr);
		G::Draw.Line(x - 3, y + h + 3 - nTick, x - 3, y + h + 3, clr);
		G::Draw.Line(x - 3, y + h + 3, x - 3 + nTick, y + h + 3, clr);
		G::Draw.Line(x + w + 3, y + h + 3 - nTick, x + w + 3, y + h + 3, clr);
		G::Draw.Line(x + w + 3 - nTick, y + h + 3, x + w + 3, y + h + 3, clr);
	}
}

void CFeatures_ESP::Render()
{
	U::Log.Crumb("ESP::Render");
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
		case Hunter:
		case Smoker:
		case Jockey:
		case Spitter:
		case Charger:
		case Tank:
		{
			if (Vars::ESP::bSpecialBoxes)
				DrawSpecial(pLocal, pEntity->As<C_BaseEntity*>(), pCC->m_ClassID);
			break;
		}
		case Witch:
		{
			if (Vars::ESP::bBossBoxes)
				DrawBoss(pEntity->As<C_BaseEntity*>());
			break;
		}
		default:
		{
			// ID нет в нашем дампе (бумер!) или чужой билд со сдвинутыми ID —
			// опознаём по имени класса, оно не меняется.
			if (Vars::ESP::bSpecialBoxes && pCC->m_pNetworkName)
				DrawUnknown(pLocal, pEntity->As<C_BaseEntity*>(), pCC->m_pNetworkName);
			break;
		}
		}
	}

	DrawTeam(pLocal);
	DrawThrowables();
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

	player_info_t pi = { };

	//Box outline around the model bounds (pulses red on low HP).
	if (Vars::ESP::bBox)
	{
		Color clrBox = clrTeam;

		if (nHealth <= 25)
		{
			const float flPulse = 0.5f + 0.5f * sinf(static_cast<float>(GetTickCount64() % 6283) / 1000.0f);
			clrBox = {
				clrTeam.r() + static_cast<int>((255 - clrTeam.r()) * flPulse),
				clrTeam.g() + static_cast<int>((0 - clrTeam.g()) * flPulse),
				clrTeam.b() + static_cast<int>((0 - clrTeam.b()) * flPulse),
				255
			};
		}

		if (Vars::ESP::bFilled)
			G::Draw.Rect(x, y, w, h, { clrBox.r(), clrBox.g(), clrBox.b(), 40 });
		//Чистый бокс как у space: тёмная подложка + одна цветная рамка.
		G::Draw.OutlinedRect(x - 1, y - 1, w + 2, h + 2, { 10, 10, 12, 200 });
		G::Draw.OutlinedRect(x, y, w, h, clrBox);
	}

	//Snapline from the bottom of the screen.
	if (Vars::ESP::bSnaplines)
		G::Draw.Line(G::Draw.m_nScreenW / 2, G::Draw.m_nScreenH, x + (w / 2), y + h, clrTeam);

	//Vertical health bar on the left side of the box (red -> green).
	if (Vars::ESP::bHealthBar)
	{
		constexpr int nBarW = 4;
		const int nBarH = static_cast<int>((h * nHealth) / 100.0f);
		const int nBarTop = (y + h) - nBarH;

		G::Draw.Rect(x - nBarW - 3, y - 1, nBarW + 2, h + 2, { 10, 10, 12, 200 });
		G::Draw.OutlinedRect(x - nBarW - 3, y - 1, nBarW + 2, h + 2, { 0, 0, 0, 160 });
		G::Draw.Rect(x - nBarW - 2, nBarTop, nBarW, nBarH / 2,
			{ clrHP.r() + 60 > 255 ? 255 : clrHP.r() + 60, clrHP.g() + 60 > 255 ? 255 : clrHP.g() + 60, clrHP.b(), 255 });
		G::Draw.Rect(x - nBarW - 2, nBarTop + nBarH / 2, nBarW, nBarH - nBarH / 2, clrHP);

		if (Vars::ESP::bHealthText)
			G::Draw.String(EFonts::ESP, x - nBarW - 8, y + (h / 2) - G::Draw.GetFontHeight(EFonts::ESP) / 2, clrHP, TXT_CENTERXY, "%i", nHealth);
	}

	//Nickname (+distance) and HP text above the box.
	if (Vars::ESP::bName && I::EngineClient->GetPlayerInfo(nEntityIndex, &pi))
	{
		pi.name[31] = '\0';
		const int nCenterX = x + (w / 2);

		char szDistNarrow[16] = { };
		if (wszDist[0])
			sprintf_s(szDistNarrow, " [%.0fm]", G::Util.GetEyePosition(pLocal).DistTo(G::Util.GetEyePosition(pPlayer)) / 52.5f);

		int nTextY = y - G::Draw.GetFontHeight(EFonts::ESP_NAME);
		G::Draw.String(EFonts::ESP_NAME, nCenterX, nTextY, clrTeam, TXT_CENTERXY, "%s%s", pi.name, szDistNarrow);

		nTextY -= G::Draw.GetFontHeight(EFonts::ESP);
		G::Draw.String(EFonts::ESP, nCenterX, nTextY, clrHP, TXT_CENTERXY, "%ihp", nHealth);

	// Held weapon - через netvar m_hActiveWeapon (надёжнее виртуалки)
	if (Vars::ESP::bWeaponText)
	{
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
			// pActive из хендла без проверки класса: виртуалку дёргаем
			// только если класс известен, иначе статичная строка.
			ClientClass* pWCC = pActive->GetClientClass();
			const char* szName = (pWCC && pWCC->m_pNetworkName) ? pWCC->m_pNetworkName : "weapon";
				if (szName && szName[0])
				{
					if (szName[0] == 'C') szName++;
					if (szName[0] == '#') szName++;
				if (!strncmp(szName, "Weapon", 6)) szName += 6;
				swprintf_s(wszDisplay, L"%hs", szName);
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
	} // bWeaponText
	} // bName
}

void CFeatures_ESP::DrawItem(C_TerrorPlayer* pLocal, C_BaseEntity* pEntity)
{
	if (!pEntity)
		return;

	int x, y, w, h;

	if (!GetBounds(pEntity, x, y, w, h) || w <= 0 || h <= 0)
		return;

	ClientClass* pCC = pEntity->GetClientClass();
	if (!pCC)
		return;

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

	// m_weaponID/GetWeaponID существуют ТОЛЬКО у CWeaponSpawn. Таблетки,
	// аптечки, bile и прочие пикапы — другие серверные классы с другой
	// таблицей виртуалок: дёргать их методы = вылет. Им только имя класса.
	const Color clrItemNone(200, 200, 200, 255);
	if (pCC->m_ClassID != CWeaponSpawn)
	{
		const char* szCls = (pCC->m_pNetworkName && pCC->m_pNetworkName[0]) ? pCC->m_pNetworkName : "item";
		wchar_t wszCls[64] = { };
		swprintf_s(wszCls, L"%hs", szCls);
		if (Vars::ESP::bDistance)
		{
			wchar_t wszD[16] = { };
			swprintf_s(wszD, L" [%.0fm]", G::Util.GetEyePosition(pLocal).DistTo(pEntity->m_vecOrigin()) / 52.5f);
			wcscat_s(wszCls, wszD);
		}
		const int nTick = 5;
		G::Draw.Line(x, y, x + nTick, y, clrItemNone);
		G::Draw.Line(x, y, x, y + nTick, clrItemNone);
		G::Draw.Line(x + w - nTick, y, x + w, y, clrItemNone);
		G::Draw.Line(x + w, y, x + w, y + nTick, clrItemNone);
		G::Draw.Line(x, y + h - nTick, x, y + h, clrItemNone);
		G::Draw.Line(x, y + h, x + nTick, y + h, clrItemNone);
		G::Draw.Line(x + w, y + h - nTick, x + w, y + h, clrItemNone);
		G::Draw.Line(x + w - nTick, y + h, x + w, y + h, clrItemNone);
		G::Draw.String(EFonts::ESP, x + (w / 2), y + (h / 2), clrItemNone, TXT_CENTERXY, L"%ls", wszCls);
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
			swprintf_s(wszFallback, L"%hs", szClass);
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

	const int nTick = 5;
	G::Draw.Line(x, y, x + nTick, y, clrItem);
	G::Draw.Line(x, y, x, y + nTick, clrItem);
	G::Draw.Line(x + w - nTick, y, x + w, y, clrItem);
	G::Draw.Line(x + w, y, x + w, y + nTick, clrItem);
	G::Draw.Line(x, y + h - nTick, x, y + h, clrItem);
	G::Draw.Line(x, y + h, x + nTick, y + h, clrItem);
	G::Draw.Line(x + w, y + h - nTick, x + w, y + h, clrItem);
	G::Draw.Line(x + w - nTick, y + h, x + w, y + h, clrItem);
	G::Draw.String(EFonts::ESP, x + (w / 2), y + (h / 2), clrItem, TXT_CENTERXY, L"%ls", wszLine);
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
	if (Vars::ESP::bFilled)
		G::Draw.Rect(x, y, w, h, { 170, 60, 60, 40 });
	G::Draw.OutlinedRect(x - 1, y - 1, w + 2, h + 2, { 10, 10, 12, 200 });
	G::Draw.OutlinedRect(x, y, w, h, clrCommon);
	DrawCorners(x, y, w, h, clrCommon);
}

void CFeatures_ESP::DrawSpecial(C_TerrorPlayer* pLocal, C_BaseEntity* pEntity, const int nClassID)
{
	if (!pLocal || !pEntity)
		return;

	int x, y, w, h;

	if (!GetBounds(pEntity, x, y, w, h) || w <= 0 || h <= 0)
		return;

	//Light checks only (plain reads, never crash): dormant handled by caller.
	C_BasePlayer* pPl = pEntity->As<C_BasePlayer*>();

	if (!pPl || pPl->m_lifeState() != 0)
		return;

	const int nTeam = pEntity->m_iTeamNum();

	if ((nTeam != TEAM_SURVIVOR && nTeam != TEAM_INFECTED) || nTeam == pLocal->GetTeamNumber())
		return;

	const char* szName = "?";

	switch (nClassID)
	{
		case Hunter: szName = "HUNTER"; break;
		case Smoker: szName = "SMOKER"; break;
		case Jockey: szName = "JOCKEY"; break;
		case Spitter: szName = "SPITTER"; break;
		case Charger: szName = "CHARGER"; break;
		case Tank: szName = "TANK"; break;
		default: break;
	}

	const Color& clrTeam = Vars::Chams::clrEnemy;
	if (Vars::ESP::bFilled)
		G::Draw.Rect(x, y, w, h, { clrTeam.r(), clrTeam.g(), clrTeam.b(), 40 });
	G::Draw.OutlinedRect(x - 1, y - 1, w + 2, h + 2, { 10, 10, 12, 200 });
	G::Draw.OutlinedRect(x, y, w, h, clrTeam);
	DrawCorners(x, y, w, h, clrTeam);
	G::Draw.String(EFonts::ESP_NAME, x + (w / 2), y - G::Draw.GetFontHeight(EFonts::ESP_NAME), clrTeam, TXT_CENTERXY, "%s", szName);

	//HP и дистанция для спец-заражённых
	if (Vars::ESP::bHealthText || Vars::ESP::bDistance)
	{
		char szInfo[32] = { };
		const int nHP = pPl->GetHealth();
		if (Vars::ESP::bHealthText) sprintf_s(szInfo, "%ihp", nHP);
		if (Vars::ESP::bDistance) sprintf_s(szInfo + strlen(szInfo), sizeof(szInfo) - strlen(szInfo), "%s%.0fm", szInfo[0] ? " " : "", G::Util.GetEyePosition(pLocal).DistTo(pEntity->m_vecOrigin()) / 52.5f);
		if (szInfo[0])
			G::Draw.String(EFonts::ESP, x + (w / 2), y + h + 2, Color(230, 230, 230, 255), TXT_CENTERXY, "%s", szInfo);
	}
}

void CFeatures_ESP::DrawBoss(C_BaseEntity* pEntity)
{
	C_Infected* pInf = pEntity ? pEntity->As<C_Infected*>() : nullptr;

	if (!pInf || !G::Util.IsInfectedAlive(pInf->m_usSolidFlags(), pInf->m_nSequence()))
		return;

	int x, y, w, h;

	if (!GetBounds(pEntity, x, y, w, h) || w <= 0 || h <= 0)
		return;

	const Color clrBoss(200, 0, 255, 255);
	G::Draw.OutlinedRect(x - 1, y - 1, w + 2, h + 2, { 10, 10, 12, 200 });
	G::Draw.OutlinedRect(x, y, w, h, clrBoss);
	DrawCorners(x, y, w, h, clrBoss);
	G::Draw.String(EFonts::ESP_NAME, x + (w / 2), y - G::Draw.GetFontHeight(EFonts::ESP_NAME), clrBoss, TXT_CENTERXY, "WITCH");
}

void CFeatures_ESP::DrawUnknown(C_TerrorPlayer* pLocal, C_BaseEntity* pEntity, const char* szNetworkName)
{
	if (!pLocal || !pEntity || !szNetworkName || !szNetworkName[0])
		return;

	// Имя класса в нижний регистр, ищем подстроку (переживает префиксы типа CBoomer).
	char szLower[64] = { };
	for (int i = 0; i < 63 && szNetworkName[i]; i++)
		szLower[i] = (char)tolower((unsigned char)szNetworkName[i]);

	struct Known_t { const char* sub; const char* show; bool bCommon; };
	static const Known_t kKnown[] = {
		{ "hunter", "HUNTER", false }, { "smoker", "SMOKER", false }, { "boomer", "BOOMER", false },
		{ "jockey", "JOCKEY", false }, { "spitter", "SPITTER", false }, { "charger", "CHARGER", false },
		{ "tank", "TANK", false }, { "witch", "WITCH", false },
		{ "common", "COMMON", true }, { "infected", "COMMON", true }, { "zombie", "COMMON", true },
	};
	const Known_t* pFound = nullptr;
	for (size_t i = 0; i < sizeof(kKnown) / sizeof(kKnown[0]); i++)
	{
		if (strstr(szLower, kKnown[i].sub)) { pFound = &kKnown[i]; break; }
	}
	if (!pFound)
		return;
	const char* szShow = pFound->show;

	int x, y, w, h;
	if (!GetBounds(pEntity, x, y, w, h) || w <= 0 || h <= 0)
		return;

	// Обычные: team=0, жизненный цикл не как у игроков — только бокс.
	// Ведьма тоже идёт сюда: она C_Infected, а не C_BasePlayer — каст ниже
	// читал бы чужой оффсет. Проверка живости как у DrawBoss.
	if (pFound->bCommon || !strcmp(szShow, "WITCH"))
	{
		if (!strcmp(szShow, "WITCH"))
		{
			C_Infected* pWitch = pEntity->As<C_Infected*>();
			if (!pWitch || !G::Util.IsInfectedAlive(pWitch->m_usSolidFlags(), pWitch->m_nSequence()))
				return;
			if (!Vars::ESP::bBossBoxes)
				return;
		}
		else if (!Vars::ESP::bCommon)
			return;
		const Color clrCommon(170, 60, 60, 220);
		if (Vars::ESP::bFilled)
			G::Draw.Rect(x, y, w, h, { 170, 60, 60, 40 });
		G::Draw.OutlinedRect(x - 1, y - 1, w + 2, h + 2, { 10, 10, 12, 200 });
		G::Draw.OutlinedRect(x, y, w, h, clrCommon);
		DrawCorners(x, y, w, h, clrCommon);
		G::Draw.String(EFonts::ESP, x + (w / 2), y - G::Draw.GetFontHeight(EFonts::ESP), clrCommon, TXT_CENTERXY, "%s", szShow);
		return;
	}

	// Лёгкие проверки как у DrawSpecial (раскладка СИ shared с CTerrorPlayer).
	C_BasePlayer* pPl = pEntity->As<C_BasePlayer*>();
	if (!pPl || pPl->m_lifeState() != 0)
		return;
	const int nTeam = pEntity->m_iTeamNum();
	if ((nTeam != TEAM_SURVIVOR && nTeam != TEAM_INFECTED) || nTeam == pLocal->GetTeamNumber())
		return;

	const Color& clrTeam = Vars::Chams::clrEnemy;
	if (Vars::ESP::bFilled)
		G::Draw.Rect(x, y, w, h, { clrTeam.r(), clrTeam.g(), clrTeam.b(), 40 });
	G::Draw.OutlinedRect(x - 1, y - 1, w + 2, h + 2, { 10, 10, 12, 200 });
	G::Draw.OutlinedRect(x, y, w, h, clrTeam);
	G::Draw.String(EFonts::ESP_NAME, x + (w / 2), y - G::Draw.GetFontHeight(EFonts::ESP_NAME), clrTeam, TXT_CENTERXY, "%s", szShow);
}

bool CFeatures_ESP::GetBounds(C_BaseEntity* pBaseEntity, int& x, int& y, int& w, int& h)
{
	if (!pBaseEntity)
		return false;

	// Как у space: никаких виртуалок (RenderableToWorldTransform лежит на
	// непроверенном слоте и кривил боксы). Только нетвары: низ/верх корпуса
	// проецируем в экран, ширину берём пропорцией от высоты.
	const Vector vOrigin = pBaseEntity->m_vecOrigin();
	const float flBottomZ = vOrigin.z + pBaseEntity->m_vecMins().z;
	const float flTopZ = vOrigin.z + pBaseEntity->m_vecMaxs().z;
	const float flHullH = flTopZ - flBottomZ;

	if (flHullH < 2.0f)
		return false;

	Vector vFeetS, vHeadS;
	if (!G::Util.W2S(Vector(vOrigin.x, vOrigin.y, flBottomZ), vFeetS)
		|| !G::Util.W2S(Vector(vOrigin.x, vOrigin.y, flTopZ), vHeadS))
		return false;

	const float flH = vFeetS.y - vHeadS.y;

	if (flH < 4.0f)
		return false;

	const float flW = flH * 0.55f;
	const float flCX = (vFeetS.x + vHeadS.x) * 0.5f;

	x = static_cast<int>(flCX - flW * 0.5f);
	y = static_cast<int>(vHeadS.y);
	w = static_cast<int>(flW);
	h = static_cast<int>(flH);

	return !(x > G::Draw.m_nScreenW || (x + w) < 0 || y > G::Draw.m_nScreenH || (y + h) < 0);
}

void CFeatures_ESP::DrawTeam(C_TerrorPlayer* pLocal)
{
	if (!Vars::ESP::bTeamPanel || !I::EngineClient || !pLocal || !G::Draw.m_nScreenW)
		return;

	const int nLocalTeam = pLocal->GetTeamNumber();
	const int nMax = I::ClientEntityList ? I::ClientEntityList->GetMaxEntities() : 0;
	int nY = 120;

	G::Draw.String(EFonts::ESP, 16, nY - 16, Color(140, 160, 152, 255), TXT_DEFAULT, "TEAM");
	for (int n = 1; n <= nMax && nY < G::Draw.m_nScreenH - 40; n++)
	{
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
		const int nHp = pT->GetHealth();
		if (pT->deadflag() || pT->m_lifeState() != 0 || nHp <= 0)
			continue;
		player_info_t pi = { };
		if (!I::EngineClient->GetPlayerInfo(n, &pi) || !pi.name[0])
			continue;

		const int nMaxHp = U::Math.Clamp(pT->m_iMaxHealth(), 1, 200);
		const int nPct = U::Math.Clamp(nHp * 100 / nMaxHp, 0, 100);
		G::Draw.String(EFonts::ESP, 16, nY, Color(235, 245, 240, 255), TXT_DEFAULT, "%s", pi.name);
		G::Draw.Rect(16, nY + 13, 120, 5, Color(10, 12, 11, 200));
		const Color clrBar(nPct > 50 ? 0 : 255, nPct > 50 ? 255 : (nPct > 25 ? 200 : 70), 70, 255);
		G::Draw.Rect(16, nY + 13, 120 * nPct / 100, 5, clrBar);
		nY += 26;
	}
}

void CFeatures_ESP::DrawThrowables()
{
	static std::map<int, float> s_mSeen; // entindex -> curtime первого кадра
	if (!I::EngineClient || !I::EngineClient->IsInGame())
	{
		// Карта сменилась/выход: часы curtime сбросятся, старые метки врут.
		if (!s_mSeen.empty())
			s_mSeen.clear();
		return;
	}
	if (!Vars::ESP::bThrowTimers || !I::GlobalVars)
		return;
	std::map<int, float> mNow;
	const float flNow = I::GlobalVars->curtime;
	const int nMax = I::ClientEntityList ? I::ClientEntityList->GetMaxEntities() : 0;

	for (int n = 1; n <= nMax; n++)
	{
		IClientEntity* pEntity = I::ClientEntityList->GetClientEntity(n);
		if (!pEntity || pEntity->IsDormant())
			continue;
		ClientClass* pCC = pEntity->GetClientClass();
		if (!pCC)
			continue;

		const char* szLabel = nullptr;
		float flFuse = 0.0f; // 0 = без таймера (ломается о землю)
		switch (pCC->m_ClassID)
		{
			case CPipeBombProjectile: szLabel = "PIPE"; flFuse = 6.0f; break;
			case CMolotovProjectile: szLabel = "MOLOTOV"; break;
			case CVomitJarProjectile: szLabel = "BILE"; break;
			default: continue;
		}

		C_BaseEntity* pEnt = pEntity->As<C_BaseEntity*>();
		if (!pEnt)
			continue;

		if (!s_mSeen.count(n))
			s_mSeen[n] = flNow;
		mNow[n] = s_mSeen[n];

		float flAge = flNow - s_mSeen[n];
		if (flAge < 0.0f)
		{
			// Часы отмотало назад (смена карты без выхода): метка stale, обновляем.
			s_mSeen[n] = flNow;
			mNow[n] = flNow;
			flAge = 0.0f;
		}

		Vector vS;
		if (!G::Util.W2S(pEnt->m_vecOrigin(), vS))
			continue;

		char sz[48] = { };
		if (flFuse > 0.0f)
		{
			const float flLeft = flFuse - flAge;
			sprintf_s(sz, "%s %.1f", szLabel, flLeft > 0.0f ? flLeft : 0.0f);
		}
		else
			sprintf_s(sz, "%s", szLabel);
		G::Draw.String(EFonts::ESP_NAME, (int)vS.x, (int)vS.y, Color(255, 220, 0, 255), TXT_CENTERXY, "%s", sz);

		// Кольцо растёт пока горит запал пайпа.
		if (flFuse > 0.0f)
		{
			const float flGrow = flAge * 4.0f;
			const int nR = 6 + (int)(flGrow > 24.0f ? 24.0f : flGrow);
			G::Draw.OutlinedCircle((int)vS.x, (int)vS.y + 16, nR, 20, Color(255, 220, 0, 200));
		}
	}

	s_mSeen.swap(mNow);
}
