#pragma once

#include "../../SDK/SDK.h"

//KZ-style jump statistics: tracks one airtime from takeoff to landing and
//shows distance / prestrafe / max speed / strafe count / sync plus edge and
//edgebug verdicts. Called from CreateMove (tick) and Paint (draw).
class CFeatures_JumpStats
{
public:
	void OnTick(C_TerrorPlayer* pLocal, CUserCmd* cmd, float flRawSide, int nRawMouseX);
	void Draw();
	// Текущий синхрон незавершённого полёта (скользящее окно тиков).
	// -1 = не в воздухе / нечего считать.
	int LiveSyncPct() const
	{
		if (!m_bAir || m_nMoveTicks <= 0)
			return -1;
		return (m_nGoodTicks * 100) / m_nMoveTicks;
	}

private:
	struct Jump_t
	{
		float dist = 0.0f;
		float pre = 0.0f;
		float max = 0.0f;
		float height = 0.0f; // peak takeoff->apex, units
		float airSec = 0.0f; // airtime, seconds
		float fall = 0.0f; // peak fall speed (negative), units/s
		int strafes = 0;
		int syncPct = 0;
		int landTick = 0;
		bool edge = false;
		bool eb = false;
		bool valid = false;
	};

	bool m_bAir = false;
	Vector m_vTakeoff;
	float m_fTakeSpeed = 0.0f;
	float m_fMaxSpeed = 0.0f;
	float m_fMaxFall = 0.0f;
	float m_fMaxHeight = 0.0f;
	int m_nTakeTick = 0;
	int m_nAirTicks = 0;
	int m_nMoveTicks = 0;
	int m_nGoodTicks = 0;
	int m_nStrafes = 0;
	int m_nLastSide = 0;
	bool m_bDuckAtLand = false;
	int m_nLastGroundTick = 0;
	int m_nShowUntil = 0;

	Jump_t m_last = { };
};

namespace F { inline CFeatures_JumpStats JumpStats; }
