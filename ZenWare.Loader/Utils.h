#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <vector>

namespace LoaderUtil
{
	//Heap-allocated text packet posted to the UI thread.
	struct LogPacket_t
	{
		int nKind;              //0 = append to log, 1 = set status bar text
		wchar_t wszText[512];
	};

	constexpr int KIND_LOG = 0;
	constexpr int KIND_STATUS = 1;

	constexpr UINT WM_APP_LOADER = WM_APP + 1;

	//UI language follows the system locale (true = Russian).
	extern bool g_bRuLang;

	//Picks a string by the current UI language.
	const char* S(const char* const szRu, const char* const szEn);
	const wchar_t* SW(const wchar_t* wszRu, const wchar_t* wszEn);

	//Opens %TEMP%\ZenWare.Loader.log; every Log() line lands here too,
	//so a crashed loader still leaves its trace on disk.
	void InitFileLog();

		//Finds the PID of a process by its executable file name (e.g. L"left4dead2.exe").
	DWORD FindProcessId(const wchar_t* const wszName);

	// Tries to load embedded DLL from RCDATA resource IDR_ZENWARE_DLL
	bool LoadDllFromResource(std::vector<BYTE>& out);

	//[HH:MM:SS.mmm] stamp for detailed logs.
	void TimeStamp(char* szOut, const size_t nOutLen);

	//Appends a stamped line to the log edit control AND to the file log. Thread-safe via PostMessage.
	void Log(const HWND hwndLog, const char* const szFormat, ...);

	//Short progress text for the status bar (also mirrored into the log).
	void Status(const HWND hwndLog, const char* const szText);
}
