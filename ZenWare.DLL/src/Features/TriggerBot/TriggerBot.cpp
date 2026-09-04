#include "TriggerBot.h"

#include "../Vars.h"

namespace
{
	//Ray straight down the crosshair; returns the entity it ends on.
	C_BaseEntity* TraceCrosshair(C_TerrorPlayer* pLocal)
	{
		const Vector vEye = G::Util.GetEyePosition(pLocal);
		Vector vForward = { };
		Vector vView = { };
		I::EngineClient->GetViewAngles(vView);
		U::Math.AngleVectors(vView, &vForward);

		CTraceFilterHitAll filter(reinterpret_cast<IHandleEntity*>(pLocal));

		trace_t tr;
		G::Util.Trace(vEye, vEye + vForward * 8192.0f, MASK_SHOT, &filter, &tr);

		return tr.m_pEnt;
	}
}

void CFeatures_TriggerBot::Run(C_TerrorPlayer* pLocal, C_TerrorWeapon* pWeapon, CUserCmd* cmd)
{
	if (!Vars::TriggerBot::bEnabled || !pLocal || !pWeapon || !cmd || !cmd->command_number)
		return;

	if (Vars::TriggerBot::nKey && !(GetAsyncKeyState(Vars::TriggerBot::nKey) & 0x8000))
		return;

	if (!pWeapon->CanPrimaryAttack())
		return;

	C_BaseEntity* pHit = TraceCrosshair(pLocal);

	if (!pHit)
		return;

	C_TerrorPlayer* pTarget = pHit->As<C_TerrorPlayer*>();

	if (!G::Util.IsValidTarget(pLocal, pTarget, Vars::TriggerBot::bVisibleOnly))
		return;

	cmd->buttons |= IN_ATTACK;
}
