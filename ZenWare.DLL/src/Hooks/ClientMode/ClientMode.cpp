#include "ClientMode.h"

#include "../../Util/Logger/Logger.h"
#include "../../SDK/GameUtil/GameUtil.h"
#include "../../Entry/Entry.h"
#include "../../Features/Vars.h"
#include "../../Features/Aimbot/Aimbot.h"
#include "../../Features/AutoPistol/AutoPistol.h"
#include "../../Features/AutoStrafe/AutoStrafe.h"
#include "../../Features/AutoShove/AutoShove.h"
#include "../../Features/BunnyHop/BunnyHop.h"
#include "../../Features/NoSpread/NoSpread.h"

#include "../../Features/TriggerBot/TriggerBot.h"
#include "../../Features/Hitmarker/Hitmarker.h"
#include "../../Features/JumpStats/JumpStats.h"

using namespace Hooks;

#define PASSIVE_IF_SHUTDOWN(originalCall) \
	if (G::ModuleEntry.IsShuttingDown()) \
		return originalCall;

bool __fastcall ClientMode::ShouldDrawFog::Detour(void* ecx, void* edx)
{
	PASSIVE_IF_SHUTDOWN(Table.Original<FN>(Index)(ecx, edx));

	if (Vars::Visuals::bNoFog && I::EngineClient && I::EngineClient->IsInGame())
		return false;

	return Table.Original<FN>(Index)(ecx, edx);
}

bool __fastcall ClientMode::CreateMove::Detour(void* ecx, void* edx, float input_sample_frametime, CUserCmd* cmd)
{
	ZTRACE_FIRST("ClientMode::CreateMove");
	U::Log.Crumb("CreateMove");
	PASSIVE_IF_SHUTDOWN(Table.Original<FN>(Index)(ecx, edx, input_sample_frametime, cmd));

	//The original is called exactly once: a double run per tick duplicated
	//engine movement/weapon switching and broke prediction accounting.
	//cmd is checked BEFORE the original: with null the original may already have crashed inside.
	if (!cmd || !cmd->command_number)
		return Table.Original<FN>(Index)(ecx, edx, input_sample_frametime, cmd);

	const bool bEngineHandled = Table.Original<FN>(Index)(ecx, edx, input_sample_frametime, cmd);

	if (bEngineHandled && I::Prediction)
		I::Prediction->SetLocalViewAngles(cmd->viewangles);

  C_TerrorPlayer* pLocal = nullptr;
  if (I::EngineClient && I::ClientEntityList)
  {
   const int nLocalIdx = I::EngineClient->GetLocalPlayer();
   if (nLocalIdx > 0)
   {
    IClientEntity* pEnt = I::ClientEntityList->GetClientEntity(nLocalIdx);
    if (G::Util.IsPlayerEntity(pEnt)) pLocal = pEnt->As<C_TerrorPlayer*>();
   }
  }

	if (pLocal && !pLocal->deadflag())
	{
		// Movement and combat run on the engine's own prediction, as in the
		// references (CSGOSimple, l4d2-internal-base): an entity in CreateMove =
		// the tail of cmd N-1, the freshest prediction. Manual SetupMove/
		// ProcessMovement/FinishMove inside CreateMove simulated cmd twice
		// (our run + engine prediction) and left half the state advanced
		// (m_hGroundEntity, m_flFallVelocity, ducktime were not restored) —
		// the ground-entity desync with flags broke CheckJumpButton, bhop
		// jumped crookedly or not at all.
		const int nRawMouseX = cmd->mousedx;
		F::BunnyHop.Run(pLocal, cmd);
		F::AutoStrafe.Run(pLocal, cmd);
		//Sync must measure the strafe actually applied this tick (AutoStrafe
		//writes cmd->sidemove above); the old pre-feature snapshot was always 0
		//with autostrafe on, so every landing read sync 0% and 0 strafes.
		const float flAppliedSide = cmd->sidemove;
		F::JumpStats.OnTick(pLocal, cmd, flAppliedSide, nRawMouseX);

		//Guns only: melee/chainsaw/grenade launcher are siblings of
		//C_BaseCombatWeapon; a cast to C_TerrorWeapon would call vfuncs on a foreign vtable slot.
		C_BaseCombatWeapon* pBaseWeapon = pLocal->GetActiveWeapon();
		C_TerrorWeapon* pWeapon = (pBaseWeapon && G::Util.IsGunEntity(pBaseWeapon)) ? pBaseWeapon->As<C_TerrorWeapon*>() : nullptr;

		if (pWeapon)
		{
			//Order is load-bearing: NoSpread sees IN_ATTACK forced by
			//aimbot/trigger in the same tick only with this ordering.
			F::Aimbot.Run(pLocal, pWeapon, cmd);
			F::TriggerBot.Run(pLocal, pWeapon, cmd);
			F::AutoPistol.Run(pWeapon, cmd);
			F::NoSpread.Run(pLocal, pWeapon, cmd);
		}
		//Shove last: it sets its own angles on the attacker; Aimbot above
		//would overwrite them with its snap — the shove would miss.
		F::AutoShove.Run(pLocal, cmd);

		//IN_ATTACK front edge for session accuracy: sample AFTER features,
		//otherwise aimbot/trigger shots (forced buttons) were not counted by the hitmarker.
		static bool s_bPrevAtk = false;
		const bool bAtk = (cmd->buttons & IN_ATTACK) != 0;
		if (bAtk && !s_bPrevAtk)
			F::Hitmarker.OnShot();
		s_bPrevAtk = bAtk;
	}

	return false;
}

void __fastcall ClientMode::DoPostScreenSpaceEffects::Detour(void* ecx, void* edx, const void* pSetup)
{
	PASSIVE_IF_SHUTDOWN(Table.Original<FN>(Index)(ecx, edx, pSetup));

	//Clean screen: skipping the original removes vomit/blur/stun entirely.
	//Default true = previous behavior (the detour never called the original anyway).
	if (Vars::Visuals::bNoScreenFx || !I::EngineClient || !I::EngineClient->IsInGame())
		return;

	Table.Original<FN>(Index)(ecx, edx, pSetup);
}

float __fastcall ClientMode::GetViewModelFOV::Detour(void* ecx, void* edx)
{
	const float flBase = Table.Original<FN>(Index)(ecx, edx);

	if (G::ModuleEntry.IsShuttingDown() || !I::EngineClient || !I::EngineClient->IsInGame())
		return flBase;

	return flBase * U::Math.Clamp(Vars::Visuals::flVmFOV, 0.5f, 3.0f);
}

void ClientMode::Init()
{
	XASSERT(Table.Init(I::ClientMode) == false);
	XASSERT(Table.Hook(&ShouldDrawFog::Detour, ShouldDrawFog::Index) == false);
	XASSERT(Table.Hook(&CreateMove::Detour, CreateMove::Index) == false);
	XASSERT(Table.Hook(&DoPostScreenSpaceEffects::Detour, DoPostScreenSpaceEffects::Index) == false);
	XASSERT(Table.Hook(&GetViewModelFOV::Detour, GetViewModelFOV::Index) == false);
}
