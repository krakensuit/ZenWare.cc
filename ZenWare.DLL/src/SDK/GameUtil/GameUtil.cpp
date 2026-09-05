#include "GameUtil.h"

void CGlobal_GameUtil::FixMovement(const Vector vAngle, CUserCmd* cmd)
{
	Vector vMove = { cmd->forwardmove, cmd->sidemove, cmd->upmove }, vMoveAng;
	U::Math.VectorAngles(vMove, vMoveAng);

	const float flSpeed = ::sqrtf(vMove.x * vMove.x + vMove.y * vMove.y);
	const float flYaw = DEG2RAD(vAngle.y - cmd->viewangles.y + vMoveAng.y);

	cmd->forwardmove = (::cosf(flYaw) * flSpeed);
	cmd->sidemove = (::sinf(flYaw) * flSpeed);
}

void CGlobal_GameUtil::Trace(const Vector& start, const Vector& end, unsigned int mask, ITraceFilter* filter, trace_t* trace)
{
	Ray_t ray = { start, end };
	I::EngineTrace->TraceRay(ray, mask, filter, trace);
}

bool CGlobal_GameUtil::W2S(const Vector vWorld, Vector& vScreen)
{
	return !(I::DebugOverlay->ScreenPosition(vWorld, vScreen));
}

bool CGlobal_GameUtil::IsOnScreen(const Vector vWorld)
{
	Vector vScreen;
	return W2S(vWorld, vScreen);
}

bool CGlobal_GameUtil::IsValidTeam(const int nTeam)
{
	return ((nTeam == TEAM_SURVIVOR) || (nTeam == TEAM_INFECTED));
}

bool CGlobal_GameUtil::IsInfectedAlive(const int nSolidFlags, const int nSequence)
{
	if ((nSolidFlags & FSOLID_NOT_SOLID) || (nSequence >= 305))
		return false;

	//These are from l4d1 and do not work with the mudfuckers on l4d2 at least.
	return !(U::Math.CompareGroup(nSequence, 303, 279, 295, 266, 302, 301, 281, 283, 261, 293,
		294, 297, 278, 277, 300, 299, 282, 276, 304, 292, 272, 396, 259, 260,
		271, 257, 280, 275, 285, 267, 258, 268, 273));
}

Color CGlobal_GameUtil::GetHealthColor(const int nHealth, const int nMaxHealth)
{
	if (nHealth > nMaxHealth)
		return { 44u, 130u, 201u, 255u };

	const int nCurHP = U::Math.Max(0, U::Math.Min(nHealth, nMaxHealth));

	return {
		U::Math.Min((510 * (nMaxHealth - nCurHP)) / nMaxHealth, 200),
		U::Math.Min((510 * nCurHP) / nMaxHealth, 200),
		0u,
		255u
	};
}

IMaterial* CGlobal_GameUtil::CreateMaterial(const char* const szVars)
{
	// Robust fallback: avoid KeyValues patterns which break on updates.
	// Return a cached flat material from the engine's own library.
	// Caller (Chams) will set ignorez/color per-draw.
	UNREFERENCED_PARAMETER(szVars);

	static IMaterial* s_pFlat = nullptr;
	if (s_pFlat && !IsErrorMaterial(s_pFlat))
		return s_pFlat;

	s_pFlat = I::MaterialSystem->FindMaterial("debug/debugambientcube", TEXTURE_GROUP_MODEL);
	if (IsErrorMaterial(s_pFlat))
		s_pFlat = I::MaterialSystem->FindMaterial("debug/debugdrawflat", TEXTURE_GROUP_MODEL);

	if (!IsErrorMaterial(s_pFlat) && s_pFlat)
	{
		s_pFlat->AddRef();
		return s_pFlat;
	}

	// Absolute fallback: try old KeyValues path (may show MessageBox if pattern dead)
	static int nCreated = 0;
	char szOut[DT_MAX_STRING_BUFFERSIZE];
	sprintf_s(szOut, sizeof(szOut), _("pol_mat_%i.vmt"), nCreated++);

	char szMat[DT_MAX_STRING_BUFFERSIZE];
	sprintf_s(szMat, sizeof(szMat), szVars);

	KeyValues* pKvals = new KeyValues;
	if (!G::KeyVals.Init(pKvals, (char*)szOut))
		return nullptr;
	if (!G::KeyVals.LoadFromBuffer(pKvals, szOut, szMat))
		return nullptr;

	IMaterial* pMat = I::MaterialSystem->CreateMaterial(szOut, pKvals);
	if (!IsErrorMaterial(pMat) && pMat)
		pMat->AddRef();

	return pMat;
}
// ---------------------------------------------------------------------------
// Shared target filtering: runtime-resolved netvars only. Virtuals like
// EyePosition()/WorldSpaceCenter()/GetAbsOrigin() have unverified vtable
// slots in this SDK dump and must not be trusted.
// ---------------------------------------------------------------------------

Vector CGlobal_GameUtil::GetEyePosition(C_TerrorPlayer* pEntity)
{
	if (!pEntity)
		return Vector(0.0f, 0.0f, 0.0f);

	return pEntity->m_vecOrigin() + pEntity->m_vecViewOffset();
}

bool CGlobal_GameUtil::IsValidTarget(C_TerrorPlayer* pLocal, C_TerrorPlayer* pPlayer, bool bCheckVisible)
{
	if (!pPlayer || !pPlayer->As<C_TerrorPlayer*>())
		return false;

	if (pPlayer == pLocal)
		return false;

	if (pPlayer->IsDormant())
		return false;

	ClientClass* pCC = pPlayer->GetClientClass();

	if (!pCC || !U::Math.CompareGroup(pCC->m_ClassID, CTerrorPlayer, SurvivorBot))
		return false;

	const int nTeam = pPlayer->GetTeamNumber();

	if (!IsValidTeam(nTeam) || (pLocal && nTeam == pLocal->GetTeamNumber()))
		return false;

	//Alive checks: deadflag + m_lifeState + positive health.
	if (pPlayer->deadflag() || pPlayer->m_lifeState() != 0 || pPlayer->GetHealth() <= 0)
		return false;

	//Ghosts (dead infected waiting to spawn) are never valid targets.
	if (pPlayer->m_isGhost())
		return false;

	if (bCheckVisible && !IsTargetVisible(pLocal, pPlayer, GetEyePosition(pLocal)))
		return false;

	return true;
}

bool CGlobal_GameUtil::IsTargetVisible(C_TerrorPlayer* pLocal, C_TerrorPlayer* pTarget, const Vector& vEyePos)
{
	if (!pLocal || !pTarget)
		return false;

	CBaseTrace baseTrace;
	memset(&baseTrace, 0, sizeof(baseTrace));

	trace_t tr;
	CTraceFilterHitAll filter(static_cast<IHandleEntity*>(pLocal));

	Trace(vEyePos, GetEyePosition(pTarget), MASK_SHOT, &filter, &tr);

	//Visible when the trace ended on the target itself or hit nothing solid before it.
	return (tr.m_pEnt == pTarget) || (!tr.DidHit());
}

