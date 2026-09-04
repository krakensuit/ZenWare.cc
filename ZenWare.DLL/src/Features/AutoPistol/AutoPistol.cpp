#include "AutoPistol.h"

#include "../Vars.h"

void CFeatures_AutoPistol::Run(C_TerrorWeapon* pWeapon, CUserCmd* cmd)
{
	//Semi-auto weapons: release IN_ATTACK on the tick right after a shot so
	//the game re-arms the next one as soon as it can - hold-to-fire feel.
	if (!Vars::AutoPistol::bEnabled || !pWeapon || !cmd || !cmd->command_number)
		return;

	if (!(cmd->buttons & IN_ATTACK))
		return;

	switch (pWeapon->GetWeaponID())
	{
		case WEAPON_PISTOL:
		case WEAPON_DEAGLE:
			break;
		default:
			return;
	}

	static int s_nLastShotTick = 0;

	if (pWeapon->CanPrimaryAttack())
	{
		s_nLastShotTick = cmd->tick_count;
		return; //let this shot through
	}

	//Fired very recently: lift the button for exactly this command.
	if (s_nLastShotTick != 0 && (cmd->tick_count - s_nLastShotTick) <= 2)
		cmd->buttons &= ~IN_ATTACK;
}
