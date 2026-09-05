#pragma once

#include "../../SDK/SDK.h"

class CFeatures_Config
{
public:
	void Save();
	void Load();
	static const char* FilePath();

private:
};

namespace F { inline CFeatures_Config Config; }
