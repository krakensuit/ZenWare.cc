#include "NoSpread.h"

#include "../Vars.h"

void CFeatures_NoSpread::Run(C_TerrorPlayer* pLocal, C_TerrorWeapon* pWeapon, CUserCmd* cmd)
{
	static const auto pfSharedRandomFloat = reinterpret_cast<float(*)(const char*, float, float, int)>(U::Offsets.m_dwSharedRandomFloat);

	if (!ShouldRun(pLocal, pWeapon, cmd))
		return;

	if (!pfSharedRandomFloat)
		return;

	Vector vAngle = cmd->viewangles;

	//Remove spread from current viewangles
	{
		const float flOldSpread = pWeapon->GetCurrentSpread();
		pWeapon->UpdateSpread();
		const float flSpread = pWeapon->GetCurrentSpread();

		//Garbage out of UpdateSpread after a game update: SharedRandomFloat(min>max)
		//with NaN/negative spread — skip the tick instead of corrupting the angles.
		if (!isfinite(flSpread) || flSpread < 0.0f || flSpread > 0.6f)
		{
			pWeapon->GetCurrentSpread() = flOldSpread;
			return;
		}

		vAngle.x -= pfSharedRandomFloat("CTerrorGun::FireBullet HorizSpread", -flSpread, flSpread, 0);
		vAngle.y -= pfSharedRandomFloat("CTerrorGun::FireBullet VertSpread", -flSpread, flSpread, 0);

		pWeapon->GetCurrentSpread() = flOldSpread;
	}

	//Remove punch from current viewangles
	{
		vAngle -= pLocal->GetPunchAngle();
	}

	U::Math.ClampAngles(vAngle);
	G::Util.FixMovement(vAngle, cmd);

	cmd->viewangles = vAngle;
}

bool CFeatures_NoSpread::ShouldRun(C_TerrorPlayer* pLocal, C_TerrorWeapon* pWeapon, CUserCmd* cmd)
{
	if (!Vars::NoSpread::bEnabled || !pLocal || !pWeapon || !cmd || !cmd->command_number)
		return false;

	//Compensate only a real shot: on cooldown/reload the aim used to drift.
	if (!pWeapon->CanPrimaryAttack())
		return false;

	// Patterns may fail to resolve (game update): calling virtuals on null = crash.
	if (!U::Offsets.m_dwSharedRandomFloat || !U::Offsets.m_dwUpdateSpread)
		return false;

	if (!(cmd->buttons & IN_ATTACK) || (cmd->buttons & IN_USE))
		return false;

	if (pLocal->m_isHangingFromLedge() || pLocal->m_isHangingFromTongue() || !pLocal->CanAttackFull())
		return false;

	//You could also check if the current spread is -1.0f and not run nospread I guess.
	//But since I wanted to filter out shotungs and just be sure that it isnt ran for other stuff I check the weaponid.

	switch (pWeapon->GetWeaponID())
	{
		case WEAPON_AK47:
		case WEAPON_AWP:
		case WEAPON_DEAGLE:
		case WEAPON_HUNTING_RIFLE:
		case WEAPON_M16A1:
		case WEAPON_M60:
		case WEAPON_MAC10:
		case WEAPON_MILITARY_SNIPER:
		case WEAPON_MP5:
		case WEAPON_PISTOL:
		case WEAPON_SCAR:
		case WEAPON_SCOUT:
		case WEAPON_SSG552:
		case WEAPON_UZI:
			return true;
		default:
			break;
	}

	return false;
}