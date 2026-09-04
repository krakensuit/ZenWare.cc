#pragma once

#include "../../SDK/SDK.h"

class CFeatures_Chams
{
public:
	//Call before the original DrawModelExecute. Returns true when a material override was applied.
	bool OnDrawModel(const ModelRenderInfo_t& pInfo);

	//Syncs clrEnemy/clrAlly/clrTank from the selected preset palette.
	static void ApplyPalette();

	//Call after the original DrawModelExecute to reset the override.
	void OnDrawModelEnd();

private:
	void LazyInit();

	bool m_bInitialized = false;

	IMaterial* m_pEnemy = nullptr;
	IMaterial* m_pAlly = nullptr;
	IMaterial* m_pTank = nullptr;
};

namespace F { inline CFeatures_Chams Chams; }
