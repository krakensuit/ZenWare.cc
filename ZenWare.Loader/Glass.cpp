// ZenWare Loader - системный бэкдроп (реализация). См. Glass.h.

#include "Glass.h"

#include <dwmapi.h>   // DwmSetWindowAttribute, DwmExtendFrameIntoClientArea, MARGINS
#include <uxtheme.h>  // MARGINS (на случай старого SDK)

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
// Подготовлено под DirectComposition (следующий шаг).
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dcomp.lib")

namespace
{
	// ------------------- undocumented: SetWindowCompositionAttribute -------------------
	struct ACCENT_POLICY
	{
		DWORD nAccentState;
		DWORD nFlags;
		DWORD nColor;      // 0xAABBGGRR (ABGR!)
		DWORD nAnimationId;
	};

	struct WINDOWCOMPOSITIONATTRIBDATA
	{
		DWORD nAttribute;
		PVOID pvData;
		SIZE_T cbData;
	};

	enum
	{
		WCA_ACCENT_POLICY = 19,
		ACCENT_ENABLE_BLURBEHIND = 3,
		ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
		ACCENT_FLAG_DRAW_ALL_BORDERS = 2,
	};

	typedef BOOL(WINAPI* PFN_SetWindowCompositionAttribute)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);

	PFN_SetWindowCompositionAttribute g_pfnSetWCA = nullptr;
	bool  g_bProbed = false;
	bool  g_bBackdrop = false;      // бэкдроп реально применён
	bool  g_bSystemMaterial = false;// применён системный материал Win11 (тинт не управляется)
	DWORD g_nAccentState = 0;
	COLORREF g_tint = Glass::kMint;
	BYTE  g_alpha = 0;

	void Probe()
	{
		if (g_bProbed)
			return;

		g_bProbed = true;

		const HMODULE hUser = GetModuleHandleW(L"user32.dll");

		if (hUser)
			g_pfnSetWCA = reinterpret_cast<PFN_SetWindowCompositionAttribute>(
				GetProcAddress(hUser, "SetWindowCompositionAttribute"));
	}

	// Применяет accent policy с заданным состоянием и тинтом.
	bool ApplyAccent(HWND hwnd, DWORD state, DWORD abgr)
	{
		ACCENT_POLICY policy{};
		policy.nAccentState = state;
		policy.nFlags = ACCENT_FLAG_DRAW_ALL_BORDERS;
		policy.nColor = abgr;

		WINDOWCOMPOSITIONATTRIBDATA data{};
		data.nAttribute = WCA_ACCENT_POLICY;
		data.pvData = &policy;
		data.cbData = sizeof(policy);

		return g_pfnSetWCA(hwnd, &data) != FALSE;
	}

	// Тинт в ABGR: 0xAABBGGRR. Для RGB 0x0A0E0D получаем B=0D, G=0E, R=0A.
	DWORD TintAbgr(COLORREF rgb, BYTE alpha)
	{
		return (static_cast<DWORD>(alpha) << 24) |
			(static_cast<DWORD>(GetBValue(rgb)) << 16) |
			(static_cast<DWORD>(GetGValue(rgb)) << 8) |
			static_cast<DWORD>(GetRValue(rgb));
	}
}

namespace Glass
{
	DWORD OsBuild()
	{
		// RtlGetVersion не врёт, в отличие от GetVersionEx с манифестом.
		typedef LONG(WINAPI* PFN_RtlGetVersion)(PRTL_OSVERSIONINFOW);

		const HMODULE hNt = GetModuleHandleW(L"ntdll.dll");
		if (!hNt)
			return 0;

		PFN_RtlGetVersion pfn = reinterpret_cast<PFN_RtlGetVersion>(GetProcAddress(hNt, "RtlGetVersion"));
		if (!pfn)
			return 0;

		RTL_OSVERSIONINFOW vi{};
		vi.dwOSVersionInfoSize = sizeof(vi);

		return pfn(&vi) == 0 ? vi.dwBuildNumber : 0;
	}

	bool IsAvailable()
	{
		return g_bBackdrop;
	}

