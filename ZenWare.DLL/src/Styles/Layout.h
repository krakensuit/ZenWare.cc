#pragma once

// Layout grid for the in-game menu (stage 2 of the rework).
//
// Everything is expressed in 8 px units so that stages 3-4 can drop the magic
// pixel numbers from the widgets. The panel targets a roomier 400x560; the
// active code switches to it in stage 3, until then Menu.cpp keeps its own
// constants and this header documents the target.

namespace Layout
{
	constexpr int kGrid = 8;

	// Units -> pixels.
	constexpr int G(const int nUnits) { return nUnits * kGrid; }

	// Target panel geometry (stage 3 turns these on).
	constexpr int kPanelW  = G(50);   // 400
	constexpr int kPanelH  = G(70);   // 560
	constexpr int kHeaderH = G(6);    //  48
	constexpr int kTabsH   = G(5);    //  40
	constexpr int kFooterH = G(3);    //  24
	constexpr int kPadding = G(2);    //  16
	constexpr int kRowH    = G(3) + 4; //  28
	constexpr int kDividerH = 1;

	// HiDPI hook: every pixel constant goes through here once nPanelScale lands.
	inline int Scale(const int nValue, const float flScale)
	{
		return static_cast<int>(static_cast<float>(nValue) * flScale + 0.5f);
	}

	// Clamp a window position so the panel cannot be dragged off screen.
	inline int ClampX(const int nX, const int nPanelW, const int nScreenW)
	{
		if (nX + nPanelW > nScreenW)
			return nScreenW - nPanelW;

		return nX < 0 ? 0 : nX;
	}

	inline int ClampY(const int nY, const int nPanelH, const int nScreenH)
	{
		if (nY + nPanelH > nScreenH)
			return nScreenH - nPanelH;

		return nY < 0 ? 0 : nY;
	}
}
