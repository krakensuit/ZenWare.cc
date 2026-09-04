#pragma once

#include "../../SDK/SDK.h"

class CFeatures_AutoPistol
{
public:
	void Run(C_TerrorWeapon* pWeapon, CUserCmd* cmd);
};

namespace F { inline CFeatures_AutoPistol AutoPistol; }
