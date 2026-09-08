#pragma once

#include "../SDK/SDK.h"

// Без windows.h (ломает byte в SDK-заголовках): только нужное из kernel32.
extern "C" __declspec(dllimport) unsigned short __stdcall GetUserDefaultUILanguage(void);

namespace Vars
{
	namespace Aimbot
	{
		inline bool bEnabled = false;
		inline bool bSilent = true;
		inline bool bAutoShoot = false;
		inline bool bVisibleOnly = true;
		inline bool bIgnoreIncapped = false;
		inline int nTargetPriority = 0; //0 = FOV, 1 = Distance
		inline int nHitbox = 0; //0 = Head, 1 = Center
		inline float flFOV = 5.0f;
		inline int nFOVSlider = 50; //x10 proxy for menu
		inline float flSmoothing = 0.0f;
		inline int nSmoothSlider = 0;
		inline int nKey = 0; // 0 = always on
		inline bool bTargetCommons = true; //also lock common infected + witch
		inline bool bTargetSpecials = true; //also lock hunters/smokers/.../tank
	}

	// Per-weapon overrides: FOV/smoothing/hitbox per gun group.
	// Groups: 0 rifles, 1 smg, 2 shotguns, 3 snipers, 4 pistols.
	namespace AimbotWpn
	{
		inline bool bEnabled = false;
		inline int nGroup = 0; // menu selector
		inline float flRifleFov = 5.0f;   inline int nRifleFovS = 50;
		inline int nRifleSmooth = 0;      inline int nRifleHitbox = 0;
		inline float flSmgFov = 6.0f;     inline int nSmgFovS = 60;
		inline int nSmgSmooth = 0;        inline int nSmgHitbox = 1;
		inline float flShotgunFov = 4.0f; inline int nShotgunFovS = 40;
		inline int nShotgunSmooth = 0;    inline int nShotgunHitbox = 1;
		inline float flSniperFov = 8.0f;  inline int nSniperFovS = 80;
		inline int nSniperSmooth = 0;     inline int nSniperHitbox = 0;
		inline float flPistolFov = 5.0f;  inline int nPistolFovS = 50;
		inline int nPistolSmooth = 0;     inline int nPistolHitbox = 0;
	}

	namespace TriggerBot
	{
		inline bool bEnabled = false;
		inline bool bVisibleOnly = true;
		inline int nKey = 0; //0 = always while crosshair on target
	}

	namespace AutoShove
	{
		inline bool bEnabled = false;
	}

	namespace AutoPistol
	{
		inline bool bEnabled = false;
	}

	namespace BunnyHop
	{
		inline bool bEnabled = false;          // bunny hop master switch
		inline int nBhopStyle = 0;             // 0 perfect (force every tick), 1 legit (own keypress only)
		inline bool bAutoStrafe = false;
		inline int nAutoStrafeMode = 0;        // 0 legit mousedx, 1 rage circle, 2 w-only, 3 directional
		inline bool bEdgeJump = false;         // jump at ledge edge
		inline bool bEdgeBug = false;          // duck at landing to keep speed
		inline bool bJumpBug = false;          // duck-tap to negate fall landing
		inline bool bNullMove = false;         // cancel opposite keys (A+D, W+S)
		inline bool bLongJumpHelper = false;   // LJ prestrafe helper
		inline bool bAutoDuck = false;         // hold duck while airborne
		inline int nJbShowTick = 0;            // jumpbug notify timestamp (not saved)
		inline int nEbShowTick = 0;            // edgebug notify timestamp (not saved)
		inline int nEjShowTick = 0;            // edgejump notify timestamp (not saved)
		inline bool bFastStop = false;         // counter-strafe to stop instantly
		inline bool bSpeedHUD = false;         // velocity display
		inline bool bJumpStats = false;        // KZ-style jump statistics panel
		inline bool bPrestrafe = false;        // +30% ground prestrafe
		inline int nJumpDelayTicks = 0;        // minimal delay between jumps, in ticks
	}

