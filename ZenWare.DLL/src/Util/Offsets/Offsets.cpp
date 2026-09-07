#include "Offsets.h"

#include "../Logger/Logger.h"

#define LOG_PATTERN(name, addr) U::Log.Write("    %-24s : 0x%08X %s", name, (addr), ((addr) != 0x0) ? "(ok)" : "(NOT FOUND!)")

namespace
{
	// Кэш ресолва: <gamedir>\ZenWare.offsets. Формат:
	//   # ZenWare offsets cache v1
	//   mod client.dll <identity>
	//   rva SharedRandomFloat=00A1B2C0
	// Identity = путь|время записи|размер: после обновы игры кэш молча
	// инвалидируется и идёт полный перескан.
	bool CacheGameDir(char* szOut, size_t nOut)
	{
		HMODULE hClient = GetModuleHandleA("client.dll");
		if (!hClient)
			return false;
		char sz[MAX_PATH] = { };
		if (!GetModuleFileNameA(hClient, sz, MAX_PATH))
			return false;
		char* pSlash = strrchr(sz, '\\');
		if (!pSlash)
			return false;
		*pSlash = '\0';
		pSlash = strrchr(sz, '\\');
		if (pSlash && !_stricmp(pSlash, "\\bin"))
			*pSlash = '\0';
		strcpy_s(szOut, nOut, sz);
		return true;
	}

	bool ModuleIdentity(const char* szModule, char* szKeyOut, size_t nOut)
	{
		HMODULE h = GetModuleHandleA(szModule);
		if (!h)
			return false;
		char szPath[MAX_PATH] = { };
		if (!GetModuleFileNameA(h, szPath, MAX_PATH))
			return false;
		WIN32_FILE_ATTRIBUTE_DATA fad = { };
		if (!GetFileAttributesExA(szPath, GetFileExInfoStandard, &fad))
			return false;
		sprintf_s(szKeyOut, nOut, "%s|%08X%08X|%08X%08X", szPath,
			fad.ftLastWriteTime.dwHighDateTime, fad.ftLastWriteTime.dwLowDateTime,
			fad.nFileSizeHigh, fad.nFileSizeLow);
		return true;
	}

	void CachePath(char* szOut, size_t nOut)
	{
		szOut[0] = '\0';
		char szDir[MAX_PATH] = { };
		if (!CacheGameDir(szDir, sizeof(szDir)))
			return;
		sprintf_s(szOut, nOut, "%s\\ZenWare.offsets", szDir);
	}

