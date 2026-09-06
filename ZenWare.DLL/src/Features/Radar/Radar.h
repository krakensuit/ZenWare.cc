#pragma once

#include "../../SDK/SDK.h"

// 2D-радар сверху (вид сверху, вверх = куда смотрим) + список наблюдателей
// (кто смотрит за локальным игроком через m_hObserverTarget).
// Только чтение памяти сущностей, вызывается из Paint.
class CFeatures_Radar
{
public:
	void Render();

private:
	static constexpr int kX = 16;
	static constexpr int kY = 16;
	static constexpr int kSize = 150;
	static constexpr float kRange = 1600.0f; // ~30 метров
};

namespace F { inline CFeatures_Radar Radar; }
