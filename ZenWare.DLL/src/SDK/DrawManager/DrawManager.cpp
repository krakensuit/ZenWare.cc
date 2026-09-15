#include "DrawManager.h"
#include "../../Util/Logger/Logger.h"

namespace {
	// Our literals are valid UTF-8 (/utf-8). Nicknames from the engine are ANSI.
	// Try strict UTF-8 first, otherwise fall back to the system code page.
	void ToWide(const char* src, wchar_t* dst, int dstLen)
	{
		if (!src || !src[0]) { if (dstLen > 0) dst[0] = L'\0'; return; }
		if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, src, -1, dst, dstLen) > 0)
			return;
		MultiByteToWideChar(CP_ACP, 0, src, -1, dst, dstLen);
	}
}

void CGlobal_DrawManager::Init()
{
	if (!I::MatSystemSurface)
		return;
		// Development and ESP faces stay on system fonts: they must always exist.
	m_Fonts[EFonts::DEBUG]      = { "Consolas", 16, FW_DONTCARE, EFontFlags::FONTFLAG_OUTLINE };
	m_Fonts[EFonts::ESP]        = { "Tahoma",   11, FW_DONTCARE, EFontFlags::FONTFLAG_OUTLINE };
	m_Fonts[EFonts::ESP_NAME]   = { "Arial",    14, FW_DONTCARE, EFontFlags::FONTFLAG_OUTLINE };
	m_Fonts[EFonts::ESP_WEAPON] = { "Verdana",  12, FW_DONTCARE, EFontFlags::FONTFLAG_OUTLINE };

	// Menu faces: Inter / JetBrains Mono / Font Awesome with antialiasing and a
	// soft drop shadow instead of the old outline, which made the menu look dated.
	const int nMenuFlags = EFontFlags::FONTFLAG_ANTIALIAS | EFontFlags::FONTFLAG_DROPSHADOW;

	m_Fonts[EFonts::MENU_BODY]   = { "Inter",               13, FW_MEDIUM,   nMenuFlags };
	m_Fonts[EFonts::MENU_SMALL]  = { "Inter",               11, FW_DONTCARE, nMenuFlags };
	m_Fonts[EFonts::MENU_HEADER] = { "Inter",               18, FW_SEMIBOLD, nMenuFlags };
	m_Fonts[EFonts::MENU_MONO]   = { "JetBrains Mono",      12, FW_DONTCARE, nMenuFlags };
	m_Fonts[EFonts::MENU_ICONS]  = { "Font Awesome 6 Free", 12, FW_DONTCARE, nMenuFlags };
	m_Fonts[EFonts::MENU_TAB]    = { "Inter",               30, FW_HEAVY,    nMenuFlags };

	// Fallback chain: if a family cannot be created, retry with system fonts so the
	// menu never renders without text.
	const char* aFallback[3] = { "Segoe UI Variable", "Segoe UI", "Tahoma" };

	for (std::pair<const EFonts, CFont>& f : m_Fonts)
	{
		f.second.m_hFont = I::MatSystemSurface->CreateFont();

		bool bOk = I::MatSystemSurface->SetFontGlyphSet(f.second.m_hFont, f.second.m_szName,
			f.second.m_nTall, f.second.m_nWeight, 0, 0, f.second.m_nFlags, 0, 0);

		if (!bOk)
		{
			for (int i = 0; i < 3 && !bOk; ++i)
			{
				bOk = I::MatSystemSurface->SetFontGlyphSet(f.second.m_hFont, aFallback[i],
					f.second.m_nTall, f.second.m_nWeight, 0, 0, f.second.m_nFlags, 0, 0);

				if (bOk)
					U::Log.Write("Draw: %s -> fallback %s", f.second.m_szName, aFallback[i]);
			}
		}

		U::Log.Write("Draw: font %s h=%d w=%d flags=0x%X hFont=%d %s",
			f.second.m_szName, f.second.m_nTall, f.second.m_nWeight, f.second.m_nFlags,
			static_cast<int>(f.second.m_hFont), bOk ? "ok" : "FAILED");
	}

	// Stage 5: the icon face must cover the Font Awesome private-use area. Without
	// an explicit range VGUI builds no glyphs for it, and every icon renders blank.
	{
		const CFont& icons = m_Fonts[EFonts::MENU_ICONS];

		if (icons.m_hFont)
		{
			const bool bIcons = I::MatSystemSurface->SetFontGlyphSet(icons.m_hFont, icons.m_szName,
				icons.m_nTall, icons.m_nWeight, 0, 0, icons.m_nFlags, 0xF000, 0xF8FF);

			U::Log.Write("Draw: icons range 0xF000-0xF8FF on %s -> %s", icons.m_szName, bIcons ? "ok" : "FAILED");
		}
	}
}

