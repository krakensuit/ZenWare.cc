#include "BunnyHop.h"
#include "../Vars.h"
#include "../../Util/Logger/Logger.h"

#ifndef FL_ONGROUND
#define FL_ONGROUND (1 << 0)
#endif
#ifndef FL_DUCKING
#define FL_DUCKING (1 << 1)
#endif

void CFeatures_BunnyHop::Run(C_TerrorPlayer* pLocal, CUserCmd* cmd)
{
	if (!Vars::BunnyHop::bEnabled || !pLocal || !cmd || !cmd->command_number)
		return;

	if (pLocal->deadflag() || pLocal->m_lifeState() != 0 || pLocal->m_isGhost())
		return;

	if (!G::Util.IsValidTeam(pLocal->GetTeamNumber()))
		return;

	//Never touch movement on ladders / noclip / observer cam.
	const unsigned char nMoveType = pLocal->m_MoveType();

	if (nMoveType == MOVETYPE_LADDER || nMoveType == MOVETYPE_NOCLIP || nMoveType == MOVETYPE_OBSERVER)
		return;

	const bool bOnGround = (pLocal->m_fFlags() & FL_ONGROUND) != 0;
	const bool bDucking = (pLocal->m_fFlags() & FL_DUCKING) != 0;

	//NullMovement: opposing keys cancel out so strafes stay clean.
	if (Vars::BunnyHop::bNullMove)
	{
		if ((cmd->buttons & IN_MOVELEFT) && (cmd->buttons & IN_MOVERIGHT))
			cmd->buttons &= ~(IN_MOVELEFT | IN_MOVERIGHT);

		if ((cmd->buttons & IN_FORWARD) && (cmd->buttons & IN_BACK))
			cmd->buttons &= ~(IN_FORWARD | IN_BACK);
	}

	//JumpBug: duck-tap right before a hard landing to negate it,
	//release the duck the moment we touch ground again.
	static bool s_bJbDuck = false;
	if (Vars::BunnyHop::bJumpBug)
	{
		if (!bOnGround)
		{
			const Vector vel = pLocal->m_vecVelocity();

			if (vel.z < -350.0f)
			{
				Vector origin = pLocal->m_vecOrigin();
				Vector down = origin; down.z -= 160.0f;

				trace_t tr;
				CTraceFilterHitAll filter(pLocal);
				G::Util.Trace(origin, down, MASK_PLAYERSOLID, &filter, &tr);

				if (tr.fraction < 1.0f && !tr.startsolid)
				{
					//Duck only inside the real bug window (~2 ticks to impact),
					//ducking earlier just lands ducked without the bug.
					const float flInterval = (I::GlobalVars ? I::GlobalVars->interval_per_tick : (1.0f / 66.0f));
					const float flDist = tr.fraction * 160.0f;

					if (flDist / (-vel.z * flInterval) <= 2.0f)
					{
						cmd->buttons |= IN_DUCK;
						s_bJbDuck = true;
						Vars::BunnyHop::nJbShowTick = cmd->tick_count;
						ZTRACE_FIRST("BunnyHop:jumpbug");
					}
				}
			}
		}
		else if (s_bJbDuck)
		{
			cmd->buttons &= ~IN_DUCK;
			s_bJbDuck = false;
		}
	}
	else
		s_bJbDuck = false;

	//AutoDuck: hold duck through the whole airtime (longer jumps, duck-landings).
	if (Vars::BunnyHop::bAutoDuck && !bOnGround)
		cmd->buttons |= IN_DUCK;

	//Perfect style accepts any jump source (space / wheel / bound key);
	//legit style only honors the game's own IN_JUMP bit for this tick.
	const bool bPhysicalJump = ((GetAsyncKeyState(VK_SPACE) & 0x8000) || (GetAsyncKeyState(VK_XBUTTON1) & 0x8000));
	const bool bWantJump = (Vars::BunnyHop::nBhopStyle == 0)
		? ((cmd->buttons & IN_JUMP) || bPhysicalJump)
		: ((cmd->buttons & IN_JUMP) != 0);

	//EdgeJump: walked off an edge while holding jump -> jump anyway.
	static bool s_bWasOnGround = true;
	bool bEdgeJumped = false;

	if (Vars::BunnyHop::bEdgeJump && s_bWasOnGround && !bOnGround && bWantJump)
	{
		cmd->buttons |= IN_JUMP;
		bEdgeJumped = true;
		Vars::BunnyHop::nEjShowTick = cmd->tick_count;
		ZTRACE_FIRST("BunnyHop:edgejump");
	}

	s_bWasOnGround = bOnGround;

	//Prestrafe: slight forward boost when on ground to build speed faster.
	if (Vars::BunnyHop::bPrestrafe && bOnGround && bWantJump)
	{
		if (cmd->forwardmove >= 0.0f && cmd->forwardmove < 300.0f)
			cmd->forwardmove = 450.0f;
	}

	if (bWantJump)
	{
		if (bOnGround)
		{
			static int s_nLastJumpTick = 0;

			if (Vars::BunnyHop::nJumpDelayTicks > 0 && s_nLastJumpTick != 0 &&
				(cmd->tick_count - s_nLastJumpTick) < Vars::BunnyHop::nJumpDelayTicks)
				return;

			//Don't jump if ducking and edgebug wants to keep duck.
			if (!(bDucking && Vars::BunnyHop::bEdgeBug))
				cmd->buttons |= IN_JUMP;

			//Long-jump helper: crouch-jump gives extra distance.
			if (Vars::BunnyHop::bLongJumpHelper)
				cmd->buttons |= IN_DUCK;

			s_nLastJumpTick = cmd->tick_count;
		}
		else
		{
			//In air - release jump so next landing can be perfect.
			//Keep duck if edgebug wants to bug the landing, and never strip
			//an edgejump that just fired on this tick.
			if (!Vars::BunnyHop::bEdgeBug && !bEdgeJumped)
				cmd->buttons &= ~IN_JUMP;
		}
	}

	//EdgeBug: auto-duck right before landing at high speed to preserve velocity.
	if (Vars::BunnyHop::bEdgeBug && !bOnGround)
	{
		const Vector vel = pLocal->m_vecVelocity();

		if (vel.Lenght2D() > 250.0f && vel.z < -100.0f)
		{
			Vector origin = pLocal->m_vecOrigin();
			Vector down = origin; down.z -= 160.0f;

			trace_t tr;
			CTraceFilterHitAll filter(pLocal);
			G::Util.Trace(origin, down, MASK_PLAYERSOLID, &filter, &tr);

			if (tr.fraction < 1.0f && !tr.startsolid)
			{
				//Same tight window as jumpbug: duck ~2 ticks before impact.
				const float flInterval = (I::GlobalVars ? I::GlobalVars->interval_per_tick : (1.0f / 66.0f));
				const float flDist = tr.fraction * 160.0f;

				if (flDist / (-vel.z * flInterval) <= 2.0f)
				{
					cmd->buttons |= IN_DUCK;
					Vars::BunnyHop::nEbShowTick = cmd->tick_count;
					ZTRACE_FIRST("BunnyHop:edgebug");
				}
			}
		}
	}

	//FastStop - instant counter-strafe when no keys pressed (never fights an active bhop jump).
	if (Vars::BunnyHop::bFastStop && bOnGround && !bWantJump)
	{
		if (cmd->forwardmove == 0.0f && cmd->sidemove == 0.0f)
		{
			Vector vel = pLocal->m_vecVelocity();
			vel.z = 0.0f;

			const float len = vel.Lenght2D();

			if (len > 25.0f)
			{
				Vector ang; U::Math.VectorAngles(vel, ang);

				float yaw = ang.y - cmd->viewangles.y;

				while (yaw > 180.0f) yaw -= 360.0f;
				while (yaw < -180.0f) yaw += 360.0f;

				const float rad = DEG2RAD(yaw);
				cmd->forwardmove = cosf(rad) * -450.0f;
				cmd->sidemove = sinf(rad) * -450.0f;
			}
		}
	}
}
