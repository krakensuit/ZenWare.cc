#include "Visuals.h"

#include "../Vars.h"
#include "../Hitmarker/Hitmarker.h"
#include "../../SDK/L4D2/Interfaces/Cvar.h"

#include <cwchar>
#include <cstring>

static const Color CLR_TEXT_HINT(140, 160, 152, 255);

void CFeatures_Visuals::UpdateThirdPerson()
{
	static bool s_bWasOn = false;
	static int s_nLastDist = -1;
	const bool bWant = Vars::Visuals::bThirdPerson && I::EngineClient && I::EngineClient->IsInGame();
	const int nDist = U::Math.Clamp(Vars::Visuals::nThirdPersonDist, 30, 200);

	// Как у space: консоль не трогаем вообще (ClientCmd_Unrestricted лежит
	// на непроверенном слоте vtable и ронял игру при включении).
	// Камера отъезжает через z_view_distance: 0 = от 1-го лица, <0 = дистанция.
	if (!I::Cvar)
		return;

	if (bWant)
	{
		// Дистанцию досылаем и на ходу, а не только в момент включения.
		if (!s_bWasOn || nDist != s_nLastDist)
		{
			if (ConVar* pView = I::Cvar->FindVar("z_view_distance"))
				pView->SetValue(-nDist);
			s_nLastDist = nDist;
		}
		s_bWasOn = true;
		return;
	}

	if (s_bWasOn)
	{
		s_bWasOn = false;
		s_nLastDist = -1;
		if (ConVar* pView = I::Cvar->FindVar("z_view_distance"))
			pView->SetValue(0);
	}
}

void CFeatures_Visuals::UpdateFullbright()
{
	// Как 3-е лицо: только через ICvar, консоль не трогаем. При выключении
	// возвращаем 0, иначе фулбрайт залипает до перезапуска карты.
	static bool s_bWasOn = false;
	if (!I::Cvar)
		return;

	const bool bWant = Vars::Visuals::bFullbright && I::EngineClient && I::EngineClient->IsInGame();
	if (bWant == s_bWasOn)
		return;

	if (ConVar* pBright = I::Cvar->FindVar("mat_fullbright"))
		pBright->SetValue(bWant ? 1 : 0);
	s_bWasOn = bWant;
}

void CFeatures_Visuals::UpdateHideHands()
{
	// Как фулбрайт: только через ICvar, консоль не трогаем. При выключении
	// возвращаем 1, иначе руки пропадают до перезапуска карты.
	static bool s_bWasOn = false;
	if (!I::Cvar)
		return;

	const bool bWant = Vars::Visuals::bHideHands && I::EngineClient && I::EngineClient->IsInGame();
	if (bWant == s_bWasOn)
		return;

	if (ConVar* pHands = I::Cvar->FindVar("r_drawviewmodel"))
		pHands->SetValue(bWant ? 0 : 1);
	s_bWasOn = bWant;
}

