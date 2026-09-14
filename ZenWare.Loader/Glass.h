#pragma once

// ZenWare Loader - system window backdrop (frosted glass).
// Implemented per the spec: Win11 22H2+ -> Acrylic (DWMSBT_TRANSIENTWINDOW),
// Win10 1803+ -> SetWindowCompositionAttribute + ACCENT_ENABLE_ACRYLICBLURBEHIND,
// Win10 < 1803 -> ACCENT_ENABLE_BLURBEHIND. If nothing applies we return
// false and the loader draws its usual dark background.

#include <windows.h>

namespace Glass
{
	// Mint accent (matches the Zen2D palette).
	constexpr COLORREF kMint = RGB(0x6E, 0xE7, 0xB7);
	constexpr BYTE     kMintAlpha = 0xCC;
	constexpr int      kCornerRadius = 10;

	// Windows build number via RtlGetVersion (ntdll). GetVersionEx lies under a manifest.
	DWORD OsBuild();

	// Main entry: enables the backdrop and extends the DWM frame into the client area.
	// true = the backdrop was really applied (Win11 Acrylic or Win10 acrylic/blur).
	bool EnableSystemBackdrop(HWND hwnd);

	// Fallback compatibility with the loader's previous call.
	bool Enable(HWND hwnd, COLORREF tint = kMint, BYTE alpha = 0);

	// true if the backdrop is applied (the window must NOT be layered).
	bool IsAvailable();

	// Tint opacity (for fade-in; a no-op on the system material).
	void SetAlpha(HWND hwnd, BYTE alpha);

	// Remove the backdrop (before destroying the window).
	void Disable(HWND hwnd);

	// Rounded corners: DWM on Win11, window region on Win10.
	void RoundCorners(HWND hwnd, int radius = kCornerRadius);

	// Release internal resources.
	void Shutdown();
}
