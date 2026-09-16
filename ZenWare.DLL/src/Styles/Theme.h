#pragma once

// Style tokens for the in-game menu (stage 2 of the rework).
//
// Single source of truth: widgets must not carry raw colours, radii, spacing or
// timings. Stage 3/4 migrate the call sites; this header is the contract.

#include "../../SDK/L4D2/Includes/color.h"

namespace Theme
{
	// ------------------------------------------------------------------ palette
	// Dark only. Near-black base with a muted mint accent: the acid #7FFFD4
	// vibrates on a dark panel and reads as boilerplate neon.
	namespace Clr
	{
		inline const Color bg          = Color(14, 16, 15, 248);
		inline const Color surface     = Color(18, 21, 20, 255);
		inline const Color surfaceHover= Color(24, 28, 26, 255);
		inline const Color header      = Color(18, 21, 20, 255);
		inline const Color footer      = Color(11, 12, 12, 255);
		inline const Color shadow      = Color(0, 0, 0, 80);

		inline const Color border      = Color(30, 42, 37, 255);
		inline const Color borderAccent= Color(61, 184, 143, 255);

		inline const Color textPrimary = Color(232, 255, 245, 255);
		inline const Color textSecondary = Color(122, 148, 138, 255);
		inline const Color textDim     = Color(108, 118, 113, 255);

		inline const Color accent      = Color(110, 231, 183, 255);
		inline const Color accentDim   = Color(61, 184, 143, 255);
		inline const Color accentSoft  = Color(110, 231, 183, 45);

		inline const Color danger      = Color(255, 92, 92, 255);
		inline const Color success     = Color(110, 231, 183, 255);

		inline const Color rowHover    = Color(255, 255, 255, 8);
		inline const Color outline     = Color(40, 48, 44, 255);
		inline const Color outlineSoft = Color(32, 38, 35, 255);
	}

	// ------------------------------------------------------------------ metrics
	namespace Size
	{
		constexpr int radiusSm   = 6;
		constexpr int radiusMd   = 8;
		constexpr int radiusLg   = 12;
		constexpr int radiusXl   = 16;

		constexpr int padXs = 4;
		constexpr int padSm = 8;
		constexpr int padMd = 12;
		constexpr int padLg = 16;
		constexpr int padXl = 24;

		constexpr int fontSm = 11;
		constexpr int fontMd = 13;
		constexpr int fontLg = 14;
		constexpr int fontHeader = 18;
		constexpr int fontTitle  = 22;

		constexpr int rowH      = 28;
		constexpr int rowHSmall = 22;
		constexpr int headerH   = 48;
		constexpr int tabsH     = 36;
		constexpr int footerH   = 24;
		constexpr int toggleW   = 32;
		constexpr int toggleH   = 18;
		constexpr int knobR     = 6;
	}

	// ------------------------------------------------------------------ timings
	namespace Time
	{
		constexpr float hoverMs   = 150.0f;
		constexpr float pressMs   = 80.0f;
		constexpr float tabMs     = 200.0f;
		constexpr float staggerMs = 30.0f;   // per row when a tab opens
		constexpr float springK   = 180.0f;  // spring stiffness
		constexpr float springC   = 20.0f;   // spring damping
	}
}
