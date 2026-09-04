#include "Overlay.h"
#include <cstdio>
#include <cstdarg>

LRESULT CALLBACK Overlay::Proc(HWND h, UINT m, WPARAM w, LPARAM l)
{
	if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
	return DefWindowProcW(h, m, w, l);
}

bool Overlay::Create()
{
	WNDCLASSEXW wc = { sizeof(wc) };
	wc.lpfnWndProc = Proc;
	wc.hInstance = GetModuleHandleW(nullptr);
	wc.lpszClassName = L"ZenWareExtOverlay";
	wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
	if (!RegisterClassExW(&wc))
		return false;

	m_hwnd = CreateWindowExW(
		WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
		wc.lpszClassName, L"ZenWare.External",
		WS_POPUP, 0, 0, 800, 600, nullptr, nullptr, wc.hInstance, nullptr);
	if (!m_hwnd)
		return false;

	HDC scr = GetDC(nullptr);
	m_mem = CreateCompatibleDC(scr);
	ReleaseDC(nullptr, scr);

	m_font = CreateFontW(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
	ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
	return true;
}

void Overlay::Destroy()
{
	if (m_mem)
	{
		if (m_oldBmp) { SelectObject(m_mem, m_oldBmp); m_oldBmp = nullptr; }
		if (m_bmp) { DeleteObject(m_bmp); m_bmp = nullptr; }
		if (m_oldFont) { SelectObject(m_mem, m_oldFont); m_oldFont = nullptr; }
		if (m_font) { DeleteObject(m_font); m_font = nullptr; }
		DeleteDC(m_mem);
		m_mem = nullptr;
	}
	if (m_hwnd) { DestroyWindow(m_hwnd); m_hwnd = nullptr; }
}

void Overlay::FollowGame()
{
	HWND game = FindWindowW(L"Valve001", nullptr);
	m_gameOk = (game && IsWindowVisible(game));
	if (!m_gameOk)
		return;
	RECT rc = { };
	GetWindowRect(game, &rc);
	int w = rc.right - rc.left, h = rc.bottom - rc.top;
	if (w != m_w || h != m_h || true)
	{
		// Пересоздаём бэкбуфер под новый размер
		if (m_oldBmp) { SelectObject(m_mem, m_oldBmp); m_oldBmp = nullptr; }
		if (m_bmp) { DeleteObject(m_bmp); m_bmp = nullptr; }
		HDC scr = GetDC(nullptr);
		BITMAPINFO bi = { };
		bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
		bi.bmiHeader.biWidth = w;
		bi.bmiHeader.biHeight = -h;
		bi.bmiHeader.biPlanes = 1;
		bi.bmiHeader.biBitCount = 32;
		bi.bmiHeader.biCompression = BI_RGB;
		void* bits = nullptr;
		m_bmp = CreateDIBSection(scr, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
		ReleaseDC(nullptr, scr);
		if (m_bmp)
		{
			m_oldBmp = (HBITMAP)SelectObject(m_mem, m_bmp);
			m_oldFont = (HFONT)SelectObject(m_mem, m_font);
			SetBkMode(m_mem, TRANSPARENT);
		}
		m_w = w; m_h = h;
		// Позиция оверлея = позиция окна игры (рамка в windowed-режиме входит)
		SetWindowPos(m_hwnd, HWND_TOPMOST, rc.left, rc.top, w, h, SWP_NOACTIVATE | SWP_SHOWWINDOW);
	}
}

void Overlay::BeginFrame()
{
	if (!m_mem || !m_bmp)
		return;
	RECT rc = { 0, 0, m_w, m_h };
	// Ключ прозрачности: этот цвет вырезается в EndFrame через ULW_COLORKEY.
	// Контент таким цветом не рисуем.
	HBRUSH clear = CreateSolidBrush(RGB(1, 2, 3));
	FillRect(m_mem, &rc, clear);
	DeleteObject(clear);
}

void Overlay::Rect(int x, int y, int w, int h, COLORREF c, int thick)
{
	if (!m_mem) return;
	HPEN pen = CreatePen(PS_SOLID, thick, c);
	HGDIOBJ old = SelectObject(m_mem, pen);
	HGDIOBJ oldBr = SelectObject(m_mem, GetStockObject(NULL_BRUSH));
	Rectangle(m_mem, x, y, x + w, y + h);
	SelectObject(m_mem, oldBr);
	SelectObject(m_mem, old);
	DeleteObject(pen);
}

void Overlay::Line(int x0, int y0, int x1, int y1, COLORREF c)
{
	if (!m_mem) return;
	HPEN pen = CreatePen(PS_SOLID, 1, c);
	HGDIOBJ old = SelectObject(m_mem, pen);
	MoveToEx(m_mem, x0, y0, nullptr);
	LineTo(m_mem, x1, y1);
	SelectObject(m_mem, old);
	DeleteObject(pen);
}

void Overlay::Text(int x, int y, COLORREF c, const wchar_t* fmt, ...)
{
	if (!m_mem || !fmt) return;
	wchar_t buf[256] = { };
	va_list ap;
	va_start(ap, fmt);
	_vsnwprintf_s(buf, _countof(buf), _TRUNCATE, fmt, ap);
	va_end(ap);
	SetTextColor(m_mem, c);
	ExtTextOutW(m_mem, x, y, 0, nullptr, buf, (UINT)wcslen(buf), nullptr);
}

void Overlay::EndFrame()
{
	if (!m_hwnd || !m_mem || !m_w || !m_h)
		return;
	POINT src = { 0, 0 };
	SIZE sz = { m_w, m_h };
	POINT dst = { 0, 0 };
	RECT wr = { };
	GetWindowRect(m_hwnd, &wr);
	dst.x = wr.left; dst.y = wr.top;
	BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, 0 };
	HDC scr = GetDC(nullptr);
	UpdateLayeredWindow(m_hwnd, scr, &dst, &sz, m_mem, &src, RGB(1, 2, 3), &bf, ULW_COLORKEY);
	ReleaseDC(nullptr, scr);
}
