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

	//Оригинал вызывается ровно один раз: двойной прогон за тик дублировал
	//движение/выбор оружия движком и ломал учёт предикта.
	//cmd проверяем ДО оригинала: при null оригинал уже мог упасть внутри.
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
		// Движение и бой — на собственном предикте движка, как в референсах
		// (CSGOSimple, l4d2-internal-base): сущность в CreateMove = конец
		// cmd N-1, самый свежий предикт. Ручной SetupMove/ProcessMovement/
		// FinishMove внутри CreateMove симулировал cmd дважды (наш прогон +
		// предикт движка) и оставлял половину состояния продвинутой
		// (m_hGroundEntity, m_flFallVelocity, ducktime не восстанавливались) —
		// рассинхрон ground-entity с флагами рвал CheckJumpButton, бхоп прыгал
		// криво или не прыгал вовсе.
		const int nRawMouseX = cmd->mousedx;
		F::BunnyHop.Run(pLocal, cmd);
		F::AutoStrafe.Run(pLocal, cmd);
		//Sync must measure the strafe actually applied this tick (AutoStrafe
		//writes cmd->sidemove above); the old pre-feature snapshot was always 0
		//with autostrafe on, so every landing read sync 0% and 0 strafes.
		const float flAppliedSide = cmd->sidemove;
		F::JumpStats.OnTick(pLocal, cmd, flAppliedSide, nRawMouseX);

		//Только стволы: меле/пила/гренник — сиблинги C_BaseCombatWeapon,
		//каст к C_TerrorWeapon дал бы виртуалки по чужому слоту vtable.
		C_BaseCombatWeapon* pBaseWeapon = pLocal->GetActiveWeapon();
		C_TerrorWeapon* pWeapon = (pBaseWeapon && G::Util.IsGunEntity(pBaseWeapon)) ? pBaseWeapon->As<C_TerrorWeapon*>() : nullptr;

		if (pWeapon)
		{
			//Порядок load-bearing: NoSpread видит IN_ATTACK, форсированный
			//аимом/триггером в этом же тике, только при этом порядке.
			F::Aimbot.Run(pLocal, pWeapon, cmd);
			F::TriggerBot.Run(pLocal, pWeapon, cmd);
			F::AutoPistol.Run(pWeapon, cmd);
			F::NoSpread.Run(pLocal, pWeapon, cmd);
		}
		//Шов последним: ставит свои углы на атакующего, Aimbot выше
		//перезаписал бы их своим снапом — шов уходил бы мимо.
		F::AutoShove.Run(pLocal, cmd);

		// Фронт IN_ATTACK для точности сессии: сэмпл ПОСЛЕ фич, иначе
		// выстрелы аима/триггера (форс кнопок) не считались хитмаркером.
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

	//Чистый экран: пропуск оригинала режет рвоту/блюр/стан целиком.
	//Дефолт true = прежнее поведение (детур и так никогда не вызывал оригинал).
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
