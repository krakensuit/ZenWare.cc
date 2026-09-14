#pragma once

// ZenWare Loader - Direct2D/DirectWrite/WIC рендерер.
// РЕШЕНИЕ: полный перевод UI на D2D делается с сохранением прежнего GDI-пути:
// WM_PAINT использует D2D, только если Init() прошёл успешно (Ready()) и путь
// включён флагом (Enabled()).

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>

#include "Theme.h"

namespace Zen2D
{
	class Renderer2D
	{
	public:
		bool Init(HWND hwnd, HINSTANCE hInst);
		void Resize(int w, int h);
		void Shutdown();

		bool Ready() const { return m_rt != nullptr; }
		bool Enabled() const { return m_rt != nullptr && m_bEnabled; }
		void SetEnabled(bool b) { m_bEnabled = b; }

		HWND Target() const { return m_hwnd; }

		void RenderFrame(const FrameState_t& st, const Theme_t& th,
			const wchar_t* wszStatus, const wchar_t* wszVersion, const wchar_t* wszLang);

		void SetTheme(const Theme_t& th);

	private:
		void DrawBackground(const FrameState_t& st);
		void DrawHeader(const FrameState_t& st);
		void DrawLogo(const FrameState_t& st);
		void DrawModePill(const FrameState_t& st);
		void DrawButtons(const FrameState_t& st);
		void DrawProgress(const FrameState_t& st);
		void DrawStatus(const FrameState_t& st, const wchar_t* wszText);
		void DrawFooter(const FrameState_t& st, const wchar_t* wszVersion, const wchar_t* wszLang);

		// Примитивы. Rect-варианты остаются для фоновых заливок,
		// Round-варианты - для всего, что должно иметь скругление.
		void FillRect(const D2D1_RECT_F& rc, DWORD rgb, float alpha = 1.0f);
		void StrokeRect(const D2D1_RECT_F& rc, DWORD rgb, float alpha = 1.0f, float width = 1.0f);
		void FillRound(const D2D1_RECT_F& rc, float radius, DWORD rgb, float alpha = 1.0f);
		void StrokeRound(const D2D1_RECT_F& rc, float radius, DWORD rgb, float alpha = 1.0f, float width = 1.0f);

		void Text(const wchar_t* wsz, const D2D1_RECT_F& rc, DWORD rgb, float size,
			DWRITE_FONT_WEIGHT weight, DWRITE_TEXT_ALIGNMENT align, float alpha = 1.0f);
		void Line(float x0, float y0, float x1, float y1, DWORD rgb, float alpha, float width);

		// Градиент фона и логотип.
		void CreateBackgroundGradient();
		void ReleaseBackgroundGradient();
		bool LoadLogoFromResource(HINSTANCE hInst);

		ID2D1Factory*           m_factory = nullptr;
		ID2D1HwndRenderTarget*  m_rt = nullptr;
		IDWriteFactory*         m_dwrite = nullptr;
		IWICImagingFactory*     m_wic = nullptr;

		IDWriteTextFormat*      m_fmtTitle = nullptr;
		IDWriteTextFormat*      m_fmtBody = nullptr;
		IDWriteTextFormat*      m_fmtSmall = nullptr;
		IDWriteTextFormat*      m_fmtMicro = nullptr;

		ID2D1SolidColorBrush*   m_brush = nullptr;
		ID2D1LinearGradientBrush* m_bgGrad = nullptr;
		ID2D1Bitmap*            m_logo = nullptr;

		bool  m_bEnabled = false;
		HWND  m_hwnd = nullptr;
		int   m_w = 0;
		int   m_h = 0;
		Theme_t m_theme = Dark();
	};

	Renderer2D& R();
}
