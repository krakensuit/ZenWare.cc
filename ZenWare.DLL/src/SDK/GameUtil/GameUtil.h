#pragma once

#include "../KeyValues/KeyValues.h"

class CGlobal_GameUtil
{
public:
	void FixMovement(const Vector vAngle, CUserCmd* cmd);
	void Trace(const Vector& start, const Vector& end, unsigned int mask, ITraceFilter* filter, trace_t* trace);

	bool W2S(const Vector vWorld, Vector& vScreen);
	bool IsOnScreen(const Vector vWorld);
	bool IsValidTeam(const int nTeam);
	bool IsInfectedAlive(const int nSolidFlags, const int nSequence);

	Color GetHealthColor(const int nHealth, const int nMaxHealth);

	IMaterial* CreateMaterial(const char* const szVars);

	//Shared target filtering (Aimbot/ESP/Chams). Netvars only, no unverified virtuals.
	Vector GetEyePosition(C_BaseEntity* pEntity);
	bool IsValidTarget(C_TerrorPlayer* pLocal, C_TerrorPlayer* pPlayer, bool bCheckVisible = false);
	bool IsTargetVisible(C_TerrorPlayer* pLocal, C_TerrorPlayer* pTarget, const Vector& vEyePos);
	//ClassID gates before downcast: As<T> is a static_cast and does not check the type;
	//a C_TerrorWeapon vfunc on a viewmodel/arms = crash.
	bool IsPlayerEntity(IClientEntity* pEntity);
	bool IsWeaponEntity(IClientEntity* pEntity);
	//Only guns with the C_TerrorWeapon layout: melee/chainsaw/grenade launcher are
	//C_BaseCombatWeapon siblings; static_cast on them + a vfunc = foreign vtable slot.
	bool IsGunEntity(IClientEntity* pEntity);
	//Boomer and shifted IDs of foreign builds: identify specials by class name.
	bool IsSpecialByName(const char* szNet);
	//Fixed palette of special-infected classes (ESP + chams), fallback for unknowns.
	Color SIClassColor(const int nClassID, const Color& clrFallback);
};

namespace G { inline CGlobal_GameUtil Util; }
