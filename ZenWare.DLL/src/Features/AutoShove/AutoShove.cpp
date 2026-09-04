#include "AutoShove.h"

#include "../Vars.h"

void CFeatures_AutoShove::Run(C_TerrorPlayer* pLocal, CUserCmd* cmd)
{
	//Survivor quality-of-life: a teammate grabbed by a Smoker tongue or pinned
	//by a Hunter pounce gets an automatic shove attempt aimed at the attacker.
	if (!Vars::AutoShove::bEnabled || !pLocal || !cmd || !cmd->command_number)
		return;

	if (pLocal->GetTeamNumber() != TEAM_SURVIVOR)
		return;

	for (int n = 1; n <= I::ClientEntityList->GetMaxEntities(); n++)
	{
		IClientEntity* pEntity = I::ClientEntityList->GetClientEntity(n);

		if (!pEntity)
			continue;

		C_TerrorPlayer* pMate = pEntity->As<C_TerrorPlayer*>();

		if (!G::Util.IsValidTarget(pLocal, pMate, false))
			continue;

		if (pMate->GetTeamNumber() != TEAM_SURVIVOR)
			continue;

		const bool bTongued = (pMate->m_tongueOwner().Get() != nullptr);
		const bool bPounced = (pMate->m_pounceAttacker().Get() != nullptr);

		if (!bTongued && !bPounced)
			continue;

		//Aim at the attacker and shove.
		C_BaseEntity* pAttBase = bTongued ? pMate->m_tongueOwner().Get() : pMate->m_pounceAttacker().Get();

		C_TerrorPlayer* pAttacker = pAttBase ? pAttBase->As<C_TerrorPlayer*>() : nullptr;

		if (!pAttacker || pAttacker->deadflag())
			continue;

		const Vector vFrom = G::Util.GetEyePosition(pLocal);
		const Vector vTo = G::Util.GetEyePosition(pAttacker);

		if ((vTo - vFrom).LenghtSqr() > 160.0f * 160.0f) //shove range is short
			continue;

		Vector vAngle = U::Math.GetAngleToPosition(vFrom, vTo);
		U::Math.ClampAngles(vAngle);

		G::Util.FixMovement(vAngle, cmd);
		cmd->viewangles = vAngle;
		cmd->buttons |= IN_ATTACK2;

		return; //one shove per tick is enough
	}
}
