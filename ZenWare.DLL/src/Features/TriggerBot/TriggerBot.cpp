#include "TriggerBot.h"

#include "../Vars.h"
#include "../../Util/Logger/Logger.h"

namespace
{
	//Ray straight down the crosshair; returns the entity it ends on.
	//Триггер по углам cmd после аима, а не по прошлому кадру из движка:
	//иначе при включённом аиме выстрел отстаёт на тик.
	C_BaseEntity* TraceCrosshair(C_TerrorPlayer* pLocal, const Vector& vView)
	{
		const Vector vEye = G::Util.GetEyePosition(pLocal);
		Vector vForward = { };
		U::Math.AngleVectors(vView, &vForward);

		CTraceFilterHitAll filter(static_cast<IHandleEntity*>(pLocal));

		trace_t tr{};
		G::Util.Trace(vEye, vEye + vForward * 8192.0f, MASK_SHOT, &filter, &tr);

		return tr.m_pEnt;
	}
}

void CFeatures_TriggerBot::Run(C_TerrorPlayer* pLocal, C_TerrorWeapon* pWeapon, CUserCmd* cmd)
{
	U::Log.Crumb("TriggerBot::Run");
	if (!Vars::TriggerBot::bEnabled || !pLocal || !pWeapon || !cmd || !cmd->command_number)
		return;

	if (Vars::TriggerBot::nKey && !(GetAsyncKeyState(Vars::TriggerBot::nKey) & 0x8000))
		return;

	//Как в Aimbot::ShouldRun: не файрим в инкапе/висе/госте/мёртвым.
	if (pLocal->deadflag() || pLocal->m_lifeState() != 0 || pLocal->m_isGhost()
		|| pLocal->m_isIncapacitated() || !G::Util.IsValidTeam(pLocal->GetTeamNumber()))
		return;

	if (!pWeapon->CanPrimaryAttack())
		return;

	C_BaseEntity* pHit = TraceCrosshair(pLocal, cmd->viewangles);

	if (!pHit)
		return;

	//tr.m_pEnt — любой объект (проп/оружие/стена): As<> до гейта = чужая таблица.
	C_TerrorPlayer* pTarget = G::Util.IsPlayerEntity(pHit) ? pHit->As<C_TerrorPlayer*>() : nullptr;

	if (G::Util.IsValidTarget(pLocal, pTarget, Vars::TriggerBot::bVisibleOnly))
	{
		cmd->buttons |= IN_ATTACK;
		return;
	}

	if (Vars::Aimbot::bTargetCommons)
	{
		ClientClass* pCC = pHit->GetClientClass();

		if (pCC && (pCC->m_ClassID == Infected || pCC->m_ClassID == Witch))
		{
			C_Infected* pInf = pHit->As<C_Infected*>();

			if (pInf && G::Util.IsInfectedAlive(pInf->m_usSolidFlags(), pInf->m_nSequence()))
				cmd->buttons |= IN_ATTACK;
		}
	}

	if (Vars::Aimbot::bTargetSpecials)
	{
		ClientClass* pCC = pHit->GetClientClass();

		if (pCC)
		{
			const int nID = pCC->m_ClassID;

			if (nID == Hunter || nID == Smoker || nID == Jockey || nID == Spitter || nID == Charger || nID == Tank)
			{
				C_BasePlayer* pPl = pHit->As<C_BasePlayer*>();

				if (pPl && pPl->m_lifeState() == 0)
				{
					const int nTeam = pHit->m_iTeamNum();

					if ((nTeam == TEAM_SURVIVOR || nTeam == TEAM_INFECTED) && pLocal && nTeam != pLocal->GetTeamNumber())
						cmd->buttons |= IN_ATTACK;
				}
			}
			else if (G::Util.IsSpecialByName(pCC->m_pNetworkName))
			{
				//Бумер и классы со сдвинутыми ID: точного типа не знаем — виртуалок
				//не дёргаем, только нетвар команды C_BaseEntity. Для триггера хватает.
				const int nTeam = pHit->m_iTeamNum();

				if ((nTeam == TEAM_SURVIVOR || nTeam == TEAM_INFECTED) && pLocal && nTeam != pLocal->GetTeamNumber())
					cmd->buttons |= IN_ATTACK;
			}
		}
	}
}
