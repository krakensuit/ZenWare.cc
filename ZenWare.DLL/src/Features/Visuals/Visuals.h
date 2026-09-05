#pragma once

#include "../../SDK/SDK.h"

class CFeatures_Visuals
{
public:
	void DrawCrosshair();
	void DrawOverlay();
	void UpdateThirdPerson(); //call once per frame; toggles cam cvars
};

namespace F { inline CFeatures_Visuals Visuals; }