	bool TryLoadCache(CUtil_Offsets& o)
	{
		char szPath[MAX_PATH] = { };
		CachePath(szPath, sizeof(szPath));
		if (!szPath[0])
			return false;

		FILE* f = nullptr;
		if (fopen_s(&f, szPath, "r") != 0 || !f)
			return false;

		// Сначала identity трёх модулей, потом RVA.
		char szWant[3][MAX_PATH] = { };
		static const char* kMods[] = { "client.dll", "engine.dll", "vguimatsurface.dll" };
		for (int i = 0; i < 3; i++)
			if (!ModuleIdentity(kMods[i], szWant[i], sizeof(szWant[i]))) { fclose(f); return false; }

		struct Got_t { const char* m_szName; DWORD m_dwRva; bool m_bGot; };
		static const char* kNames[] = {
			"SharedRandomFloat", "CheckForSequenceChange", "CalcPlayerView",
			"UpdateSpread", "DrawModels", "AvoidPlayers", "PhysicsRunThink",
			"SetPredictionRandomSeed", "GetSurvivorSet", "CL_Move",
			"ClientMode", "GlobalVars", "MoveHelper", "StartDrawing", "FinishDrawing"
		};
		Got_t aGot[15] = { };
		for (int i = 0; i < 15; i++) aGot[i].m_szName = kNames[i];

		int nModsOk = 0;
		char szLine[512] = { };
		while (fgets(szLine, sizeof(szLine), f))
		{
			if (szLine[0] == '#' || szLine[0] == '\n' || szLine[0] == '\r')
				continue;
			if (!strncmp(szLine, "mod ", 4))
			{
				// "mod <module> <identity...>": в identity есть пробелы (путь),
				// поэтому делим по ПЕРВОМУ пробелу вручную, а не sscanf.
				char* pSp = strchr(szLine + 4, ' ');
				if (!pSp)
					continue;
				*pSp = '\0';
				const char* szMod = szLine + 4;
				char* szId = pSp + 1;
				szId[strcspn(szId, "\r\n")] = '\0';
				for (int i = 0; i < 3; i++)
					if (!_stricmp(szMod, kMods[i]) && !strcmp(szId, szWant[i]))
						nModsOk++;
				continue;
			}
			if (!strncmp(szLine, "rva ", 4))
			{
				char szName[64] = { };
				unsigned int uRva = 0;
				if (sscanf_s(szLine + 4, "%63[^=]=%x", szName, (unsigned)sizeof(szName), &uRva) != 2)
					continue;
				for (int i = 0; i < 15; i++)
					if (!strcmp(szName, aGot[i].m_szName)) { aGot[i].m_dwRva = (DWORD)uRva; aGot[i].m_bGot = true; }
			}
		}
		fclose(f);

		if (nModsOk < 3)
			return false;
		for (int i = 0; i < 15; i++)
			if (!aGot[i].m_bGot || !aGot[i].m_dwRva)
				return false;

		static const char* kEntMod[] = {
			"client.dll", "client.dll", "client.dll",
			"client.dll", "client.dll", "client.dll", "client.dll",
			"client.dll", "client.dll", "engine.dll",
			"client.dll", "client.dll", "client.dll", "vguimatsurface.dll", "vguimatsurface.dll"
		};
		DWORD* ppDst[] = {
			&o.m_dwSharedRandomFloat, &o.m_dwCheckForSequenceChange, &o.m_dwCalcPlayerView,
			&o.m_dwUpdateSpread, &o.m_dwDrawModels, &o.m_dwAvoidPlayers, &o.m_dwPhysicsRunThink,
			&o.m_dwSetPredictionRandomSeed, &o.m_dwGetSurvivorSet, &o.m_dwCLMove,
			&o.m_dwClientMode, &o.m_dwGlobalVars, &o.m_dwMoveHelper, &o.m_dwStartDrawing, &o.m_dwFinishDrawing
		};
		for (int i = 0; i < 15; i++)
		{
			HMODULE h = GetModuleHandleA(kEntMod[i]);
			if (!h)
				return false;
			*ppDst[i] = (DWORD)h + aGot[i].m_dwRva;
		}
		if (o.m_dwSharedRandomFloat)
			o.m_dwRandomSeed = (o.m_dwSharedRandomFloat + 0x7);
		return true;
	}

	void SaveCache(CUtil_Offsets& o)
	{
		char szPath[MAX_PATH] = { };
		CachePath(szPath, sizeof(szPath));
		if (!szPath[0])
			return;

		FILE* f = nullptr;
		if (fopen_s(&f, szPath, "w") != 0 || !f)
			return;

		fprintf(f, "# ZenWare offsets cache v1 (auto, delete to force rescan)\n");
		static const char* kMods[] = { "client.dll", "engine.dll", "vguimatsurface.dll" };
		for (int i = 0; i < 3; i++)
		{
			char szId[MAX_PATH] = { };
			if (ModuleIdentity(kMods[i], szId, sizeof(szId)))
				fprintf(f, "mod %s %s\n", kMods[i], szId);
		}

		struct Ent_t { const char* m_szName; const char* m_szModule; DWORD m_dwAbs; };
		const Ent_t aEnts[] = {
			{ "SharedRandomFloat", "client.dll", o.m_dwSharedRandomFloat },
			{ "CheckForSequenceChange", "client.dll", o.m_dwCheckForSequenceChange },
			{ "CalcPlayerView", "client.dll", o.m_dwCalcPlayerView },
			{ "UpdateSpread", "client.dll", o.m_dwUpdateSpread },
			{ "DrawModels", "client.dll", o.m_dwDrawModels },
			{ "AvoidPlayers", "client.dll", o.m_dwAvoidPlayers },
			{ "PhysicsRunThink", "client.dll", o.m_dwPhysicsRunThink },
			{ "SetPredictionRandomSeed", "client.dll", o.m_dwSetPredictionRandomSeed },
			{ "GetSurvivorSet", "client.dll", o.m_dwGetSurvivorSet },
			{ "CL_Move", "engine.dll", o.m_dwCLMove },
			{ "ClientMode", "client.dll", o.m_dwClientMode },
			{ "GlobalVars", "client.dll", o.m_dwGlobalVars },
			{ "MoveHelper", "client.dll", o.m_dwMoveHelper },
			{ "StartDrawing", "vguimatsurface.dll", o.m_dwStartDrawing },
			{ "FinishDrawing", "vguimatsurface.dll", o.m_dwFinishDrawing },
		};
		for (size_t i = 0; i < sizeof(aEnts) / sizeof(aEnts[0]); i++)
		{
			if (!aEnts[i].m_dwAbs)
				continue;
			HMODULE h = GetModuleHandleA(aEnts[i].m_szModule);
			if (!h || aEnts[i].m_dwAbs < (DWORD)h)
				continue;
			fprintf(f, "rva %s=%08X\n", aEnts[i].m_szName, aEnts[i].m_dwAbs - (DWORD)h);
		}
		fclose(f);
	}
}

