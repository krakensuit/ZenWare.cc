#include "AutoStrafe.h"
#include "../Vars.h"
#include <cmath>

#ifndef FL_ONGROUND
#define FL_ONGROUND (1 << 0)
#endif
#ifndef FL_DUCKING
#define FL_DUCKING (1 << 1)
#endif

void CFeatures_AutoStrafe::Run(C_TerrorPlayer* pLocal, CUserCmd* cmd)
{
	if (!Vars::BunnyHop::bAutoStrafe || !pLocal || !cmd || !cmd->command_number)
		return;

	const unsigned char nMoveType = pLocal->m_MoveType();

	if (nMoveType == MOVETYPE_LADDER || nMoveType == MOVETYPE_NOCLIP || nMoveType == MOVETYPE_OBSERVER)
		return;

	const int flags = pLocal->m_fFlags();
	const bool bOnGround = (flags & FL_ONGROUND) != 0;

	if (bOnGround)
		return;

	static int s_nLastSide = 1;
	static int s_nCircleSide = 1;

	const int mode = Vars::BunnyHop::nAutoStrafeMode;

	if (mode == 0) //Legit - mousedx based
	{
		if (cmd->mousedx == 0)
		{
			cmd->sidemove = 450.0f * static_cast<float>(s_nLastSide);
			return;
		}

		const int nSide = (cmd->mousedx > 0) ? 1 : -1;
		s_nLastSide = nSide;
		//плавный отклик: слабое движение мыши -> меньший side, резкий флик -> полный 450
		const float flRatio = U::Math.Clamp(fabsf((float)cmd->mousedx) / 12.0f, 0.25f, 1.0f);
		cmd->sidemove = 450.0f * flRatio * static_cast<float>(nSide);
	}
	else if (mode == 1) //Rage - perfect circle strafe, persistent side
	{
		Vector vel = pLocal->m_vecVelocity();
		float speed = vel.Lenght2D();

		if (speed < 50.0f)
			speed = 50.0f;

		//Optimal per-tick yaw delta shrinks as speed grows (atan formula,
		//scaled to the real tick interval instead of a fixed 15 deg spin).
		const float flInterval = (I::GlobalVars ? I::GlobalVars->interval_per_tick : (1.0f / 66.0f));
		float ideal = (atanf(30.0f / speed) * 57.29578f) * (flInterval / (1.0f / 66.0f));

		if (ideal < 1.0f)
			ideal = 1.0f;
		else if (ideal > 8.0f)
			ideal = 8.0f;

		//Flip only on explicit opposite mouse flick, never every tick
		//(flipping every tick stalls the circle).
		if (cmd->mousedx > 0)
			s_nCircleSide = 1;
		else if (cmd->mousedx < 0)
			s_nCircleSide = -1;

		s_nLastSide = s_nCircleSide;

		cmd->viewangles.y += ideal * static_cast<float>(s_nCircleSide);
		U::Math.ClampAngles(cmd->viewangles);
		cmd->sidemove = 450.0f * static_cast<float>(s_nCircleSide);
		cmd->forwardmove = 0.0f;
	}
	else if (mode == 2) //W-only strafe
	{
		//Keep forward pressed, strafe with mouse.
		cmd->forwardmove = 450.0f;

		if (cmd->mousedx == 0)
		{
			cmd->sidemove = 450.0f * static_cast<float>(s_nLastSide);
		}
		else
		{
			const int side = (cmd->mousedx > 0) ? 1 : -1;
			s_nLastSide = side;
			cmd->sidemove = 450.0f * static_cast<float>(side);
		}
	}
	else //Directional - full side strafe, respects held A/D keys
	{
		int held = 0;

		if (cmd->buttons & IN_MOVERIGHT)
			held = 1;
		else if (cmd->buttons & IN_MOVELEFT)
			held = -1;

		int side = held;

		if (!side)
		{
			if (cmd->mousedx > 0)
				side = 1;
			else if (cmd->mousedx < 0)
				side = -1;
			else
				side = s_nLastSide;
		}

		s_nLastSide = side;

		if (!(cmd->buttons & (IN_FORWARD | IN_BACK)))
			cmd->forwardmove = 0.0f;

		cmd->sidemove = 450.0f * static_cast<float>(side);
	}
}
