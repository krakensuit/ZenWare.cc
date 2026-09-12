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

struct DmgLog_t {
	char szText[48] = { };
	Color clr = { 235, 245, 240, 255 };
	float flT = 0.0f; // curtime записи
};

class CFeatures_Hitmarker {
public:
	void OnTick(); // опрос HP врагов (ивентов в движке нет) — из Paint
	void Draw();   // цифры, вспышка, стата — из Paint
	void OnShot(); // фронт IN_ATTACK из CreateMove — для точности сессии
	void Clear() { m_mHp.clear(); m_aNums.clear(); m_aLog.clear(); m_flLastHitT = -1000.0f; m_nHits = 0; m_nKills = 0; m_nShots = 0; m_nLocalHp = -1; m_flLastDmgT = -1000.0f; m_bHasAttacker = false; m_flAttackerT = -1000.0f; }
	float SecondsSinceHit() const; // <0 если попаданий ещё не было
	int HitCount() const { return m_nHits; }
	int KillCount() const { return m_nKills; }
	int ShotCount() const { return m_nShots; }
private:
	void PlayHit(bool bKill);
	std::map<int, int> m_mHp;      // entindex -> HP на прошлом кадре
	std::vector<HitNum_t> m_aNums; // летящие цифры
	std::vector<DmgLog_t> m_aLog;  // лог урона (макс 6, живут 4 c)
	float m_flLastHitT = -1000.0f; // curtime последнего зачтённого попадания
	int m_nHits = 0;               // попаданий за сессию
	int m_nKills = 0;              // добиваний за сессию
	int m_nShots = 0;              // нажатий огня за сессию
	int m_nLocalHp = -1;           // HP локального на прошлом кадре
	float m_flLastDmgT = -1000.0f; // curtime последнего урона по нам
	Vector m_vAttacker;            // якорь вероятного атакующего (best-effort, без ивентов)
	float m_flAttackerT = -1000.0f;
	bool m_bHasAttacker = false;
	unsigned long long m_ullLastSnd = 0; // антиспам звука
};

namespace F { inline CFeatures_Hitmarker Hitmarker; }
