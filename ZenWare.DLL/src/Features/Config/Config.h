#pragma once

#include "../../SDK/SDK.h"

class CFeatures_Config
{
public:
	void Save();
	void Load();

private:
	static const char* FilePath();
};

namespace F { inline CFeatures_Config Config; }
