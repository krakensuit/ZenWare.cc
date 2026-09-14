#pragma once
#include "../../SDK/SDK.h"
#include <map>
#include <vector>

struct HitNum_t {
	Vector vWorld;
	int nDamage = 0;
	float flT = 0.0f;   // I::GlobalVars->curtime at the moment of the hit
	bool bKill = false; // killing blow damage
};

struct DmgLog_t {
	char szText[48] = { };
	Color clr = { 235, 245, 240, 255 };
	float flT = 0.0f; // curtime of the entry
};

class CFeatures_Hitmarker {
public:
	void OnTick(); // enemy HP polling (no engine events) — from Paint
	void Draw();   // numbers, flash, stats — from Paint
	void OnShot(); // IN_ATTACK front from CreateMove — for session accuracy
	void Clear() { m_mHp.clear(); m_aNums.clear(); m_aLog.clear(); m_flLastHitT = -1000.0f; m_nHits = 0; m_nKills = 0; m_nShots = 0; m_nLocalHp = -1; m_flLastDmgT = -1000.0f; m_bHasAttacker = false; m_flAttackerT = -1000.0f; }
	float SecondsSinceHit() const; // <0 if no hits yet
	int HitCount() const { return m_nHits; }
	int KillCount() const { return m_nKills; }
	int ShotCount() const { return m_nShots; }
private:
	void PlayHit(bool bKill);
	std::map<int, int> m_mHp;      // entindex -> HP on the previous frame
	std::vector<HitNum_t> m_aNums; // floating damage numbers
	std::vector<DmgLog_t> m_aLog;  // damage log (max 6, live 4 s)
	float m_flLastHitT = -1000.0f; // curtime of the last counted hit
	int m_nHits = 0;               // hits this session
	int m_nKills = 0;              // kills this session
	int m_nShots = 0;              // fire presses this session
	int m_nLocalHp = -1;           // local HP on the previous frame
	float m_flLastDmgT = -1000.0f; // curtime of the last damage taken
	Vector m_vAttacker;            // anchor of the probable attacker (best-effort, no events)
	float m_flAttackerT = -1000.0f;
	bool m_bHasAttacker = false;
	unsigned long long m_ullLastSnd = 0; // sound anti-spam
};

namespace F { inline CFeatures_Hitmarker Hitmarker; }
