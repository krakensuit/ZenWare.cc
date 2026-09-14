#include "Aimbot.h"

#include "../Vars.h"
#include "../../Util/Logger/Logger.h"

#include <cctype>
#include <cstring>

namespace
{
	// ID not in the dump (boomer!) or a different build: identify the SI by class name.
	bool IsSpecialByName(const char* szNet)
	{
		if (!szNet || !szNet[0])
			return false;
		char szLower[64] = { };
		int i = 0;
		for (; i < 63 && szNet[i]; i++)
			szLower[i] = (char)tolower((unsigned char)szNet[i]);
		szLower[i] = '\0';
		static const char* kSubs[] = { "hunter", "smoker", "boomer", "jockey", "spitter", "charger", "tank" };
		for (size_t k = 0; k < sizeof(kSubs) / sizeof(kSubs[0]); k++)
			if (strstr(szLower, kSubs[k]))
				return true;
		return false;
	}

	// The aim point lies INSIDE the target body (head/chest), so a strict
	// !DidHit() is always false: the ray legitimately hits the target itself.
	// Treat it visible if the trace ends on our target.
	bool IsPointVisible(C_TerrorPlayer* pLocal, const Vector& vEyePos, const Vector& vAimPoint, C_BaseEntity* pTarget)
	{
		trace_t tr{};
		CTraceFilterHitAll filter(static_cast<IHandleEntity*>(pLocal));
		G::Util.Trace(vEyePos, vAimPoint, MASK_SHOT, &filter, &tr);
		return !tr.DidHit() || (pTarget && tr.m_pEnt == pTarget);
	}

	// Per-Run snapshot: global aimbot values, overridden by weapon group.
	struct WpnCfg_t { float flFOV; float flSmooth; int nHitbox; int nPrio; };
	WpnCfg_t s_wpn = { 5.0f, 0.0f, 0, 0 };

	// -1 = other (global defaults), otherwise 0..4 index into the Vars::AimbotWpn group.
	int WpnGroupFor(int nID)
	{
		switch (nID)
		{
			case WEAPON_M16A1: case WEAPON_SCAR: case WEAPON_AK47: case WEAPON_SSG552: case WEAPON_M60:
				return 0; // rifles
			case WEAPON_UZI: case WEAPON_MAC10: case WEAPON_MP5:
				return 1; // smg
			case WEAPON_PUMP_SHOTGUN: case WEAPON_AUTO_SHOTGUN: case WEAPON_CHROME_SHOTGUN: case WEAPON_SPAS:
				return 2; // shotguns
			case WEAPON_HUNTING_RIFLE: case WEAPON_MILITARY_SNIPER: case WEAPON_SCOUT: case WEAPON_AWP:
				return 3; // snipers
			case WEAPON_PISTOL: case WEAPON_DEAGLE:
				return 4; // pistols
			default:
				return -1;
		}
	}

	void ResolveWpnCfg(C_TerrorWeapon* pWpn)
	{
		s_wpn = { Vars::Aimbot::flFOV, Vars::Aimbot::flSmoothing, Vars::Aimbot::nHitbox, Vars::Aimbot::nTargetPriority };
		if (!Vars::AimbotWpn::bEnabled || !pWpn)
			return;
		switch (WpnGroupFor(pWpn->GetWeaponID()))
		{
			case 0: s_wpn = { Vars::AimbotWpn::flRifleFov, (float)Vars::AimbotWpn::nRifleSmooth, Vars::AimbotWpn::nRifleHitbox, s_wpn.nPrio }; break;
			case 1: s_wpn = { Vars::AimbotWpn::flSmgFov, (float)Vars::AimbotWpn::nSmgSmooth, Vars::AimbotWpn::nSmgHitbox, s_wpn.nPrio }; break;
			case 2: s_wpn = { Vars::AimbotWpn::flShotgunFov, (float)Vars::AimbotWpn::nShotgunSmooth, Vars::AimbotWpn::nShotgunHitbox, s_wpn.nPrio }; break;
			case 3: s_wpn = { Vars::AimbotWpn::flSniperFov, (float)Vars::AimbotWpn::nSniperSmooth, Vars::AimbotWpn::nSniperHitbox, s_wpn.nPrio }; break;
			case 4: s_wpn = { Vars::AimbotWpn::flPistolFov, (float)Vars::AimbotWpn::nPistolSmooth, Vars::AimbotWpn::nPistolHitbox, s_wpn.nPrio }; break;
			default: break;
		}
		//Corrupt config (NaN/garbage) would otherwise yield a 360-degree lock:
		//flFov > NaN is always false and the FOV filter silently turns off.
		if (!isfinite(s_wpn.flFOV) || s_wpn.flFOV <= 0.0f || s_wpn.flFOV > 180.0f)
			s_wpn.flFOV = 5.0f;
		if (!isfinite(s_wpn.flSmooth) || s_wpn.flSmooth < 0.0f || s_wpn.flSmooth > 64.0f)
			s_wpn.flSmooth = 0.0f;
		//Corrupt hitbox/prio from the config: GetAimPoint would always return
		//false and the aim would never lock, silently.
		if (s_wpn.nHitbox < 0 || s_wpn.nHitbox > 1)
			s_wpn.nHitbox = 0;
		if (s_wpn.nPrio < 0 || s_wpn.nPrio > 1)
			s_wpn.nPrio = 0;
	}

