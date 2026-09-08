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
	Vector GetEyePosition(C_TerrorPlayer* pEntity);
	bool IsValidTarget(C_TerrorPlayer* pLocal, C_TerrorPlayer* pPlayer, bool bCheckVisible = false);
	bool IsTargetVisible(C_TerrorPlayer* pLocal, C_TerrorPlayer* pTarget, const Vector& vEyePos);
	//ClassID гейты перед даункастом: As<T> это static_cast и тип не проверяет,
	//виртуалка C_TerrorWeapon на viewmodel/руках = краш.
	bool IsPlayerEntity(IClientEntity* pEntity);
	bool IsWeaponEntity(IClientEntity* pEntity);
	//Бумер и сдвинутые ID чужих билдов: опознаём СИ по имени класса.
	bool IsSpecialByName(const char* szNet);
};

namespace G { inline CGlobal_GameUtil Util; }
