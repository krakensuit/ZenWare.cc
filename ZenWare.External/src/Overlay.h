#pragma once
#include <windows.h>
#include <string>

// Transparent click-through overlay on top of the game window (GDI, no DX dependencies).
// Only our process draws; nothing is written to or injected into the game.
class Overlay
{
public:
	bool Create();
	void Destroy();
	bool IsAlive() const { return m_hwnd != nullptr; }

	// Fit the overlay to the game window (call ~every 500ms)
	void FollowGame();

	void BeginFrame();
	void Rect(int x, int y, int w, int h, COLORREF c, int thick = 1);
	void Line(int x0, int y0, int x1, int y1, COLORREF c);
	void Text(int x, int y, COLORREF c, const wchar_t* fmt, ...);
	void EndFrame();

	int Width() const { return m_w; }
	int Height() const { return m_h; }
	bool GameVisible() const { return m_gameOk; }

private:
	static LRESULT CALLBACK Proc(HWND h, UINT m, WPARAM w, LPARAM l);
	HWND m_hwnd = nullptr;
	HDC m_mem = nullptr;
	HBITMAP m_bmp = nullptr, m_oldBmp = nullptr;
	HFONT m_font = nullptr, m_oldFont = nullptr;
	int m_w = 0, m_h = 0;
	bool m_gameOk = false;
	RECT m_gameRect = { };
};
