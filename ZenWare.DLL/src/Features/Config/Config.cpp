#include "Config.h"

#include "../Vars.h"

#include <vector>

//Explicit key=value list: boring, predictable, and never breaks when a new
//variable is added (missing keys simply keep their defaults on load).

namespace
{
	struct Entry_t
	{
		const char* m_szKey;
		bool* m_pBool = nullptr;
		int* m_pInt = nullptr;
		float* m_pFloat = nullptr;
		Color* m_pColor = nullptr;

		Entry_t(const char* k, bool* p) : m_szKey(k), m_pBool(p) {}
		Entry_t(const char* k, int* p) : m_szKey(k), m_pInt(p) {}
		Entry_t(const char* k, float* p) : m_szKey(k), m_pFloat(p) {}
		Entry_t(const char* k, Color* p) : m_szKey(k), m_pColor(p) {}
	};

	std::vector<Entry_t> GetEntries()
	{
		return {
			//Aimbot
			{ "aimbot.enabled", &Vars::Aimbot::bEnabled },
			{ "aimbot.silent", &Vars::Aimbot::bSilent },
			{ "aimbot.autoshoot", &Vars::Aimbot::bAutoShoot },
			{ "aimbot.visible", &Vars::Aimbot::bVisibleOnly },
			{ "aimbot.noincapped", &Vars::Aimbot::bIgnoreIncapped },
			{ "aimbot.priority", &Vars::Aimbot::nTargetPriority },
			{ "aimbot.hitbox", &Vars::Aimbot::nHitbox },
			{ "aimbot.fov", &Vars::Aimbot::flFOV },
			{ "aimbot.smoothing", &Vars::Aimbot::flSmoothing },
			{ "aimbot.key", &Vars::Aimbot::nKey },
		{ "aimbot.commons", &Vars::Aimbot::bTargetCommons },
		{ "aimbot.specials", &Vars::Aimbot::bTargetSpecials },

		//Aimbot per-weapon groups
		{ "aimwpn.enabled", &Vars::AimbotWpn::bEnabled },
		{ "aimwpn.group", &Vars::AimbotWpn::nGroup },
		{ "aimwpn.rifle.fov", &Vars::AimbotWpn::flRifleFov },
		{ "aimwpn.rifle.smooth", &Vars::AimbotWpn::nRifleSmooth },
		{ "aimwpn.rifle.hitbox", &Vars::AimbotWpn::nRifleHitbox },
		{ "aimwpn.smg.fov", &Vars::AimbotWpn::flSmgFov },
		{ "aimwpn.smg.smooth", &Vars::AimbotWpn::nSmgSmooth },
		{ "aimwpn.smg.hitbox", &Vars::AimbotWpn::nSmgHitbox },
		{ "aimwpn.shotgun.fov", &Vars::AimbotWpn::flShotgunFov },
		{ "aimwpn.shotgun.smooth", &Vars::AimbotWpn::nShotgunSmooth },
		{ "aimwpn.shotgun.hitbox", &Vars::AimbotWpn::nShotgunHitbox },
		{ "aimwpn.sniper.fov", &Vars::AimbotWpn::flSniperFov },
		{ "aimwpn.sniper.smooth", &Vars::AimbotWpn::nSniperSmooth },
		{ "aimwpn.sniper.hitbox", &Vars::AimbotWpn::nSniperHitbox },
		{ "aimwpn.pistol.fov", &Vars::AimbotWpn::flPistolFov },
		{ "aimwpn.pistol.smooth", &Vars::AimbotWpn::nPistolSmooth },
		{ "aimwpn.pistol.hitbox", &Vars::AimbotWpn::nPistolHitbox },

			//Trigger / shove / pistol
			{ "trigger.enabled", &Vars::TriggerBot::bEnabled },
			{ "trigger.visible", &Vars::TriggerBot::bVisibleOnly },
			{ "trigger.key", &Vars::TriggerBot::nKey },
			{ "autoshove.enabled", &Vars::AutoShove::bEnabled },
			{ "autopistol.enabled", &Vars::AutoPistol::bEnabled },
			{ "nospread.enabled", &Vars::NoSpread::bEnabled },
			{ "killfeed.enabled", &Vars::Killfeed::bEnabled },
		{ "hitmarker.enabled", &Vars::Hitmarker::bEnabled },
		{ "hitmarker.sound", &Vars::Hitmarker::bSound },
		{ "hitmarker.numbers", &Vars::Hitmarker::bNumbers },
		{ "hitmarker.xmark", &Vars::Hitmarker::bXMark },
		{ "hitmarker.stats", &Vars::Hitmarker::bStats },
		{ "hitmarker.dmgflash", &Vars::Hitmarker::bDmgFlash },
		{ "hitmarker.dmarrow", &Vars::Hitmarker::bDmgArrow },
		{ "hitmarker.dmglog", &Vars::Hitmarker::bDmgLog },
		{ "hitmarker.pitch", &Vars::Hitmarker::nPitch },
		{ "hitmarker.duration", &Vars::Hitmarker::nDurationMs },
		{ "hitmarker.mindmg", &Vars::Hitmarker::nMinDmg },
			{ "radar.enabled", &Vars::Radar::bEnabled },
			{ "radar.spectators", &Vars::Radar::bSpectators },
		{ "alerts.enabled", &Vars::Alerts::bEnabled },
		{ "alerts.tank", &Vars::Alerts::bTank },
		{ "alerts.witch", &Vars::Alerts::bWitch },
		{ "alerts.silist", &Vars::Alerts::bSIList },
		{ "alerts.pinned", &Vars::Alerts::bPinned },
		{ "alerts.revive", &Vars::Alerts::bRevive },
		{ "alerts.tankhp", &Vars::Alerts::bTankHp },
		{ "alerts.spit", &Vars::Alerts::bSpitAlert },

			//Movement
			{ "bhop.enabled", &Vars::BunnyHop::bEnabled },
			{ "bhop.style", &Vars::BunnyHop::nBhopStyle },
			{ "bhop.autostrafe", &Vars::BunnyHop::bAutoStrafe },
			{ "bhop.strafemode", &Vars::BunnyHop::nAutoStrafeMode },
			{ "bhop.delay", &Vars::BunnyHop::nJumpDelayTicks },
			{ "bhop.jumpbug", &Vars::BunnyHop::bJumpBug },
			{ "bhop.nullmove", &Vars::BunnyHop::bNullMove },
			{ "bhop.jumpstats", &Vars::BunnyHop::bJumpStats },
			{ "bhop.edgejump", &Vars::BunnyHop::bEdgeJump },
			{ "bhop.edgebug", &Vars::BunnyHop::bEdgeBug },
			{ "bhop.faststop", &Vars::BunnyHop::bFastStop },
			{ "bhop.prestrafe", &Vars::BunnyHop::bPrestrafe },
			{ "bhop.longjump", &Vars::BunnyHop::bLongJumpHelper },
			{ "bhop.speedhud", &Vars::BunnyHop::bSpeedHUD },
			{ "bhop.autoduck", &Vars::BunnyHop::bAutoDuck },

			//Chams
			{ "chams.enabled", &Vars::Chams::bEnabled },
			{ "chams.throughwalls", &Vars::Chams::bThroughWalls },
			{ "chams.palette", &Vars::Chams::nPalette },
			{ "chams.enemy", &Vars::Chams::clrEnemy },
		{ "chams.sicolors", &Vars::Chams::bSIColors },
			{ "chams.ally", &Vars::Chams::clrAlly },
			{ "chams.tank", &Vars::Chams::clrTank },

		//ESP
		{ "esp.enabled", &Vars::ESP::bEnabled },
		{ "esp.box", &Vars::ESP::bBox },
		{ "esp.healthbar", &Vars::ESP::bHealthBar },
		{ "esp.name", &Vars::ESP::bName },
		{ "esp.distance", &Vars::ESP::bDistance },
		{ "esp.items", &Vars::ESP::bItems },
		{ "esp.common", &Vars::ESP::bCommon },
		{ "esp.filled", &Vars::ESP::bFilled },
		{ "esp.snaplines", &Vars::ESP::bSnaplines },
		{ "esp.healthtext", &Vars::ESP::bHealthText },
		{ "esp.weapontext", &Vars::ESP::bWeaponText },
		{ "esp.ammo", &Vars::ESP::bAmmo },
		{ "esp.showteam", &Vars::ESP::bShowTeam },
		{ "esp.specialboxes", &Vars::ESP::bSpecialBoxes },
		{ "esp.bossboxes", &Vars::ESP::bBossBoxes },
		{ "esp.teampanel", &Vars::ESP::bTeamPanel },
		{ "esp.throwtimers", &Vars::ESP::bThrowTimers },
		{ "esp.panickey", &Vars::ESP::nPanicKey },

		//Visuals
		{ "visuals.nofog", &Vars::Visuals::bNoFog },
		{ "visuals.fullbright", &Vars::Visuals::bFullbright },
		{ "visuals.viewfov", &Vars::Visuals::flViewFOV },
		{ "visuals.vmfov", &Vars::Visuals::flVmFOV },
		{ "visuals.crosshair", &Vars::Visuals::bCrosshair },
		{ "visuals.chsize", &Vars::Visuals::nCrosshairSize },
		{ "visuals.overlay", &Vars::Visuals::bOverlay },
		{ "visuals.chcolor", &Vars::Visuals::clrCrosshair },
		{ "visuals.3rdperson", &Vars::Visuals::bThirdPerson },
		{ "visuals.3rdpersondist", &Vars::Visuals::nThirdPersonDist },
		{ "visuals.noscreenfx", &Vars::Visuals::bNoScreenFx },
		{ "visuals.commoncount", &Vars::Visuals::bCommonCount },
		{ "visuals.weaponhud", &Vars::Visuals::bWeaponHud },
		{ "visuals.spreadcircle", &Vars::Visuals::bSpreadCircle },
		{ "visuals.bindlist", &Vars::Visuals::bBindList },
		{ "visuals.reload", &Vars::Visuals::bReloadAlert },
		{ "visuals.espmaxdist", &Vars::Visuals::flEspMaxDist },
		{ "visuals.hidehands", &Vars::Visuals::bHideHands },
		{ "grenade.path", &Vars::Grenade::bEnabled },
		{ "grenade.landing", &Vars::Grenade::bLanding },

			//Recoil
			{ "norecoil.visual", &Vars::VisualRecoil::bEnabled },

		//Menu style
		{ "menu.accent", &Vars::Menu::clrAccent },
		{ "menu.key", &Vars::Menu::nKey },
		{ "menu.lang", &Vars::Menu::nLang },
		};
	}
}

