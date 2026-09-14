// ZenWare Loader - Liquid Glass (реализация)
// WinAPI + GDI. См. Glass.h.

#include "Glass.h"
#include <dwmapi.h>

#include <math.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "msimg32.lib")
// Подготовка к бэкдропу через DirectComposition (следующий шаг): библиотеки уже подключены.
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dcomp.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

namespace
{
	// ----------------- undocumented: SetWindowCompositionAttribute -----------------
	struct ACCENT_POLICY
	{
		DWORD nAccentState;
		DWORD nFlags;
		DWORD nColor;      // 0xAABBGGRR
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
	bool  g_bGlass = false;         // блюр реально включён
	DWORD g_nAccentState = 0;       // 4 = acrylic, 3 = blur
	COLORREF g_tint = Glass::kMint;
	BYTE  g_alpha = Glass::kMintAlpha;

	// Кэш GDI: 1x1 DIB для полупрозрачных заливок и радиальный DIB для пятна.
	HDC     g_hMemDC = nullptr;
	HBITMAP g_hSolid = nullptr;
	PVOID   g_pSolidBits = nullptr;
	HDC     g_hGlowDC = nullptr;
	HBITMAP g_hGlow = nullptr;
	PVOID   g_pGlowBits = nullptr;
	const int kGlowSize = 256;

	void EnsureProbe()
	{
		if (g_bProbed)
			return;

		g_bProbed = true;

		const HMODULE hUser = GetModuleHandleW(L"user32.dll");

		if (hUser)
			g_pfnSetWCA = reinterpret_cast<PFN_SetWindowCompositionAttribute>(
				GetProcAddress(hUser, "SetWindowCompositionAttribute"));
	}

	// Полупрозрачная заливка прямоугольника (GDI без per-pixel alpha).
	void BlendSolid(HDC hdc, int x, int y, int w, int h, COLORREF color, BYTE alpha)
	{
		if (w <= 0 || h <= 0 || alpha == 0)
			return;

		if (!g_hMemDC)
		{
			BITMAPINFO bmi{};
			bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth = 1;
			bmi.bmiHeader.biHeight = 1;
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 32;
			bmi.bmiHeader.biCompression = BI_RGB;

			g_hMemDC = CreateCompatibleDC(nullptr);
			g_hSolid = CreateDIBSection(g_hMemDC, &bmi, DIB_RGB_COLORS, &g_pSolidBits, nullptr, 0);

			if (g_hSolid)
				SelectObject(g_hMemDC, g_hSolid);

			if (g_pSolidBits)
			{
				// DIB хранит BGRA; источник 1x1 будет растянут AlphaBlend'ом.
				DWORD* px = static_cast<DWORD*>(g_pSolidBits);
				*px = (static_cast<DWORD>(GetBValue(color)) << 16) |
					(static_cast<DWORD>(GetGValue(color)) << 8) |
					static_cast<DWORD>(GetRValue(color));
			}
		}

		if (!g_hMemDC || !g_hSolid)
			return;

		BLENDFUNCTION bf{};
		bf.BlendOp = AC_SRC_OVER;
		bf.SourceConstantAlpha = alpha;
		bf.AlphaFormat = 0;

		AlphaBlend(hdc, x, y, w, h, g_hMemDC, 0, 0, 1, 1, bf);
	}

	// Радиальный «мягкий» источник света для пятна под курсором.
	void BuildGlow()
	{
		if (g_hGlow)
			return;

		BITMAPINFO bmi{};
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi.bmiHeader.biWidth = kGlowSize;
		bmi.bmiHeader.biHeight = -kGlowSize; // top-down
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;

		g_hGlowDC = CreateCompatibleDC(nullptr);
		g_hGlow = CreateDIBSection(g_hGlowDC, &bmi, DIB_RGB_COLORS, &g_pGlowBits, nullptr, 0);

		if (!g_hGlow || !g_pGlowBits)
			return;

		SelectObject(g_hGlowDC, g_hGlow);

		const float c = (kGlowSize - 1) * 0.5f;
		BYTE* base = static_cast<BYTE*>(g_pGlowBits);

		for (int y = 0; y < kGlowSize; ++y)
		{
			for (int x = 0; x < kGlowSize; ++x)
			{
				const float dx = (x - c) / c;
				const float dy = (y - c) / c;
				float d = sqrtf(dx * dx + dy * dy);

				if (d > 1.0f)
					d = 1.0f;

				const float fall = (1.0f - d) * (1.0f - d);
				const BYTE a = static_cast<BYTE>(fall * 255.0f);

				BYTE* px = base + ((static_cast<size_t>(y) * kGlowSize + x) * 4);
				px[0] = 255;      // B
				px[1] = 255;      // G
				px[2] = 255;      // R
				px[3] = a;        // A
			}
		}
	}
}

namespace Glass
{
	// -------------------------- системный материал Win11 --------------------------
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

	bool EnableSystemBackdrop(HWND hwnd)
	{
		if (!hwnd || OsBuild() < 22000)
			return false;

		// DWMWA_SYSTEMBACKDROP_TYPE = 38, DWMSBT_MAINWINDOW = 2 (Win11 22H2+).
		INT material = 2;

		if (FAILED(DwmSetWindowAttribute(hwnd, 38, &material, sizeof(material))))
			return false;

		g_bGlass = true;
		g_nAccentState = 0xFFFF; // материал, а не акрил: тинт не нужен
		return true;
	}
	bool IsAvailable()
	{
		EnsureProbe();
		return g_pfnSetWCA != nullptr;
	}

