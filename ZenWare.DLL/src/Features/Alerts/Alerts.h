#pragma once

#include "../../SDK/SDK.h"

// Баннеры угроз: танк заспавнился / ведьма рядом (+дистанция).
// Только чтение сущностей, вызывается из Paint.
class CFeatures_Alerts
{
public:
	void Render();
};

namespace F { inline CFeatures_Alerts Alerts; }
