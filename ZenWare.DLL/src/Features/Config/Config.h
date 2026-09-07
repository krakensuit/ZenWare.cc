#pragma once

#include "../../SDK/SDK.h"

class CFeatures_Config
{
public:
	void Save();
	void Load();
	static const char* FilePath();
	static void SetSlot(int nSlot); // 1..3 -> ZenWare.cfg / ZenWare2.cfg / ZenWare3.cfg
	static int GetSlot();
	static const char* SlotName(int nSlot);

private:
};

namespace F { inline CFeatures_Config Config; }
