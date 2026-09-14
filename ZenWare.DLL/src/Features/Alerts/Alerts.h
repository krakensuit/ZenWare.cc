#pragma once

#include "../../SDK/SDK.h"

// Threat banners: tank spawned / witch nearby (+distance).
// Read-only entity access, called from Paint.
class CFeatures_Alerts
{
public:
	void Render();
};

namespace F { inline CFeatures_Alerts Alerts; }
