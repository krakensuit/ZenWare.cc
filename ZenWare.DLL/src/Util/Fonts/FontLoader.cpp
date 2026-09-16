#include "FontLoader.h"

#include "../Logger/Logger.h"

// Fonts live in this module's resources; GetModuleHandleW(nullptr) returns the
// module that owns them (the DLL itself once injected).
namespace
{
	constexpr int kMaxFonts = 8;

	HANDLE g_hFonts[kMaxFonts] = { };
	int g_nFonts = 0;
}

namespace Fonts
{
	HANDLE Load(int nResourceId, const char* szName)
	{
		if (g_nFonts >= kMaxFonts)
		{
			U::Log.Write("Fonts: table full, skipping %s", szName ? szName : "?");
			return nullptr;
		}

		const HMODULE hModule = GetModuleHandleW(nullptr);

		if (!hModule)
			return nullptr;

		const HRSRC hRes = FindResourceW(hModule, MAKEINTRESOURCEW(nResourceId), MAKEINTRESOURCEW(10) /*RT_RCDATA*/);

		if (!hRes)
		{
			// Normal state until the .ttf files are added to the resources.
			U::Log.Write("Fonts: resource %d (%s) not found, fallback will be used",
				nResourceId, szName ? szName : "?");
			return nullptr;
		}

		const DWORD dwSize = SizeofResource(hModule, hRes);

		if (dwSize == 0)
		{
			U::Log.Write("Fonts: resource %d (%s) is empty", nResourceId, szName ? szName : "?");
			return nullptr;
		}

		const HGLOBAL hMem = LoadResource(hModule, hRes);
		const void* pData = hMem ? LockResource(hMem) : nullptr;

		if (!pData)
		{
			U::Log.Write("Fonts: cannot lock resource %d (%s)", nResourceId, szName ? szName : "?");
			return nullptr;
		}

		DWORD nFonts = 0;
		HANDLE hFont = AddFontMemResourceEx(const_cast<void*>(pData), dwSize, nullptr, &nFonts);

		if (!hFont || nFonts == 0)
		{
			U::Log.Write("Fonts: AddFontMemResourceEx failed for %s (resource %d)",
				szName ? szName : "?", nResourceId);
			return nullptr;
		}

		g_hFonts[g_nFonts++] = hFont;

		U::Log.Write("Fonts: %s registered from resource %d (%lu bytes, %lu face(s))",
			szName ? szName : "?", nResourceId, static_cast<unsigned long>(dwSize),
			static_cast<unsigned long>(nFonts));

		return hFont;
	}

	void LoadAll()
	{
		Load(INTER_REGULAR, "Inter Regular");
		Load(INTER_MEDIUM, "Inter Medium");
		Load(INTER_SEMIBOLD, "Inter SemiBold");
		Load(JETBRAINS_MONO, "JetBrains Mono Regular");
		Load(FONT_AWESOME6_SOLID, "Font Awesome 6 Free Solid");
	}

	void FreeAll()
	{
		for (int i = 0; i < g_nFonts; ++i)
		{
			RemoveFontMemResourceEx(g_hFonts[i]);
			g_hFonts[i] = nullptr;
		}

		if (g_nFonts > 0)
			U::Log.Write("Fonts: %d custom face(s) released", g_nFonts);

		g_nFonts = 0;
	}

	int Count()
	{
		return g_nFonts;
	}
}
