#include "Utils.h"
#include "resource.h"

#include <tlhelp32.h>
#include <cstdarg>

namespace LoaderUtil
{
	bool g_bRuLang = false;
}

namespace
{
	HANDLE s_hFile = INVALID_HANDLE_VALUE;
	CRITICAL_SECTION s_csFile = { };
	bool s_bCsInit = false;

	void FileLock() { if (!s_bCsInit) { InitializeCriticalSection(&s_csFile); s_bCsInit = true; } EnterCriticalSection(&s_csFile); }
	void FileUnlock() { LeaveCriticalSection(&s_csFile); }

	void WriteFileLine(const char* const szLine)
	{
		if (s_hFile == INVALID_HANDLE_VALUE)
			return;

		DWORD dwWritten = 0;
		WriteFile(s_hFile, szLine, static_cast<DWORD>(strlen(szLine)), &dwWritten, nullptr);
		WriteFile(s_hFile, "\r\n", 2, &dwWritten, nullptr);
		FlushFileBuffers(s_hFile);
	}

	void PostPacket(const HWND hwndLog, const int nKind, const char* const szText)
	{
		if (!szText)
			return;

		auto* pPacket = new (std::nothrow) LoaderUtil::LogPacket_t();

		if (!pPacket)
			return;

		pPacket->nKind = nKind;

		const int nWritten = MultiByteToWideChar(CP_UTF8, 0, szText, -1,
			pPacket->wszText, static_cast<int>(sizeof(pPacket->wszText) / sizeof(wchar_t)) - 2);

		if (nWritten <= 0)
		{
			delete pPacket;
			return;
		}

		wcscat_s(pPacket->wszText, L"\r\n");

		if (hwndLog && IsWindow(hwndLog))
			PostMessageW(hwndLog, LoaderUtil::WM_APP_LOADER, static_cast<WPARAM>(nKind), reinterpret_cast<LPARAM>(pPacket));
		else
			delete pPacket; //window already gone: file log keeps the line anyway
	}
}

const char* LoaderUtil::S(const char* const szRu, const char* const szEn)
{
	return g_bRuLang ? szRu : szEn;
}

const wchar_t* LoaderUtil::SW(const wchar_t* wszRu, const wchar_t* wszEn)
{
	return g_bRuLang ? wszRu : wszEn;
}

void LoaderUtil::InitFileLog()
{
	FileLock();

	char szTemp[MAX_PATH] = { };

	if (GetTempPathA(MAX_PATH, szTemp) == 0)
		strcpy_s(szTemp, "C:\\");

	char szPath[MAX_PATH] = { };
	strcpy_s(szPath, szTemp);
	strcat_s(szPath, "ZenWare.Loader.log");

	s_hFile = CreateFileA(szPath, FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, 0, nullptr);
	SetFilePointer(s_hFile, 0, nullptr, FILE_END);

	char szStamp[16] = { };
	TimeStamp(szStamp, sizeof(szStamp));

	char szHeader[128] = { };
	sprintf_s(szHeader, "===== loader session started %s =====", szStamp);
	WriteFileLine(szHeader);

	FileUnlock();
}

DWORD LoaderUtil::FindProcessId(const wchar_t* const wszName)
{
	if (!wszName)
		return 0;

	const HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hSnapshot == INVALID_HANDLE_VALUE)
		return 0;

	PROCESSENTRY32W pe = { };
	pe.dwSize = sizeof(pe);

	DWORD dwPid = 0;

	if (Process32FirstW(hSnapshot, &pe))
	{
		do
		{
			if (_wcsicmp(pe.szExeFile, wszName) == 0)
			{
				dwPid = pe.th32ProcessID;
				break;
			}
		} while (Process32NextW(hSnapshot, &pe));
	}

	CloseHandle(hSnapshot);
	return dwPid;
}