void CFeatures_Visuals::DrawGrenade()
{
	if (!Vars::Grenade::bEnabled || !I::EngineClient || !I::EngineClient->IsInGame() || !I::ClientEntityList)
		return;

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

	// Только с throwable в руках (молотов/пайп/желчь) или гранатомётом.
	// Виртуалку GetWeaponID дёргаем только на этих 4 классах: в руках может
	// быть медкит/меле/пила с чужой таблицей (IsWeaponEntity их пропускает).
	C_BaseCombatWeapon* pBase = pLocal->GetActiveWeapon();
	C_TerrorWeapon* pWpn = nullptr;
	if (pBase && G::Util.IsWeaponEntity(pBase))
	{
		ClientClass* pWCC = pBase->GetClientClass();
		if (pWCC && U::Math.CompareGroup(pWCC->m_ClassID, CMolotov, CPipeBomb, CItem_VomitJar, CGrenadeLauncher))
			pWpn = pBase->As<C_TerrorWeapon*>();
	}
	if (!pWpn)
		return;
	float flSpeed = 900.0f, flUp = 150.0f, flElast = 0.45f;
	if (pWpn->GetWeaponID() == WEAPON_GRENADE_LAUNCHER)
	{
		flSpeed = 1200.0f; flUp = 100.0f; flElast = 0.5f;
	}
	else if (!U::Math.CompareGroup(pWpn->GetWeaponID(), WEAPON_MOLOTOV, WEAPON_PIPEBOMB, WEAPON_VOMITJAR))
		return;

	Vector vAng;
	I::EngineClient->GetViewAngles(vAng);
	Vector vFwd;
	U::Math.AngleVectors(vAng, &vFwd);

	// Старт из глаз + чуть вперёд, начальная скорость вдоль взгляда + наследие бега.
	Vector vPos = G::Util.GetEyePosition(pLocal) + vFwd * 16.0f;
	Vector vVel = vFwd * flSpeed + Vector(0.0f, 0.0f, flUp) + pLocal->m_vecVelocity() * 0.5f;

	constexpr float flStep = 1.0f / 30.0f;
	constexpr float flGravity = 800.0f;
	const Color clrPath(0, 255, 171, 220);
	const Color clrLand(255, 70, 70, 255);

	CTraceFilterHitAll filter(static_cast<IHandleEntity*>(pLocal));

	// Кэш сегментов: симуляция делает до 90 трейсов движка, каждый кадр это
	// роняло FPS с гранатой в руках. Пересчёт только когда съехали глаза/
	// взгляд/скорость/оружие или прошло 100мс. Рисуем всегда из кэша.
	static Vector s_vEyeK; static Vector s_vVelK; static int s_nWpnK = -1;
	static unsigned long long s_ullK = 0;
	static Vector s_aPath[96]; static int s_nPath = 0;
	static Vector s_vLandK; static bool s_bLandK = false;

	const unsigned long long ullNow = GetTickCount64();
	const Vector vEye0 = G::Util.GetEyePosition(pLocal);
	const Vector vVelKey = vVel; // vVel ниже мутирует в симуляции — ключ снимаем до.
	const int nWpnID = pWpn->GetWeaponID();
	const bool bSame = (s_nPath > 1 && s_nWpnK == nWpnID
		&& (vEye0 - s_vEyeK).LenghtSqr() < 1.0f
		&& (vVelKey - s_vVelK).LenghtSqr() < 1.0f
		&& ullNow - s_ullK < 100);

	if (!bSame)
	{
		Vector vSeg = vPos;
		s_aPath[0] = vSeg;
		s_nPath = 1;
		s_bLandK = false;
		int nBounces = 0;

		for (int i = 0; i < 90 && !s_bLandK && s_nPath < 96; i++)
		{
			vVel.z -= flGravity * flStep;
			const Vector vNext = vSeg + vVel * flStep;

			trace_t tr{};
			G::Util.Trace(vSeg, vNext, MASK_SOLID, &filter, &tr);

			Vector vEnd = vNext;
			if (tr.DidHit())
			{
				vEnd = tr.endpos;
				// Отражение от плоскости с потерей скорости.
				const Vector& n = tr.plane.normal;
				vVel = (vVel - n * (2.0f * vVel.Dot(n))) * flElast;
				if (++nBounces >= 2 || vVel.LenghtSqr() < 900.0f) // <30 u/s — легла
				{
					s_vLandK = vEnd;
					s_bLandK = true;
				}
			}

			s_aPath[s_nPath++] = vEnd;
			vSeg = s_bLandK ? vEnd : (tr.DidHit() ? vEnd + tr.plane.normal * 1.0f : vNext);
		}

		s_vEyeK = vEye0;
		s_vVelK = vVelKey;
		s_nWpnK = nWpnID;
		s_ullK = ullNow;
	}

	for (int i = 0; i + 1 < s_nPath; i++)
	{
		Vector vA, vB;
		if (G::Util.W2S(s_aPath[i], vA) && G::Util.W2S(s_aPath[i + 1], vB))
			G::Draw.Line((int)vA.x, (int)vA.y, (int)vB.x, (int)vB.y, clrPath);
	}

	if (s_bLandK && Vars::Grenade::bLanding)
	{
		Vector vS;
		if (G::Util.W2S(s_vLandK, vS))
		{
			const int x = (int)vS.x, y = (int)vS.y;
			G::Draw.OutlinedCircle(x, y, 8, 16, clrLand);
			G::Draw.Line(x - 12, y, x - 4, y, clrLand);
			G::Draw.Line(x + 4, y, x + 12, y, clrLand);
			G::Draw.Line(x, y - 12, x, y - 4, clrLand);
			G::Draw.Line(x, y + 4, x, y + 12, clrLand);
		}
	}
}