	bool FindCommonTarget(C_TerrorPlayer* pLocal, const Vector& vEyePos, const Vector& vViewAngles, Vector& vOut)
	{
		if (!pLocal || !I::ClientEntityList)
			return false;

		bool bFound = false;
		float flBest = 1e30f;

		for (int n = 1; n <= I::ClientEntityList->GetMaxEntities(); n++)
		{
			IClientEntity* pEntity = I::ClientEntityList->GetClientEntity(n);

			if (!pEntity || pEntity->IsDormant())
				continue;

			ClientClass* pCC = pEntity->GetClientClass();
			if (!pCC)
				continue;

			const int nID = pCC->m_ClassID;
			if (nID != Infected && nID != Witch)
				continue;

			// Commons/Infected — netvars only, no virtual calls.
		C_BaseEntity* pEnt = pEntity->As<C_BaseEntity*>();
		C_Infected* pInf = pEntity->As<C_Infected*>();

		if (!pEnt || !pInf)
			continue;

		// Crash guard for Aimbot:common (0x5AD5): verify the class really is Infected
		ClientClass* pCC2 = pEntity->GetClientClass();
		if (!pCC2 || pCC2->m_ClassID != Infected)
			continue;

		if (!G::Util.IsInfectedAlive(pInf->m_usSolidFlags(), pInf->m_nSequence()))
				continue;

			Vector vAim = pEnt->m_vecOrigin() + Vector(0.0f, 0.0f, pEnt->m_vecMaxs().z * 0.85f);

			if ((vAim - vEyePos).LenghtSqr() < 1.0f)
				continue;

			if (Vars::Aimbot::bVisibleOnly && !IsPointVisible(pLocal, vEyePos, vAim, pEnt))
				continue;

			const float flFov = U::Math.GetFovBetween(vViewAngles, U::Math.GetAngleToPosition(vEyePos, vAim));

			if (flFov > s_wpn.flFOV)
				continue;

			if (flFov < flBest)
			{
				flBest = flFov;
				vOut = vAim;
				bFound = true;
			}
		}

		return bFound;
	}

	bool FindSpecialTarget(C_TerrorPlayer* pLocal, const Vector& vEyePos, const Vector& vViewAngles, Vector& vOut)
	{
		if (!pLocal || !I::ClientEntityList)
			return false;

		bool bFound = false;
		float flBest = 1e30f;
		const int nLocalTeam = pLocal->GetTeamNumber();

		for (int n = 1; n <= I::ClientEntityList->GetMaxEntities(); n++)
		{
			IClientEntity* pEntity = I::ClientEntityList->GetClientEntity(n);

			if (!pEntity || pEntity->IsDormant())
				continue;

			ClientClass* pCC = pEntity->GetClientClass();
			if (!pCC)
				continue;

			const int nID = pCC->m_ClassID;

			if (nID != Hunter && nID != Smoker && nID != Jockey && nID != Spitter && nID != Charger && nID != Tank)
			{
				if (!IsSpecialByName(pCC->m_pNetworkName))
					continue;
			}

			C_BaseEntity* pEnt = pEntity->As<C_BaseEntity*>();
			if (!pEnt)
				continue;

			const int nTeam = pEnt->m_iTeamNum();
			if ((nTeam != TEAM_SURVIVOR && nTeam != TEAM_INFECTED) || nTeam == nLocalTeam)
				continue;

			// Alive check: netvars only, no virtual calls.
			C_BasePlayer* pPl = pEntity->As<C_BasePlayer*>();
			if (!pPl || pPl->m_lifeState() != 0)
				continue;

			Vector vAim = pEnt->m_vecOrigin() + Vector(0.0f, 0.0f, pEnt->m_vecMaxs().z * 0.85f);

			if ((vAim - vEyePos).LenghtSqr() < 1.0f)
				continue;

			if (Vars::Aimbot::bVisibleOnly && !IsPointVisible(pLocal, vEyePos, vAim, pEnt))
				continue;

			const float flFov = U::Math.GetFovBetween(vViewAngles, U::Math.GetAngleToPosition(vEyePos, vAim));

			if (flFov > s_wpn.flFOV)
				continue;

			if (flFov < flBest)
			{
				flBest = flFov;
				vOut = vAim;
				bFound = true;
			}
		}

		return bFound;
	}
}

