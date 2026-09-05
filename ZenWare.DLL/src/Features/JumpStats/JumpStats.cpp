#include "JumpStats.h"
#include "../Lang/Lang.h"
#include "../Vars.h"

#ifndef FL_ONGROUND
#define FL_ONGROUND (1 << 0)
#endif

void CFeatures_JumpStats::OnTick(C_TerrorPlayer* pLocal, CUserCmd* cmd)
{
	if (!Vars::BunnyHop::bJumpStats || !pLocal || !cmd || !cmd->command_number)
		return;

	if (pLocal->deadflag() || pLocal->m_lifeState() != 0)
	{
		m_bAir = false;
		m_last.valid = false;
		return;
	}

	const bool bOnGround = (pLocal->m_fFlags() & FL_ONGROUND) != 0;
	const Vector vel = pLocal->m_vecVelocity();
	const float speed2d = vel.Lenght2D();
	const int tick = cmd->tick_count;

	if (!m_bAir && !bOnGround)
	{
		//Takeoff.
		m_bAir = true;
		m_vTakeoff = pLocal->m_vecOrigin();
		m_fTakeSpeed = speed2d;
		m_fMaxSpeed = speed2d;
		m_fMaxFall = vel.z;
		m_nTakeTick = tick;
		m_nAirTicks = 0;
		m_nMoveTicks = 0;
		m_nGoodTicks = 0;
		m_nStrafes = 0;
		m_nLastSide = 0;
		m_bDuckAtLand = false;
		return;
	}

	if (m_bAir)
	{
		m_nAirTicks++;

		if (speed2d > m_fMaxSpeed)
			m_fMaxSpeed = speed2d;

		if (vel.z < m_fMaxFall)
			m_fMaxFall = vel.z;

		int side = 0;

		if (cmd->sidemove > 10.0f)
			side = 1;
		else if (cmd->sidemove < -10.0f)
			side = -1;

		if (side)
		{
			if (m_nLastSide && side != m_nLastSide)
				m_nStrafes++;

			m_nLastSide = side;
			m_nMoveTicks++;

			//Synced when the mouse turn matches the strafe side
			//(or when strafing cleanly with no mouse input).
			if (cmd->mousedx == 0 || ((cmd->mousedx > 0) == (side > 0)))
				m_nGoodTicks++;
		}

		if (cmd->buttons & IN_DUCK)
			m_bDuckAtLand = true;
		else if (m_nAirTicks > 3)
			m_bDuckAtLand = false;

		if (!bOnGround)
			return;

		//Landing.
		m_bAir = false;

		const float dist = (pLocal->m_vecOrigin() - m_vTakeoff).Lenght2D();

		//Filter out stair steps and teleports: need real airtime and sane distance.
		if (m_nAirTicks >= 6 && dist > 40.0f && dist < 2000.0f)
		{
			m_last.dist = dist;
			m_last.pre = m_fTakeSpeed;
			m_last.max = m_fMaxSpeed;
			m_last.strafes = m_nStrafes;
			m_last.syncPct = (m_nMoveTicks > 0) ? (m_nGoodTicks * 100 / m_nMoveTicks) : 0;
			m_last.landTick = tick;
			m_last.edge = (m_nTakeTick - m_nLastGroundTick) <= 2;
			m_last.eb = ((cmd->buttons & IN_DUCK) != 0) && m_fMaxFall < -500.0f && dist > 200.0f;
			m_last.valid = true;
			m_nShowUntil = tick + 264; //~4 seconds at 66 ticks
		}
		else
		{
			m_last.valid = false;
		}
	}

	if (bOnGround)
		m_nLastGroundTick = tick;
}

void CFeatures_JumpStats::Draw()
{
	if (!Vars::BunnyHop::bJumpStats || !m_last.valid || !I::GlobalVars)
		return;

	if (I::GlobalVars->tickcount > m_nShowUntil)
		return;

	const int cx = G::Draw.m_nScreenW / 2;
	const int cy = G::Draw.m_nScreenH / 2 + 76;

	if (I::GlobalVars->tickcount < Vars::BunnyHop::nJbShowTick + 66)
		G::Draw.String(EFonts::MENU_CONSOLAS, cx, cy - 18, Color(0, 255, 171, 255), TXT_CENTERXY, "JUMPBUG");
	if (I::GlobalVars->tickcount < Vars::BunnyHop::nEbShowTick + 66)
		G::Draw.String(EFonts::MENU_CONSOLAS, cx, cy - 34, Color(255, 220, 0, 255), TXT_CENTERXY, "EDGEBUG");
	if (I::GlobalVars->tickcount < Vars::BunnyHop::nEjShowTick + 66)
		G::Draw.String(EFonts::MENU_CONSOLAS, cx, cy - 50, Color(0, 200, 255, 255), TXT_CENTERXY, "EDGEJUMP");

	const Color clrGood(0, 255, 171, 255);
	const bool bGood = (m_last.syncPct >= 90 && m_nStrafes >= 0);
	const Color& clrVerdict = bGood ? clrGood : Color(200, 200, 200, 255);

	char szMain[64] = { };
	sprintf_s(szMain, sizeof(szMain), "%.0fu  pre %.0f  max %.0f",
		m_last.dist, m_last.pre, m_last.max);
	G::Draw.String(EFonts::MENU_CONSOLAS, cx, cy, Color(235, 245, 240, 255), TXT_CENTERXY, szMain);

	char szSub[64] = { };
	sprintf_s(szSub, sizeof(szSub), "%d %s  %d%% %s%s%s",
		m_last.strafes, Lang::T("strafes"), m_last.syncPct, Lang::T("sync"),
		m_last.edge ? "  [edge]" : "",
		m_last.eb ? "  [eb]" : "");
	G::Draw.String(EFonts::MENU_CONSOLAS, cx, cy + 16, clrVerdict, TXT_CENTERXY, szSub);
}
