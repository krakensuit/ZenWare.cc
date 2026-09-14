#include "Radar.h"

#include "../Lang/Lang.h"
#include "../Vars.h"
#include "../../Util/Logger/Logger.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace
{
	constexpr int kTeamSpectator = 1; // the dump only has SURVIVOR=2/INFECTED=3

	// World -> radar screen. Returns false if the point coincides with us.
	bool ToRadar(const Vector& vLocal, float flYawRad, const Vector& vEnt, float flScale, int& sx, int& sy, int nCX, int nCY, int nR)
	{
		const float dx = vEnt.x - vLocal.x;
		const float dy = vEnt.y - vLocal.y;
		if (dx == 0.0f && dy == 0.0f)
			return false;
		const float s = sinf(flYawRad), c = cosf(flYawRad);
		const float fwd = dx * c + dy * s;   // forward
		const float rgt = dx * s - dy * c;   // right
		float px = rgt * flScale, py = -fwd * flScale;
		const float len = sqrtf(px * px + py * py);
		if (len > (float)nR)
		{
			px = px / len * nR;
			py = py / len * nR;
		}
		sx = nCX + (int)px;
		sy = nCY + (int)py;
		return true;
	}
}

void CFeatures_Radar::Render()
{
	U::Log.Crumb("Radar::Render");
	if ((!Vars::Radar::bEnabled && !Vars::Radar::bSpectators) || !I::EngineClient || !I::EngineClient->IsInGame())
		return;

	const int nLocalIdx = I::EngineClient->GetLocalPlayer();
	C_TerrorPlayer* pLocal = nullptr;
	if (nLocalIdx >= 0 && I::ClientEntityList)
	{
		IClientEntity* pEnt = I::ClientEntityList->GetClientEntity(nLocalIdx);
		if (G::Util.IsPlayerEntity(pEnt)) pLocal = pEnt->As<C_TerrorPlayer*>();
	}
	if (!pLocal)
		return;

	const Vector vLocal = pLocal->m_vecOrigin();
	Vector vAng{};
	I::EngineClient->GetViewAngles(vAng);
	const float flYaw = (float)(vAng.y * (M_PI / 180.0));

	const int nCX = kX + kSize / 2, nCY = kY + kSize / 2, nR = kSize / 2 - 4;
	const float flScale = (float)nR / kRange;

	if (Vars::Radar::bEnabled)
	{
		// Background + outline + crosshair in the menu style.
		G::Draw.Rect(kX, kY, kSize, kSize, Color(10, 12, 11, 210));
		G::Draw.OutlinedRect(kX, kY, kSize, kSize, Color(40, 48, 44, 255));
		G::Draw.OutlinedRect(kX + 1, kY + 1, kSize - 2, kSize - 2, Color(0, 255, 171, 28));
		G::Draw.Line(nCX - nR, nCY, nCX + nR, nCY, Color(255, 255, 255, 14));
		G::Draw.Line(nCX, nCY - nR, nCX, nCY + nR, Color(255, 255, 255, 14));
		G::Draw.Circle(nCX, nCY, nR, 32, Color(0, 255, 171, 40));

		const int nLocalTeam = pLocal->GetTeamNumber();
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

			Color clr(255, 255, 255, 255);
			bool bWant = false;
			const int nID = pCC->m_ClassID;
		if (nID == CTerrorPlayer || nID == SurvivorBot)
		{
			C_TerrorPlayer* pPl = pEntity->As<C_TerrorPlayer*>();
			if (!pPl || pPl->deadflag() || pPl->m_lifeState() != 0 || pPl->GetHealth() <= 0)
				continue;
			//Chams and IsValidTarget cut ghosts, but the radar drew them.
			if (pPl->m_isGhost())
				continue;
				const int nTeam = pPl->GetTeamNumber();
				if (!G::Util.IsValidTeam(nTeam))
					continue;
				clr = (nTeam == nLocalTeam) ? Vars::Chams::clrAlly : Vars::Chams::clrEnemy;
				bWant = true;
			}
		else if (nID == Hunter || nID == Smoker || nID == Jockey || nID == Spitter || nID == Charger || nID == Tank)
		{
				C_BasePlayer* pPl = pEntity->As<C_BasePlayer*>();
				if (!pPl || pPl->m_lifeState() != 0)
					continue;
				C_BaseEntity* pBaseT = pEntity->As<C_BaseEntity*>();
				const int nTeam = pBaseT ? pBaseT->m_iTeamNum() : TEAM_INFECTED;
				clr = (nTeam == nLocalTeam) ? Vars::Chams::clrAlly : Vars::Chams::clrEnemy;
				bWant = true;
			}
		//The witch everywhere (ESP.DrawBoss, Hitmarker) — C_Infected via
		//IsInfectedAlive, not C_BasePlayer with foreign offsets.
		else if (nID == Infected || nID == Witch)
		{
				C_Infected* pInf = pEntity->As<C_Infected*>();
				if (!pInf || !G::Util.IsInfectedAlive(pInf->m_usSolidFlags(), pInf->m_nSequence()))
					continue;
				clr = Color(200, 200, 80, 255);
				bWant = true;
			}
			if (!bWant)
				continue;

			C_BaseEntity* pBase = pEntity->As<C_BaseEntity*>();
			if (!pBase)
				continue;
			int sx = 0, sy = 0;
			if (!ToRadar(vLocal, flYaw, pBase->m_vecOrigin(), flScale, sx, sy, nCX, nCY, nR))
				continue;
			G::Draw.Rect(sx - 2, sy - 2, 5, 5, clr);
			G::Draw.OutlinedRect(sx - 2, sy - 2, 5, 5, Color(0, 0, 0, 180));
		}

		// Us — an up-pointing triangle (forward).
		G::Draw.Line(nCX, nCY - 6, nCX - 4, nCY + 4, Color(245, 255, 250, 255));
		G::Draw.Line(nCX, nCY - 6, nCX + 4, nCY + 4, Color(245, 255, 250, 255));
		G::Draw.Line(nCX - 4, nCY + 4, nCX + 4, nCY + 4, Color(245, 255, 250, 255));
	}

	if (Vars::Radar::bSpectators)
	{
		// Who is watching us: an observer (dead/spectator) targeting us.
		char aNames[8][32] = { };
		int nCount = 0;
		const int nMax = I::ClientEntityList ? I::ClientEntityList->GetMaxEntities() : 0;
		for (int n = 1; n <= nMax && nCount < 8; n++)
		{
			if (n == nLocalIdx)
				continue;
		IClientEntity* pEntity = I::ClientEntityList->GetClientEntity(n);
		if (!pEntity || pEntity->IsDormant())
			continue;
		// Without the class check As<> hands out any object as a player, and then
		// deadflag()/m_lifeState() read a foreign/half-created object
		// (crash on map load and on transitional entities in game).
		if (!G::Util.IsPlayerEntity(pEntity))
			continue;
	C_TerrorPlayer* pPl = pEntity->As<C_TerrorPlayer*>();
	if (!pPl)
		continue;
		// Cheap netvars first, GetPlayerInfo last and only for real candidates:
		// the engine used to be hit on all slots every frame.
		// Only those not actually playing (spectators and the dead).
		const bool bOut = pPl->deadflag() || pPl->m_lifeState() != 0;
		const int nTeam = pPl->GetTeamNumber();
		if (!bOut && nTeam != kTeamSpectator)
			continue;
		if (!pPl->m_hObserverTarget().IsValid())
			continue;
		if (pPl->m_hObserverTarget().GetEntryIndex() != nLocalIdx)
			continue;
		player_info_t pi = {};
		//GetPlayerInfo expects a client slot: do not pass indices above maxclients
		//(bot SIs live on entity indices above the slots).
		if (n > I::EngineClient->GetMaxClients() || !I::EngineClient->GetPlayerInfo(n, &pi) || !pi.name[0])
			continue;
		pi.name[31] = '\0';
		strcpy_s(aNames[nCount], pi.name);
		nCount++;
		}
		const int nY0 = Vars::Radar::bEnabled ? (kY + kSize + 6) : 16;
		//Format is a literal: a translation losing %d would otherwise read the stack.
		G::Draw.String(EFonts::ESP, 16, nY0, Color(140, 160, 152, 255), TXT_DEFAULT, "%s (%d)", Lang::T("Spectators"), nCount);
		for (int i = 0; i < nCount; i++)
			G::Draw.String(EFonts::ESP, 16, nY0 + 14 + i * 13, Color(235, 245, 240, 255), TXT_DEFAULT, "%s", aNames[i]);
	}
}