	bool Enable(HWND hwnd, COLORREF tint, BYTE alpha)
	{
		EnsureProbe();

		if (!g_pfnSetWCA || !hwnd)
			return false;

		g_tint = tint;
		g_alpha = alpha;

		// Сначала акрил (Win10 1803+), затем обычный блюр.
		const DWORD states[2] = { ACCENT_ENABLE_ACRYLICBLURBEHIND, ACCENT_ENABLE_BLURBEHIND };

		for (int i = 0; i < 2; ++i)
		{
			ACCENT_POLICY policy{};
			policy.nAccentState = states[i];
			policy.nFlags = ACCENT_FLAG_DRAW_ALL_BORDERS;
			policy.nColor = (static_cast<DWORD>(alpha) << 24) |
				(static_cast<DWORD>(GetBValue(tint)) << 16) |
				(static_cast<DWORD>(GetGValue(tint)) << 8) |
				static_cast<DWORD>(GetRValue(tint));

			WINDOWCOMPOSITIONATTRIBDATA data{};
			data.nAttribute = WCA_ACCENT_POLICY;
			data.pvData = &policy;
			data.cbData = sizeof(policy);

			if (g_pfnSetWCA(hwnd, &data))
			{
				g_nAccentState = states[i];
				g_bGlass = true;
				return true;
			}
		}

		g_bGlass = false;
		return false;
	}

	void SetAlpha(HWND hwnd, BYTE alpha)
	{
		if (!g_pfnSetWCA || !hwnd || !g_bGlass)
			return;

		g_alpha = alpha;

		ACCENT_POLICY policy{};
		policy.nAccentState = g_nAccentState;
		policy.nFlags = ACCENT_FLAG_DRAW_ALL_BORDERS;
		policy.nColor = (static_cast<DWORD>(alpha) << 24) |
			(static_cast<DWORD>(GetBValue(g_tint)) << 16) |
			(static_cast<DWORD>(GetGValue(g_tint)) << 8) |
			static_cast<DWORD>(GetRValue(g_tint));

		WINDOWCOMPOSITIONATTRIBDATA data{};
		data.nAttribute = WCA_ACCENT_POLICY;
		data.pvData = &policy;
		data.cbData = sizeof(policy);

		g_pfnSetWCA(hwnd, &data);
	}

	void Disable(HWND hwnd)
	{
		if (!g_pfnSetWCA || !hwnd)
			return;

		ACCENT_POLICY policy{};
		policy.nAccentState = 0; // ACCENT_DISABLED

		WINDOWCOMPOSITIONATTRIBDATA data{};
		data.nAttribute = WCA_ACCENT_POLICY;
		data.pvData = &policy;
		data.cbData = sizeof(policy);

		g_pfnSetWCA(hwnd, &data);
		g_bGlass = false;
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
			SetWindowRgn(hwnd, rgn, TRUE); // владение регионом переходит окну
	}

	void PaintGlass(HDC hdc, const RECT& rc, POINT mouse)
	{
		if (!hdc)
			return;

		const int w = rc.right - rc.left;
		const int h = rc.bottom - rc.top;

		if (w <= 2 || h <= 2)
			return;

		const ULONGLONG ms = GetTickCount64();
		const float t = static_cast<float>(ms % 100000) / 1000.0f;

		// 1) Анимированный отблеск: диагональная полоса, идущая справа налево.
		{
			const int band = w / 5;
			if (band > 8)
			{
				const float period = 4.2f;
				const float phase = fmodf(t, period) / period;
				const int cx = static_cast<int>((w + band) - phase * (w + band * 2));

				for (int i = 0; i < band; i += 2)
				{
					const int x = cx + i;
					if (x < 0 || x >= w)
						continue;

					const float k = 1.0f - fabsf((i - band * 0.5f) / (band * 0.5f));
					const BYTE a = static_cast<BYTE>(k * k * 26.0f);

					BlendSolid(hdc, x, 0, 2, h, RGB(255, 255, 255), a);
				}
			}
		}

		// 2) Мягкое пятно под курсором (реакция на мышь).
		{
			BuildGlow();

			if (g_hGlow && g_hGlowDC)
			{
				const int size = 260;
				const int gx = mouse.x - size / 2;
				const int gy = mouse.y - size / 2;

				if (mouse.x > -size && mouse.y > -size && mouse.x < w + size && mouse.y < h + size)
				{
					BLENDFUNCTION bf{};
					bf.BlendOp = AC_SRC_OVER;
					bf.SourceConstantAlpha = 38;
					bf.AlphaFormat = AC_SRC_ALPHA;

					AlphaBlend(hdc, gx, gy, size, size, g_hGlowDC, 0, 0, kGlowSize, kGlowSize, bf);
				}
			}
		}

		// 3) Блик по краям окна: светлая кромка сверху/слева, тёмная снизу/справа.
		BlendSolid(hdc, 0, 0, w, 1, RGB(255, 255, 255), 34);
		BlendSolid(hdc, 0, 0, 1, h, RGB(255, 255, 255), 26);
		BlendSolid(hdc, 0, h - 1, w, 1, RGB(0, 0, 0), 30);
		BlendSolid(hdc, w - 1, 0, 1, h, RGB(0, 0, 0), 22);
	}

	void Shutdown()
	{
		if (g_hGlow)
		{
			DeleteObject(g_hGlow);
			g_hGlow = nullptr;
			g_pGlowBits = nullptr;
		}

		if (g_hGlowDC)
		{
			DeleteDC(g_hGlowDC);
			g_hGlowDC = nullptr;
		}

		if (g_hSolid)
		{
			DeleteObject(g_hSolid);
			g_hSolid = nullptr;
			g_pSolidBits = nullptr;
		}

		if (g_hMemDC)
		{
			DeleteDC(g_hMemDC);
			g_hMemDC = nullptr;
		}
	}
}
