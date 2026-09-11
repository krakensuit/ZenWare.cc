#include "EnginePrediction.h"

#include "../../SDK/GameUtil/GameUtil.h"

//Very minimal prediction, misses a lot of stuff and the rest of the stuff like button offsets are hardcoded.

void CFeatures_EnginePrediction::Start(C_BasePlayer* pLocal, CUserCmd* cmd)
{
	//Fail-closed: дохлый паттерн/интерфейс = пропуск предикта, а не зов по нулю.
	if (!pLocal || !cmd)
		return;
	if (!I::GlobalVars || !I::MoveHelper || !I::GameMovement || !I::Prediction || !I::ClientEntityList)
		return;
	if (!U::Offsets.m_dwSetPredictionRandomSeed)
		return;

	memset(&m_MoveData, 0, sizeof(CMoveData));

	m_flOldCurTime = I::GlobalVars->curtime;
	m_flOldFrameTime = I::GlobalVars->frametime;
	m_nOldTickCount = I::GlobalVars->tickcount;

	const int nOldTickBase = pLocal->m_nTickBase();
	const int nOldFlags = pLocal->m_fFlags();

	const int nTickBase = GetTickBase(nOldTickBase, cmd);

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
		//Stale-индекс мог указывать на игрока/проп: виртуалка по чужой таблице.
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

	pLocal->m_nTickBase() = nOldTickBase;
	pLocal->m_fFlags() = nOldFlags;
}

void CFeatures_EnginePrediction::Finish(C_BasePlayer* pLocal, CUserCmd* cmd)
{
	if (!pLocal)
		return;

	if (I::GameMovement)
		I::GameMovement->FinishTrackPredictionErrors(pLocal);

	//FinishCommand
	{
		if (I::MoveHelper)
			I::MoveHelper->SetHost(nullptr);
		if (U::Offsets.m_dwSetPredictionRandomSeed)
			reinterpret_cast<void(*)(CUserCmd*)>(U::Offsets.m_dwSetPredictionRandomSeed)(nullptr);
	}

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
//Ключ по command_number, а не по указателю на переиспользуемый буфер CInput:
//после рестарта/смены карты счётчик ресинкается, а не улетает в hugely-будущее.
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