void CFeatures_Visuals::DrawCrosshair()
{
	//IsInGame как у соседей (DrawGrenade/DrawOverlay): иначе прицел в меню/табе.
	if (!Vars::Visuals::bCrosshair || !G::Draw.m_nScreenW || !G::Draw.m_nScreenH
		|| !I::EngineClient || !I::EngineClient->IsInGame() || !I::ClientEntityList)
		return;

	const int nCX = G::Draw.m_nScreenW / 2;
	const int nCY = G::Draw.m_nScreenH / 2;
	const int nS = U::Math.Clamp(Vars::Visuals::nCrosshairSize, 2, 40);
	const Color& clr = Vars::Visuals::clrCrosshair;

 //Gap breathes with movement speed.
 float flSpeed = 0.0f;
 C_TerrorPlayer* pLocal = nullptr;
 {
  const int nLocalIdx = I::EngineClient->GetLocalPlayer();
  if (nLocalIdx > 0 && I::ClientEntityList)
  {
   IClientEntity* pEnt = I::ClientEntityList->GetClientEntity(nLocalIdx);
   if (G::Util.IsPlayerEntity(pEnt)) pLocal = pEnt->As<C_TerrorPlayer*>();
  }
 }

 if (pLocal)
  flSpeed = pLocal->m_vecVelocity().Lenght2D();

	const int nGap = 4 + U::Math.Clamp((int)(flSpeed / 50.0f), 0, 12);
	constexpr int nThick = 2;

	G::Draw.Rect(nCX - nGap - nS, nCY - (nThick / 2), nS, nThick, clr);
	G::Draw.Rect(nCX + nGap, nCY - (nThick / 2), nS, nThick, clr);
	G::Draw.Rect(nCX - (nThick / 2), nCY - nGap - nS, nThick, nS, clr);
	G::Draw.Rect(nCX - (nThick / 2), nCY + nGap, nThick, nS, clr);
	G::Draw.Rect(nCX - 1, nCY - 1, 2, 2, clr);

	//HUD оружия: имя + магазин + запас под прицелом. Хендл активного оружия
	//в тике смены может указывать на viewmodel/руки/меле: GetWeaponID только
	//за IsGunEntity, иначе виртуалка идёт по чужой таблице.
	if (Vars::Visuals::bWeaponHud && pLocal && I::ClientEntityList)
	{
		EHANDLE hActive = pLocal->m_hActiveWeapon();
		C_BaseCombatWeapon* pActive = nullptr;
		if (hActive.IsValid())
		{
			IClientEntity* pViaHandle = I::ClientEntityList->GetClientEntityFromHandle(hActive);
			if (G::Util.IsWeaponEntity(pViaHandle)) pActive = pViaHandle->As<C_BaseCombatWeapon*>();
		}
		if (pActive)
		{
			const wchar_t* wszName = nullptr;
			wchar_t wszFallback[48] = { };
			C_TerrorWeapon* pTW = pActive->As<C_TerrorWeapon*>();
			// Меле/пила/гренник — сиблинги с чужой vtable: виртуалку только за IsGunEntity.
			const int nID = (pTW && G::Util.IsGunEntity(pActive)) ? pTW->GetWeaponID() : 0;
			if (nID > 0 && nID < 38 && wcscmp(g_aSpawnInfo[nID].m_szName, L"unknown") != 0)
				wszName = g_aSpawnInfo[nID].m_szName;
			else
			{
				ClientClass* pWCC = pActive->GetClientClass();
				if (pWCC && pWCC->m_pNetworkName && pWCC->m_pNetworkName[0])
				{
					swprintf_s(wszFallback, L"%hs", pWCC->m_pNetworkName[0] == 'C' ? pWCC->m_pNetworkName + 1 : pWCC->m_pNetworkName);
					wszName = wszFallback;
				}
			}
			if (wszName && wszName[0])
			{
				G::Draw.String(EFonts::ESP, nCX, nCY + 24, Color(220, 220, 220, 255), TXT_CENTERXY, "%ls", wszName);

				//Запас: m_iAmmo — массив int[32] на игроке, индекс = ammo type ствола.
				int nReserve = -1;
				const int nType = pActive->m_iPrimaryAmmoType();
				if (nType >= 0 && nType < 32)
				{
					const int* pAmmo = (const int*)pLocal->m_iAmmo();
					if (pAmmo && pAmmo[nType] >= 0 && pAmmo[nType] <= 9999)
						nReserve = pAmmo[nType];
				}
				const int nClip = pActive->m_iClip1();
				const bool bReloading = pActive->m_bInReload();
				wchar_t wszAmmo[32] = { };
				Color clrAmmo(235, 245, 240, 255);
				if (Vars::Visuals::bReloadAlert && bReloading)
				{
					wcscpy_s(wszAmmo, L"RELOADING");
					clrAmmo = Color(255, 220, 0, 255);
				}
				else if (nClip >= 0 && nClip <= 999)
				{
					if (nReserve >= 0)
						swprintf_s(wszAmmo, L"%d | %d", nClip, nReserve);
					else
						swprintf_s(wszAmmo, L"%d", nClip);
					if (Vars::Visuals::bReloadAlert && nClip <= 5)
						clrAmmo = Color(255, 70, 70, 255); //LOW AMMO
				}
				if (wszAmmo[0])
					G::Draw.String(EFonts::ESP, nCX, nCY + 38, clrAmmo, TXT_CENTERXY, "%ls", wszAmmo);
			}
		}
	}

	// Круг разброса NoSpread: радиус ~ текущий спред ствола.
	if (Vars::Visuals::bSpreadCircle && pLocal && I::ClientEntityList)
	{
		float flSpread = 0.0f;
		EHANDLE hActive = pLocal->m_hActiveWeapon();
		if (hActive.IsValid())
		{
			IClientEntity* pViaHandle = I::ClientEntityList->GetClientEntityFromHandle(hActive);
			//IsGunEntity гейтит чужую таблицу: GetCurrentSpread — виртуалка
			//C_TerrorWeapon, на меле/пиле/греннике слот чужой (краш).
			if (G::Util.IsGunEntity(pViaHandle))
			{
				C_TerrorWeapon* pTW = pViaHandle->As<C_TerrorWeapon*>();
				if (pTW) flSpread = pTW->GetCurrentSpread();
			}
		}
		if (flSpread > 0.0001f)
		{
			const int nR = U::Math.Clamp((int)(flSpread * 1200.0f), 4, 120);
			G::Draw.Circle(nCX, nCY, nR, 32, Color(clr.r(), clr.g(), clr.b(), 160));
		}
	}

	// Крест попадания: 0.25 c после зачтённого урона.
	if (Vars::Hitmarker::bEnabled && Vars::Hitmarker::bXMark)
	{
		const float flAge = F::Hitmarker.SecondsSinceHit();
		if (flAge >= 0.0f && flAge < 0.25f)
		{
			const Color clrX(255, 70, 70, 255);
			G::Draw.Line(nCX - 12, nCY - 12, nCX - 6, nCY - 6, clrX);
			G::Draw.Line(nCX + 6, nCY - 12, nCX + 12, nCY - 6, clrX);
			G::Draw.Line(nCX - 12, nCY + 12, nCX - 6, nCY + 6, clrX);
			G::Draw.Line(nCX + 6, nCY + 12, nCX + 12, nCY + 6, clrX);
		}
	}
}
//Имя клавиши для панели биндов: мышь отдельно, остальное через Win32.
static void KeyLabel(const int nVk, char* const szOut, const int nSize)
{
	if (nVk <= 0) { strcpy_s(szOut, nSize, "always"); return; }
	switch (nVk)
	{
		case VK_LBUTTON: strcpy_s(szOut, nSize, "LMB"); return;
		case VK_RBUTTON: strcpy_s(szOut, nSize, "RMB"); return;
		case VK_MBUTTON: strcpy_s(szOut, nSize, "MMB"); return;
		case VK_XBUTTON1: strcpy_s(szOut, nSize, "M4"); return;
		case VK_XBUTTON2: strcpy_s(szOut, nSize, "M5"); return;
		case VK_SPACE: strcpy_s(szOut, nSize, "Space"); return;
		default: break;
	}
	const UINT nSc = MapVirtualKeyA((UINT)nVk, MAPVK_VK_TO_VSC);
	if (nSc && GetKeyNameTextA((LONG)(nSc << 16), szOut, nSize))
		return;
	sprintf_s(szOut, nSize, "%d", nVk);
}

