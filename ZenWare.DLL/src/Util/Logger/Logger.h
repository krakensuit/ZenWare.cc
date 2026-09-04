#pragma once

#include "../../SDK/SDK.h"

//Crash-diagnosis file logger. Exists because a manually mapped DLL has no
//console and XASSERT message boxes do not cover every failure mode.
//
//Lifecycle:
//  Init()               -> opens %TEMP%\ZenWare.log right away (before any
//                          interface/pattern work), so early failures land here.
//  RelocateToGameDir()  -> once IVEngineClient is up, reopens the log inside
//                          the game directory (<gamedir>\ZenWare.log).
//
//Every line is flushed immediately and mirrored via OutputDebugStringA.
//All writes are guarded by a critical section (init thread + render thread).

class CUtil_Logger
{
public:
	void Init();
	bool RelocateToGameDir();

	void Write(const char* const szFormat, ...);

private:
	void Open(const char* const szPath);

	CRITICAL_SECTION m_cs = { };
	bool m_bCsInit = false;

	FILE* m_pFile = nullptr;
	char m_szPath[MAX_PATH] = { };
};

namespace U { inline CUtil_Logger Log; }

//One-shot tracer for hot detours: logs only the FIRST invocation of each site.
#define ZTRACE_FIRST(szName) do { static bool s_bTraced = false; if (!s_bTraced) { s_bTraced = true; U::Log.Write("trace: first call -> %s", szName); } } while (false)

