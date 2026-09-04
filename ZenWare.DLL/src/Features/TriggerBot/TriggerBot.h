#pragma once

#include "../../SDK/SDK.h"

class CFeatures_TriggerBot
{
public:
	void Run(C_TerrorPlayer* pLocal, C_TerrorWeapon* pWeapon, CUserCmd* cmd);
};

namespace F { inline CFeatures_TriggerBot TriggerBot; }
