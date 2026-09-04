#pragma once

#include "../../SDK/SDK.h"

class CFeatures_Visuals
{
public:
	void DrawCrosshair();
	void DrawOverlay();
};

namespace F { inline CFeatures_Visuals Visuals; }