void CFeatures_Aimbot::Run(C_TerrorPlayer* pLocal, C_TerrorWeapon* pWeapon, CUserCmd* cmd)
{
	U::Log.Crumb("Aimbot::Run");
	if (!ShouldRun(pLocal, pWeapon, cmd))
		return;

	ResolveWpnCfg(pWeapon);

	const Vector vEyePos = G::Util.GetEyePosition(pLocal);
	Vector vViewAngles = cmd->viewangles;

	C_TerrorPlayer* pTarget = FindTarget(pLocal, vEyePos, vViewAngles);
	Vector vAimPoint;
	bool bHavePlayer = (pTarget && GetAimPoint(pTarget, vAimPoint));

	Vector vCommonPoint;
	bool bHaveCommon = (Vars::Aimbot::bTargetCommons && FindCommonTarget(pLocal, vEyePos, vViewAngles, vCommonPoint));

	Vector vSpecialPoint;
	bool bHaveSpecial = (Vars::Aimbot::bTargetSpecials && FindSpecialTarget(pLocal, vEyePos, vViewAngles, vSpecialPoint));

	float flBestPick = 1e30f;

	if (bHavePlayer)
		flBestPick = U::Math.GetFovBetween(vViewAngles, U::Math.GetAngleToPosition(vEyePos, vAimPoint));

	if (bHaveCommon)
	{
		const float flC = U::Math.GetFovBetween(vViewAngles, U::Math.GetAngleToPosition(vEyePos, vCommonPoint));

		if (flC < flBestPick)
		{
			flBestPick = flC;
			vAimPoint = vCommonPoint;
			pTarget = nullptr;
			bHavePlayer = false;
		}
		else
			bHaveCommon = false;
	}

	if (bHaveSpecial)
	{
		const float flS = U::Math.GetFovBetween(vViewAngles, U::Math.GetAngleToPosition(vEyePos, vSpecialPoint));

		if (flS < flBestPick)
		{
			vAimPoint = vSpecialPoint;
			pTarget = nullptr;
			bHavePlayer = false;
			bHaveCommon = false;
			ZTRACE_FIRST("Aimbot:special");
		}
		else
			bHaveSpecial = false;
	}

	if (!bHavePlayer && !bHaveCommon && !bHaveSpecial)
		return;

	if (bHaveCommon)
		ZTRACE_FIRST("Aimbot:common");

	//Guard against degenerate direction (NaN protection for GetAngleToPosition).
	if ((vAimPoint - vEyePos).LenghtSqr() < 1.0f)
		return;

	if (bHavePlayer && Vars::Aimbot::bVisibleOnly && !IsPointVisible(pLocal, vEyePos, vAimPoint, pTarget))
		return;

	Vector vAngleTo = U::Math.GetAngleToPosition(vEyePos, vAimPoint);

	//Compute smoothing always (for silent too — the server receives the smoothed angle),
	//only show the angles on screen without silent (below).
	if (s_wpn.flSmooth > 0.0f)
	{
		const float flSmooth = U::Math.Clamp(s_wpn.flSmooth, 1.0f, 64.0f);
		vAngleTo -= vViewAngles;
		U::Math.ClampAngles(vAngleTo);
		vAngleTo /= flSmooth;
		vAngleTo += vViewAngles;
	}

	U::Math.ClampAngles(vAngleTo);
	G::Util.FixMovement(vAngleTo, cmd);
	cmd->viewangles = vAngleTo;

	if (!Vars::Aimbot::bSilent && I::EngineClient)
		I::EngineClient->SetViewAngles(cmd->viewangles);

	if (Vars::Aimbot::bAutoShoot && pWeapon && pWeapon->CanPrimaryAttack())
		cmd->buttons |= IN_ATTACK;
}

