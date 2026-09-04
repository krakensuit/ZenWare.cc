#pragma once

#include "../../SDK/SDK.h"

class CFeatures_Aimbot
{
public:
	void Run(C_TerrorPlayer* pLocal, C_TerrorWeapon* pWeapon, CUserCmd* cmd);

private:
	C_TerrorPlayer* FindTarget(C_TerrorPlayer* pLocal, const Vector& vEyePos, const Vector& vViewAngles);
	bool GetAimPoint(C_TerrorPlayer* pTarget, Vector& vOut);
	float GetWeight(C_TerrorPlayer* pTarget, const Vector& vFrom, const Vector& vEyePos, const Vector& vAngleTo) const;
	bool ShouldRun(C_TerrorPlayer* pLocal, C_TerrorWeapon* pWeapon, CUserCmd* cmd) const;
};

namespace F { inline CFeatures_Aimbot Aimbot; }