void CUtil_Offsets::Init()
{
	// Быстрый путь: модули не менялись — берём RVA из кэша без скана.
	if (TryLoadCache(*this))
	{
		m_bCacheUsed = true;
		U::Log.Write("    offsets from cache (ZenWare.offsets), scan skipped");
		LOG_PATTERN("SharedRandomFloat", m_dwSharedRandomFloat);
		LOG_PATTERN("CalcPlayerView", m_dwCalcPlayerView);
		LOG_PATTERN("UpdateSpread", m_dwUpdateSpread);
		LOG_PATTERN("DrawModels", m_dwDrawModels);
		LOG_PATTERN("AvoidPlayers", m_dwAvoidPlayers);
		LOG_PATTERN("PhysicsRunThink", m_dwPhysicsRunThink);
		LOG_PATTERN("SetPredictionRandomSeed", m_dwSetPredictionRandomSeed);
		LOG_PATTERN("GetSurvivorSet", m_dwGetSurvivorSet);
		LOG_PATTERN("CL_Move", m_dwCLMove);
		LOG_PATTERN("ClientMode(ptr)", m_dwClientMode);
		LOG_PATTERN("GlobalVars(ptr)", m_dwGlobalVars);
		LOG_PATTERN("MoveHelper(ptr)", m_dwMoveHelper);
		LOG_PATTERN("RandomSeed(derived)", m_dwRandomSeed);
		LOG_PATTERN("StartDrawing", m_dwStartDrawing);
		LOG_PATTERN("FinishDrawing", m_dwFinishDrawing);
		return;
	}
	m_dwSharedRandomFloat = U::Pattern.Find(_("client.dll"), _("55 8B EC 83 EC 08 A1 ? ? ? ? 53 56 57 8B 7D 14 8D 4D 14 51 89 7D F8 89 45 FC E8 ? ? ? ? 6A 04 8D 55 FC 52 8D 45 14 50 E8 ? ? ? ? 6A 04 8D 4D F8 51 8D 55 14 52 E8 ? ? ? ? 8B 75 08 56 E8 ? ? ? ? 50 8D 45 14 56 50 E8 ? ? ? ? 8D 4D 14 51 E8 ? ? ? ? 8B 15 ? ? ? ? 8B 5D 14 83 C4 30 83 7A 30 00 74 26 57 53 56 68 68"));
	XASSERT(m_dwSharedRandomFloat == 0x0);

	m_dwCheckForSequenceChange = U::Pattern.Find(_("client.dll"), _("55 8B EC 83 7D 08 00 56 8B F1 0F 84 ? ? ? ? 83 7E 0C 00 75 10 6A 00 E8 ? ? ? ? 8B 0E 6A 00 E8 ? ? ? ?"));
	XASSERT(m_dwCheckForSequenceChange == 0x0);
	U::Log.Write("    %-24s : 0x%08X (AMBIGUOUS on build 23990068 - hook disabled, see Hooks.cpp)", "CheckForSequenceChange", m_dwCheckForSequenceChange);

	m_dwCalcPlayerView = U::Pattern.Find(_("client.dll"), _("55 8B EC 83 EC 1C 53 56 8B F1 8B 0D ? ? ? ? 8B 01 8B 50 38 57 FF D2 84 C0 75 0D"));
	XASSERT(m_dwCalcPlayerView == 0x0);

	m_dwUpdateSpread = U::Pattern.Find(_("client.dll"), _("53 8B DC 83 EC 08 83 E4 F0 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 83 EC 28 56 57 8B F9 E8 ? ? ? ? 8B CF 89 45 F0 E8 ? ? ? ? 8B F0 85 F6 75 1B"));
	XASSERT(m_dwUpdateSpread == 0x0);

	m_dwDrawModels = U::Pattern.Find(_("client.dll"), _("55 8B EC 83 EC 74 A1 ? ? ? ? 33 C5 89 45 FC 8B 45 08 53 56 57 8B 7D 0C 33 F6 8B D9 89 5D CC 89 45 D0 89 7D D4 3B FE 0F 84 ? ? ? ?"));
	XASSERT(m_dwDrawModels == 0x0);

	m_dwAvoidPlayers = U::Pattern.Find(_("client.dll"), _("53 8B DC 83 EC 08 83 E4 F0 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 45 FC 8B 43 08 56 57 8B F9 80 BF ? ? ? ? ? 89 45 90 0F 85 ? ? ? ?"));
	XASSERT(m_dwAvoidPlayers == 0x0);

	m_dwPhysicsRunThink = U::Pattern.Find(_("client.dll"), _("55 8B EC 53 56 8B F1 8B 86 ? ? ? ? C1 E8 16 A8 01 57 B0 01 0F 85 ? ? ? ?"));
	XASSERT(m_dwPhysicsRunThink == 0x0);

	m_dwSetPredictionRandomSeed = U::Pattern.Find(_("client.dll"), _("55 8B EC 8B 45 08 85 C0 75 0C C7 05 ? ? ? ? ? ? ? ? 5D"));
	XASSERT(m_dwSetPredictionRandomSeed == 0x0);

	m_dwGetSurvivorSet = U::Pattern.Find(_("client.dll"), _("55 8B EC 51 8B 0D ? ? ? ? 8B 01 8B 50 28 53 56 BB ? ? ? ? FF D2 8B 10 8B C8 8B 42 04 6A 00 FF D0 8B F0 85 F6 74 45"));
	XASSERT(m_dwGetSurvivorSet == 0x0);

	m_dwCLMove = U::Pattern.Find(_("engine.dll"), _("55 8B EC 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 45 FC 56 E8 ? ? ? ? 8B F0 83 7E 68 02 0F 8C"));
	XASSERT(m_dwCLMove == 0x0);

	if (const DWORD dwClientMode = U::Pattern.Find(_("client.dll"), _("89 04 B5 ? ? ? ? E8")))
		m_dwClientMode = (dwClientMode + 0x3);

	if (const DWORD dwGlobalVars = U::Pattern.Find(_("client.dll"), _("A1 ? ? ? ? D9 40 0C 51 D9 1C 24 57")))
		m_dwGlobalVars = (dwGlobalVars + 0x1);

	if (const DWORD dwMoveHelper = U::Pattern.Find(_("client.dll"), _("8B 0D ? ? ? ? 8B 11 8B 52 34")))
		m_dwMoveHelper = (dwMoveHelper + 0x2);

	if (const DWORD dwStartDrawing = U::Pattern.Find(_("vguimatsurface.dll"), _("33 C5 50 8D 45 F4 64 A3 ? ? ? ? 8B F9 80 3D")))
		m_dwStartDrawing = (dwStartDrawing - 0x1B);

	if (const DWORD dwFinishDrawing = U::Pattern.Find(_("vguimatsurface.dll"), _("51 56 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 ? ? ? ? 6A")))
		m_dwFinishDrawing = (dwFinishDrawing - 0x11);

	if (m_dwSharedRandomFloat)
		m_dwRandomSeed = (m_dwSharedRandomFloat + 0x7);

	// Полный скан прошёл — сохраняем RVA в кэш для следующего старта.
	SaveCache(*this);

	//Per-pattern results for the log file.
	LOG_PATTERN("SharedRandomFloat", m_dwSharedRandomFloat);
	LOG_PATTERN("CalcPlayerView", m_dwCalcPlayerView);
	LOG_PATTERN("UpdateSpread", m_dwUpdateSpread);
	LOG_PATTERN("DrawModels", m_dwDrawModels);
	LOG_PATTERN("AvoidPlayers", m_dwAvoidPlayers);
	LOG_PATTERN("PhysicsRunThink", m_dwPhysicsRunThink);
	LOG_PATTERN("SetPredictionRandomSeed", m_dwSetPredictionRandomSeed);
	LOG_PATTERN("GetSurvivorSet", m_dwGetSurvivorSet);
	LOG_PATTERN("CL_Move", m_dwCLMove);
	LOG_PATTERN("ClientMode(ptr)", m_dwClientMode);
	LOG_PATTERN("GlobalVars(ptr)", m_dwGlobalVars);
	LOG_PATTERN("MoveHelper(ptr)", m_dwMoveHelper);
	LOG_PATTERN("RandomSeed(derived)", m_dwRandomSeed);
	LOG_PATTERN("StartDrawing", m_dwStartDrawing);
	LOG_PATTERN("FinishDrawing", m_dwFinishDrawing);

	XASSERT(m_dwStartDrawing == 0x0);
	XASSERT(m_dwFinishDrawing == 0x0);
	XASSERT(m_dwClientMode == 0x0);
	XASSERT(m_dwGlobalVars == 0x0);
	XASSERT(m_dwMoveHelper == 0x0);
	XASSERT(m_dwRandomSeed == 0x0);
}