void CGlobal_DrawManager::String(const EFonts& font, int x, int y, const Color& clr, const short align, const char* const str, ...)
{
	//Lang::T returns nullptr on a nullptr input: vsprintf_s(0) crashes the detour.
	if (!str)
		return;
	va_list va_alist;
	char cbuffer[1024] = { '\0' };
	wchar_t wstr[1024] = { '\0' };

	va_start(va_alist, str);
	vsprintf_s(cbuffer, str, va_alist);
	va_end(va_alist);

	// Source literals are UTF-8 (/utf-8); convert explicitly: does not depend on the Windows locale.
	ToWide(cbuffer, wstr, 1024);

	//find instead of operator[]: a bad enum would otherwise insert HFont 0 into the map
	//and later calls would fly with a null font.
	const auto itF = m_Fonts.find(font);
	if (itF == m_Fonts.end() || !I::MatSystemSurface)
		return;
	const HFont fnt = itF->second.m_hFont;

	if (align)
	{
		int w = 0, h = 0;
		I::MatSystemSurface->GetTextSize(fnt, wstr, w, h);

		if (align & TXT_LEFT)
			x -= w;

		if (align & TXT_TOP)
			y -= h;

		if (align & TXT_CENTERX)
			x -= (w / 2);

		if (align & TXT_CENTERY)
			y -= (h / 2);
	}

	I::MatSystemSurface->DrawSetTextPos(x, y);
	I::MatSystemSurface->DrawSetTextFont(fnt);
	I::MatSystemSurface->DrawSetTextColor(clr);
	I::MatSystemSurface->DrawPrintText(wstr, wcslen(wstr));
}

void CGlobal_DrawManager::String(const EFonts& font, int x, int y, const Color& clr, const short align, const wchar_t* const str, ...)
{
	if (!str)
		return;
	va_list va_alist;
	wchar_t wstr[1024] = { '\0' };

	va_start(va_alist, str);
	vswprintf_s(wstr, str, va_alist);
	va_end(va_alist);

	const auto itF = m_Fonts.find(font);
	if (itF == m_Fonts.end() || !I::MatSystemSurface)
		return;
	const HFont fnt = itF->second.m_hFont;

	if (align)
	{
		int w = 0, h = 0;
		I::MatSystemSurface->GetTextSize(fnt, wstr, w, h);

		if (align & TXT_LEFT)
			x -= w;

		if (align & TXT_TOP)
			y -= h;

		if (align & TXT_CENTERX)
			x -= (w / 2);

		if (align & TXT_CENTERY)
			y -= (h / 2);
	}

	I::MatSystemSurface->DrawSetTextPos(x, y);
	I::MatSystemSurface->DrawSetTextFont(fnt);
	I::MatSystemSurface->DrawSetTextColor(clr);
	I::MatSystemSurface->DrawPrintText(wstr, wcslen(wstr));
}

void CGlobal_DrawManager::Line(const int x, const int y, const int x1, const int y1, const Color& clr)
{
	I::MatSystemSurface->DrawSetColor(clr);
	I::MatSystemSurface->DrawLine(x, y, x1, y1);
}

void CGlobal_DrawManager::Rect(const int x, const int y, const int w, const int h, const Color& clr)
{
	I::MatSystemSurface->DrawSetColor(clr);
	I::MatSystemSurface->DrawFilledRect(x, y, x + w, y + h);
}

void CGlobal_DrawManager::OutlinedRect(const int x, const int y, const int w, const int h, const Color& clr)
{
	I::MatSystemSurface->DrawSetColor(clr);
	I::MatSystemSurface->DrawOutlinedRect(x, y, x + w, y + h);
}

