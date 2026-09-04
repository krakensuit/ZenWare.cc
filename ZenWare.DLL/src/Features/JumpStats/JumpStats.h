#pragma once

#include "../../SDK/SDK.h"

//KZ-style jump statistics: tracks one airtime from takeoff to landing and
//shows distance / prestrafe / max speed / strafe count / sync plus edge and
//edgebug verdicts. Called from CreateMove (tick) and Paint (draw).
class CFeatures_JumpStats
{
public:
	void OnTick(C_TerrorPlayer* pLocal, CUserCmd* cmd);
	void Draw();

private:
	struct Jump_t
	{
		float dist = 0.0f;
		float pre = 0.0f;
		float max = 0.0f;
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