C_TerrorPlayer* CFeatures_Aimbot::FindTarget(C_TerrorPlayer* pLocal, const Vector& vEyePos, const Vector& vViewAngles)
{
	C_TerrorPlayer* pBest = nullptr;
	float flBestWeight = FLT_MAX;

	for (int n = 1; n <= I::ClientEntityList->GetMaxEntities(); n++)
	{
		IClientEntity* pEntity = I::ClientEntityList->GetClientEntity(n);

		if (!pEntity)
			continue;

		C_TerrorPlayer* pPlayer = pEntity->As<C_TerrorPlayer*>();

		//Shared filter: null/dormant/ClassID/team/lifeState/ghost/health.
		if (!G::Util.IsValidTarget(pLocal, pPlayer, false))
			continue;

		if (Vars::Aimbot::bIgnoreIncapped && pPlayer->m_isIncapacitated())
			continue;

		Vector vAimPoint;
		if (!GetAimPoint(pPlayer, vAimPoint))
			continue;

		if ((vAimPoint - vEyePos).LenghtSqr() < 1.0f)
			continue;

		//Visibility to the aim point, not eye-to-eye: a head behind a roof with the
		//chest visible (and vice versa) used to give a wrong decision.
		if (Vars::Aimbot::bVisibleOnly && !IsPointVisible(pLocal, vEyePos, vAimPoint, pPlayer))
			continue;

		const Vector vAngleTo = U::Math.GetAngleToPosition(vEyePos, vAimPoint);
		const float flFov = U::Math.GetFovBetween(vViewAngles, vAngleTo);

		if (!isfinite(flFov) || flFov > s_wpn.flFOV)
			continue;

		const float flWeight = GetWeight(pPlayer, vViewAngles, vEyePos, vAngleTo);

		if (flWeight < flBestWeight)
		{
			flBestWeight = flWeight;
			pBest = pPlayer;
		}
	}

	return pBest;
}

bool CFeatures_Aimbot::GetAimPoint(C_TerrorPlayer* pTarget, Vector& vOut)
{
	if (!pTarget)
		return false;

	switch (s_wpn.nHitbox)
	{
		case 0: //Head
		{
			vOut = G::Util.GetEyePosition(pTarget);
			break;
		}
		case 1: //Center
		{
			vOut = pTarget->m_vecOrigin() + (pTarget->m_vecMins() + pTarget->m_vecMaxs()) * 0.5f;
			break;
		}
		default:
			return false;
	}

	return !vOut.IsZero();
}

float CFeatures_Aimbot::GetWeight(C_TerrorPlayer* pTarget, const Vector& vFrom, const Vector& vEyePos, const Vector& vAngleTo) const
{
	switch (s_wpn.nPrio)
	{
		case 1: //Distance
		{
			return vEyePos.DistToSqr(pTarget->m_vecOrigin());
		}
		default: //FOV (closest to crosshair)
		{
			break;
		}
	}

	return U::Math.GetFovBetween(vFrom, vAngleTo);
}

bool CFeatures_Aimbot::ShouldRun(C_TerrorPlayer* pLocal, C_TerrorWeapon* pWeapon, CUserCmd* cmd) const
{
	if (!Vars::Aimbot::bEnabled || !cmd || !cmd->command_number)
		return false;

	if (!pLocal || !pWeapon || pLocal->deadflag() || pLocal->m_lifeState() != 0)
		return false;

	if (!(Vars::Aimbot::nKey ? GetAsyncKeyState(Vars::Aimbot::nKey) & 0x8000 : true))
		return false;

	if (pLocal->m_isGhost() || pLocal->m_isIncapacitated() || pLocal->m_isHangingFromLedge() || pLocal->m_isHangingFromTongue())
		return false;

	if (!G::Util.IsValidTeam(pLocal->GetTeamNumber()))
		return false;

	//Primary only: autoshoot presses IN_ATTACK, and snapping angles on the tick of
	//a ready shove (secondary) would not fire anyway.
	return pWeapon->CanPrimaryAttack();
}