const char* CFeatures_Config::SlotName(int nSlot)
{
	switch (nSlot)
	{
		case 2: return "ZenWare2.cfg";
		case 3: return "ZenWare3.cfg";
		default: return "ZenWare.cfg"; // слот 1 = старый файл, совместимость
	}
}

namespace { int s_nCfgSlot = 1; }

void CFeatures_Config::SetSlot(int nSlot)
{
	if (nSlot < 1 || nSlot > 3)
		return;
	s_nCfgSlot = nSlot;
}

int CFeatures_Config::GetSlot()
{
	return s_nCfgSlot;
}

const char* CFeatures_Config::FilePath()
{
	static char szPath[MAX_PATH] = { };
	static int s_nCachedSlot = 0;

	if (!szPath[0] || s_nCachedSlot != s_nCfgSlot)
	{
		//Same as Logger: module path, no engine virtuals at startup.
		char szGameDir[MAX_PATH] = { };
		char szMod[MAX_PATH] = { };

		if (GetModuleFileNameA(GetModuleHandleA("client.dll"), szMod, MAX_PATH) && szMod[0])
		{
			char* szSlash = strrchr(szMod, '\\');

			if (szSlash)
			{
				*szSlash = '\0';
				szSlash = strrchr(szMod, '\\');

				if (szSlash)
				{
					if (_stricmp(szSlash, "\\bin") == 0)
						*szSlash = '\0';

					if (szMod[0])
						strcpy_s(szGameDir, szMod);
				}
			}
		}

		if (szGameDir[0])
		{
			sprintf_s(szPath, "%s\\%s", szGameDir, SlotName(s_nCfgSlot));
			s_nCachedSlot = s_nCfgSlot;
		}
	}

	return szPath;
}

