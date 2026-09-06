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
	void DrawSpecial(C_TerrorPlayer* pLocal, C_BaseEntity* pEntity, const int nClassID);
	void DrawBoss(C_BaseEntity* pEntity);
	// Fallback по имени класса (m_pNetworkName): ловит бумера (ID нет в дампе)
	// и вообще все классы при смене ID на чужом билде. Только чтение, без новых вызовов.
	void DrawUnknown(C_TerrorPlayer* pLocal, C_BaseEntity* pEntity, const char* szNetworkName);
};

namespace F { inline CFeatures_ESP ESP; }
