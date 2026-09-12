#include "VisualRecoil.h"

#include "../Vars.h"

void CFeatures_VisualRecoil::FrameStageNotify(ClientFrameStage_t curStage)
{
	if (!Vars::VisualRecoil::bEnabled || curStage != FRAME_RENDER_START)
		return;

	if (!I::EngineClient || !I::EngineClient->IsInGame())
		return;

	const int nLocalIndex = I::EngineClient->GetLocalPlayer();

	C_TerrorPlayer* pLocal = nullptr;
	if (nLocalIndex >= 0 && I::ClientEntityList)
	{
		IClientEntity* pEnt = I::ClientEntityList->GetClientEntity(nLocalIndex);
		if (pEnt)
		{
			// Пишем 12 байт по оффсету: убеждаемся что это правда игрок,
			// иначе молча выходим (NetVarManager мог вернуть 0).
			ClientClass* pCC = pEnt->GetClientClass();
			if (!pCC || (pCC->m_ClassID != CTerrorPlayer && pCC->m_ClassID != SurvivorBot))
				return;
			pLocal = pEnt->As<C_TerrorPlayer*>();
		}
	}

	if (!pLocal)
		return;

	if (pLocal->deadflag() || pLocal->m_lifeState() != 0)
		return;

	//Only affects the visual punch, the server-side punch is untouched (NoSpread compensates for it in angles).
	pLocal->m_vecPunchAngle() = Vector(0.0f, 0.0f, 0.0f);
	pLocal->m_vecPunchAngleVel() = Vector(0.0f, 0.0f, 0.0f);
}
