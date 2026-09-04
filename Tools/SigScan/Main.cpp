// SigScan - verifies pattern signatures against game binaries on disk.
// Usage: SigScan.exe <path-to-client.dll> <path-to-engine.dll> <path-to-vguimatsurface.dll>

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

struct Pattern_t
{
	const char* m_szName;
	const char* m_szModule;
	const char* m_szPattern;
	int m_nAdjust; //offset added by Lak3 after the match (-1 = report raw match)
};

//Exactly the patterns used by ZenWare.DLL /src/Util/Offsets/Offsets.cpp.
static Pattern_t g_aPatterns[] =
{
	{ "SharedRandomFloat",       "client",          "55 8B EC 83 EC 08 A1 ? ? ? ? 53 56 57 8B 7D 14 8D 4D 14 51 89 7D F8 89 45 FC E8 ? ? ? ? 6A 04 8D 55 FC 52 8D 45 14 50 E8 ? ? ? ? 6A 04 8D 4D F8 51 8D 55 14 52 E8 ? ? ? ? 8B 75 08 56 E8 ? ? ? ? 50 8D 45 14 56 50 E8 ? ? ? ? 8D 4D 14 51 E8 ? ? ? ? 8B 15 ? ? ? ? 8B 5D 14 83 C4 30 83 7A 30 00 74 26 57 53 56 68 68", -1 },
	{ "CheckForSequenceChange",  "client",          "55 8B EC 83 7D 08 00 56 8B F1 0F 84 ? ? ? ? 83 7E 0C 00 75 10 6A 00 E8 ? ? ? ? 8B 0E 6A 00 E8 ? ? ? ?", -1 },
	{ "CalcPlayerView",          "client",          "55 8B EC 83 EC 1C 53 56 8B F1 8B 0D ? ? ? ? 8B 01 8B 50 38 57 FF D2 84 C0 75 0D", -1 },
	{ "UpdateSpread",            "client",          "53 8B DC 83 EC 08 83 E4 F0 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 83 EC 28 56 57 8B F9 E8 ? ? ? ? 8B CF 89 45 F0 E8 ? ? ? ? 8B F0 85 F6 75 1B", -1 },
	{ "DrawModels",              "client",          "55 8B EC 83 EC 74 A1 ? ? ? ? 33 C5 89 45 FC 8B 45 08 53 56 57 8B 7D 0C 33 F6 8B D9 89 5D CC 89 45 D0 89 7D D4 3B FE 0F 84 ? ? ? ?", -1 },
	{ "AvoidPlayers",            "client",          "53 8B DC 83 EC 08 83 E4 F0 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 45 FC 8B 43 08 56 57 8B F9 80 BF ? ? ? ? ? 89 45 90 0F 85 ? ? ? ?", -1 },
	{ "PhysicsRunThink",         "client",          "55 8B EC 53 56 8B F1 8B 86 ? ? ? ? C1 E8 16 A8 01 57 B0 01 0F 85 ? ? ? ?", -1 },
	{ "SetPredictionRandomSeed", "client",          "55 8B EC 8B 45 08 85 C0 75 0C C7 05 ? ? ? ? ? ? ? ? 5D", -1 },
	{ "GetSurvivorSet",          "client",          "55 8B EC 51 8B 0D ? ? ? ? 8B 01 8B 50 28 53 56 BB ? ? ? ? FF D2 8B 10 8B C8 8B 42 04 6A 00 FF D0 8B F0 85 F6 74 45", -1 },
	{ "CL_Move",                 "engine",          "55 8B EC 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 45 FC 56 E8 ? ? ? ? 8B F0 83 7E 68 02 0F 8C", -1 },
	{ "ClientMode(ptr)",         "client",          "89 04 B5 ? ? ? ? E8", 3 },
	{ "GlobalVars(ptr)",         "client",          "A1 ? ? ? ? D9 40 0C 51 D9 1C 24 57", 1 },
	{ "MoveHelper(ptr)",         "client",          "8B 0D ? ? ? ? 8B 11 8B 52 34", 2 },
	{ "StartDrawing",            "vguimatsurface",  "33 C5 50 8D 45 F4 64 A3 ? ? ? ? 8B F9 80 3D", -19 },
	{ "FinishDrawing",           "vguimatsurface",  "51 56 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 ? ? ? ? 6A", -17 },
};

static bool ReadFileToBuffer(const char* szPath, std::vector<BYTE>& vecOut)
{
	std::ifstream f(szPath, std::ios::binary | std::ios::ate);

	if (!f)
		return false;

	const std::streampos nSize = f.tellg();
	f.seekg(0);
	vecOut.resize(static_cast<size_t>(nSize));
	f.read(reinterpret_cast<char*>(vecOut.data()), vecOut.size());

	return static_cast<bool>(f);
}

