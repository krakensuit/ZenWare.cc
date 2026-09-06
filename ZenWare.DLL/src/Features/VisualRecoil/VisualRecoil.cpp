#include "VisualRecoil.h"

#include "../Vars.h"

void CFeatures_VisualRecoil::FrameStageNotify(ClientFrameStage_t curStage)
{
	if (!Vars::VisualRecoil::bEnabled || curStage != FRAME_RENDER_START)
		return;

	if (!I::EngineClient->IsInGame())
		return;

	const int nLocalIndex = I::EngineClient->GetLocalPlayer();

	C_TerrorPlayer* pLocal = nullptr;
	if (nLocalIndex >= 0)
	{
		IClientEntity* pEnt = I::ClientEntityList->GetClientEntity(nLocalIndex);
		if (pEnt) pLocal = pEnt->As<C_TerrorPlayer*>();
	}

	if (!pLocal)
		return;

	if (pLocal->deadflag() || pLocal->m_lifeState() != 0)
		return;

	//Only affects the visual punch, the server-side punch is untouched (NoSpread compensates for it in angles).
	pLocal->m_vecPunchAngle() = Vector(0.0f, 0.0f, 0.0f);
	pLocal->m_vecPunchAngleVel() = Vector(0.0f, 0.0f, 0.0f);
}
