#pragma once

// ZenWare Loader - Direct2D/DirectWrite/WIC рендерер.
// РЕШЕНИЕ: полный перевод UI на D2D делается с сохранением прежнего GDI-пути:
// WM_PAINT использует D2D, только если Init() прошёл успешно (Ready()), иначе
// остаётся старый рендер. Так лоадер не может «не запуститься» из-за D2D.

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
		bool Init(HWND hwnd);
		void Resize(int w, int h);
		void Shutdown();

		// true = D2D готов и кадр можно рисовать им.
		bool Ready() const { return m_rt != nullptr; }
		// РЕШЕНИЕ: D2D-путь выключен по умолчанию: он рисует непрозрачный фон и
		// перекрыл бы акрил v3.10. Флаг даёт включить новый рендер для проверки.
		bool Enabled() const { return m_rt != nullptr && m_bEnabled; }
		void SetEnabled(bool b) { m_bEnabled = b; }

		HWND Target() const { return m_hwnd; }

		// Кадр целиком: фон -> шапка -> лого -> пилюля режима -> кнопки ->
		// прогресс -> статус -> футер.
		void RenderFrame(const FrameState_t& st, const Theme_t& th,
			const wchar_t* wszStatus, const wchar_t* wszVersion, const wchar_t* wszLang);

		void SetTheme(const Theme_t& th) { m_theme = th; }

	private:
		// Слои кадра.
		void DrawBackground(const FrameState_t& st);
		void DrawHeader(const FrameState_t& st);
		void DrawLogo(const FrameState_t& st);
		void DrawModePill(const FrameState_t& st);
		void DrawButtons(const FrameState_t& st);
		void DrawProgress(const FrameState_t& st);
		void DrawStatus(const FrameState_t& st, const wchar_t* wszText);
		void DrawFooter(const FrameState_t& st, const wchar_t* wszVersion, const wchar_t* wszLang);

		// Мелкие помощники.
		void FillRect(const D2D1_RECT_F& rc, DWORD rgb, float alpha = 1.0f);
		void StrokeRect(const D2D1_RECT_F& rc, DWORD rgb, float alpha = 1.0f, float width = 1.0f);
		void Text(const wchar_t* wsz, const D2D1_RECT_F& rc, DWORD rgb, float size,
			DWRITE_FONT_WEIGHT weight, DWRITE_TEXT_ALIGNMENT align, float alpha = 1.0f);
		void Line(float x0, float y0, float x1, float y1, DWORD rgb, float alpha, float width);

		ID2D1Factory*          m_factory = nullptr;
		ID2D1HwndRenderTarget* m_rt = nullptr;
		IDWriteFactory*        m_dwrite = nullptr;
		IWICImagingFactory*    m_wic = nullptr;

		IDWriteTextFormat*     m_fmtTitle = nullptr;
		IDWriteTextFormat*     m_fmtBody = nullptr;
		IDWriteTextFormat*     m_fmtSmall = nullptr;
		IDWriteTextFormat*     m_fmtMicro = nullptr;

		ID2D1SolidColorBrush*  m_brush = nullptr;

		bool m_bEnabled = false;
		HWND m_hwnd = nullptr;
		int  m_w = 0;
		int  m_h = 0;
		Theme_t m_theme = Dark();
	};

	// Глобальный экземпляр (лоадер однопоточный, окно одно).
	Renderer2D& R();
}
