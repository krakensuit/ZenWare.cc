#include "ESP.h"

#include "../Vars.h"
#include "../../Util/Logger/Logger.h"

#include <cctype>
#include <cstring>
#include <map>

namespace
{
	// Рамка как у space: цветная 1px + чёрная снаружи + чёрная изнутри.
	// Уголки убраны: тройная вложенность выглядела криво на мелких боксах.
	void DrawEspBox(int x, int y, int w, int h, const Color& clr)
	{
		if (w <= 2 || h <= 2)
		{
			G::Draw.OutlinedRect(x, y, w, h, clr);
			return;
		}
		G::Draw.OutlinedRect(x - 1, y - 1, w + 2, h + 2, { 0, 0, 0, 255 });
		G::Draw.OutlinedRect(x, y, w, h, clr);
		G::Draw.OutlinedRect(x + 1, y + 1, w - 2, h - 2, { 0, 0, 0, 255 });
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
				// Show Team как у space: своих рисуем только по тумблеру.
				else if (Vars::ESP::bShowTeam && pPlayer && !pPlayer->deadflag()
					&& pPlayer->m_lifeState() == 0 && pPlayer->GetHealth() > 0
					&& pPlayer->GetTeamNumber() == pLocal->GetTeamNumber()
					&& G::Util.IsValidTeam(pPlayer->GetTeamNumber()))
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

	if (!GetBounds(pPlayer, x, y, w, h))
		return;

	const bool bIsEnemy = (pPlayer->GetTeamNumber() != pLocal->GetTeamNumber());
	const Color clrTeam = bIsEnemy ? Vars::Chams::clrEnemy : Vars::Chams::clrAlly;
	const int nMaxHp = U::Math.Clamp(pPlayer->m_iMaxHealth(), 1, 200);
	const int nHealth = U::Math.Clamp(pPlayer->GetHealth(), 0, nMaxHp);
	const int nCenterX = x + (w / 2);
	const float flDistM = G::Util.GetEyePosition(pLocal).DistTo(G::Util.GetEyePosition(pPlayer)) / 52.5f;

	if (Vars::ESP::bBox)
	{
		if (Vars::ESP::bFilled)
			G::Draw.Rect(x, y, w, h, { clrTeam.r(), clrTeam.g(), clrTeam.b(), 40 });
		DrawEspBox(x, y, w, h, clrTeam);
	}

	//Snapline from the bottom of the screen.
	if (Vars::ESP::bSnaplines)
		G::Draw.Line(G::Draw.m_nScreenW / 2, G::Draw.m_nScreenH, x + (w / 2), y + h, clrTeam);

	// ХП-бар слева как у space: чёрная подложка на всю высоту + зелёная
	// заливка снизу вверх по доле от max HP (не от 100 — у СИ больше).
	if (Vars::ESP::bHealthBar)
	{
		const int nBarX = x - 5;

		if (nBarX >= 0)
		{
			const int nFillH = (h * nHealth) / nMaxHp;
			G::Draw.Rect(nBarX, y, 4, h, { 0, 0, 0, 255 });

			if (nFillH > 0)
				G::Draw.Rect(nBarX + 1, y + h - nFillH, 2, nFillH, { 0, 255, 0, 255 });
		}

		if (Vars::ESP::bHealthText)
			G::Draw.String(EFonts::ESP, nBarX - 4, y + (h / 2), Color(255, 255, 255, 255), TXT_CENTERXY, "%d", nHealth);
	}

	// Ник жёлтым над боксом как у space (не цветом команды).
	if (Vars::ESP::bName)
	{
		player_info_t pi = { };

		if (I::EngineClient->GetPlayerInfo(nEntityIndex, &pi))
		{
			pi.name[31] = '\0';

			if (pi.name[0])
				G::Draw.String(EFonts::ESP_NAME, nCenterX, y - 10, Color(230, 212, 50, 255), TXT_CENTERXY, "%s", pi.name);
		}
	}

	// Низ бокса сверху вниз как у space: дистанция, под ней оружие+патроны.
	int nBelow = y + h + 8;

	if (Vars::ESP::bDistance)
	{
		G::Draw.String(EFonts::ESP, nCenterX, nBelow, Color(255, 255, 255, 255), TXT_CENTERXY, "[%.0fm]", flDistM);
		nBelow += 13;
	}

	// Оружие в руках — через netvar m_hActiveWeapon (надёжнее виртуалки).
	// Блок НЕ вложен в bName: раньше текст оружия пропадал вместе с ником.
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
				wchar_t wszLine[80] = { };

				// Патроны как у space: только m_iClip1 активного оружия.
				// Оффсет 0 (нетвар не снялся) читал бы vtable — отсекаем sanity.
				if (Vars::ESP::bAmmo)
				{
					const int nClip = pActive->m_iClip1();

					if (nClip >= 0 && nClip <= 999)
					{
						swprintf_s(wszLine, L"%ls [%d]", wszDisplay, nClip);
						wcscpy_s(wszDisplay, wszLine);
					}
				}

				G::Draw.String(EFonts::ESP_WEAPON, nCenterX, nBelow, Color(220, 220, 220, 255), TXT_CENTERXY, "%ls", wszDisplay);
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
		// Как у space: предметам бокс не рисуем вообще, только текст по центру.
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

	// Как у space: оружию на земле бокс не рисуем, только цветной текст по центру.
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
	DrawEspBox(x, y, w, h, clrCommon);
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
	DrawEspBox(x, y, w, h, clrTeam);
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
	DrawEspBox(x, y, w, h, clrBoss);
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
		DrawEspBox(x, y, w, h, clrCommon);
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
	DrawEspBox(x, y, w, h, clrTeam);
	G::Draw.String(EFonts::ESP_NAME, x + (w / 2), y - G::Draw.GetFontHeight(EFonts::ESP_NAME), clrTeam, TXT_CENTERXY, "%s", szShow);
}

bool CFeatures_ESP::GetBounds(C_BaseEntity* pBaseEntity, int& x, int& y, int& w, int& h)
{
	if (!pBaseEntity)
		return false;

	// Как у space: проекция всех 8 углов AABB (origin+mins/maxs), только нетвары.
	// Старая проекция 2 точек (ступни/голова по центру + ширина 0.55 от высоты)
	// врала ширину и центр на близких и на краях экрана — боксы "отставали".
	if ((pBaseEntity->m_vecMaxs().z - pBaseEntity->m_vecMins().z) < 2.0f)
		return false;

	const Vector vOrigin = pBaseEntity->m_vecOrigin();
	const Vector vMins = pBaseEntity->m_vecMins();
	const Vector vMaxs = pBaseEntity->m_vecMaxs();

	float flMinX = 1e9f, flMinY = 1e9f, flMaxX = -1e9f, flMaxY = -1e9f;

	for (int i = 0; i < 8; i++)
	{
		const Vector vP(
			vOrigin.x + ((i & 1) ? vMaxs.x : vMins.x),
			vOrigin.y + ((i & 2) ? vMaxs.y : vMins.y),
			vOrigin.z + ((i & 4) ? vMaxs.z : vMins.z));
		Vector vS;

		if (!G::Util.W2S(vP, vS))
			return false;

		if (vS.x < flMinX) flMinX = vS.x;
		if (vS.y < flMinY) flMinY = vS.y;
		if (vS.x > flMaxX) flMaxX = vS.x;
		if (vS.y > flMaxY) flMaxY = vS.y;
	}

	x = static_cast<int>(flMinX);
	y = static_cast<int>(flMinY);
	w = static_cast<int>(flMaxX - flMinX);
	h = static_cast<int>(flMaxY - flMinY);

	if (w < 3 || h < 6)
		return false;

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