	namespace Chams
	{
		inline bool bEnabled = false;
		inline bool bThroughWalls = true;
		inline int nPalette = 0; //index into the preset table in Chams.cpp

		//Filled from the palette by Chams.cpp at draw time.
		inline Color clrEnemy = { 150, 15, 15, 255 };
		inline Color clrAlly = { 15, 150, 150, 255 };
		inline Color clrTank = { 150, 100, 15, 255 };
	}

	namespace ESP
	{
		inline bool bEnabled = false;
		inline bool bBox = true;
		inline bool bHealthBar = true;
		inline bool bName = true;
		inline bool bDistance = true;
		inline bool bItems = true;      //ground weapons / meds / throwables
		inline bool bCommon = true;    //common infected boxes
		inline bool bSnaplines = false; //line from screen bottom to each box
		inline bool bFilled = false;    //translucent fill inside boxes
		inline bool bHealthText = false; //HP number next to health bar
		inline bool bWeaponText = true;  //active weapon name under nickname
		inline bool bSpecialBoxes = true; //SI boxes/names
		inline bool bBossBoxes = true;   //witch box
		inline bool bTeamPanel = false;  //team HP panel (left side)
		inline bool bThrowTimers = true; //thrown grenade timers (pipe/molotov/bile)
		inline int nPanicKey = 0;        //quick ESP on/off, 0 = off
	}

	namespace Visuals
	{
		inline bool bNoFog = false;
		inline float flViewFOV = 1.0f;
		inline int nViewFOVSlider = 100; //x100 proxy
		inline bool bCrosshair = false;
		inline int nCrosshairSize = 6;
		inline Color clrCrosshair = { 0, 255, 171, 255 };
		inline bool bOverlay = false;   //FPS + position overlay
		inline bool bThirdPerson = false; //3rd person camera (local server)
		inline int nThirdPersonDist = 100; //cam_idealdist
	}

	namespace Radar
	{
		inline bool bEnabled = true;    // 2D-радар сверху слева
		inline bool bSpectators = true; // список наблюдателей под радаром
	}

	namespace Alerts
	{
		inline bool bEnabled = true; // баннеры угроз
		inline bool bTank = true;    // танк + дистанция
		inline bool bWitch = true;   // ведьма + дистанция
		inline bool bSIList = true;  // список особых рядом (слева)
		inline bool bPinned = true;  // тебя взяли в пин (хантер/жокей/...)
		inline bool bRevive = true;  // союзник в инкапе — реанимируй
		inline bool bTankHp = true;  // полоса HP танка под баннером
	}

	namespace Menu
	{
		inline bool bOpen = false;
		inline int nKey = VK_INSERT;
		inline Color clrAccent = { 0, 255, 171, 255 };
		// RU/EN всего меню: системный язык по умолчанию (до Config.Load),
		// хранится в конфиге, переключается кнопкой и F7.
		inline bool bRussian = ((GetUserDefaultUILanguage() & 0x3FF) == 0x19);
	}

	namespace NoSpread
	{
		inline bool bEnabled = true;
	}

	namespace Killfeed
	{
		inline bool bEnabled = true;
	}

	namespace Hitmarker
	{
		inline bool bEnabled = false;
		inline bool bSound = true;
		inline bool bNumbers = true;
		inline bool bXMark = true;     // крест на прицеле в момент попадания
		inline bool bStats = false;    // панель статистики сессии
		inline bool bDmgFlash = true;  // красная вспышка при уроне по тебе
		inline int nPitch = 1000;      // Hz, menu 200..2000
		inline int nDurationMs = 1200; // ms, menu 400..3000
		inline int nMinDmg = 0;        // скрывать цифры урона меньше N, 0 = все
	}

	namespace VisualRecoil
	{
		inline bool bEnabled = false;
	}

	namespace Grenade
	{
		inline bool bEnabled = false; // trajectory preview for held throwables
		inline bool bLanding = true;  // landing marker circle
	}
}
