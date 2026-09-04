#include "AutoStrafe.h"
#include "../Vars.h"

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
		cmd->sidemove = 450.0f * static_cast<float>(nSide);
	}
	else if (mode == 1) //Rage - perfect circle strafe, persistent side
	{
		Vector vel = pLocal->m_vecVelocity();
		float speed = vel.Lenght2D();

		if (speed < 50.0f)
			speed = 50.0f;

		//Optimal strafe yaw delta shrinks as speed grows (~15 deg at 300u/s).
		float ideal = 15.0f;

		if (speed > 300.0f) ideal = 10.0f;
		if (speed > 500.0f) ideal = 7.0f;
		if (speed > 800.0f) ideal = 5.0f;

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

		cmd->forwardmove = 0.0f;
		cmd->sidemove = 450.0f * static_cast<float>(side);
	}
}
