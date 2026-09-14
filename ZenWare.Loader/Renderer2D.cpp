// ZenWare Loader - Direct2D/DirectWrite/WIC рендерер (реализация).
// РЕШЕНИЕ: никаких сторонних библиотек и HLSL - только d2d1/dwrite/wic + WinAPI.

#include "Renderer2D.h"

#include <math.h>
#include <d2d1helper.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")

namespace
{
	// Плавный подход к цели за dt (тот же закон, что Approach в Main.cpp).
	inline float Approach(float cur, float target, float dt, float speed)
	{
		return cur + (target - cur) * (1.0f - expf(-dt * speed));
	}

	inline D2D1_RECT_F RectF(float l, float t, float r, float b)
	{
		return D2D1::RectF(l, t, r, b);
	}

	inline D2D1_COLOR_F ColorOf(UINT32 rgb, float alpha)
	{
		return D2D1::ColorF(rgb, alpha);
	}
}

namespace Zen2D
{
	Renderer2D& R()
	{
		static Renderer2D s_renderer;
		return s_renderer;
	}

	bool Renderer2D::Init(HWND hwnd)
	{
		Shutdown();

		m_hwnd = hwnd;

		// COM нужен WIC; если поток уже инициализирован в другом режиме - не беда.
		const HRESULT hrCo = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		(void)hrCo;

		if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_factory)) || !m_factory)
			return false;

		RECT rc{};
		GetClientRect(hwnd, &rc);
		m_w = rc.right > 0 ? rc.right : 620;
		m_h = rc.bottom > 0 ? rc.bottom : 334;

		const D2D1_SIZE_U size = D2D1::SizeU(static_cast<UINT32>(m_w), static_cast<UINT32>(m_h));
		const D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
		const D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(hwnd, size);

		if (FAILED(m_factory->CreateHwndRenderTarget(rtProps, hwndProps, &m_rt)) || !m_rt)
		{
			Shutdown();
			return false;
		}

		m_rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		m_rt->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

		if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(&m_dwrite))) || !m_dwrite)
		{
			Shutdown();
			return false;
		}

		// Шрифт: Segoe UI Variable Display (Win11), иначе Segoe UI.
		// РЕШЕНИЕ: Inter не подключаю из ресурса - он не входит в поставку и не
		// лежит в системе; Segoe UI Variable даёт ту же сдержанную нейтральность.
		const wchar_t* faces[2] = { L"Segoe UI Variable Display", L"Segoe UI" };
		const DWRITE_FONT_WEIGHT weights[4] = { DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_WEIGHT_MEDIUM,
			DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_WEIGHT_NORMAL };
		const FLOAT sizes[4] = { 24.0f, 13.0f, 11.0f, 10.0f };
		IDWriteTextFormat** slots[4] = { &m_fmtTitle, &m_fmtBody, &m_fmtSmall, &m_fmtMicro };

		for (int i = 0; i < 4; ++i)
		{
			for (int f = 0; f < 2; ++f)
			{
				if (SUCCEEDED(m_dwrite->CreateTextFormat(faces[f], nullptr, weights[i],
					DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, sizes[i], L"en-us", slots[i]))
					&& *slots[i])
					break;
			}

			if (!*slots[i])
			{
				Shutdown();
				return false;
			}

			(*slots[i])->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
			(*slots[i])->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		}

		if (FAILED(m_rt->CreateSolidColorBrush(ColorOf(m_theme.textPrimary, 1.0f), &m_brush)) || !m_brush)
		{
			Shutdown();
			return false;
		}

		// WIC - необязателен: если фабрика не поднялась, логотип просто не рисуем.
		CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&m_wic));

		return true;
	}

	void Renderer2D::Resize(int w, int h)
	{
		if (!m_rt || w <= 0 || h <= 0)
			return;

		m_w = w;
		m_h = h;
		m_rt->Resize(D2D1::SizeU(static_cast<UINT32>(w), static_cast<UINT32>(h)));
	}

	void Renderer2D::Shutdown()
	{
		if (m_brush) { m_brush->Release(); m_brush = nullptr; }

		IDWriteTextFormat** slots[4] = { &m_fmtTitle, &m_fmtBody, &m_fmtSmall, &m_fmtMicro };

		for (int i = 0; i < 4; ++i)
		{
			if (*slots[i]) { (*slots[i])->Release(); *slots[i] = nullptr; }
		}

		if (m_wic) { m_wic->Release(); m_wic = nullptr; }
		if (m_dwrite) { m_dwrite->Release(); m_dwrite = nullptr; }
		if (m_rt) { m_rt->Release(); m_rt = nullptr; }
		if (m_factory) { m_factory->Release(); m_factory = nullptr; }
	}

	// ------------------------------- примитивы -------------------------------

	void Renderer2D::FillRect(const D2D1_RECT_F& rc, DWORD rgb, float alpha)
	{
		if (!m_rt || !m_brush)
			return;

		m_brush->SetColor(ColorOf(rgb, alpha));
		m_rt->FillRectangle(rc, m_brush);
	}

	void Renderer2D::StrokeRect(const D2D1_RECT_F& rc, DWORD rgb, float alpha, float width)
	{
		if (!m_rt || !m_brush)
			return;

		m_brush->SetColor(ColorOf(rgb, alpha));
		m_rt->DrawRectangle(rc, m_brush, width);
	}

	void Renderer2D::Line(float x0, float y0, float x1, float y1, DWORD rgb, float alpha, float width)
	{
		if (!m_rt || !m_brush)
			return;

		m_brush->SetColor(ColorOf(rgb, alpha));
		m_rt->DrawLine(D2D1::Point2F(x0, y0), D2D1::Point2F(x1, y1), m_brush, width);
	}

	void Renderer2D::Text(const wchar_t* wsz, const D2D1_RECT_F& rc, DWORD rgb, float size,
		DWRITE_FONT_WEIGHT weight, DWRITE_TEXT_ALIGNMENT align, float alpha)
	{
		(void)size;
		(void)weight;

		if (!m_rt || !m_brush || !wsz || !wsz[0])
			return;


		
		IDWriteTextFormat* fmt = (rc.bottom - rc.top >= 28.0f) ? m_fmtTitle : ((rc.bottom - rc.top >= 16.0f) ? m_fmtBody : m_fmtSmall);
		fmt->SetTextAlignment(align);

		m_brush->SetColor(ColorOf(rgb, alpha));
		m_rt->DrawTextW(wsz, static_cast<UINT32>(wcslen(wsz)), fmt, rc, m_brush,
			D2D1_DRAW_TEXT_OPTIONS_CLIP, DWRITE_MEASURING_MODE_NATURAL);
	}

	// --------------------------------- кадр ---------------------------------

	void Renderer2D::RenderFrame(const FrameState_t& st, const Theme_t& th,
		const wchar_t* wszStatus, const wchar_t* wszVersion, const wchar_t* wszLang)
	{
		if (!m_rt)
			return;

		m_theme = th;

		m_rt->BeginDraw();
		m_rt->Clear(ColorOf(th.background, 1.0f));

		DrawBackground(st);
		DrawHeader(st);
		DrawLogo(st);
		DrawModePill(st);
		DrawButtons(st);
		DrawProgress(st);
		DrawStatus(st, wszStatus);
		DrawFooter(st, wszVersion, wszLang);

		const HRESULT hr = m_rt->EndDraw();

		if (hr == D2DERR_RECREATE_TARGET)
		{
			// Устройство потеряно: пересоздаём цель на том же окне.
			if (m_rt) { m_rt->Release(); m_rt = nullptr; }

			const D2D1_SIZE_U size = D2D1::SizeU(static_cast<UINT32>(m_w), static_cast<UINT32>(m_h));
			m_factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
				D2D1::HwndRenderTargetProperties(m_hwnd, size), &m_rt);

			if (m_rt)
			{
				m_rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
				m_rt->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

				if (m_brush) { m_brush->Release(); m_brush = nullptr; }

				m_rt->CreateSolidColorBrush(ColorOf(m_theme.textPrimary, 1.0f), &m_brush);
			}
		}
	}

	void Renderer2D::DrawBackground(const FrameState_t& st)
	{
		const float w = static_cast<float>(m_w);
		const float h = static_cast<float>(m_h);

		// База + мягкая «подложка» под контентом.
		FillRect(RectF(0, 0, w, h), m_theme.background, 1.0f);
		FillRect(RectF(0, 0, w, 66.0f), m_theme.surface, 0.55f);

		// Два слоя синусоид (без частиц): очень слабые горизонтальные полосы.
		for (int layer = 0; layer < 2; ++layer)
		{
			const float speed = layer == 0 ? 0.22f : 0.13f;
			const float amp = layer == 0 ? 26.0f : 42.0f;
			const float y = h * (layer == 0 ? 0.62f : 0.38f) + sinf(st.elapsed * speed * 6.28318f) * amp;

			FillRect(RectF(0, y, w, y + 1.0f), m_theme.accent, layer == 0 ? 0.05f : 0.03f);
		}
	}

	void Renderer2D::DrawHeader(const FrameState_t& st)
	{
		(void)st;
		const float w = static_cast<float>(m_w);

		// Тонкая линия под шапкой + мягкая пульсация акцента (вместо бегущего блика).
		const float pulse = 0.35f + 0.25f * (0.5f + 0.5f * sinf(st.elapsed * 1.4f));
		Line(0.0f, 66.0f, w, 66.0f, m_theme.accent, pulse, 1.0f);
	}

	void Renderer2D::DrawLogo(const FrameState_t& st)
	{
		// Мятный логотип: один слой свечения (упрощённый, без bitmap-блюра).
		const float pulse = 0.9f + 0.1f * sinf(st.elapsed * 2.094f); // 3 c цикл
		Text(L"ZenWare.cc", RectF(24.0f, 12.0f, 320.0f, 52.0f), m_theme.accent,
			24.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_LEADING, pulse);
	}

	void Renderer2D::DrawModePill(const FrameState_t& st)
	{
		const float w = static_cast<float>(m_w);
		const D2D1_RECT_F pill = RectF(w - 150.0f, 20.0f, w - 24.0f, 46.0f);

		FillRect(pill, m_theme.surface, 0.9f);
		StrokeRect(pill, m_theme.accentDim, 0.7f, 1.0f);

		const wchar_t* wszMode = st.external ? L"EXTERNAL" : L"INTERNAL";
		Text(wszMode, pill, m_theme.textPrimary, 11.0f, DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_TEXT_ALIGNMENT_CENTER);
	}

	void Renderer2D::DrawButtons(const FrameState_t& st)
	{
		const float w = static_cast<float>(m_w);

		// Кнопка 1: LAUNCH GAME (hover светлее на 6%).
		const D2D1_RECT_F b1 = RectF(24.0f, 92.0f, w - 24.0f, 128.0f);
		FillRect(b1, m_theme.surface, 0.85f + 0.06f * st.hoverLaunch);
		StrokeRect(b1, m_theme.border, 1.0f, 1.0f);
		Text(L"LAUNCH GAME", b1, m_theme.textPrimary, 13.0f, DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_TEXT_ALIGNMENT_CENTER);

		// Кнопка 2: INJECT / LAUNCH EXTERNAL (акцентная).
		const D2D1_RECT_F b2 = RectF(24.0f, 136.0f, w - 24.0f, 188.0f);
		FillRect(b2, m_theme.surface, 0.85f + 0.06f * st.hoverInject);
		StrokeRect(b2, m_theme.accent, 0.35f + 0.65f * st.hoverInject, 1.0f);
		Text(st.external ? L"LAUNCH EXTERNAL" : L"INJECT", b2, m_theme.accent,
			13.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_CENTER);
	}

	void Renderer2D::DrawProgress(const FrameState_t& st)
	{
		const float w = static_cast<float>(m_w);
		const float trackY = 214.0f;
		const D2D1_RECT_F track = RectF(24.0f, trackY, w - 24.0f, trackY + 2.0f);

		FillRect(track, m_theme.border, 0.9f);

		if (!st.busy && st.progress <= 0.0f)
			return;

		// Индетерминированная бегущая полоса шириной 30% трека, цикл 1.8 c.
		float x0 = 0.0f;
		float x1 = 0.0f;
		const float trackW = w - 48.0f;

		if (st.progress > 0.0f)
		{
			x0 = 24.0f;
			x1 = 24.0f + trackW * (st.progress < 0.0f ? 0.0f : (st.progress > 1.0f ? 1.0f : st.progress));
		}
		else
		{
			const float phase = fmodf(st.elapsed, 1.8f) / 1.8f;
			const float eased = phase * phase * (3.0f - 2.0f * phase);
			const float band = trackW * 0.3f;

			x0 = 24.0f + (trackW + band) * eased - band;
			x1 = x0 + band;

			if (x0 < 24.0f) x0 = 24.0f;
			if (x1 > 24.0f + trackW) x1 = 24.0f + trackW;
		}

		if (x1 > x0)
			FillRect(RectF(x0, trackY, x1, trackY + 2.0f), m_theme.accent, 0.95f);
	}

	void Renderer2D::DrawStatus(const FrameState_t& st, const wchar_t* wszText)
	{
		const float pulse = st.busy ? (0.6f + 0.4f * (0.5f + 0.5f * sinf(st.elapsed * 4.18879f))) : 1.0f;

		FillRect(RectF(28.0f, 246.0f, 34.0f, 252.0f), m_theme.accent, pulse);

		if (wszText && wszText[0])
			Text(wszText, RectF(44.0f, 240.0f, static_cast<float>(m_w) - 24.0f, 258.0f),
				m_theme.textPrimary, 13.0f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_TEXT_ALIGNMENT_LEADING);
	}

	void Renderer2D::DrawFooter(const FrameState_t& st, const wchar_t* wszVersion, const wchar_t* wszLang)
	{
		(void)st;
		const float w = static_cast<float>(m_w);
		const float h = static_cast<float>(m_h);

		Line(24.0f, h - 44.0f, w - 24.0f, h - 44.0f, m_theme.border, 1.0f, 1.0f);

		if (wszVersion)
			Text(wszVersion, RectF(24.0f, h - 40.0f, 200.0f, h - 20.0f), m_theme.textSecondary,
				11.0f, DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_TEXT_ALIGNMENT_LEADING);

		if (wszLang)
			Text(wszLang, RectF(w - 200.0f, h - 40.0f, w - 24.0f, h - 20.0f), m_theme.textSecondary,
				11.0f, DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_TEXT_ALIGNMENT_TRAILING);
	}
}
