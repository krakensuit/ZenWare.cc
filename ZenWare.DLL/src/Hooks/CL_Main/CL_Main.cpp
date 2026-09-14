#include "CL_Main.h"
#include "../../Util/Logger/Logger.h"

#include "../../Features/Vars.h"

using namespace Hooks;

void __cdecl CL_Main::CL_Move::Detour(float accumulated_extra_samples, bool bFinalTick)
{
	ZTRACE_FIRST("CL_Main::CL_Move");
	//The original is called exactly once. The old XBUTTON1 loop (5 more calls
	//in a row while the side mouse button = bhop key is held) ran the movement
	//simulation and prediction 6 times per frame: ragged packets, nested
	//prediction, unpredictable engine deaths. That is not how doubletap works.
	Func.Original<FN>()(accumulated_extra_samples, bFinalTick);
}

void CL_Main::Init()
{
	//CL_Move
	{
		using namespace CL_Move;

		const FN pfCLMove = reinterpret_cast<FN>(U::Offsets.m_dwCLMove);
		XASSERT(pfCLMove == nullptr);

		if (pfCLMove)
			XASSERT(Func.Init(pfCLMove, &Detour) == false);
	}
}