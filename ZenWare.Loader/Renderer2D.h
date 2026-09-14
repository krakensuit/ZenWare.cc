#pragma once

// ZenWare Loader - рендерер: DirectComposition (D3D11 + swapchain со сквозной
// альфой + D2D device context) с фолбэком на ID2D1HwndRenderTarget.
// Если композиция не поднялась - работаем как раньше (плоский тёмный кадр).

#include <windows.h>
#include <d2d1_1.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dcomp.h>
#include <dwrite.h>
#include <wincodec.h>

#include "Theme.h"

namespace Zen2D
{
	// Проверка ДО создания окна: доступны ли D3D11 + DComp (нужно для выбора
	// стиля окна WS_EX_NOREDIRECTIONBITMAP).
	bool ProbeComposition();

	class Renderer2D
	{
	public:
		bool Init(HWND hwnd, HINSTANCE hInst);
		void Resize(int w, int h);
		void Shutdown();

		bool Ready() const { return m_bReady; }
		bool Enabled() const { return m_bReady && m_bEnabled; }
		void SetEnabled(bool b) { m_bEnabled = b; }

		// true = рисуем через композицию (окно создано с WS_EX_NOREDIRECTIONBITMAP).
		bool IsUsingComposition() const { return m_bComposition; }

		void RenderFrame(const FrameState_t& st, const Theme_t& th,
			const wchar_t* wszStatus, const wchar_t* wszVersion, const wchar_t* wszLang);

		void SetTheme(const Theme_t& th);

	private:
		// Полукадровые слои.
		void DrawBackground(const FrameState_t& st);
		void DrawHeader(const FrameState_t& st);
		void DrawLogo(const FrameState_t& st);
		void DrawModePill(const FrameState_t& st);
		void DrawButtons(const FrameState_t& st);
		void DrawProgress(const FrameState_t& st);
		void DrawStatus(const FrameState_t& st, const wchar_t* wszText);
		void DrawFooter(const FrameState_t& st, const wchar_t* wszVersion, const wchar_t* wszLang);

		// Примитивы (работают и для device context, и для hwnd render target).
		void FillRect(const D2D1_RECT_F& rc, DWORD rgb, float alpha = 1.0f);
		void StrokeRect(const D2D1_RECT_F& rc, DWORD rgb, float alpha = 1.0f, float width = 1.0f);
		void FillRound(const D2D1_RECT_F& rc, float radius, DWORD rgb, float alpha = 1.0f);
		void StrokeRound(const D2D1_RECT_F& rc, float radius, DWORD rgb, float alpha = 1.0f, float width = 1.0f);
		void Text(const wchar_t* wsz, const D2D1_RECT_F& rc, DWORD rgb, DWRITE_FONT_WEIGHT weight,
			DWRITE_TEXT_ALIGNMENT align, float alpha = 1.0f);
		void Line(float x0, float y0, float x1, float y1, DWORD rgb, float alpha, float width);

		bool CreateTextFormats();
		bool LoadLogoFromResource(HINSTANCE hInst);
		bool CreateCompositionTarget();
		bool CreateLegacyTarget();
		bool CreateTargetBitmapFromBackBuffer();
		void ReleaseComposition();
		void ReleaseLegacy();

		// Общие ресурсы.
		ID2D1Factory1*        m_factory = nullptr;
		IDWriteFactory*       m_dwrite = nullptr;
		IWICImagingFactory*   m_wic = nullptr;
		IDWriteTextFormat*    m_fmtTitle = nullptr;
		IDWriteTextFormat*    m_fmtBody = nullptr;
		IDWriteTextFormat*    m_fmtSmall = nullptr;
		IDWriteTextFormat*    m_fmtMicro = nullptr;
		ID2D1SolidColorBrush* m_brush = nullptr;
		ID2D1Bitmap*          m_logo = nullptr;

		// Путь композиции.
		ID3D11Device*         m_d3dDevice = nullptr;
		ID3D11DeviceContext*  m_d3dContext = nullptr;
		IDXGISwapChain1*      m_swapChain = nullptr;
		ID2D1Device*          m_d2dDevice = nullptr;
		ID2D1DeviceContext*   m_d2dContext = nullptr;
		ID2D1Bitmap1*         m_targetBitmap = nullptr;
		IDCompositionDevice*  m_dcompDevice = nullptr;
		IDCompositionTarget*  m_dcompTarget = nullptr;
		IDCompositionVisual*  m_dcompVisual = nullptr;

		// Фолбэк.
		ID2D1HwndRenderTarget* m_hwndRT = nullptr;

		// Текущая цель отрисовки (device context либо hwnd RT).
		ID2D1RenderTarget*     m_target = nullptr;

		bool  m_bReady = false;
		bool  m_bComposition = false;
		bool  m_bEnabled = false;
		HWND  m_hwnd = nullptr;
		int   m_w = 0;      // логические пиксели (вёрстка)
		int   m_h = 0;      // логические пиксели (вёрстка)
		int   m_wPx = 0;    // физические пиксели (swapchain)
		int   m_hPx = 0;
		UINT  m_dpi = 96;
		Theme_t m_theme = Dark();
	};

	Renderer2D& R();
}
