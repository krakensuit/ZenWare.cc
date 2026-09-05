#include "ClientMode.h"

#include "../../Util/Logger/Logger.h"
#include "../../Entry/Entry.h"
#include "../../Features/Vars.h"
#include "../../Features/Aimbot/Aimbot.h"
#include "../../Features/AutoPistol/AutoPistol.h"
#include "../../Features/AutoStrafe/AutoStrafe.h"
#include "../../Features/AutoShove/AutoShove.h"
#include "../../Features/BunnyHop/BunnyHop.h"
#include "../../Features/EnginePrediction/EnginePrediction.h"
#include "../../Features/NoSpread/NoSpread.h"
#include "../../Features/TriggerBot/TriggerBot.h"
#include "../../Features/JumpStats/JumpStats.h"

using namespace Hooks;

#define PASSIVE_IF_SHUTDOWN(originalCall) \
	if (G::ModuleEntry.IsShuttingDown()) \
		return originalCall;

bool __fastcall ClientMode::ShouldDrawFog::Detour(void* ecx, void* edx)
{
	PASSIVE_IF_SHUTDOWN(Table.Original<FN>(Index)(ecx, edx));

	if (Vars::Visuals::bNoFog && I::EngineClient->IsInGame())
		return false;

	return Table.Original<FN>(Index)(ecx, edx);
}

bool __fastcall ClientMode::CreateMove::Detour(void* ecx, void* edx, float input_sample_frametime, CUserCmd* cmd)
{
	ZTRACE_FIRST("ClientMode::CreateMove");
	PASSIVE_IF_SHUTDOWN(Table.Original<FN>(Index)(ecx, edx, input_sample_frametime, cmd));

	if (!cmd || !cmd->command_number)
		return Table.Original<FN>(Index)(ecx, edx, input_sample_frametime, cmd);

	if (Table.Original<FN>(Index)(ecx, edx, input_sample_frametime, cmd))
		I::Prediction->SetLocalViewAngles(cmd->viewangles);

 C_TerrorPlayer* pLocal = nullptr;
 {
  const int nLocalIdx = I::EngineClient->GetLocalPlayer();
  if (nLocalIdx >= 0)
  {
   IClientEntity* pEnt = I::ClientEntityList->GetClientEntity(nLocalIdx);
   if (pEnt) pLocal = pEnt->As<C_TerrorPlayer*>();
  }
 }

	if (pLocal && !pLocal->deadflag())
	{
		F::EnginePrediction.Start(pLocal, cmd);
		{
			// Movement features work without active weapon (infected claws etc.)
			F::BunnyHop.Run(pLocal, cmd);
			F::AutoStrafe.Run(pLocal, cmd);
			F::JumpStats.OnTick(pLocal, cmd);
			F::AutoShove.Run(pLocal, cmd);

			C_BaseCombatWeapon* pBaseWeapon = pLocal->GetActiveWeapon();
			C_TerrorWeapon* pWeapon = pBaseWeapon ? pBaseWeapon->As<C_TerrorWeapon*>() : nullptr;

			if (pWeapon)
			{
				F::Aimbot.Run(pLocal, pWeapon, cmd);
				F::TriggerBot.Run(pLocal, pWeapon, cmd);
				F::AutoPistol.Run(pWeapon, cmd);
				F::NoSpread.Run(pLocal, pWeapon, cmd);
			}
		}
		F::EnginePrediction.Finish(pLocal, cmd);
	}

	return false;
}

void __fastcall ClientMode::DoPostScreenSpaceEffects::Detour(void* ecx, void* edx, const void* pSetup)
{
	PASSIVE_IF_SHUTDOWN(Table.Original<FN>(Index)(ecx, edx, pSetup));
}

float __fastcall ClientMode::GetViewModelFOV::Detour(void* ecx, void* edx)
{
	const float flBase = Table.Original<FN>(Index)(ecx, edx);

	if (G::ModuleEntry.IsShuttingDown() || !I::EngineClient->IsInGame())
		return flBase;

	return flBase * U::Math.Clamp(Vars::Visuals::flViewFOV, 0.5f, 3.0f);
}

void ClientMode::Init()
{
	XASSERT(Table.Init(I::ClientMode) == false);
	XASSERT(Table.Hook(&ShouldDrawFog::Detour, ShouldDrawFog::Index) == false);
	XASSERT(Table.Hook(&CreateMove::Detour, CreateMove::Index) == false);
	XASSERT(Table.Hook(&DoPostScreenSpaceEffects::Detour, DoPostScreenSpaceEffects::Index) == false);
	XASSERT(Table.Hook(&GetViewModelFOV::Detour, GetViewModelFOV::Index) == false);
}
