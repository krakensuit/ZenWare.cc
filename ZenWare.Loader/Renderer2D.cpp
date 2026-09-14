// ZenWare Loader - Direct2D/DirectWrite/WIC рендерер (реализация).
// ИЗМЕНЕНО: светлая тема больше не используется (см. Theme.h), добавлены
// скругления, градиент фона, логотип картинкой; убраны мятные полосы.

#include "Renderer2D.h"
#include "resource.h"

#include <math.h>
#include <d2d1helper.h>
#include <shlwapi.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shlwapi.lib")

namespace
{
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

	bool Renderer2D::Init(HWND hwnd, HINSTANCE hInst)
	{
		Shutdown();

		m_hwnd = hwnd;

		const HRESULT hrCo = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		(void)hrCo;

		if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_factory)) || !m_factory)
			return false;

		RECT rc{};
		GetClientRect(hwnd, &rc);
		m_w = rc.right > 0 ? rc.right : 620;
		m_h = rc.bottom > 0 ? rc.bottom : 334;

		if (FAILED(m_factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(static_cast<UINT32>(m_w), static_cast<UINT32>(m_h))),
			&m_rt)) || !m_rt)
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
		// РЕШЕНИЕ: Inter не подключаю - его нет ни в системе, ни в поставке;
		// Segoe UI Variable даёт ту же сдержанную нейтральность.
		const wchar_t* faces[2] = { L"Segoe UI Variable Display", L"Segoe UI" };
		const DWRITE_FONT_WEIGHT weights[4] = { DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_FONT_WEIGHT_MEDIUM,
			DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_FONT_WEIGHT_MEDIUM };
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

		if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&m_wic))))
			m_wic = nullptr;

		CreateBackgroundGradient();

		// Логотип не критичен: если не загрузился, останется текст.
		LoadLogoFromResource(hInst);

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
		if (m_logo) { m_logo->Release(); m_logo = nullptr; }

		ReleaseBackgroundGradient();

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

	// --------------------------------- градиент и логотип ---------------------------------

	void Renderer2D::ReleaseBackgroundGradient()
	{
		if (m_bgGrad) { m_bgGrad->Release(); m_bgGrad = nullptr; }
	}

	void Renderer2D::CreateBackgroundGradient()
	{
		if (!m_rt)
			return;

		ReleaseBackgroundGradient();

		// Очень слабый вертикальный градиент: surface (0.25) сверху -> background (0) снизу.
		D2D1_GRADIENT_STOP stops[2]{};
		stops[0].position = 0.0f;
		stops[0].color = ColorOf(m_theme.surface, 0.25f);
		stops[1].position = 1.0f;
		stops[1].color = ColorOf(m_theme.background, 0.0f);

		ID2D1GradientStopCollection* coll = nullptr;

		if (FAILED(m_rt->CreateGradientStopCollection(stops, 2, &coll)) || !coll)
			return;

		m_rt->CreateLinearGradientBrush(
			D2D1::LinearGradientBrushProperties(D2D1::Point2F(0.0f, 0.0f),
				D2D1::Point2F(0.0f, static_cast<FLOAT>(m_h))),
			coll, &m_bgGrad);

		coll->Release();
	}

	void Renderer2D::SetTheme(const Theme_t& th)
	{
		m_theme = th;
		CreateBackgroundGradient();
	}

	bool Renderer2D::LoadLogoFromResource(HINSTANCE hInst)
	{
		if (!m_rt || !m_wic || !hInst)
			return false;

		const HRSRC hRes = FindResourceW(hInst, MAKEINTRESOURCEW(IDR_LOGO_PNG), RT_RCDATA);

		if (!hRes)
			return false;

		const HGLOBAL hMem = LoadResource(hInst, hRes);
		const DWORD dwSize = SizeofResource(hInst, hRes);

		if (!hMem || dwSize == 0)
			return false;

		const void* pData = LockResource(hMem);

		if (!pData)
			return false;

		IWICStream* stream = nullptr;
		IWICBitmapDecoder* decoder = nullptr;
		IWICBitmapFrameDecode* frame = nullptr;
		IWICFormatConverter* converter = nullptr;

		bool ok = false;

		if (SUCCEEDED(m_wic->CreateStream(&stream)) && stream)
		{
			if (SUCCEEDED(stream->InitializeFromMemory(static_cast<BYTE*>(const_cast<void*>(pData)), dwSize)))
			{
				if (SUCCEEDED(m_wic->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnDemand, &decoder))
					&& decoder
					&& SUCCEEDED(decoder->GetFrame(0, &frame)) && frame)
				{
					if (SUCCEEDED(m_wic->CreateFormatConverter(&converter)) && converter
						&& SUCCEEDED(converter->Initialize(frame, GUID_WICPixelFormat32bppPBGRA,
							WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeMedianCut)))
					{
						ok = SUCCEEDED(m_rt->CreateBitmapFromWicBitmap(converter, nullptr, &m_logo)) && m_logo != nullptr;
					}
				}
			}
		}

		if (converter) converter->Release();
		if (frame) frame->Release();
		if (decoder) decoder->Release();
		if (stream) stream->Release();

		return ok;
	}

	// --------------------------------- примитивы ---------------------------------

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

	// ИЗМЕНЕНО: скруглённые заливка и обводка (в GDI был RoundRect, в D2D нужен явный радиус).
	void Renderer2D::FillRound(const D2D1_RECT_F& rc, float radius, DWORD rgb, float alpha)
	{
		if (!m_rt || !m_brush)
			return;

		m_brush->SetColor(ColorOf(rgb, alpha));
		m_rt->FillRoundedRectangle(D2D1::RoundedRect(rc, radius, radius), m_brush);
	}

	void Renderer2D::StrokeRound(const D2D1_RECT_F& rc, float radius, DWORD rgb, float alpha, float width)
	{
		if (!m_rt || !m_brush)
			return;

		m_brush->SetColor(ColorOf(rgb, alpha));
		m_rt->DrawRoundedRectangle(D2D1::RoundedRect(rc, radius, radius), m_brush, width);
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

		IDWriteTextFormat* fmt = (rc.bottom - rc.top >= 28.0f) ? m_fmtTitle
			: ((rc.bottom - rc.top >= 16.0f) ? m_fmtBody : m_fmtSmall);

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
			if (m_rt) { m_rt->Release(); m_rt = nullptr; }

			m_factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
				D2D1::HwndRenderTargetProperties(m_hwnd, D2D1::SizeU(static_cast<UINT32>(m_w), static_cast<UINT32>(m_h))),
				&m_rt);

			if (m_rt)
			{
				m_rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
				m_rt->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);

				if (m_brush) { m_brush->Release(); m_brush = nullptr; }

				m_rt->CreateSolidColorBrush(ColorOf(m_theme.textPrimary, 1.0f), &m_brush);

				CreateBackgroundGradient();
			}
		}
	}

	// ИЗМЕНЕНО: убраны мятные полосы; остался чистый фон + слабый градиент сверху.
	void Renderer2D::DrawBackground(const FrameState_t& st)
	{
		(void)st;

		const float w = static_cast<float>(m_w);
		const float h = static_cast<float>(m_h);

		FillRect(RectF(0, 0, w, h), m_theme.background, 1.0f);

		if (m_bgGrad)
			m_rt->FillRectangle(RectF(0, 0, w, h), m_bgGrad);
	}

	void Renderer2D::DrawHeader(const FrameState_t& st)
	{
		(void)st;

		const float w = static_cast<float>(m_w);
		const float pulse = 0.30f + 0.20f * (0.5f + 0.5f * sinf(st.elapsed * 1.4f));

		Line(0.0f, 66.0f, w, 66.0f, m_theme.accent, pulse, 1.0f);
	}

	// ИЗМЕНЕНО: вместо мятного текста - PNG-логотип + текст рядом.
	void Renderer2D::DrawLogo(const FrameState_t& st)
	{
		(void)st;

		const float pulse = 0.9f + 0.1f * sinf(st.elapsed * 2.094f);

		float textX = 24.0f;

		if (m_logo)
		{
			const float size = 40.0f;
			const D2D1_RECT_F dst = RectF(24.0f, 14.0f, 24.0f + size, 14.0f + size);

			m_rt->DrawBitmap(m_logo, dst, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
			textX = 24.0f + size + 10.0f;
		}

		Text(L"ZenWare.cc", RectF(textX, 14.0f, textX + 300.0f, 54.0f),
			m_theme.textPrimary, 18.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD,
			DWRITE_TEXT_ALIGNMENT_LEADING, pulse);
	}

	// ИЗМЕНЕНО: скругление 6 px.
	void Renderer2D::DrawModePill(const FrameState_t& st)
	{
		const float w = static_cast<float>(m_w);
		const D2D1_RECT_F pill = RectF(w - 150.0f, 20.0f, w - 24.0f, 46.0f);

		FillRound(pill, 6.0f, m_theme.surface, 0.9f);
		StrokeRound(pill, 6.0f, m_theme.accentDim, 0.7f, 1.0f);

		Text(st.external ? L"EXTERNAL" : L"INTERNAL", pill, m_theme.textPrimary,
			11.0f, DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_TEXT_ALIGNMENT_CENTER);
	}

	// ИЗМЕНЕНО: скругление 8 px.
	void Renderer2D::DrawButtons(const FrameState_t& st)
	{
		const float w = static_cast<float>(m_w);

		const D2D1_RECT_F b1 = RectF(24.0f, 92.0f, w - 24.0f, 128.0f);
		FillRound(b1, 8.0f, m_theme.surface, 0.85f + 0.06f * st.hoverLaunch);
		StrokeRound(b1, 8.0f, m_theme.border, 1.0f, 1.0f);
		Text(L"LAUNCH GAME", b1, m_theme.textPrimary, 13.0f,
			DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_TEXT_ALIGNMENT_CENTER);

		const D2D1_RECT_F b2 = RectF(24.0f, 136.0f, w - 24.0f, 188.0f);
		FillRound(b2, 8.0f, m_theme.surface, 0.85f + 0.06f * st.hoverInject);
		StrokeRound(b2, 8.0f, m_theme.accent, 0.35f + 0.65f * st.hoverInject, 1.0f);
		Text(st.external ? L"LAUNCH EXTERNAL" : L"INJECT", b2, m_theme.accent, 13.0f,
			DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_CENTER);
	}

	void Renderer2D::DrawProgress(const FrameState_t& st)
	{
		const float w = static_cast<float>(m_w);
		const float trackY = 214.0f;
		const float trackW = w - 48.0f;

		FillRound(RectF(24.0f, trackY, 24.0f + trackW, trackY + 3.0f), 1.5f, m_theme.border, 0.9f);

		if (!st.busy && st.progress <= 0.0f)
			return;

		float x0 = 24.0f;
		float x1 = 24.0f + trackW;

		if (st.progress > 0.0f)
		{
			const float p = st.progress > 1.0f ? 1.0f : st.progress;
			x1 = 24.0f + trackW * p;
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
			FillRound(RectF(x0, trackY, x1, trackY + 3.0f), 1.5f, m_theme.accent, 0.95f);
	}

	void Renderer2D::DrawStatus(const FrameState_t& st, const wchar_t* wszText)
	{
		const float pulse = st.busy ? (0.6f + 0.4f * (0.5f + 0.5f * sinf(st.elapsed * 4.18879f))) : 1.0f;

		FillRound(RectF(26.0f, 244.0f, 34.0f, 252.0f), 4.0f, m_theme.accent, pulse);

		if (wszText && wszText[0])
			Text(wszText, RectF(44.0f, 238.0f, static_cast<float>(m_w) - 24.0f, 258.0f),
				m_theme.textPrimary, 13.0f, DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_TEXT_ALIGNMENT_LEADING);
	}

	void Renderer2D::DrawFooter(const FrameState_t& st, const wchar_t* wszVersion, const wchar_t* wszLang)
	{
		(void)st;

		const float w = static_cast<float>(m_w);
		const float h = static_cast<float>(m_h);

		Line(24.0f, h - 44.0f, w - 24.0f, h - 44.0f, m_theme.border, 1.0f, 1.0f);

		if (wszVersion && wszVersion[0])
			Text(wszVersion, RectF(24.0f, h - 40.0f, 220.0f, h - 20.0f), m_theme.textSecondary,
				11.0f, DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_TEXT_ALIGNMENT_LEADING);

		if (wszLang && wszLang[0])
			Text(wszLang, RectF(w - 220.0f, h - 40.0f, w - 24.0f, h - 20.0f), m_theme.textSecondary,
				11.0f, DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_TEXT_ALIGNMENT_TRAILING);
	}
}
