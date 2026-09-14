#pragma once

// ZenWare Loader - palette and typography (Direct2D layer).
// FIX: the loader is always dark. The light theme was removed entirely so that
// nobody could trigger it by accident: on the light theme the window was flooded
// with mint and the text became unreadable (verified in a real run).

#include <windows.h>

namespace Zen2D
{
	struct Theme_t
	{
		DWORD background;    // #0A0E0D near-black
		DWORD surface;       // #121815
		DWORD border;        // #1E2A25
		DWORD textPrimary;   // #E8FFF5
		DWORD textSecondary; // #7A948A
		DWORD accent;        // #6EE7B7 calm mint (used to be acid #7FFFD4)
		DWORD accentDim;     // #3DB88F
		DWORD danger;        // #FF5C5C
	};

	inline DWORD Rgb(BYTE r, BYTE g, BYTE b)
	{
		return (static_cast<DWORD>(r) << 16) | (static_cast<DWORD>(g) << 8) | b;
	}

	// The loader's only theme.
	inline Theme_t Dark()
	{
		Theme_t t{};
		t.background    = Rgb(0x0A, 0x0E, 0x0D);
		t.surface       = Rgb(0x12, 0x18, 0x15);
		t.border        = Rgb(0x1E, 0x2A, 0x25);
		t.textPrimary   = Rgb(0xE8, 0xFF, 0xF5);
		t.textSecondary = Rgb(0x7A, 0x94, 0x8A);
		t.accent        = Rgb(0x6E, 0xE7, 0xB7);
		t.accentDim     = Rgb(0x3D, 0xB8, 0x8F);
		t.danger        = Rgb(0xFF, 0x5C, 0x5C);
		return t;
	}

	struct FrameState_t
	{
		float dt = 0.016f;
		float elapsed = 0.0f;
		bool  busy = false;
		float progress = 0.0f;
		bool  external = false;
		float hoverLaunch = 0.0f;
		float hoverInject = 0.0f;
		float modeT = 0.0f;
		POINT cursor = { 0, 0 };
		bool  dark = true;   // unused: the theme is always dark
	};
}
