#pragma once

#include "../../SDK/SDK.h"

class CFeatures_AutoShove
{
public:
	void Run(C_TerrorPlayer* pLocal, CUserCmd* cmd);
};

namespace F { inline CFeatures_AutoShove AutoShove; }
