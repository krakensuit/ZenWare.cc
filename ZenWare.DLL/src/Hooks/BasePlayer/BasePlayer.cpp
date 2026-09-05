#include "BasePlayer.h"
#include "../../Util/Logger/Logger.h"

#include "../../Features/Vars.h"

using namespace Hooks;

void __fastcall BasePlayer::CalcPlayerView::Detour(C_BasePlayer* pThis, void* edx, Vector& eyeOrigin, Vector& eyeAngles, float& fov)
{
	ZTRACE_FIRST("BasePlayer::CalcPlayerView");
	if (pThis && !pThis->deadflag()) //Thanks Spook for telling me to do it here.
	{
		const Vector vOldPunch = pThis->GetPunchAngle();

		pThis->m_vecPunchAngle().Init();
		Func.Original<FN>()(pThis, edx, eyeOrigin, eyeAngles, fov);
		pThis->m_vecPunchAngle() = vOldPunch;
	}
	else
	{
		Func.Original<FN>()(pThis, edx, eyeOrigin, eyeAngles, fov);
	}

	//FOV от лица: множитель из меню (90 * 1.0 = дефолт)
	if (Vars::Visuals::flViewFOV > 0.01f && pThis && !pThis->deadflag())
	{
		const int nLocalIdx = I::EngineClient->GetLocalPlayer();
		IClientEntity* pLocalEnt = (nLocalIdx >= 0) ? I::ClientEntityList->GetClientEntity(nLocalIdx) : nullptr;

		if (pLocalEnt && pLocalEnt->As<C_TerrorPlayer*>() == pThis)
			fov = U::Math.Clamp(fov * Vars::Visuals::flViewFOV, 10.0f, 160.0f);
	}
}

void BasePlayer::Init()
{
	//CalcPlayerView
	{
		using namespace CalcPlayerView;

		const FN pfCalcPlayerView = reinterpret_cast<FN>(U::Offsets.m_dwCalcPlayerView);
		XASSERT(pfCalcPlayerView == nullptr);

		if (pfCalcPlayerView)
			XASSERT(Func.Init(pfCalcPlayerView, &Detour) == false);
	}
}