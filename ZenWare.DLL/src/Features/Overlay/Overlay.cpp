#include "Overlay.h"

#include <Windows.h>

#include "../Vars.h"
#include "../../Util/Logger/Logger.h"

namespace
{
	constexpr wchar_t kWndClass[] = L"ZenWareOverlayClass";
	constexpr int kTickMs = 1;         // loop pace; the rate is measured, not assumed
	constexpr int kLogEveryMs = 5000;

	HANDLE g_hThread = nullptr;
	DWORD g_dwThreadId = 0;
	volatile LONG g_lRun = 0;

	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
	{
		switch (msg)
		{
		case WM_ERASEBKGND:
			return 1;              // fully painted in WM_PAINT
		case WM_NCHITTEST:
			return HTTRANSPARENT;  // never steal input from the game
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		default:
			break;
		}

		return DefWindowProcW(hwnd, msg, wp, lp);
	}

	// Draws the whole surface into a memory DC and blits it in one go: no flicker, and
	// the game's compositor only sees finished frames.
	void Paint(HWND hwnd, HFONT font, int nFps, const wchar_t* wszText)
	{
		RECT rc = { };
		GetClientRect(hwnd, &rc);

		const int nW = rc.right - rc.left;
		const int nH = rc.bottom - rc.top;

		HDC dcWindow = GetDC(hwnd);

		if (!dcWindow)
			return;

		HDC dcMem = CreateCompatibleDC(dcWindow);
		HBITMAP bmp = CreateCompatibleBitmap(dcWindow, nW, nH);
		HGDIOBJ oldBmp = nullptr;

		if (dcMem && bmp)
		{
			oldBmp = SelectObject(dcMem, bmp);

			HBRUSH bg = CreateSolidBrush(RGB(8, 12, 10));
			RECT rcAll = { 0, 0, nW, nH };
			FillRect(dcMem, &rcAll, bg);
			DeleteObject(bg);

			HPEN pen = CreatePen(PS_SOLID, 1, RGB(61, 184, 143));
			HGDIOBJ oldPen = SelectObject(dcMem, pen);
			HGDIOBJ oldBrush = SelectObject(dcMem, GetStockObject(NULL_BRUSH));
			Rectangle(dcMem, 0, 0, nW, nH);
			SelectObject(dcMem, oldBrush);
			SelectObject(dcMem, oldPen);
			DeleteObject(pen);

			SetBkMode(dcMem, TRANSPARENT);
			SetTextColor(dcMem, RGB(232, 255, 245));
			HGDIOBJ oldFont = SelectObject(dcMem, font);
			TextOutW(dcMem, 8, (nH - 15) / 2, wszText, (int)wcslen(wszText));
			SelectObject(dcMem, oldFont);

			BitBlt(dcWindow, 0, 0, nW, nH, dcMem, 0, 0, SRCCOPY);

			SelectObject(dcMem, oldBmp);
		}

		if (bmp)
			DeleteObject(bmp);

		if (dcMem)
			DeleteDC(dcMem);

		ReleaseDC(hwnd, dcWindow);

		(void)nFps;
	}

	DWORD WINAPI ThreadProc(LPVOID)
	{
		WNDCLASSEXW wc = { };
		wc.cbSize = sizeof(wc);
		wc.lpfnWndProc = WndProc;
		wc.hInstance = GetModuleHandleW(nullptr);
		wc.lpszClassName = kWndClass;

		if (!RegisterClassExW(&wc))
		{
			U::Log.Write("Overlay: RegisterClassExW failed (%lu)", GetLastError());
			return 0;
		}

		const DWORD dwExStyle = WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT
			| WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;

		const int nW = 236;
		const int nH = 26;

		HWND hwnd = CreateWindowExW(dwExStyle, kWndClass, L"ZenWare overlay", WS_POPUP,
			16, 16, nW, nH, nullptr, nullptr, wc.hInstance, nullptr);

		if (!hwnd)
		{
			U::Log.Write("Overlay: CreateWindowExW failed (%lu)", GetLastError());
			UnregisterClassW(kWndClass, wc.hInstance);
			return 0;
		}

		SetLayeredWindowAttributes(hwnd, 0, 214, LWA_ALPHA);
		ShowWindow(hwnd, SW_SHOWNOACTIVATE);

		HFONT font = CreateFontW(-13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
			DEFAULT_PITCH, L"Segoe UI");

		// Own clock: the frame rate below is measured from this loop, never from the
		// game's frame counter, so a paused game cannot fake it.
		ULONGLONG ullFpsStart = GetTickCount64();
		ULONGLONG ullLogStart = ullFpsStart;
		int nFrames = 0;
		int nFps = 0;

		while (InterlockedCompareExchange(&g_lRun, 1, 1) == 1)
		{
			MSG msg;

			while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}

			const ULONGLONG ullNow = GetTickCount64();

			if (ullNow - ullFpsStart >= 500)
			{
				nFps = (int)(nFrames * 1000ULL / (ullNow - ullFpsStart));
				nFrames = 0;
				ullFpsStart = ullNow;
			}

			if (ullNow - ullLogStart >= kLogEveryMs)
			{
				U::Log.Write("Overlay: own clock %d fps (game render loop not involved)", nFps);
				ullLogStart = ullNow;
			}

			wchar_t wszText[96] = { };
			swprintf_s(wszText, L"ZenWare.cc %S   |   overlay %d fps", Vars::Menu::kVersion, nFps);

			Paint(hwnd, font, nFps, wszText);

			nFrames++;
			Sleep(kTickMs);
		}

		if (font)
			DeleteObject(font);

		DestroyWindow(hwnd);
		UnregisterClassW(kWndClass, wc.hInstance);

		return 0;
	}
}

bool Overlay::Init()
{
	if (!Vars::Menu::bOverlayWatermark)
	{
		U::Log.Write("Overlay: disabled by config");
		return false;
	}

	if (InterlockedCompareExchange(&g_lRun, 1, 0) != 0)
		return true; // already running

	g_hThread = CreateThread(nullptr, 0, ThreadProc, nullptr, 0, &g_dwThreadId);

	if (!g_hThread)
	{
		InterlockedExchange(&g_lRun, 0);
		U::Log.Write("Overlay: CreateThread failed (%lu)", GetLastError());
		return false;
	}

	U::Log.Write("Overlay: window thread started (id %lu)", g_dwThreadId);
	return true;
}

void Overlay::Shutdown()
{
	if (InterlockedExchange(&g_lRun, 0) == 0)
		return;

	if (g_hThread)
	{
		WaitForSingleObject(g_hThread, 2000);
		CloseHandle(g_hThread);
		g_hThread = nullptr;
	}

	U::Log.Write("Overlay: window thread stopped");
}
