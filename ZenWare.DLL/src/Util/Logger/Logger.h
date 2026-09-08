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

	//Вариант без критической секции для пути падения: если падающий поток
	//застал лок занятым (или CRT в негодном состоянии), обычный Write
	//повиснет/упадёт и следа не останется. Сначала строка уходит в
	//OutputDebugStringA, потом best-effort в файл. Рваные строки допустимы.
	void WriteNoLock(const char* const szFormat, ...);

	// Хлебные крошки для диагностики вылетов: короткий след последнего места.
	// Без аллокаций и без лока — безопасно звать из любого потока и читать
	// из CrashRecorder на падающем потоке (строка может быть рваной, это ок).
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

