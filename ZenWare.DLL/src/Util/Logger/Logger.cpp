#include "Logger.h"

#include <cstdarg>

void CUtil_Logger::Init()
{
	if (!m_bCsInit)
	{
		InitializeCriticalSection(&m_cs);
		m_bCsInit = true;
	}

	char szTempDir[MAX_PATH] = { };

	if (GetTempPathA(MAX_PATH, szTempDir) == 0)
		strcpy_s(szTempDir, "C:\\"); //paranoid fallback

	char szPath[MAX_PATH] = { };
	strcpy_s(szPath, szTempDir);
	strcat_s(szPath, "ZenWare.log");

	Open(szPath);

	U::Log.Write("=== ZenWare session started (early stage: interfaces are not up yet) ===");
}

bool CUtil_Logger::RelocateToGameDir()
{
	if (!m_bCsInit || !I::EngineClient)
		return false;

	const char* const szGameDir = I::EngineClient->GetGameDirectory();

	if (!szGameDir || !szGameDir[0])
		return false;

	char szNewPath[MAX_PATH] = { };
	strcpy_s(szNewPath, szGameDir);
	strcat_s(szNewPath, "\\ZenWare.log");

	//Already there.
	if (_stricmp(szNewPath, m_szPath) == 0)
		return true;

	char szOldPath[MAX_PATH] = { };
	strcpy_s(szOldPath, m_szPath);

	EnterCriticalSection(&m_cs);

	if (m_pFile)
	{
		fclose(m_pFile);
		m_pFile = nullptr;
	}

	const char* const szFailedPath = m_szPath;
	Open(szNewPath);

	if (m_pFile)
		Write("[*] Log relocated from \"%s\". Early-stage lines are in the old file.", szOldPath);
	else
		strncpy_s(m_szPath, szFailedPath, _TRUNCATE); //keep pointing at the working file

	LeaveCriticalSection(&m_cs);

	return (m_pFile != nullptr);
}

void CUtil_Logger::Open(const char* const szPath)
{
	//Caller holds the critical section (or is single-threaded during Init).
	strncpy_s(m_szPath, szPath, _TRUNCATE);

	if (fopen_s(&m_pFile, m_szPath, "a") != 0)
	{
		m_pFile = nullptr;
		return;
	}

	SYSTEMTIME st = { };
	GetLocalTime(&st);
	fprintf(m_pFile, "=== opened %02u.%02u.%04u %02u:%02u:%02u ===\n",
		st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
	fflush(m_pFile);
}

void CUtil_Logger::Write(const char* const szFormat, ...)
{
	if (!m_pFile)
		return;

	char szBody[900] = { };

	va_list args = nullptr;
	va_start(args, szFormat);
	_vsnprintf_s(szBody, sizeof(szBody), _TRUNCATE, szFormat, args);
	va_end(args);

	char szLine[1024] = { };
	SYSTEMTIME st = { };
	GetLocalTime(&st);
	const int nPrefix = sprintf_s(szLine, sizeof(szLine), "[%02u:%02u:%02u.%03u] ",
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	strcat_s(szLine, szBody);

	EnterCriticalSection(&m_cs);

	fwrite(szLine, 1, strlen(szLine), m_pFile);
	fwrite("\n", 1, 1, m_pFile);
	fflush(m_pFile);

	LeaveCriticalSection(&m_cs);

	OutputDebugStringA(szLine);
}