void LoaderUtil::TimeStamp(char* szOut, const size_t nOutLen)
{
	SYSTEMTIME st = { };
	GetLocalTime(&st);
	sprintf_s(szOut, nOutLen, "[%02u:%02u:%02u.%03u]", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

void LoaderUtil::Log(const HWND hwndLog, const char* const szFormat, ...)
{
	char szBody[440] = { };

	va_list args = nullptr;
	va_start(args, szFormat);
	_vsnprintf_s(szBody, sizeof(szBody), _TRUNCATE, szFormat, args);
	va_end(args);

	char szStamp[16] = { };
	TimeStamp(szStamp, sizeof(szStamp));

	char szLine[512] = { };
	sprintf_s(szLine, "%s ", szStamp);
	strcat_s(szLine, szBody);

	FileLock();
	WriteFileLine(szLine);
	FileUnlock();

	PostPacket(hwndLog, KIND_LOG, szLine);
}

void LoaderUtil::Status(const HWND hwndLog, const char* const szText)
{
	Log(hwndLog, "--- %s ---", szText);
	PostPacket(hwndLog, KIND_STATUS, szText);
}

bool LoaderUtil::LoadResourceBytes(int nResId, std::vector<BYTE>& out)
{
	HRSRC hRes = FindResourceW(NULL, MAKEINTRESOURCEW(nResId), RT_RCDATA);
	if (!hRes) return false;
	HGLOBAL hMem = LoadResource(NULL, hRes);
	if (!hMem) return false;
	DWORD sz = SizeofResource(NULL, hRes);
	if (!sz) return false;
	const BYTE* p = (const BYTE*)LockResource(hMem);
	if (!p) return false;
	out.assign(p, p + sz);
	return !out.empty();
}

bool LoaderUtil::LoadDllFromResource(std::vector<BYTE>& out)
{
	if (!LoadResourceBytes(IDR_ZENWARE_DLL, out))
		return false;
	// de-xor if was xored (our build xors with 0x5A)
	// try detect: if first two bytes are not MZ, try xor
	if (out.size() >= 2 && !(out[0] == 'M' && out[1] == 'Z')) {
		for (auto &b : out) b ^= 0x5A;
	}
	return out.size() > 0 && out[0] == 'M' && out[1] == 'Z';
}

bool LoaderUtil::LoadExternalFromResource(std::vector<BYTE>& out)
{
	if (!LoadResourceBytes(IDR_ZENWARE_EXTERNAL, out))
		return false;
	return out.size() >= 2 && out[0] == 'M' && out[1] == 'Z';
}

bool LoaderUtil::LoadLogoFromResource(std::vector<BYTE>& out)
{
	static const BYTE kPngMagic[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
	if (!LoadResourceBytes(IDR_LOGO_PNG, out))
		return false;
	return out.size() > 8 && memcmp(out.data(), kPngMagic, sizeof(kPngMagic)) == 0;
}

bool LoaderUtil::WriteTempFile(const wchar_t* wszName, const std::vector<BYTE>& data, wchar_t* wszOutPath)
{
	if (!wszName || !wszName[0] || data.empty() || data.size() > 0x4000000)
		return false;

	wchar_t wszTemp[MAX_PATH] = { };
	if (!GetTempPathW(MAX_PATH, wszTemp))
		return false;

	for (int i = 0; i < 8; i++)
	{
		wchar_t wszFile[MAX_PATH] = { };
		if (i == 0)
		{
			wcscpy_s(wszFile, wszName);
		}
		else
		{
			// Locked (still mapped/running)? use a unique sibling name.
			const wchar_t* dot = wcsrchr(wszName, L'.');
			if (dot)
				swprintf_s(wszFile, L"%.*s_%lu_%lu%ls", (int)(dot - wszName), wszName, GetCurrentProcessId(), GetTickCount(), dot);
			else
				swprintf_s(wszFile, L"%ls_%lu_%lu", wszName, GetCurrentProcessId(), GetTickCount());
			if (i > 1) Sleep(5); // let the tick differ
		}

		wchar_t wszFull[MAX_PATH] = { };
		wcscpy_s(wszFull, wszTemp);
		wcscat_s(wszFull, wszFile);

		HANDLE h = CreateFileW(wszFull, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (h == INVALID_HANDLE_VALUE)
			continue;

		DWORD dwWritten = 0;
		const BOOL bOk = WriteFile(h, data.data(), (DWORD)data.size(), &dwWritten, nullptr);
		CloseHandle(h);

		if (!bOk || dwWritten != (DWORD)data.size())
		{
			DeleteFileW(wszFull);
			continue;
		}

		if (wszOutPath)
			wcscpy_s(wszOutPath, MAX_PATH, wszFull);
		return true;
	}
	return false;
}

void LoaderUtil::CleanupOldTempExtracts()
{
	wchar_t wszTemp[MAX_PATH] = { };
	if (!GetTempPathW(MAX_PATH, wszTemp))
		return;

	static const wchar_t* kPatterns[] = { L"ZenWare_*.dll", L"ZenWare.External_*.exe" };
	for (auto pat : kPatterns)
	{
		wchar_t wszMask[MAX_PATH] = { };
		wcscpy_s(wszMask, wszTemp);
		wcscat_s(wszMask, pat);

		WIN32_FIND_DATAW ff = { };
		HANDLE hFind = FindFirstFileW(wszMask, &ff);
		if (hFind == INVALID_HANDLE_VALUE)
			continue;
		do
		{
			wchar_t wszFull[MAX_PATH] = { };
			wcscpy_s(wszFull, wszTemp);
			wcscat_s(wszFull, ff.cFileName);
			DeleteFileW(wszFull); // best effort: locked files just stay
		} while (FindNextFileW(hFind, &ff));
		FindClose(hFind);
	}
}
