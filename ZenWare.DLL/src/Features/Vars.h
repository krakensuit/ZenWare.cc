#pragma once

#include "../SDK/SDK.h"

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

	namespace Menu
	{
		inline bool bOpen = false;
		inline int nKey = VK_INSERT;
		inline Color clrAccent = { 0, 255, 171, 255 };
	}

	namespace Killfeed
	{
		inline bool bEnabled = true;
	}

	namespace VisualRecoil
	{
		inline bool bEnabled = false;
	}
}