//Parses "AA BB ? ? CC" into byte array + mask string ('x'/'?').
static bool ParsePattern(const char* szPattern, std::vector<BYTE>& vecBytes, std::string& strMask)
{
	int nVal = -1;
	char szByte[3] = { };

	while (*szPattern)
	{
		if (*szPattern == ' ')
		{
			szPattern++;
			continue;
		}

		if (*szPattern == '?')
		{
			vecBytes.push_back(0);
			strMask.push_back('?');

			while (*szPattern == '?')
				szPattern++;

			continue;
		}

		if (sscanf_s(szPattern, "%2x", &nVal) != 1)
			return false;

		vecBytes.push_back(static_cast<BYTE>(nVal));
		strMask.push_back('x');
		szPattern += 2;
	}

	return !vecBytes.empty();
}

//Counts matches; returns first match index or npos.
static size_t Scan(const std::vector<BYTE>& vecData, const std::vector<BYTE>& vecBytes, const std::string& strMask, size_t& nMatches)
{
	constexpr size_t NPOS = static_cast<size_t>(-1);
	nMatches = 0;
	size_t nFirst = NPOS;

	if (vecBytes.empty() || strMask.length() > vecData.size())
		return NPOS;

	const size_t nScanLen = vecData.size() - strMask.length();

	for (size_t i = 0; i <= nScanLen; i++)
	{
		bool bMatch = true;

		for (size_t j = 0; j < strMask.length(); j++)
		{
			if (strMask[j] == 'x' && vecData[i + j] != vecBytes[j])
			{
				bMatch = false;
				break;
			}
		}

		if (bMatch)
		{
			if (nFirst == NPOS)
				nFirst = i;

			nMatches++;
		}
	}

	return nFirst;
}

//File offset -> RVA using the section table.
static DWORD OffsetToRva(const std::vector<BYTE>& vecData, const DWORD dwOffset)
{
	const auto pDos = reinterpret_cast<const IMAGE_DOS_HEADER*>(vecData.data());
	const auto pNt = reinterpret_cast<const IMAGE_NT_HEADERS*>(vecData.data() + pDos->e_lfanew);
	const auto pSection = reinterpret_cast<const IMAGE_SECTION_HEADER*>(reinterpret_cast<const BYTE*>(&pNt->OptionalHeader) + pNt->FileHeader.SizeOfOptionalHeader);

	for (WORD i = 0; i < pNt->FileHeader.NumberOfSections; i++)
	{
		const IMAGE_SECTION_HEADER& sect = pSection[i];

		if (dwOffset >= sect.PointerToRawData && dwOffset < (sect.PointerToRawData + sect.SizeOfRawData))
			return (dwOffset - sect.PointerToRawData) + sect.VirtualAddress;
	}

	return dwOffset;
}

int main(const int nArgc, char* szArgv[])
{
	if (nArgc < 4)
	{
		printf("Usage: SigScan.exe <client.dll> <engine.dll> <vguimatsurface.dll>\n");
		return 1;
	}

	std::vector<std::vector<BYTE>> vecModules(3);

	for (int i = 0; i < 3; i++)
	{
		if (!ReadFileToBuffer(szArgv[i + 1], vecModules[i]))
		{
			printf("[!] Failed to read %s\n", szArgv[i + 1]);
			return 1;
		}
	}

	printf("=== SigScan verification (%d patterns) ===\n\n", static_cast<int>(sizeof(g_aPatterns) / sizeof(g_aPatterns[0])));

	int nOk = 0, nAmbiguous = 0, nFail = 0;

	for (const Pattern_t& pat : g_aPatterns)
	{
		std::vector<BYTE> vecBytes;
		std::string strMask;

		if (!ParsePattern(pat.m_szPattern, vecBytes, strMask))
		{
			printf("%-24s [PARSE ERROR]\n", pat.m_szName);
			nFail++;
			continue;
		}

		const int nModuleIdx = (strcmp(pat.m_szModule, "engine") == 0) ? 1 : ((strcmp(pat.m_szModule, "vguimatsurface") == 0) ? 2 : 0);

		size_t nMatches = 0;
		const size_t nFound = Scan(vecModules[nModuleIdx], vecBytes, strMask, nMatches);

		if (nFound == static_cast<size_t>(-1))
		{
			printf("%-24s [%s] NOT FOUND\n", pat.m_szName, pat.m_szModule);
			nFail++;
			continue;
		}

		const DWORD dwRva = OffsetToRva(vecModules[nModuleIdx], static_cast<DWORD>(nFound)) + pat.m_nAdjust;

		if (nMatches > 1)
		{
			printf("%-24s [%s] AMBIGUOUS: %zu matches! first -> base+0x%08X\n", pat.m_szName, pat.m_szModule, nMatches, dwRva);
			nAmbiguous++;
		}
		else
		{
			printf("%-24s [%s] OK -> base+0x%08X\n", pat.m_szName, pat.m_szModule, dwRva);
			nOk++;
		}
	}

	printf("\nResult: %d ok, %d ambiguous, %d failed\n", nOk, nAmbiguous, nFail);
	return (nFail || nAmbiguous) ? 2 : 0;
}

