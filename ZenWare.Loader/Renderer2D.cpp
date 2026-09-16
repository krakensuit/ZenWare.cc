// ZenWare Loader - renderer: DirectComposition with a fallback to HwndRenderTarget.

#include "Renderer2D.h"
#include "resource.h"
#include "Glass.h"

#include <math.h>
#include <stdio.h>   // swprintf_s for the diagnostics log
#include <d2d1helper.h>
#include <dcomp.h>
// Wrapped colour phase for the title rainbow. The old form used
// GetTickCount64() % 100000, so the phase snapped back to zero every 100 seconds and
// the colour visibly jumped. Accumulating the frame delta in a small float keeps the
// value inside 0..360 with full precision, so the cycle is endless and seamless.
static float TitleHue()
{
	static ULONGLONG s_ullPrev = 0;
	static float s_flHue = 0.0f;

	const ULONGLONG ullNow = GetTickCount64();
	const float flDelta = (s_ullPrev == 0) ? 0.0f : static_cast<float>(ullNow - s_ullPrev);
	s_ullPrev = ullNow;
	s_flHue += flDelta / 38.0f;

	while (s_flHue >= 360.0f)
		s_flHue -= 360.0f;

	return s_flHue;
}


#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dcomp.lib")

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

	// Debug diagnostics: write to OutputDebugString so it is visible at which
	// exact step the initialization fails (requested in the spec).
	// HSB -> RGB (как Hsv() в GDI-пути): h в градусах, s/v в 0..1.
	DWORD Hsv2Rgb(float h, float s, float v)
	{
		float r = v, g = v, b = v;
		const float hh = fmodf(fabsf(h), 360.0f) / 60.0f;
		const int i = static_cast<int>(hh);
		const float f = hh - i;
		const float p = v * (1.0f - s);
		const float q = v * (1.0f - s * f);
		const float tt = v * (1.0f - s * (1.0f - f));

		switch (i)
		{
		case 0: r = v;  g = tt; b = p;  break;
		case 1: r = q;  g = v;  b = p;  break;
		case 2: r = p;  g = v;  b = tt; break;
		case 3: r = p;  g = q;  b = v;  break;
		case 4: r = tt; g = p;  b = v;  break;
		default: r = v; g = p;  b = q;  break;
		}

		return (static_cast<DWORD>(r * 255.0f) << 16) |
			(static_cast<DWORD>(g * 255.0f) << 8) |
			static_cast<DWORD>(b * 255.0f);
	}
	void LogDbg(const wchar_t* wszMsg, HRESULT hr)
	{
		wchar_t buf[160]{};
		swprintf_s(buf, L"[Zen2D] %ls (hr=0x%08X)\n", wszMsg, static_cast<unsigned>(hr));
		OutputDebugStringW(buf);
	}
}

namespace Zen2D
{
	Renderer2D& R()
	{
		static Renderer2D s_renderer;
		return s_renderer;
	}

	bool ProbeComposition()
	{
		// Try to bring up D3D11 + DComp without a window: the result decides the window style.
		ID3D11Device* d3d = nullptr;
		ID3D11DeviceContext* ctx = nullptr;
		IDCompositionDevice* dcomp = nullptr;

		const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };

		HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
			D3D11_CREATE_DEVICE_BGRA_SUPPORT, levels, 3, D3D11_SDK_VERSION, &d3d, nullptr, &ctx);

		if (FAILED(hr) || !d3d)
			return false;

		IDXGIDevice* dxgiDevice = nullptr;
		hr = d3d->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));

		if (SUCCEEDED(hr) && dxgiDevice)
			hr = DCompositionCreateDevice(dxgiDevice, __uuidof(IDCompositionDevice), reinterpret_cast<void**>(&dcomp));

		const bool ok = SUCCEEDED(hr) && dcomp != nullptr;

		if (dcomp) dcomp->Release();
		if (dxgiDevice) dxgiDevice->Release();
		if (ctx) ctx->Release();
		if (d3d) d3d->Release();

		return ok;
	}

	bool Renderer2D::CreateTextFormats()
	{
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
				LogDbg(L"CreateTextFormat failed", E_FAIL);
				return false;
			}

			(*slots[i])->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
			(*slots[i])->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		}

		return true;
	}

	bool Renderer2D::CreateCompositionTarget()
	{
		const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };

		HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
			D3D11_CREATE_DEVICE_BGRA_SUPPORT, levels, 3, D3D11_SDK_VERSION,
			&m_d3dDevice, nullptr, &m_d3dContext);

		if (FAILED(hr) || !m_d3dDevice)
		{
			LogDbg(L"D3D11CreateDevice failed", hr);
			return false;
		}

		IDXGIDevice* dxgiDevice = nullptr;
		hr = m_d3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));

		if (FAILED(hr) || !dxgiDevice)
		{
			LogDbg(L"QueryInterface(IDXGIDevice) failed", hr);
			return false;
		}

		IDXGIAdapter* adapter = nullptr;
		hr = dxgiDevice->GetAdapter(&adapter);

		IDXGIFactory2* dxgiFactory = nullptr;

		if (SUCCEEDED(hr) && adapter)
			hr = adapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory));

		if (FAILED(hr) || !dxgiFactory)
		{
			LogDbg(L"GetParent(IDXGIFactory2) failed", hr);
			if (adapter) adapter->Release();
			dxgiDevice->Release();
			return false;
		}

		DXGI_SWAP_CHAIN_DESC1 desc{};
		desc.Width = static_cast<UINT>(m_wPx > 0 ? m_wPx : 1);
		desc.Height = static_cast<UINT>(m_hPx > 0 ? m_hPx : 1);
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.Stereo = FALSE;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount = 2;
		desc.Scaling = DXGI_SCALING_STRETCH;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
		desc.Flags = 0;

		hr = dxgiFactory->CreateSwapChainForComposition(m_d3dDevice, &desc, nullptr, &m_swapChain);

		dxgiFactory->Release();
		if (adapter) adapter->Release();

		if (FAILED(hr) || !m_swapChain)
		{
			LogDbg(L"CreateSwapChainForComposition failed", hr);
			dxgiDevice->Release();
			return false;
		}

		// D2D device on top of DXGI.
		hr = D2D1CreateDevice(dxgiDevice, nullptr, &m_d2dDevice);
		dxgiDevice->Release();

		if (FAILED(hr) || !m_d2dDevice)
		{
			LogDbg(L"D2D1CreateDevice failed", hr);
			return false;
		}

		hr = m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_d2dContext);

		if (FAILED(hr) || !m_d2dContext)
		{
			LogDbg(L"CreateDeviceContext failed", hr);
			return false;
		}

		m_d2dContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		// ClearType assumes an opaque background: on the swapchain's premultiplied
		// alpha it produces colored fringes around letters, hence grayscale in composition.
		m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

		if (!CreateTargetBitmapFromBackBuffer())
			return false;

		// Bind to DComp and the window.
		IDXGIDevice* dxgiDevice2 = nullptr;

		if (FAILED(m_d3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice2))) || !dxgiDevice2)
		{
			LogDbg(L"QueryInterface(IDXGIDevice) #2 failed", E_FAIL);
			return false;
		}

		hr = DCompositionCreateDevice(dxgiDevice2, __uuidof(IDCompositionDevice), reinterpret_cast<void**>(&m_dcompDevice));
		dxgiDevice2->Release();

		if (FAILED(hr) || !m_dcompDevice)
		{
			LogDbg(L"DCompositionCreateDevice failed", hr);
			return false;
		}

		if (FAILED(m_dcompDevice->CreateTargetForHwnd(m_hwnd, TRUE, &m_dcompTarget)) || !m_dcompTarget)
		{
			LogDbg(L"CreateTargetForHwnd failed", E_FAIL);
			return false;
		}

		if (FAILED(m_dcompDevice->CreateVisual(&m_dcompVisual)) || !m_dcompVisual)
		{
			LogDbg(L"CreateVisual failed", E_FAIL);
			return false;
		}

		if (FAILED(m_dcompVisual->SetContent(m_swapChain)))
		{
			LogDbg(L"Visual::SetContent failed", E_FAIL);
			return false;
		}

		if (FAILED(m_dcompTarget->SetRoot(m_dcompVisual)))
		{
			LogDbg(L"Target::SetRoot failed", E_FAIL);
			return false;
		}

		hr = m_dcompDevice->Commit();

		if (FAILED(hr))
		{
			LogDbg(L"DComp Commit failed", hr);
			return false;
		}

		m_bComposition = true;
		return true;
	}

	bool Renderer2D::CreateTargetBitmapFromBackBuffer()
	{
		IDXGISurface* surface = nullptr;

		if (FAILED(m_swapChain->GetBuffer(0, __uuidof(IDXGISurface), reinterpret_cast<void**>(&surface))) || !surface)
		{
			LogDbg(L"SwapChain::GetBuffer failed", E_FAIL);
			return false;
		}

		const D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
			D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
			static_cast<FLOAT>(m_dpi), static_cast<FLOAT>(m_dpi));

		const HRESULT hr = m_d2dContext->CreateBitmapFromDxgiSurface(surface, &props, &m_targetBitmap);
		surface->Release();

		if (FAILED(hr) || !m_targetBitmap)
		{
			LogDbg(L"CreateBitmapFromDxgiSurface failed", hr);
			return false;
		}

		// SetTarget strictly before the first BeginDraw.
		m_d2dContext->SetTarget(m_targetBitmap);
		m_d2dContext->SetDpi(static_cast<FLOAT>(m_dpi), static_cast<FLOAT>(m_dpi));
		return true;
	}

	bool Renderer2D::CreateLegacyTarget()
	{
		const D2D1_SIZE_U size = D2D1::SizeU(static_cast<UINT32>(m_wPx > 0 ? m_wPx : 1),
			static_cast<UINT32>(m_hPx > 0 ? m_hPx : 1));

		const HRESULT hr = m_factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(m_hwnd, size), &m_hwndRT);

		if (FAILED(hr) || !m_hwndRT)
		{
			LogDbg(L"CreateHwndRenderTarget failed", hr);
			return false;
		}

		m_hwndRT->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		m_hwndRT->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
		m_bComposition = false;

		return true;
	}

	bool Renderer2D::Init(HWND hwnd, HINSTANCE hInst)
	{
		Shutdown();

		m_hwnd = hwnd;
		m_hInst = hInst;

		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

		HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1),
			nullptr, reinterpret_cast<void**>(&m_factory));

		if (FAILED(hr) || !m_factory)
		{
			LogDbg(L"D2D1CreateFactory failed", hr);
			return false;
		}

		RECT rc{};
		GetClientRect(hwnd, &rc);
		// The swapchain lives in PHYSICAL pixels, layout is in logical ones (96 dpi).
		m_wPx = rc.right > 0 ? rc.right : 620;
		m_hPx = rc.bottom > 0 ? rc.bottom : 334;
		m_dpi = 96;
		{
			const HMODULE hUserDpi = GetModuleHandleW(L"user32.dll");
			if (hUserDpi)
			{
				typedef UINT(WINAPI* PFN_GetDpiForWindow)(HWND);
				PFN_GetDpiForWindow pfn = reinterpret_cast<PFN_GetDpiForWindow>(GetProcAddress(hUserDpi, "GetDpiForWindow"));
				if (pfn) { const UINT d = pfn(hwnd); if (d != 0) m_dpi = d; }
			}
		}
		m_w = static_cast<int>(m_wPx * 96 / static_cast<int>(m_dpi));
		m_h = static_cast<int>(m_hPx * 96 / static_cast<int>(m_dpi));

		hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(&m_dwrite));

		if (FAILED(hr) || !m_dwrite)
		{
			LogDbg(L"DWriteCreateFactory failed", hr);
			return false;
		}

		if (!CreateTextFormats())
			return false;

		if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&m_wic))))
			m_wic = nullptr;

		// Composition first; on any error fall back to the old path (flat frame).
		if (!CreateCompositionTarget())
		{
			LogDbg(L"composition path failed, falling back to HwndRenderTarget", E_FAIL);
			ReleaseComposition();

			if (!CreateLegacyTarget())
				return false;
		}

		m_target = m_bComposition ? static_cast<ID2D1RenderTarget*>(m_d2dContext)
			: static_cast<ID2D1RenderTarget*>(m_hwndRT);

		if (FAILED(m_target->CreateSolidColorBrush(ColorOf(m_theme.textPrimary, 1.0f), &m_brush)) || !m_brush)
		{
			LogDbg(L"CreateSolidColorBrush failed", E_FAIL);
			return false;
		}

		LoadLogoFromResource(hInst); // not critical: on failure only the text remains

		m_bReady = true;
		return true;
	}

	void Renderer2D::Resize(int w, int h)
	{
		if (!m_bReady || w <= 0 || h <= 0)
			return;

		m_wPx = w;
		m_hPx = h;
		m_w = static_cast<int>(w * 96 / static_cast<int>(m_dpi));
		m_h = static_cast<int>(h * 96 / static_cast<int>(m_dpi));

		if (m_bComposition && m_swapChain && m_d2dContext)
		{
			m_d2dContext->SetTarget(nullptr);

			if (m_targetBitmap) { m_targetBitmap->Release(); m_targetBitmap = nullptr; }

			if (FAILED(m_swapChain->ResizeBuffers(0, static_cast<UINT>(m_wPx), static_cast<UINT>(m_hPx),
				DXGI_FORMAT_UNKNOWN, 0)))
			{
				LogDbg(L"ResizeBuffers failed", E_FAIL);
				return;
			}

			if (!CreateTargetBitmapFromBackBuffer())
				LogDbg(L"recreate target bitmap failed", E_FAIL);
		}
		else if (m_hwndRT)
		{
			m_hwndRT->Resize(D2D1::SizeU(static_cast<UINT32>(m_wPx), static_cast<UINT32>(m_hPx)));
		}
	}

	bool Renderer2D::RecreateAfterDeviceLost()
	{
		ReleaseComposition();

		// The logo belonged to the old target - it can only be re-created.
		if (m_titleBrush) { m_titleBrush->Release(); m_titleBrush = nullptr; }
		if (m_titleLayout) { m_titleLayout->Release(); m_titleLayout = nullptr; }
		if (m_logo) { m_logo->Release(); m_logo = nullptr; }

		if (CreateCompositionTarget())
		{
			m_target = static_cast<ID2D1RenderTarget*>(m_d2dContext);

			if (m_brush) { m_brush->Release(); m_brush = nullptr; }

			m_target->CreateSolidColorBrush(ColorOf(m_theme.textPrimary, 1.0f), &m_brush);
			LoadLogoFromResource(m_hInst);
			return m_brush != nullptr;
		}

		// Composition no longer comes up: fall back to HwndRenderTarget.
		ReleaseComposition();

		if (!CreateLegacyTarget())
		{
			m_bReady = false;
			return false;
		}

		// Without a redirection bitmap HwndRenderTarget is invisible - drop the window style.
		const LONG_PTR ex = GetWindowLongPtrW(m_hwnd, GWL_EXSTYLE);
		SetWindowLongPtrW(m_hwnd, GWL_EXSTYLE, ex & ~WS_EX_NOREDIRECTIONBITMAP);
		SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

		m_target = static_cast<ID2D1RenderTarget*>(m_hwndRT);

		if (m_brush) { m_brush->Release(); m_brush = nullptr; }

		m_target->CreateSolidColorBrush(ColorOf(m_theme.textPrimary, 1.0f), &m_brush);
		LoadLogoFromResource(m_hInst);
		return m_brush != nullptr;
	}

	void Renderer2D::ReleaseComposition()
	{
		if (m_targetBitmap) { m_targetBitmap->Release(); m_targetBitmap = nullptr; }
		if (m_dcompVisual) { m_dcompVisual->Release(); m_dcompVisual = nullptr; }
		if (m_dcompTarget) { m_dcompTarget->Release(); m_dcompTarget = nullptr; }
		if (m_dcompDevice) { m_dcompDevice->Release(); m_dcompDevice = nullptr; }
		if (m_d2dContext) { m_d2dContext->Release(); m_d2dContext = nullptr; }
		if (m_d2dDevice) { m_d2dDevice->Release(); m_d2dDevice = nullptr; }
		if (m_swapChain) { m_swapChain->Release(); m_swapChain = nullptr; }
		if (m_d3dContext) { m_d3dContext->Release(); m_d3dContext = nullptr; }
		if (m_d3dDevice) { m_d3dDevice->Release(); m_d3dDevice = nullptr; }

		m_bComposition = false;
	}

	void Renderer2D::ReleaseLegacy()
	{
		if (m_hwndRT) { m_hwndRT->Release(); m_hwndRT = nullptr; }
	}

	void Renderer2D::Shutdown()
	{
		m_bReady = false;
		m_target = nullptr;

		if (m_titleBrush) { m_titleBrush->Release(); m_titleBrush = nullptr; }
		if (m_titleLayout) { m_titleLayout->Release(); m_titleLayout = nullptr; }
		if (m_logo) { m_logo->Release(); m_logo = nullptr; }
		if (m_brush) { m_brush->Release(); m_brush = nullptr; }

		IDWriteTextFormat** slots[4] = { &m_fmtTitle, &m_fmtBody, &m_fmtSmall, &m_fmtMicro };

		for (int i = 0; i < 4; ++i)
		{
			if (*slots[i]) { (*slots[i])->Release(); *slots[i] = nullptr; }
		}

		ReleaseComposition();
		ReleaseLegacy();

		if (m_wic) { m_wic->Release(); m_wic = nullptr; }
		if (m_dwrite) { m_dwrite->Release(); m_dwrite = nullptr; }
		if (m_factory) { m_factory->Release(); m_factory = nullptr; }
	}

	void Renderer2D::SetTheme(const Theme_t& th)
	{
		m_theme = th;
	}

	bool Renderer2D::LoadLogoFromResource(HINSTANCE hInst)
	{
		if (!m_target || !m_wic || !hInst)
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

		if (SUCCEEDED(m_wic->CreateStream(&stream)) && stream
			&& SUCCEEDED(stream->InitializeFromMemory(static_cast<BYTE*>(const_cast<void*>(pData)), dwSize))
			&& SUCCEEDED(m_wic->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnDemand, &decoder)) && decoder
			&& SUCCEEDED(decoder->GetFrame(0, &frame)) && frame
			&& SUCCEEDED(m_wic->CreateFormatConverter(&converter)) && converter
			&& SUCCEEDED(converter->Initialize(frame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone,
				nullptr, 0.0f, WICBitmapPaletteTypeMedianCut)))
		{
			ok = SUCCEEDED(m_target->CreateBitmapFromWicBitmap(converter, nullptr, &m_logo)) && m_logo != nullptr;
		}

		if (converter) converter->Release();
		if (frame) frame->Release();
		if (decoder) decoder->Release();
		if (stream) stream->Release();

		return ok;
	}

	// ------------------------------- primitives -------------------------------

	void Renderer2D::FillRect(const D2D1_RECT_F& rc, DWORD rgb, float alpha)
	{
		if (!m_target || !m_brush) return;
		m_brush->SetColor(ColorOf(rgb, alpha));
		m_target->FillRectangle(rc, m_brush);
	}

	void Renderer2D::StrokeRect(const D2D1_RECT_F& rc, DWORD rgb, float alpha, float width)
	{
		if (!m_target || !m_brush) return;
		m_brush->SetColor(ColorOf(rgb, alpha));
		m_target->DrawRectangle(rc, m_brush, width);
	}

	void Renderer2D::FillRound(const D2D1_RECT_F& rc, float radius, DWORD rgb, float alpha)
	{
		if (!m_target || !m_brush) return;
		m_brush->SetColor(ColorOf(rgb, alpha));
		m_target->FillRoundedRectangle(D2D1::RoundedRect(rc, radius, radius), m_brush);
	}

	void Renderer2D::StrokeRound(const D2D1_RECT_F& rc, float radius, DWORD rgb, float alpha, float width)
	{
		if (!m_target || !m_brush) return;
		m_brush->SetColor(ColorOf(rgb, alpha));
		m_target->DrawRoundedRectangle(D2D1::RoundedRect(rc, radius, radius), m_brush, width);
	}

	void Renderer2D::Line(float x0, float y0, float x1, float y1, DWORD rgb, float alpha, float width)
	{
		if (!m_target || !m_brush) return;
		m_brush->SetColor(ColorOf(rgb, alpha));
		m_target->DrawLine(D2D1::Point2F(x0, y0), D2D1::Point2F(x1, y1), m_brush, width);
	}

	void Renderer2D::Text(const wchar_t* wsz, const D2D1_RECT_F& rc, DWORD rgb,
		DWRITE_FONT_WEIGHT weight, DWRITE_TEXT_ALIGNMENT align, float alpha)
	{
		(void)weight;

		if (!m_target || !m_brush || !wsz || !wsz[0])
			return;

		IDWriteTextFormat* fmt = (rc.bottom - rc.top >= 28.0f) ? m_fmtTitle
			: ((rc.bottom - rc.top >= 16.0f) ? m_fmtBody : m_fmtSmall);

		fmt->SetTextAlignment(align);

		// CHANGED: small labels (status, footer) are anchored to the top edge -
		// with another language the font metrics change and the text drifted up.
		fmt->SetParagraphAlignment((rc.bottom - rc.top) <= 22.0f
			? DWRITE_PARAGRAPH_ALIGNMENT_NEAR
			: DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

		m_brush->SetColor(ColorOf(rgb, alpha));
		m_target->DrawTextW(wsz, static_cast<UINT32>(wcslen(wsz)), fmt, rc, m_brush,
			D2D1_DRAW_TEXT_OPTIONS_CLIP, DWRITE_MEASURING_MODE_NATURAL);
	}

	// --------------------------------- frame ---------------------------------

	void Renderer2D::RenderFrame(const FrameState_t& st, const Theme_t& th,
		const wchar_t* wszStatus, const wchar_t* wszVersion, const wchar_t* wszLang,
		const wchar_t* wszLaunch, const wchar_t* wszInject)
	{
		if (!m_bReady || !m_target)
			return;

		m_theme = th;

		// Semi-transparent layer on top of the backdrop: Acrylic shows through,
		// but the text stays readable. Without a backdrop (old OS) there is just
		// black emptiness under the frame - so an almost opaque background there.
		// In the fallback - an opaque background.
		const float flBgAlpha = m_bComposition ? (Glass::IsAvailable() ? 0.45f : 0.92f) : 1.0f;

		m_target->BeginDraw();
		m_target->SetTransform(D2D1::Matrix3x2F::Identity());
		m_target->Clear(ColorOf(th.background, flBgAlpha));

		DrawBackground(st);
		DrawHeader(st);
		DrawLogo(st);
		DrawModePill(st);
		DrawButtons(st, wszLaunch, wszInject);
		DrawProgress(st);
		DrawStatus(st, wszStatus);
		DrawFooter(st, wszVersion, wszLang);

		const HRESULT hr = m_target->EndDraw();

		if (hr == D2DERR_RECREATE_TARGET && !m_bComposition && m_hwndRT)
		{
			// Old path: re-create the target of the same type.
			ReleaseLegacy();

			if (CreateLegacyTarget())
			{
				m_target = static_cast<ID2D1RenderTarget*>(m_hwndRT);

				if (m_brush) { m_brush->Release(); m_brush = nullptr; }

				m_target->CreateSolidColorBrush(ColorOf(m_theme.textPrimary, 1.0f), &m_brush);
			}

			return;
		}

		if (FAILED(hr))
		{
			LogDbg(L"EndDraw failed", hr);

			if (m_bComposition)
				RecreateAfterDeviceLost(); // otherwise the frame freezes forever after a TDR

			return;
		}

		if (m_bComposition && m_swapChain)
		{
			const HRESULT hrPresent = m_swapChain->Present(1, 0);

			if (FAILED(hrPresent))
				LogDbg(L"Present failed", hrPresent);
		}
	}

	void Renderer2D::DrawBackground(const FrameState_t& st)
	{
		(void)st;
		// No solid fills here: the backdrop + Clear with alpha provide the background.
	}

	void Renderer2D::DrawHeader(const FrameState_t& st)
	{
		const float w = static_cast<float>(m_w);

		// Header - a semi-transparent surface: the glass shows through.
		// (header band removed: it looked like a pale stripe)

		const float pulse = 0.30f + 0.20f * (0.5f + 0.5f * sinf(st.elapsed * 1.4f));
		Line(0.0f, 66.0f, w, 66.0f, m_theme.accent, pulse, 1.0f);
	}

	void Renderer2D::DrawLogo(const FrameState_t& st)
	{
		(void)st;
		const float pulse = 0.9f + 0.1f * sinf(st.elapsed * 2.094f);
		float textX = 24.0f;

		if (m_logo)
		{
			const float size = 40.0f;
			m_target->DrawBitmap(m_logo, RectF(24.0f, 14.0f, 24.0f + size, 14.0f + size), 1.0f,
				D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
			textX = 24.0f + size + 10.0f;
		}

		if (!m_titleLayout)
			m_dwrite->CreateTextLayout(L"ZenWare.cc", 10, m_fmtTitle, 400.0f, 60.0f, &m_titleLayout);

		const float hue = TitleHue();
		bool bDrawn = false;

		if (m_titleLayout)
		{
			UINT32 nCount = 0;

			if (SUCCEEDED(m_titleLayout->GetClusterMetrics(nullptr, 0, &nCount)) && nCount > 0 && nCount <= 32)
			{
				DWRITE_CLUSTER_METRICS metrics[32]{};

				if (SUCCEEDED(m_titleLayout->GetClusterMetrics(metrics, 32, &nCount)))
				{
					const wchar_t* wszTitle = L"ZenWare.cc";
					float cx = textX;

					for (UINT32 i = 0; i < nCount; ++i)
					{
						wchar_t ch[2] = { wszTitle[i], 0 };
						Text(ch, RectF(cx, 14.0f, cx + metrics[i].width + 2.0f, 54.0f),
							Hsv2Rgb(hue + static_cast<float>(i) * 5.0f, 0.85f, 1.0f),
							DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_LEADING, pulse);
						cx += metrics[i].width;
					}

					bDrawn = true;
				}
			}
		}

		if (!bDrawn)
		{
			// Fallback: animated rainbow gradient brush, so the title is never plain white.
			if (!m_titleBrush && m_target)
			{
				D2D1_GRADIENT_STOP stops[6]{};
				const float hues[6] = { 0.0f, 60.0f, 120.0f, 180.0f, 240.0f, 300.0f };
				for (int i = 0; i < 6; ++i)
				{
					stops[i].position = static_cast<float>(i) / 5.0f;
					stops[i].color = ColorOf(Hsv2Rgb(hues[i], 0.85f, 1.0f), 1.0f);
				}
				ID2D1GradientStopCollection* coll = nullptr;
				if (SUCCEEDED(m_target->CreateGradientStopCollection(stops, 6, &coll)) && coll)
				{
					m_target->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(D2D1::Point2F(textX, 0.0f), D2D1::Point2F(textX + 150.0f, 0.0f)), coll, &m_titleBrush);
					coll->Release();
				}
			}

			if (m_titleBrush)
			{
				const float span = 150.0f;
				const float shift = fmodf(TitleHue() * 2.0f, span);
				m_titleBrush->SetStartPoint(D2D1::Point2F(textX - shift, 0.0f));
				m_titleBrush->SetEndPoint(D2D1::Point2F(textX - shift + span, 0.0f));

				IDWriteTextFormat* fmt = m_fmtTitle;
				fmt->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
				fmt->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
				m_target->DrawTextW(L"ZenWare.cc", 10, fmt, RectF(textX, 14.0f, textX + 300.0f, 54.0f), m_titleBrush, D2D1_DRAW_TEXT_OPTIONS_CLIP, DWRITE_MEASURING_MODE_NATURAL);
			}
			else
				Text(L"ZenWare.cc", RectF(textX, 14.0f, textX + 300.0f, 54.0f), m_theme.textPrimary, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_LEADING, pulse);
		}
	}

	void Renderer2D::DrawModePill(const FrameState_t& st)
	{
		const float w = static_cast<float>(m_w);
		const D2D1_RECT_F pill = RectF(w - 150.0f, 20.0f, w - 24.0f, 46.0f);

		FillRound(pill, 6.0f, m_theme.surface, 0.55f);
		StrokeRound(pill, 6.0f, m_theme.accentDim, 0.40f, 1.0f);

		Text(st.external ? L"EXTERNAL" : L"INTERNAL", pill, m_theme.textPrimary,
			DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_TEXT_ALIGNMENT_CENTER);
	}

	void Renderer2D::DrawButtons(const FrameState_t& st, const wchar_t* wszLaunch, const wchar_t* wszInject)
	{
		const float w = static_cast<float>(m_w);

		// Secondary button: the glass gets denser on hover.
		const D2D1_RECT_F b1 = RectF(24.0f, 92.0f, w - 24.0f, 128.0f);
		FillRound(b1, 8.0f, m_theme.surface, 0.55f + 0.15f * st.hoverLaunch);
		StrokeRound(b1, 8.0f, m_theme.border, 0.40f, 1.0f);
		Text((wszLaunch && wszLaunch[0]) ? wszLaunch : L"LAUNCH GAME", b1, m_theme.textPrimary,
			DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_TEXT_ALIGNMENT_CENTER);

		// Primary button: mint with dark text - readable even on glass.
		const D2D1_RECT_F b2 = RectF(24.0f, 136.0f, w - 24.0f, 188.0f);
		FillRound(b2, 8.0f, m_theme.accent, 0.85f);
		StrokeRound(b2, 8.0f, m_theme.accent, 1.0f, 1.0f);
		Text(st.external ? L"LAUNCH EXTERNAL" : L"INJECT", b2, m_theme.background,
			DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_CENTER);
	}

	void Renderer2D::DrawProgress(const FrameState_t& st)
	{
		const float w = static_cast<float>(m_w);
		const float trackY = 214.0f;
		const float trackW = w - 48.0f;

		FillRound(RectF(24.0f, trackY, 24.0f + trackW, trackY + 3.0f), 1.5f, m_theme.border, 0.40f);

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
			FillRound(RectF(x0, trackY, x1, trackY + 3.0f), 1.5f, m_theme.accent, 0.90f);
	}

	void Renderer2D::DrawStatus(const FrameState_t& st, const wchar_t* wszText)
	{
		const float pulse = st.busy ? (0.6f + 0.4f * (0.5f + 0.5f * sinf(st.elapsed * 4.18879f))) : 1.0f;

		FillRound(RectF(26.0f, 244.0f, 34.0f, 252.0f), 4.0f, m_theme.accent, pulse);

		if (wszText && wszText[0])
			Text(wszText, RectF(44.0f, 238.0f, static_cast<float>(m_w) - 24.0f, 258.0f),
				m_theme.textPrimary, DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_TEXT_ALIGNMENT_LEADING);
	}

	void Renderer2D::DrawFooter(const FrameState_t& st, const wchar_t* wszVersion, const wchar_t* wszLang)
	{
		(void)st;
		const float w = static_cast<float>(m_w);
		const float h = static_cast<float>(m_h);

		Line(24.0f, h - 44.0f, w - 24.0f, h - 44.0f, m_theme.border, 0.30f, 1.0f);

		if (wszVersion && wszVersion[0])
			Text(wszVersion, RectF(24.0f, h - 40.0f, 220.0f, h - 20.0f), m_theme.textSecondary,
				DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_TEXT_ALIGNMENT_LEADING, 0.85f);

		if (wszLang && wszLang[0])
			Text(wszLang, RectF(w - 220.0f, h - 40.0f, w - 24.0f, h - 20.0f), m_theme.textSecondary,
				DWRITE_FONT_WEIGHT_MEDIUM, DWRITE_TEXT_ALIGNMENT_TRAILING, 0.85f);
	}
}
