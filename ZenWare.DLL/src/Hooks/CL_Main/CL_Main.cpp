#include "CL_Main.h"
#include "../../Util/Logger/Logger.h"

#include "../../Features/Vars.h"

using namespace Hooks;

void __cdecl CL_Main::CL_Move::Detour(float accumulated_extra_samples, bool bFinalTick)
{
	ZTRACE_FIRST("CL_Main::CL_Move");
	//Оригинал вызывается ровно один раз. Бывший XBUTTON1-цикл (ещё 5 вызовов
	//подряд, пока зажата боковая кнопка мыши = клавиша бхопа) крутил симуляцию
	//движения и предикт 6 раз за кадр: рваные пакеты, вложенный предикт,
	//непредсказуемые смерти движка. Даблтеп так не делается.
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