void CGlobal_DrawManager::GradientRect(const int x, const int y, const int x1, const int y1, const Color& clrTop, const Color& clrBottom, const bool bHorizontal)
{
	I::MatSystemSurface->DrawSetColor(clrTop);
	I::MatSystemSurface->DrawFilledRectFade(x, y, x1, y1, 255u, 255u, bHorizontal);

	I::MatSystemSurface->DrawSetColor(clrBottom);
	I::MatSystemSurface->DrawFilledRectFade(x, y, x1, y1, 0u, 255u, bHorizontal);
}

void CGlobal_DrawManager::OutlinedCircle(const int x, const int y, const int r, const int s, const Color clr)
{
	I::MatSystemSurface->DrawSetColor(clr);
	I::MatSystemSurface->DrawOutlinedCircle(x, y, r, s);
}

void CGlobal_DrawManager::Circle(const int x, const int y, const int r, const int s, const Color clr)
{
	//s==0: division by zero in flStep, DrawTexturedPolygon(0, ...).
	if (s <= 0)
		return;
	static int s_nTexture = I::MatSystemSurface->CreateNewTextureID(true);

	std::vector<Vertex_t> vecVertices = { };

	const float flStep = (6.28318530718f / static_cast<float>(s));

	for (float n = 0.0f; n < 6.28318530718f; n += flStep)
		vecVertices.push_back(Vertex_t({ (static_cast<float>(r) * ::cosf(n) + x), (static_cast<float>(r) * ::sinf(n) + y) }, { 0.0f, 0.0f }));

	if (!vecVertices.empty())
	{
		I::MatSystemSurface->DrawSetTexture(s_nTexture);
		I::MatSystemSurface->DrawSetColor(clr);
		I::MatSystemSurface->DrawTexturedPolygon(s, vecVertices.data(), true);
	}
}

int CGlobal_DrawManager::GetFontHeight(const EFonts& font) const
{
	//at() throws std::out_of_range across the detour boundary — fail-closed.
	const auto it = m_Fonts.find(font);
	return (it == m_Fonts.end()) ? 0 : it->second.m_nTall;
}

int CGlobal_DrawManager::GetTextWidth(const EFonts& font, const char* const str)
{
	if (!str || !str[0] || !I::MatSystemSurface)
		return 0;

	wchar_t wstr[1024] = { L'\0' };
	ToWide(str, wstr, 1024);

	int w = 0, h = 0;
	const auto itF = m_Fonts.find(font);
	if (itF == m_Fonts.end())
		return 0;
	I::MatSystemSurface->GetTextSize(itF->second.m_hFont, wstr, w, h);
	return w;
}

void CGlobal_DrawManager::Triangle(Vector2D* v, const Color clr)
{
	if (!v || !I::MatSystemSurface)
		return;
	static int s_nTexture = I::MatSystemSurface->CreateNewTextureID(true);

	Vertex_t Vertices[3] = { { v[0] }, { v[1] }, { v[2] } };

	I::MatSystemSurface->DrawSetTexture(s_nTexture);
	I::MatSystemSurface->DrawSetColor(clr);
	I::MatSystemSurface->DrawTexturedPolygon(3, Vertices, true);
}

void CGlobal_DrawManager::SetTexture(const int nTextureId)
{
	if (!I::MatSystemSurface)
		return;

	I::MatSystemSurface->DrawSetTexture(nTextureId);
}

void CGlobal_DrawManager::DrawTexturedRect(const int x, const int y, const int w, const int h)
{
	if (!I::MatSystemSurface)
		return;

	I::MatSystemSurface->DrawTexturedRect(x, y, x + w, y + h);
}

void CGlobal_DrawManager::ResetTexture()
{
	if (!I::MatSystemSurface)
		return;

	I::MatSystemSurface->DrawSetTexture(0);
}

void CGlobal_DrawManager::GetTextureSize(const int nTextureId, int& nWide, int& nTall)
{
	nWide = 0;
	nTall = 0;

	if (!I::MatSystemSurface)
		return;

	I::MatSystemSurface->DrawGetTextureSize(nTextureId, nWide, nTall);
}