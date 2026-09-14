#include "TriggerBot.h"

#include "../Vars.h"
#include "../../Util/Logger/Logger.h"

namespace
{
	//Ray straight down the crosshair; returns the entity it ends on.
	//Trace against the cmd viewangles after aimbot, not the engine's previous frame:
	//otherwise with aimbot on the shot lags a tick behind.
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

	//Like Aimbot::ShouldRun: no firing while incapped/hanging/ghost/dead.
	if (pLocal->deadflag() || pLocal->m_lifeState() != 0 || pLocal->m_isGhost()
		|| pLocal->m_isIncapacitated() || !G::Util.IsValidTeam(pLocal->GetTeamNumber()))
		return;

	if (!pWeapon->CanPrimaryAttack())
		return;

	C_BaseEntity* pHit = TraceCrosshair(pLocal, cmd->viewangles);

	if (!pHit || pHit->IsDormant())
		return;

	//tr.m_pEnt can be any object (prop/weapon/wall): As<> before the gate = wrong vtable.
	C_TerrorPlayer* pTarget = G::Util.IsPlayerEntity(pHit) ? pHit->As<C_TerrorPlayer*>() : nullptr;

	//Visibility is already proven by the crosshair trace hit itself: an eye-to-eye
	//check here would choke fire when the head is visible but the chest is covered, so
	//bVisibleOnly defaults to off, and the user opts into the strict eye-to-eye check.
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
				//Boomer and classes with shifted IDs: exact type unknown — no virtual
				//calls, only the C_BaseEntity team netvar. Good enough for the trigger.
				const int nTeam = pHit->m_iTeamNum();

				if ((nTeam == TEAM_SURVIVOR || nTeam == TEAM_INFECTED) && pLocal && nTeam != pLocal->GetTeamNumber())
					cmd->buttons |= IN_ATTACK;
			}
		}
	}
}