void CFeatures_Visuals::DrawOverlay()
{
	if (!I::EngineClient || !I::EngineClient->IsInGame() || !I::GlobalVars || !I::ClientEntityList)
		return;

 C_TerrorPlayer* pLocal = nullptr;
 {
  const int nLocalIdx = I::EngineClient->GetLocalPlayer();
  if (nLocalIdx > 0 && I::ClientEntityList)
  {
   IClientEntity* pEnt = I::ClientEntityList->GetClientEntity(nLocalIdx);
   if (G::Util.IsPlayerEntity(pEnt)) pLocal = pEnt->As<C_TerrorPlayer*>();
  }
 }

 if (!pLocal)
  return;

 //В спектаторе/трупе — мертвецкие pos/hp (DrawGrenade так гейтит).
 if (pLocal->deadflag() || pLocal->m_lifeState() != 0)
  return;

	static float s_flFpsAvg = 0.0f;
	const float flFps = (I::GlobalVars->frametime > 0.0f) ? (1.0f / I::GlobalVars->frametime) : 0.0f;
	s_flFpsAvg = (s_flFpsAvg == 0.0f) ? flFps : (s_flFpsAvg * 0.95f + flFps * 0.05f);

	const Vector vPos = pLocal->m_vecOrigin();
	const float flSpeed = pLocal->m_vecVelocity().Lenght2D();

	if (Vars::Visuals::bOverlay)
	{
		G::Draw.String(EFonts::MENU_CONSOLAS, 8, G::Draw.m_nScreenH - 34,
			CLR_TEXT_HINT, TXT_DEFAULT, "fps %4.0f | pos %.0f %.0f %.0f | hp %i",
			s_flFpsAvg, vPos.x, vPos.y, vPos.z, pLocal->GetHealth());
	}

	// Common counter: alive commons near local (poll, no engine events).
	if (Vars::Visuals::bCommonCount && I::ClientEntityList)
	{
		int nCommons = 0;
		const Vector vMyPos = pLocal->m_vecOrigin();
		const int nMax = I::ClientEntityList->GetMaxEntities();
		for (int n = 1; n <= nMax; n++)
		{
			IClientEntity* pEntity = I::ClientEntityList->GetClientEntity(n);
			if (!pEntity || pEntity->IsDormant())
				continue;
			ClientClass* pCC = pEntity->GetClientClass();
			if (!pCC || pCC->m_ClassID != Infected)
				continue;
			C_Infected* pInf = pEntity->As<C_Infected*>();
			if (!pInf || !G::Util.IsInfectedAlive(pInf->m_usSolidFlags(), pInf->m_nSequence()))
				continue;
			C_BaseEntity* pEnt = pEntity->As<C_BaseEntity*>();
			if (!pEnt)
				continue;
			if ((pEnt->m_vecOrigin() - vMyPos).LenghtSqr() > 2000.0f * 2000.0f)
				continue;
			nCommons++;
		}
		G::Draw.String(EFonts::MENU_CONSOLAS, 8, G::Draw.m_nScreenH - 74,
			(nCommons > 0) ? Color(255, 220, 0, 255) : CLR_TEXT_HINT, TXT_DEFAULT, "commons %d", nCommons);
	}

	// Speed HUD - movement feature
	if (Vars::BunnyHop::bSpeedHUD)
	{
		Color clrSpd = { 255, 255, 255, 255 };
		if (flSpeed > 300) clrSpd = { 0, 255, 171, 255 };
		if (flSpeed > 400) clrSpd = { 255, 220, 0, 255 };
		G::Draw.String(EFonts::MENU_CONSOLAS, 8, G::Draw.m_nScreenH - 54,
			clrSpd, TXT_DEFAULT, "speed %.0f u/s", flSpeed);
	}

	// Панель биндов: клавиши фич + удержание. Справа, под карточками киллфида.
	if (Vars::Visuals::bBindList)
	{
		char szAim[32], szMenu[32], szPanic[32];
		KeyLabel(Vars::Aimbot::nKey, szAim, sizeof(szAim));
		KeyLabel(Vars::Menu::nKey, szMenu, sizeof(szMenu));
		KeyLabel(Vars::ESP::nPanicKey, szPanic, sizeof(szPanic));
		const bool bAim = !Vars::Aimbot::nKey || (GetAsyncKeyState(Vars::Aimbot::nKey) & 0x8000);
		const bool bHop = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
		const int nBX = G::Draw.m_nScreenW - 228;
		G::Draw.String(EFonts::MENU_CONSOLAS, nBX, 280, Color(140, 160, 152, 255), TXT_DEFAULT, "binds");
		G::Draw.String(EFonts::MENU_CONSOLAS, nBX, 296, bAim ? Color(0, 255, 171, 255) : Color(120, 130, 126, 255), TXT_DEFAULT, "aimbot [%s]", szAim);
		G::Draw.String(EFonts::MENU_CONSOLAS, nBX, 312, bHop ? Color(0, 255, 171, 255) : Color(120, 130, 126, 255), TXT_DEFAULT, "bhop [Space]");
		G::Draw.String(EFonts::MENU_CONSOLAS, nBX, 328, Color(120, 130, 126, 255), TXT_DEFAULT, "menu [%s]", szMenu);
		G::Draw.String(EFonts::MENU_CONSOLAS, nBX, 344, Color(120, 130, 126, 255), TXT_DEFAULT, "panic [%s]", szPanic);
	}
}
