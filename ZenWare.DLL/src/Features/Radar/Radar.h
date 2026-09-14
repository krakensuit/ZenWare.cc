#pragma once

#include "../../SDK/SDK.h"

// 2D radar on top (top-down view, up = where we look) + spectator list
// (who is watching the local player via m_hObserverTarget).
// Read-only entity memory, called from Paint.
class CFeatures_Radar
{
public:
	void Render();

private:
	static constexpr int kX = 16;
	static constexpr int kY = 16;
	static constexpr int kSize = 150;
	static constexpr float kRange = 1600.0f; // ~30 meters
};

namespace F { inline CFeatures_Radar Radar; }
