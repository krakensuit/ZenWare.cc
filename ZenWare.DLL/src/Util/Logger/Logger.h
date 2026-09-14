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

	//Variant without the critical section for the crash path: if the crashing
	//thread finds the lock held (or the CRT in a bad state), the regular Write
	//would hang/crash and no trail would remain. The line goes to
	//OutputDebugStringA first, then best-effort to the file. Torn lines are acceptable.
	void WriteNoLock(const char* const szFormat, ...);

	// Breadcrumbs for crash diagnostics: a short trail of the last place.
	// No allocations and no lock — safe to call from any thread and to read
	// from CrashRecorder on the crashing thread (the line may be torn, that is fine).
	void Crumb(const char* szStage);
	const char* LastCrumb() const { return m_szCrumb; }

private:
	void Open(const char* const szPath);

	CRITICAL_SECTION m_cs = { };
	bool m_bCsInit = false;

	FILE* m_pFile = nullptr;
	char m_szPath[MAX_PATH] = { };
	char m_szCrumb[64] = { "boot" };
};

namespace U { inline CUtil_Logger Log; }

//One-shot tracer for hot detours: logs only the FIRST invocation of each site.
#define ZTRACE_FIRST(szName) do { static bool s_bTraced = false; if (!s_bTraced) { s_bTraced = true; U::Log.Write("trace: first call -> %s", szName); } } while (false)

