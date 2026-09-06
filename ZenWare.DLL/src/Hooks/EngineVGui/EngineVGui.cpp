#include "EngineVGui.h"

#include "../../Entry/Entry.h"
#include "../../Features/ESP/ESP.h"
#include "../../Features/Killfeed/Killfeed.h"
#include "../../Features/Menu/Menu.h"
#include "../../Features/Radar/Radar.h"
#include "../../Features/Alerts/Alerts.h"
#include "../../Features/Visuals/Visuals.h"
#include "../../Features/Killfeed/Killfeed.h"
#include "../../Features/JumpStats/JumpStats.h"
#include "../../Features/Vars.h"
#include "../../Hooks/WndProc/WndProc.h"
#include "../../Util/Logger/Logger.h"

using namespace Hooks;

unsigned int __fastcall EngineVGui::GetPanel::Detour(void* ecx, void* edx, VGuiPanel_t type)
{
	return Table.Original<FN>(Index)(ecx, edx, type);
}

bool __fastcall EngineVGui::IsGameUIVisible::Detour(void* ecx, void* edx)
{
	return Table.Original<FN>(Index)(ecx, edx);
}

void __fastcall EngineVGui::ActivateGameUI::Detour(void* ecx, void* edx)
{
	Table.Original<FN>(Index)(ecx, edx);
}

void __fastcall EngineVGui::Paint::Detour(void* ecx, void* edx, int mode)
{
	if (G::ModuleEntry.IsShuttingDown())
	{
		Table.Original<FN>(Index)(ecx, edx, mode);
		return;
	}

	Table.Original<FN>(Index)(ecx, edx, mode);

	if (!(mode & PAINT_UIPANELS))
		return;

	ZTRACE_FIRST("EngineVGui::Paint");

	//Panic unload (F11), edge-detected once per frame here.
	static bool s_bPrevF11 = false;

	const bool bF11 = (GetAsyncKeyState(VK_F11) & 0x8000) != 0;

	if (bF11 && !s_bPrevF11)
		G::ModuleEntry.RequestUnload();

	s_bPrevF11 = bF11;

	//Screen size WITHOUT trusting another vtable slot: the game window we
	//already hooked knows its client rectangle.
	RECT rcGame = { };

	if (Hooks::WndProc::hwGame && GetClientRect(Hooks::WndProc::hwGame, &rcGame) && rcGame.right > 0)
	{
		G::Draw.m_nScreenW = rcGame.right;
		G::Draw.m_nScreenH = rcGame.bottom;
	}

	I::MatSystemSurface->StartDrawing();
	{
		F::Visuals.UpdateThirdPerson();
		F::Killfeed.OnTick();
		F::ESP.Render();
		F::Radar.Render();
		F::Alerts.Render();
		F::Menu.Render();
		F::Visuals.DrawCrosshair();
		F::Visuals.DrawOverlay();
		if (Vars::Killfeed::bEnabled)
			F::Killfeed.Draw();

		F::JumpStats.Draw();
	}
	I::MatSystemSurface->FinishDrawing();
}

void EngineVGui::Init()
{
	XASSERT(Table.Init(I::EngineVGui) == false);
	XASSERT(Table.Hook(&GetPanel::Detour, GetPanel::Index) == false);
	XASSERT(Table.Hook(&IsGameUIVisible::Detour, IsGameUIVisible::Index) == false);
	XASSERT(Table.Hook(&ActivateGameUI::Detour, ActivateGameUI::Index) == false);
	XASSERT(Table.Hook(&Paint::Detour, Paint::Index) == false);
}
