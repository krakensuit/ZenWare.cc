#include "Utils.h"

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

bool LoaderUtil::LoadDllFromResource(std::vector<BYTE>& out)
{
	HRSRC hRes = FindResourceW(NULL, MAKEINTRESOURCEW(101), RT_RCDATA);
	if (!hRes) return false;
	HGLOBAL hMem = LoadResource(NULL, hRes);
	if (!hMem) return false;
	DWORD sz = SizeofResource(NULL, hRes);
	if (!sz) return false;
	BYTE* p = (BYTE*)LockResource(hMem);
	if (!p) return false;
	out.assign(p, p + sz);
	// de-xor if was xored (our build xors with 0x5A)
	// try detect: if first two bytes are not MZ, try xor
	if (out.size() >= 2 && !(out[0] == 'M' && out[1] == 'Z')) {
		for (auto &b : out) b ^= 0x5A;
	}
	return out.size() > 0 && out[0] == 'M' && out[1] == 'Z';
}
