#include "JumpStats.h"
#include "../Lang/Lang.h"
#include "../Vars.h"
#include "../../Util/Logger/Logger.h"

#ifndef FL_ONGROUND
#define FL_ONGROUND (1 << 0)
#endif

void CFeatures_JumpStats::OnTick(C_TerrorPlayer* pLocal, CUserCmd* cmd, float flRawSide, int nRawMouseX)
{
	U::Log.Crumb("JumpStats::OnTick");
	if (!Vars::BunnyHop::bJumpStats || !pLocal || !cmd || !cmd->command_number)
		return;

	if (pLocal->deadflag() || pLocal->m_lifeState() != 0)
	{
		m_bAir = false;
		m_bSeenGround = false;
		m_last.valid = false;
		return;
	}

	//Лестница/гость/вода — не полёт: иначе езда по лестнице считается
	//прыжком (мусор в airticks/dist/sync + ложный [eb]).
	{
		const unsigned char nMoveType = pLocal->m_MoveType();

		if (pLocal->m_isGhost() || nMoveType == MOVETYPE_LADDER
			|| nMoveType == MOVETYPE_NOCLIP || nMoveType == MOVETYPE_OBSERVER
			|| pLocal->m_nWaterLevel() > 1)
		{
			m_bAir = false;
			m_bSeenGround = false;
			m_last.valid = false;
			return;
		}
	}

	const bool bOnGround = (pLocal->m_fFlags() & FL_ONGROUND) != 0;
	const Vector vel = pLocal->m_vecVelocity();
	const float speed2d = vel.Lenght2D();
	const int tick = cmd->tick_count;

	//Взлёт только после ≥1 живого наземного тика: иначе загрузка DLL,
	//респаун в падении или выход из лестницы синтезируют взлёт из воздуха.
	if (bOnGround)
		m_bSeenGround = true;
	if (!m_bAir && !bOnGround && !m_bSeenGround)
		return;

	if (!m_bAir && !bOnGround)
	{
		//Takeoff.
		m_bAir = true;
		m_vTakeoff = pLocal->m_vecOrigin();
		m_fTakeSpeed = speed2d;
		m_fMaxSpeed = speed2d;
		m_fMaxFall = vel.z;
		m_fMaxHeight = 0.0f;
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

		const float flHeight = pLocal->m_vecOrigin().z - m_vTakeoff.z;
		if (flHeight > m_fMaxHeight)
			m_fMaxHeight = flHeight;

		//Сырой ввод из снапшота CreateMove (до мутаций AutoStrafe).
		int side = 0;

		if (flRawSide > 10.0f)
			side = 1;
		else if (flRawSide < -10.0f)
			side = -1;

		if (side)
		{
			//Первый стрейф 0→side тоже считается (раньше каждый прыжок
			//недосчитывал 1).
			if (side != m_nLastSide)
				m_nStrafes++;

			m_nLastSide = side;
			m_nMoveTicks++;

			//Synced when the mouse turn matches the strafe side
			//(or when strafing cleanly with no mouse input).
			if (nRawMouseX == 0 || ((nRawMouseX > 0) == (side > 0)))
				m_nGoodTicks++;
		}

		// Латчим присед только в воздухе: на тике касания BunnyHop уже снял
		// IN_DUCK с cmd (JB-релиз), и чтение cmd на лендинге врало бы "не присел" —
		// из-за этого [eb] никогда не показывался, а jb/eb-счётчики стояли на нуле.
		if (!bOnGround)
		{
			if (cmd->buttons & IN_DUCK)
				m_bDuckAtLand = true;
			else if (m_nAirTicks > 3)
				m_bDuckAtLand = false;
		}

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
		//[edge] — только реальный EdgeJump: файр showtick на тике схода.
		//Старое (TakeTick-LastGroundTick) врало в обе стороны: перфект-бхоп
		//без касаний давал false, простой сход с бордюра — true.
		m_last.edge = Vars::BunnyHop::bEdgeJump && Vars::BunnyHop::nEjShowTick != 0
			&& m_nTakeTick >= Vars::BunnyHop::nEjShowTick
			&& (m_nTakeTick - Vars::BunnyHop::nEjShowTick) <= 2;
		const bool bDucked = m_bDuckAtLand || ((cmd->buttons & IN_DUCK) != 0);
		//Считаем только фактические срабатывания: свежий showtick (<=6 тиков) +
		//включённая фича + посадка в приседе. Ручной дак — только без AutoDuck
		//(иначе каждый прыжок с зажатым приседом был бы "+1 JB/EB").
		//ShowTick 0 = фича ни разу не файрила (первые тики карты врали бы true).
		const bool bEbFired = Vars::BunnyHop::bEdgeBug && Vars::BunnyHop::nEbShowTick != 0
			&& tick >= Vars::BunnyHop::nEbShowTick && (tick - Vars::BunnyHop::nEbShowTick) <= 6;
		const bool bJbFired = Vars::BunnyHop::bJumpBug && Vars::BunnyHop::nJbShowTick != 0
			&& tick >= Vars::BunnyHop::nJbShowTick && (tick - Vars::BunnyHop::nJbShowTick) <= 6;
		//Ручной дак — только при включённой фиче и без AutoDuck
		//(иначе каждый прыжок с зажатым приседом был бы "+1 JB/EB").
		const bool bManualEb = Vars::BunnyHop::bEdgeBug && !Vars::BunnyHop::bAutoDuck && m_fMaxFall < -500.0f && dist > 150.0f;
		const bool bManualJb = Vars::BunnyHop::bJumpBug && !Vars::BunnyHop::bAutoDuck && m_fMaxFall < -350.0f && dist > 100.0f;
		m_last.eb = bDucked && (bEbFired || bManualEb);
		m_last.fall = m_fMaxFall;
		m_last.height = (m_fMaxHeight > 0.0f) ? m_fMaxHeight : 0.0f;
		m_last.airSec = m_nAirTicks * (I::GlobalVars ? I::GlobalVars->interval_per_tick : (1.0f / 66.0f));
		// EB важнее JB: eb включает и условия JB, считаем только раз.
		if (m_last.eb)
			Vars::BunnyHop::nEbCount++;
		else if (bDucked && (bJbFired || bManualJb))
			Vars::BunnyHop::nJbCount++;
		m_last.valid = true;
		m_nShowUntil = tick + 264; //~4 seconds at 66 ticks
		U::Log.Write("JumpStats: landed %.0fu pre %.0f max %.0f fall %.0f sync %d%% duck %d eb %d",
			dist, m_fTakeSpeed, m_fMaxSpeed, m_fMaxFall, m_last.syncPct, bDucked ? 1 : 0, m_last.eb ? 1 : 0);
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
	if (!Vars::BunnyHop::bJumpStats || !I::GlobalVars)
		return;

	// Без игры — stale-панель с прошлой карты и ложные баннеры showtick.
	if (!I::EngineClient || !I::EngineClient->IsInGame())
	{
		m_last.valid = false;
		return;
	}

	// Live-sync текущего полёта: не ждём лендинга, рисуем пока летим.
	// m_last-панель ниже всё равно требует valid, порядок не важен.
	{
		const int nLive = LiveSyncPct();
		if (nLive >= 0)
		{
			const int cx = G::Draw.m_nScreenW / 2;
			const int cy = G::Draw.m_nScreenH / 2 - 60;
			const Color clr = (nLive >= 90) ? Color(0, 255, 171, 255) : Color(235, 245, 240, 255);
			G::Draw.String(EFonts::MENU_CONSOLAS, cx, cy, clr, TXT_CENTERXY, "sync %d%%", nLive);
		}
	}

	if (!m_last.valid)
		return;

	if (I::GlobalVars->tickcount > m_nShowUntil)
		return;

	const int cx = G::Draw.m_nScreenW / 2;
	const int cy = G::Draw.m_nScreenH / 2 + 76;

	//ShowTick 0 = ни разу не файрило: без проверки баннер горит на спавне.
	if (Vars::BunnyHop::nJbShowTick != 0 && I::GlobalVars->tickcount < Vars::BunnyHop::nJbShowTick + 66)
		G::Draw.String(EFonts::MENU_CONSOLAS, cx, cy - 18, Color(0, 255, 171, 255), TXT_CENTERXY, "JUMPBUG");
	if (Vars::BunnyHop::nEbShowTick != 0 && I::GlobalVars->tickcount < Vars::BunnyHop::nEbShowTick + 66)
		G::Draw.String(EFonts::MENU_CONSOLAS, cx, cy - 34, Color(255, 220, 0, 255), TXT_CENTERXY, "EDGEBUG");
	if (Vars::BunnyHop::nEjShowTick != 0 && I::GlobalVars->tickcount < Vars::BunnyHop::nEjShowTick + 66)
		G::Draw.String(EFonts::MENU_CONSOLAS, cx, cy - 50, Color(0, 200, 255, 255), TXT_CENTERXY, "EDGEJUMP");

	const Color clrGood(0, 255, 171, 255);
	const bool bGood = (m_last.syncPct >= 90 && m_last.strafes > 0);
	const Color& clrVerdict = bGood ? clrGood : Color(200, 200, 200, 255);

	char szMain[64] = { };
	sprintf_s(szMain, sizeof(szMain), "%.0fu  pre %.0f  max %.0f  fall %.0f",
		m_last.dist, m_last.pre, m_last.max, m_last.fall);
	G::Draw.String(EFonts::MENU_CONSOLAS, cx, cy, Color(235, 245, 240, 255), TXT_CENTERXY, "%s", szMain);

	char szSub[64] = { };
	sprintf_s(szSub, sizeof(szSub), "%d %s  %d%% %s%s%s",
		m_last.strafes, Lang::T("strafes"), m_last.syncPct, Lang::T("sync"),
		m_last.edge ? "  [edge]" : "",
		m_last.eb ? "  [eb]" : "");
	G::Draw.String(EFonts::MENU_CONSOLAS, cx, cy + 16, clrVerdict, TXT_CENTERXY, "%s", szSub);

	char szExtra[64] = { };
	sprintf_s(szExtra, sizeof(szExtra), "H %.0fu  air %.2fs  |  jb %d  eb %d",
		m_last.height, m_last.airSec, Vars::BunnyHop::nJbCount, Vars::BunnyHop::nEbCount);
	G::Draw.String(EFonts::MENU_CONSOLAS, cx, cy + 32, Color(140, 160, 152, 255), TXT_CENTERXY, "%s", szExtra);
}
