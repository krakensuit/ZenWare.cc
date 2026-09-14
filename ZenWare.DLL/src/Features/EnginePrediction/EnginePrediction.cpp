#include "EnginePrediction.h"

#include "../../SDK/GameUtil/GameUtil.h"

//Very minimal prediction, misses a lot of stuff and the rest of the stuff like button offsets are hardcoded.

void CFeatures_EnginePrediction::Start(C_BasePlayer* pLocal, CUserCmd* cmd)
{
	//Fail-closed: a dead pattern/interface = skip prediction, not a call through null.
	if (!pLocal || !cmd)
		return;
	if (!I::GlobalVars || !I::MoveHelper || !I::GameMovement || !I::Prediction || !I::ClientEntityList)
		return;
	if (!U::Offsets.m_dwSetPredictionRandomSeed)
		return;

	memset(&m_MoveData, 0, sizeof(CMoveData));

	//Snapshot of the state BEFORE prediction. Bug history: flags/tickbase used
	//to be rolled back right in Start, and origin/velocity were not saved at all —
	//features read stale ground flags (bhop missed landing ticks, "does not jump
	//at all"), while origin/velocity advanced by an extra ProcessMovement run
	//broke movement timing (jerky jumps, garbage distance in JumpStats,
	//a dead JumpBug window).
	m_flOldCurTime = I::GlobalVars->curtime;
	m_flOldFrameTime = I::GlobalVars->frametime;
	m_nOldTickCount = I::GlobalVars->tickcount;
	m_vOldOrigin = pLocal->m_vecOrigin();
	m_vOldVelocity = pLocal->m_vecVelocity();
	m_nOldTickBase = pLocal->m_nTickBase();
	m_nOldFlags = pLocal->m_fFlags();

	const int nTickBase = GetTickBase(m_nOldTickBase, cmd);

	//StartCommand
	{
		I::MoveHelper->SetHost(pLocal);

		cmd->random_seed = (MD5_PseudoRandom(cmd->command_number) & INT_MAX);
		reinterpret_cast<void(*)(CUserCmd*)>(U::Offsets.m_dwSetPredictionRandomSeed)(cmd);
	}

	I::GlobalVars->curtime = TICKS_TO_TIME(nTickBase);
	I::GlobalVars->frametime = TICK_INTERVAL;
	I::GlobalVars->tickcount = nTickBase;

	cmd->buttons |= pLocal->m_afButtonForced();
	cmd->buttons &= ~pLocal->m_afButtonDisabled();

	I::GameMovement->StartTrackPredictionErrors(pLocal);

	if (cmd->weaponselect != 0)
	{
		//A stale index could point to a player/prop: a virtual call through a foreign vtable.
		C_BaseCombatWeapon* pWeapon = nullptr;
		if (IClientEntity* pWepEnt = I::ClientEntityList->GetClientEntity(cmd->weaponselect))
		{
			if (G::Util.IsWeaponEntity(pWepEnt))
				pWeapon = pWepEnt->As<C_BaseCombatWeapon*>();
		}

		if (pWeapon)
			pLocal->SelectItem(pWeapon->GetName(), cmd->weaponsubtype);
	}

	if (cmd->impulse)
		pLocal->m_nImpulse() = cmd->impulse;

	pLocal->UpdateButtonState(cmd->buttons);
	I::Prediction->SetLocalViewAngles(cmd->viewangles);

	I::Prediction->SetupMove(pLocal, cmd, I::MoveHelper, &m_MoveData);
	I::GameMovement->ProcessMovement(pLocal, &m_MoveData);
	I::Prediction->FinishMove(pLocal, cmd, &m_MoveData);

	m_nPredictedFlags = pLocal->m_fFlags();

	//The predicted state (flags/origin/velocity) stays alive until Finish():
	//BunnyHop/AutoStrafe/JumpStats read fresh ground and velocity from here.
	m_bInPrediction = true;
}

void CFeatures_EnginePrediction::Finish(C_BasePlayer* pLocal, CUserCmd* cmd)
{
	if (!pLocal || !m_bInPrediction)
		return;
	m_bInPrediction = false;

	if (I::GameMovement)
		I::GameMovement->FinishTrackPredictionErrors(pLocal);

	//FinishCommand
	{
		if (I::MoveHelper)
			I::MoveHelper->SetHost(nullptr);
		if (U::Offsets.m_dwSetPredictionRandomSeed)
			reinterpret_cast<void(*)(CUserCmd*)>(U::Offsets.m_dwSetPredictionRandomSeed)(nullptr);
	}

	//Roll back to the pre-tick state AFTER the features: the engine's own
	//prediction replays this same cmd from a clean state. Without rolling back
	//origin/velocity, movement would be simulated twice per tick (on frames
	//without a snapshot).
	pLocal->m_vecOrigin() = m_vOldOrigin;
	pLocal->m_vecVelocity() = m_vOldVelocity;
	pLocal->m_fFlags() = m_nOldFlags;
	pLocal->m_nTickBase() = m_nOldTickBase;

	if (I::GlobalVars)
	{
		I::GlobalVars->curtime = m_flOldCurTime;
		I::GlobalVars->frametime = m_flOldFrameTime;
		I::GlobalVars->tickcount = m_nOldTickCount;
	}
}

int CFeatures_EnginePrediction::GetPredictedFlags() const
{
	return m_nPredictedFlags;
}

//CasualHacker I believe posted this.
//Keyed by command_number, not by a pointer into the reused CInput buffer:
//after a restart/map change the counter resyncs instead of flying into the far future.
int CFeatures_EnginePrediction::GetTickBase(const int nCurrent, CUserCmd* cmd)
{
	static int s_nTick = 0;
	static int s_nLastCmd = 0;

	if (cmd)
	{
		if (s_nLastCmd == 0 || nCurrent < s_nTick || cmd->command_number != s_nLastCmd + 1)
			s_nTick = nCurrent;
		else
			s_nTick++;

		s_nLastCmd = cmd->command_number;
	}

	return s_nTick;
}