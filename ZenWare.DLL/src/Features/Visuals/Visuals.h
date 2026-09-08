#pragma once

#include "../../SDK/SDK.h"

class CFeatures_Visuals
{
public:
	void DrawCrosshair();
	void DrawOverlay();
	void DrawGrenade(); // preview of molotov/pipe/bile trajectory
	void UpdateThirdPerson(); //call once per frame; toggles cam cvars
	void UpdateFullbright(); //mat_fullbright without console, fail-closed
};

namespace F { inline CFeatures_Visuals Visuals; }
