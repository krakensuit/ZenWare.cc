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

			//Trigger / shove / pistol
			{ "trigger.enabled", &Vars::TriggerBot::bEnabled },
			{ "trigger.visible", &Vars::TriggerBot::bVisibleOnly },
			{ "autoshove.enabled", &Vars::AutoShove::bEnabled },
			{ "autopistol.enabled", &Vars::AutoPistol::bEnabled },
			{ "nospread.enabled", &Vars::NoSpread::bEnabled },
			{ "killfeed.enabled", &Vars::Killfeed::bEnabled },

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
			{ "chams.ally", &Vars::Chams::clrAlly },

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
		{ "esp.specialboxes", &Vars::ESP::bSpecialBoxes },
		{ "esp.bossboxes", &Vars::ESP::bBossBoxes },

		//Visuals
		{ "visuals.nofog", &Vars::Visuals::bNoFog },
		{ "visuals.viewfov", &Vars::Visuals::flViewFOV },
		{ "visuals.crosshair", &Vars::Visuals::bCrosshair },
		{ "visuals.chsize", &Vars::Visuals::nCrosshairSize },
		{ "visuals.overlay", &Vars::Visuals::bOverlay },
		{ "visuals.chcolor", &Vars::Visuals::clrCrosshair },
		{ "visuals.3rdperson", &Vars::Visuals::bThirdPerson },
		{ "visuals.3rdpersondist", &Vars::Visuals::nThirdPersonDist },

			//Recoil
			{ "norecoil.visual", &Vars::VisualRecoil::bEnabled },

		//Menu style
		{ "menu.accent", &Vars::Menu::clrAccent },
		{ "menu.key", &Vars::Menu::nKey },
		{ "menu.russian", &Vars::Menu::bRussian },
		};
	}
}

const char* CFeatures_Config::FilePath()
{
	static char szPath[MAX_PATH] = { };

	if (!szPath[0])
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
			sprintf_s(szPath, "%s\\ZenWare.cfg", szGameDir);
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

	fclose(pFile);
}
