#pragma once

#include "../../SDK/SDK.h"

class CFeatures_ESP
{
public:
	void Render();

private:
	bool GetBounds(C_BaseEntity* pBaseEntity, int& x, int& y, int& w, int& h);

	void DrawPlayer(C_TerrorPlayer* pLocal, C_TerrorPlayer* pPlayer, const int nEntityIndex);
	void DrawItem(C_TerrorPlayer* pLocal, C_BaseEntity* pEntity);
	void DrawCommon(C_BaseEntity* pEntity);
};

namespace F { inline CFeatures_ESP ESP; }
