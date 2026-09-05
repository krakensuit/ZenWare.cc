#include "Aimbot.h"

#include "../Vars.h"
#include "../../Util/Logger/Logger.h"

namespace
{
	bool IsCommonVisible(C_TerrorPlayer* pLocal, const Vector& vEyePos, const Vector& vAimPoint)
	{
		trace_t tr;
		CTraceFilterHitAll filter(static_cast<IHandleEntity*>(pLocal));
		G::Util.Trace(vEyePos, vAimPoint, MASK_SHOT, &filter, &tr);
		return !tr.DidHit();
	}

	bool FindCommonTarget(C_TerrorPlayer* pLocal, const Vector& vEyePos, const Vector& vViewAngles, Vector& vOut)
	{
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

			C_BaseEntity* pEnt = pEntity->As<C_BaseEntity*>();
			C_Infected* pInf = pEntity->As<C_Infected*>();

			if (!pEnt || !pInf)
				continue;

			if (!G::Util.IsInfectedAlive(pInf->m_usSolidFlags(), pInf->m_nSequence()))
				continue;

			Vector vAim = pEnt->m_vecOrigin() + Vector(0.0f, 0.0f, pEnt->m_vecMaxs().z * 0.85f);

			if ((vAim - vEyePos).LenghtSqr() < 1.0f)
				continue;

			if (Vars::Aimbot::bVisibleOnly && !IsCommonVisible(pLocal, vEyePos, vAim))
				continue;

			const float flFov = U::Math.GetFovBetween(vViewAngles, U::Math.GetAngleToPosition(vEyePos, vAim));

			if (flFov > Vars::Aimbot::flFOV)
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
	if (!ShouldRun(pLocal, pWeapon, cmd))
		return;

	const Vector vEyePos = G::Util.GetEyePosition(pLocal);
	Vector vViewAngles = cmd->viewangles;

	C_TerrorPlayer* pTarget = FindTarget(pLocal, vEyePos, vViewAngles);
	Vector vAimPoint;
	bool bHavePlayer = (pTarget && GetAimPoint(pTarget, vAimPoint));

	Vector vCommonPoint;
	bool bHaveCommon = (Vars::Aimbot::bTargetCommons && FindCommonTarget(pLocal, vEyePos, vViewAngles, vCommonPoint));

	if (bHavePlayer && bHaveCommon)
	{
		const float flP = U::Math.GetFovBetween(vViewAngles, U::Math.GetAngleToPosition(vEyePos, vAimPoint));
		const float flC = U::Math.GetFovBetween(vViewAngles, U::Math.GetAngleToPosition(vEyePos, vCommonPoint));

		if (flC < flP)
		{
			bHavePlayer = false;
			pTarget = nullptr;
		}
		else
			bHaveCommon = false;
	}

	if (!bHavePlayer && !bHaveCommon)
		return;

	if (bHaveCommon)
	{
		vAimPoint = vCommonPoint;
		pTarget = nullptr;
		ZTRACE_FIRST("Aimbot:common");
	}

	//Guard against degenerate direction (NaN protection for GetAngleToPosition).
	if ((vAimPoint - vEyePos).LenghtSqr() < 1.0f)
		return;

	if (bHavePlayer && Vars::Aimbot::bVisibleOnly && !G::Util.IsTargetVisible(pLocal, pTarget, vEyePos))
		return;

	Vector vAngleTo = U::Math.GetAngleToPosition(vEyePos, vAimPoint);

	if (Vars::Aimbot::flSmoothing > 0.0f && !Vars::Aimbot::bSilent)
	{
		const float flSmooth = U::Math.Clamp(Vars::Aimbot::flSmoothing, 1.0f, 64.0f);
		vAngleTo -= vViewAngles;
		U::Math.ClampAngles(vAngleTo);
		vAngleTo /= flSmooth;
		vAngleTo += vViewAngles;
	}

	U::Math.ClampAngles(vAngleTo);
	G::Util.FixMovement(vAngleTo, cmd);
	cmd->viewangles = vAngleTo;

	if (!Vars::Aimbot::bSilent)
		I::EngineClient->SetViewAngles(cmd->viewangles);

	if (Vars::Aimbot::bAutoShoot && pWeapon->CanPrimaryAttack())
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

		if (Vars::Aimbot::bVisibleOnly && !G::Util.IsTargetVisible(pLocal, pPlayer, vEyePos))
			continue;

		const Vector vAngleTo = U::Math.GetAngleToPosition(vEyePos, vAimPoint);
		const float flFov = U::Math.GetFovBetween(vViewAngles, vAngleTo);

		if (flFov > Vars::Aimbot::flFOV)
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

	switch (Vars::Aimbot::nHitbox)
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
	switch (Vars::Aimbot::nTargetPriority)
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

	return pWeapon->CanPrimaryAttack() || pWeapon->CanSecondaryAttack();
}