void CFeatures_Config::Save()
{
	const char* const szPath = FilePath();

	if (!szPath[0])
		return;

	FILE* pFile = nullptr;

	if (fopen_s(&pFile, szPath, "w") != 0 || !pFile)
		return;

	fprintf(pFile, "# ZenWare.cc config\n");

	for (const Entry_t& e : GetEntries())
	{
		if (e.m_pBool)
			fprintf(pFile, "%s=%i\n", e.m_szKey, *e.m_pBool ? 1 : 0);
		else if (e.m_pInt)
			fprintf(pFile, "%s=%i\n", e.m_szKey, *e.m_pInt);
		else if (e.m_pFloat)
			fprintf(pFile, "%s=%.3f\n", e.m_szKey, *e.m_pFloat);
		else if (e.m_pColor)
			fprintf(pFile, "%s=%u %u %u %u\n", e.m_szKey, (*e.m_pColor)[0], (*e.m_pColor)[1], (*e.m_pColor)[2], (*e.m_pColor)[3]);
	}

	fclose(pFile);
}

void CFeatures_Config::Load()
{
	const char* const szPath = FilePath();

	if (!szPath[0])
		return;

	FILE* pFile = nullptr;

	if (fopen_s(&pFile, szPath, "r") != 0 || !pFile)
		return;

	char szLine[256] = { };

	while (fgets(szLine, sizeof(szLine), pFile))
	{
		char* szValue = strchr(szLine, '=');

		if (!szValue)
			continue;

		*szValue = '\0';
		szValue++;

		for (const Entry_t& e : GetEntries())
		{
			if (_stricmp(szLine, e.m_szKey) != 0)
				continue;

			if (e.m_pBool)
				*e.m_pBool = (atoi(szValue) != 0);
			else if (e.m_pInt)
				*e.m_pInt = atoi(szValue);
			else if (e.m_pFloat)
				*e.m_pFloat = static_cast<float>(atof(szValue));
			else if (e.m_pColor)
			{
				unsigned r = 0, g = 0, b = 0, a = 255;
				if (sscanf_s(szValue, "%u %u %u %u", &r, &g, &b, &a) >= 3)
					e.m_pColor->SetColor(r, g, b, a);
			}

			break;
		}
	}

	//Слайдеры — источник правды для меню: Render каждый кадр затирает float
	//значениями слайдеров, поэтому после загрузки (включая авто-Load при старте)
	//подтягиваем слайдеры из float. +0.5f = округление вместо усечения (5.05 -> 51 -> 5.1).
	Vars::Aimbot::nFOVSlider = U::Math.Clamp((int)(Vars::Aimbot::flFOV * 10.0f + 0.5f), 5, 300);
	Vars::Aimbot::nSmoothSlider = U::Math.Clamp((int)(Vars::Aimbot::flSmoothing + 0.5f), 0, 60);
	Vars::Visuals::nViewFOVSlider = U::Math.Clamp((int)(Vars::Visuals::flViewFOV * 100.0f + 0.5f), 50, 300);
	Vars::Visuals::nVmFOVSlider = U::Math.Clamp((int)(Vars::Visuals::flVmFOV * 100.0f + 0.5f), 50, 300);
	Vars::AimbotWpn::nRifleFovS = U::Math.Clamp((int)(Vars::AimbotWpn::flRifleFov * 10.0f + 0.5f), 5, 300);
	Vars::AimbotWpn::nSmgFovS = U::Math.Clamp((int)(Vars::AimbotWpn::flSmgFov * 10.0f + 0.5f), 5, 300);
	Vars::AimbotWpn::nShotgunFovS = U::Math.Clamp((int)(Vars::AimbotWpn::flShotgunFov * 10.0f + 0.5f), 5, 300);
	Vars::AimbotWpn::nSniperFovS = U::Math.Clamp((int)(Vars::AimbotWpn::flSniperFov * 10.0f + 0.5f), 5, 300);
	Vars::AimbotWpn::nPistolFovS = U::Math.Clamp((int)(Vars::AimbotWpn::flPistolFov * 10.0f + 0.5f), 5, 300);
	//Кривой cfg не должен выводить селекторы за диапазон: меню прикрыто Clamp, логика — нет.
	Vars::BunnyHop::nBhopStyle = U::Math.Clamp(Vars::BunnyHop::nBhopStyle, 0, 1);
	Vars::BunnyHop::nAutoStrafeMode = U::Math.Clamp(Vars::BunnyHop::nAutoStrafeMode, 0, 3);
	Vars::Chams::nPalette = U::Math.Clamp(Vars::Chams::nPalette, 0, 4);
	Vars::AimbotWpn::nGroup = U::Math.Clamp(Vars::AimbotWpn::nGroup, 0, 4);
	Vars::Aimbot::nTargetPriority = U::Math.Clamp(Vars::Aimbot::nTargetPriority, 0, 1);
	Vars::Aimbot::nHitbox = U::Math.Clamp(Vars::Aimbot::nHitbox, 0, 1);
	Vars::Hitmarker::nPitch = U::Math.Clamp(Vars::Hitmarker::nPitch, 200, 2000);
	Vars::Hitmarker::nDurationMs = U::Math.Clamp(Vars::Hitmarker::nDurationMs, 400, 3000);
	Vars::Hitmarker::nMinDmg = U::Math.Clamp(Vars::Hitmarker::nMinDmg, 0, 100);
	Vars::Visuals::nCrosshairSize = U::Math.Clamp(Vars::Visuals::nCrosshairSize, 2, 40);
	Vars::Visuals::nThirdPersonDist = U::Math.Clamp(Vars::Visuals::nThirdPersonDist, 30, 200);
	Vars::BunnyHop::nJumpDelayTicks = U::Math.Clamp(Vars::BunnyHop::nJumpDelayTicks, 0, 20);
	Vars::Visuals::nEspMaxDistS = U::Math.Clamp((int)(Vars::Visuals::flEspMaxDist + 0.5f), 0, 200);
	//Кривой cfg = fail-open без этих клампов: мусор жил бы до первого
	//открытия меню (float правится только из Render).
	Vars::Menu::nLang = U::Math.Clamp(Vars::Menu::nLang, 0, 7);
	Vars::Menu::nKey = U::Math.Clamp(Vars::Menu::nKey, 0, 254);
	Vars::Aimbot::nKey = U::Math.Clamp(Vars::Aimbot::nKey, 0, 254);
	Vars::TriggerBot::nKey = U::Math.Clamp(Vars::TriggerBot::nKey, 0, 254);
	Vars::ESP::nPanicKey = U::Math.Clamp(Vars::ESP::nPanicKey, 0, 254);
	Vars::Aimbot::flFOV = U::Math.Clamp(Vars::Aimbot::flFOV, 0.5f, 30.0f);
	Vars::Aimbot::flSmoothing = U::Math.Clamp(Vars::Aimbot::flSmoothing, 0.0f, 60.0f);
	Vars::Visuals::flViewFOV = U::Math.Clamp(Vars::Visuals::flViewFOV, 0.5f, 3.0f);
	Vars::Visuals::flVmFOV = U::Math.Clamp(Vars::Visuals::flVmFOV, 0.5f, 3.0f);
	Vars::Visuals::flEspMaxDist = U::Math.Clamp(Vars::Visuals::flEspMaxDist, 0.0f, 200.0f);
	Vars::AimbotWpn::flRifleFov = U::Math.Clamp(Vars::AimbotWpn::flRifleFov, 0.5f, 30.0f);
	Vars::AimbotWpn::flSmgFov = U::Math.Clamp(Vars::AimbotWpn::flSmgFov, 0.5f, 30.0f);
	Vars::AimbotWpn::flShotgunFov = U::Math.Clamp(Vars::AimbotWpn::flShotgunFov, 0.5f, 30.0f);
	Vars::AimbotWpn::flSniperFov = U::Math.Clamp(Vars::AimbotWpn::flSniperFov, 0.5f, 30.0f);
	Vars::AimbotWpn::flPistolFov = U::Math.Clamp(Vars::AimbotWpn::flPistolFov, 0.5f, 30.0f);
	Vars::AimbotWpn::nRifleSmooth = U::Math.Clamp(Vars::AimbotWpn::nRifleSmooth, 0, 60);
	Vars::AimbotWpn::nSmgSmooth = U::Math.Clamp(Vars::AimbotWpn::nSmgSmooth, 0, 60);
	Vars::AimbotWpn::nShotgunSmooth = U::Math.Clamp(Vars::AimbotWpn::nShotgunSmooth, 0, 60);
	Vars::AimbotWpn::nSniperSmooth = U::Math.Clamp(Vars::AimbotWpn::nSniperSmooth, 0, 60);
	Vars::AimbotWpn::nPistolSmooth = U::Math.Clamp(Vars::AimbotWpn::nPistolSmooth, 0, 60);
	Vars::AimbotWpn::nRifleHitbox = U::Math.Clamp(Vars::AimbotWpn::nRifleHitbox, 0, 1);
	Vars::AimbotWpn::nSmgHitbox = U::Math.Clamp(Vars::AimbotWpn::nSmgHitbox, 0, 1);
	Vars::AimbotWpn::nShotgunHitbox = U::Math.Clamp(Vars::AimbotWpn::nShotgunHitbox, 0, 1);
	Vars::AimbotWpn::nSniperHitbox = U::Math.Clamp(Vars::AimbotWpn::nSniperHitbox, 0, 1);
	Vars::AimbotWpn::nPistolHitbox = U::Math.Clamp(Vars::AimbotWpn::nPistolHitbox, 0, 1);

	fclose(pFile);
}
