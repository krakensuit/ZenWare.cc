#pragma once
#include "../../SDK/SDK.h"
#include <map>
#include <vector>

struct HitNum_t {
	Vector vWorld;
	int nDamage = 0;
	float flT = 0.0f;   // I::GlobalVars->curtime на момент попадания
	bool bKill = false; // добивающий урон
};

class CFeatures_Hitmarker {
public:
	void OnTick(); // опрос HP врагов (ивентов в движке нет) — из Paint
	void Draw();   // всплывающие цифры — из Paint
	void Clear() { m_mHp.clear(); m_aNums.clear(); }
private:
	void PlayHit(bool bKill);
	std::map<int, int> m_mHp;      // entindex -> HP на прошлом кадре
	std::vector<HitNum_t> m_aNums; // летящие цифры
	unsigned long long m_ullLastSnd = 0; // антиспам звука
};

namespace F { inline CFeatures_Hitmarker Hitmarker; }