	bool EnableSystemBackdrop(HWND hwnd)
	{
		Probe();

		if (!hwnd || !g_pfnSetWCA)
			return false;

		const DWORD build = OsBuild();

		// Тёмный near-black тинт в ABGR: 0xCC0D0E0A (A=CC, B=0D, G=0E, R=0A).
		const DWORD abgrAcrylic = TintAbgr(RGB(0x0A, 0x0E, 0x0D), 0xCC);
		const DWORD abgrBlur = TintAbgr(RGB(0x0A, 0x0E, 0x0D), 0x99);

		// Windows 11 22H2+: системный Acrylic (полупрозрачнее Mica - лучше для лоадера).
		if (build >= 22621)
		{
			INT backdrop = 3; // DWMSBT_TRANSIENTWINDOW == Acrylic

			if (SUCCEEDED(DwmSetWindowAttribute(hwnd, 38 /*DWMWA_SYSTEMBACKDROP_TYPE*/, &backdrop, sizeof(backdrop))))
			{
				// Рамка DWM должна покрывать всю клиентскую область, иначе материал не виден.
				// РЕШЕНИЕ: ExtendFrameIntoClientArea здесь НЕ вызываем: он отдаёт всю
				// клиентскую область DWM, и при GDI-отрисовке интерфейс пропадает (только бэкдроп).
				// Материал и так виден через альфа-композит кадра (AlphaBlend 224).
				// ExtendFrame намеренно не вызываем (см. пояснение выше).

				g_bBackdrop = true;
				g_bSystemMaterial = true;
				return true;
			}
		}

		// Windows 10 1803+ и Win11 до 22H2: акрил через недокументированный вызов.
		if (build >= 17134)
		{
			if (ApplyAccent(hwnd, ACCENT_ENABLE_ACRYLICBLURBEHIND, abgrAcrylic))
			{
				g_nAccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
				g_tint = RGB(0x0A, 0x0E, 0x0D);
				g_alpha = 0xCC;
				g_bBackdrop = true;
				g_bSystemMaterial = false;
				return true;
			}
		}

		// Windows 10 < 1803: простой блюр.
		if (ApplyAccent(hwnd, ACCENT_ENABLE_BLURBEHIND, abgrBlur))
		{
			g_nAccentState = ACCENT_ENABLE_BLURBEHIND;
			g_tint = RGB(0x0A, 0x0E, 0x0D);
			g_alpha = 0x99;
			g_bBackdrop = true;
			g_bSystemMaterial = false;
			return true;
		}

		g_bBackdrop = false;
		return false;
	}

	bool Enable(HWND hwnd, COLORREF tint, BYTE alpha)
	{
		Probe();

		if (!hwnd || !g_pfnSetWCA)
			return false;

		g_tint = tint;
		g_alpha = alpha;

		if (ApplyAccent(hwnd, ACCENT_ENABLE_ACRYLICBLURBEHIND, TintAbgr(tint, alpha)))
		{
			g_nAccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
			g_bBackdrop = true;
			g_bSystemMaterial = false;
			return true;
		}

		if (ApplyAccent(hwnd, ACCENT_ENABLE_BLURBEHIND, TintAbgr(RGB(0x0A, 0x0E, 0x0D), 0x99)))
		{
			g_nAccentState = ACCENT_ENABLE_BLURBEHIND;
			g_bBackdrop = true;
			g_bSystemMaterial = false;
			return true;
		}

		g_bBackdrop = false;
		return false;
	}

	void SetAlpha(HWND hwnd, BYTE alpha)
	{
		if (!hwnd || !g_bBackdrop || g_bSystemMaterial || !g_pfnSetWCA)
			return;

		g_alpha = alpha;
		ApplyAccent(hwnd, g_nAccentState, TintAbgr(g_tint, alpha));
	}

	void Disable(HWND hwnd)
	{
		if (!hwnd || !g_pfnSetWCA)
			return;

		ApplyAccent(hwnd, 0 /*ACCENT_DISABLED*/, 0);
		g_bBackdrop = false;
		g_bSystemMaterial = false;
	}

	void RoundCorners(HWND hwnd, int radius)
	{
		if (!hwnd)
			return;

		// Win11: DWMWA_WINDOW_CORNER_PREFERENCE = 33, DWMWCP_ROUND = 2.
		INT pref = 2;

		if (SUCCEEDED(DwmSetWindowAttribute(hwnd, 33, &pref, sizeof(pref))))
			return;

		// Win10: регион со скруглением.
		RECT rc{};
		GetWindowRect(hwnd, &rc);

		const int w = rc.right - rc.left;
		const int h = rc.bottom - rc.top;

		if (w <= 0 || h <= 0)
			return;

		HRGN rgn = CreateRoundRectRgn(0, 0, w + 1, h + 1, radius * 2, radius * 2);

		if (rgn)
			SetWindowRgn(hwnd, rgn, TRUE);
	}

	void Shutdown()
	{
		g_bBackdrop = false;
		g_bSystemMaterial = false;
	}
}
