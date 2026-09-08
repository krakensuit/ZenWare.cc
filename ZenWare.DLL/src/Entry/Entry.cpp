#include "Entry.h"

#include <csignal>
#include <exception>

#include "../Util/Logger/Logger.h"
#include "../Features/Config/Config.h"
#include "../SDK/L4D2/Interfaces/Cvar.h"

namespace
{
	//Одна запись на процесс: вложенные хендлеры/потоки не дублируют след.
	static volatile LONG s_bRecorded = 0;

	static void RecordCrash(const char* szVia, DWORD dwCode, DWORD dwAddr)
	{
		if (InterlockedExchange(&s_bRecorded, 1))
			return;

		HMODULE hModule = nullptr;
		char szWhere[128] = { };

		if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCWSTR>(dwAddr), &hModule) && hModule)
		{
			char szName[MAX_PATH] = { };
			GetModuleFileNameA(hModule, szName, MAX_PATH);

			const char* szFile = strrchr(szName, '\\');

			if (const char* const szSlash2 = strrchr(szName, '/'); !szFile || szSlash2 > szFile)
				szFile = szSlash2;

			sprintf_s(szWhere, sizeof(szWhere), "%s+0x%08X", szFile ? (szFile + 1) : szName, dwAddr - reinterpret_cast<DWORD>(hModule));
		}
		else
		{
			sprintf_s(szWhere, sizeof(szWhere), "%s", "manual-mapped region");
		}

		U::Log.WriteNoLock("[!!!] EXCEPTION 0x%08X at 0x%08X (%s) via %s", dwCode, dwAddr, szWhere, szVia);
		U::Log.WriteNoLock("[!!!] last breadcrumb: %s", U::Log.LastCrumb());
		U::Log.WriteNoLock("[!!!] Process is dying. Send the last lines of this file to the developer.");
	}

	//Last-resort crash recorder: before the process dies, write the exception
	//code, faulting EIP and the owning module into ZenWare.log. Informational
	//exceptions (< 0x80000000, e.g. OutputDebugString's) are ignored, otherwise
	//logging them would recurse through Logger::Write -> this handler.
	LONG WINAPI CrashRecorder(PEXCEPTION_POINTERS pInfo)
	{
		const DWORD dwCode = pInfo->ExceptionRecord->ExceptionCode;

		static thread_local bool s_bInside = false;

		if (dwCode < 0x80000000u || s_bInside)
			return EXCEPTION_CONTINUE_SEARCH;

		s_bInside = true;

		RecordCrash("VEH", dwCode, reinterpret_cast<DWORD>(pInfo->ExceptionRecord->ExceptionAddress));

		return EXCEPTION_CONTINUE_SEARCH;
	}

	//Вторая сеть: VEH не видит terminate/purecall/abort/invalid-parameter
	//(это не SEH-исключения, а тихий выход CRT). Все ведут в тот же след.
	LONG WINAPI CrashUEF(PEXCEPTION_POINTERS pInfo)
	{
		if (pInfo && pInfo->ExceptionRecord && pInfo->ExceptionRecord->ExceptionCode >= 0x80000000u)
			CrashRecorder(pInfo);

		return EXCEPTION_EXECUTE_HANDLER;
	}

	void CrashTerminate()
	{
		U::Log.WriteNoLock("[!!!] terminate() called (unhandled C++ exception?) crumb: %s", U::Log.LastCrumb());
		abort();
	}

	void CrashAbort(int)
	{
		U::Log.WriteNoLock("[!!!] SIGABRT crumb: %s", U::Log.LastCrumb());
		_exit(3);
	}

	void __cdecl CrashPurecall()
	{
		U::Log.WriteNoLock("[!!!] pure virtual call crumb: %s", U::Log.LastCrumb());
		_exit(3);
	}

	void __cdecl CrashInvParam(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t)
	{
		U::Log.WriteNoLock("[!!!] CRT invalid parameter crumb: %s", U::Log.LastCrumb());
		_exit(3);
	}

	DWORD WINAPI UnloadThread(LPVOID)
	{
		//Let in-flight frames drain through the passivating detours first.
		Sleep(300);

		U::Log.Write("[*] Unload: removing hooks ...");
		MH_Uninitialize();

		if (Hooks::WndProc::oWndProc && Hooks::WndProc::hwGame)
			SetWindowLongW(Hooks::WndProc::hwGame, GWL_WNDPROC, reinterpret_cast<LONG>(Hooks::WndProc::oWndProc));

		ShowCursor(FALSE);

		if (I::VGuiSurface)
			I::VGuiSurface->LockCursor();

		U::Log.Write("[===] Unloaded. The image stays resident (manual mapping cannot free itself).");
		return 0;
	}
}

void CGlobal_ModuleEntry::Load()
{
	//First thing ever: file logging so early failures are diagnosable.
	U::Log.Init();
	AddVectoredExceptionHandler(1, &CrashRecorder);
	SetUnhandledExceptionFilter(&CrashUEF);
	set_terminate(&CrashTerminate);
	signal(SIGABRT, &CrashAbort);
	_set_purecall_handler(&CrashPurecall);
	_set_invalid_parameter_handler(&CrashInvParam);

	U::Log.Write("[*] Waiting for serverbrowser.dll ...");

	while (!GetModuleHandleA("serverbrowser.dll"))
		std::this_thread::sleep_for(std::chrono::seconds(1));

	U::Log.Write("[+] Game modules loaded.");

	U::Log.Write("[*] Scanning patterns ...");
	U::Offsets.Init();

	//Interfaces
	{
		I::BaseClient       = U::Interface.Get<IBaseClientDLL*>("client.dll", "VClient016");
		I::ClientEntityList = U::Interface.Get<IClientEntityList*>("client.dll", "VClientEntityList003");
		I::Prediction       = U::Interface.Get<IPrediction*>("client.dll", "VClientPrediction001");
		I::GameMovement     = U::Interface.Get<IGameMovement*>("client.dll", "GameMovement001");

		I::EngineClient     = U::Interface.Get<IVEngineClient*>("engine.dll", "VEngineClient013");
		I::EngineTrace      = U::Interface.Get<IEngineTrace*>("engine.dll", "EngineTraceClient003");
		I::EngineVGui       = U::Interface.Get<IEngineVGui*>("engine.dll", "VEngineVGui001");
		I::RenderView       = U::Interface.Get<IVRenderView*>("engine.dll", "VEngineRenderView013");
		I::DebugOverlay     = U::Interface.Get<IVDebugOverlay*>("engine.dll", "VDebugOverlay003");
		I::ModelInfo        = U::Interface.Get<IVModelInfo*>("engine.dll", "VModelInfoClient004");
		I::ModelRender      = U::Interface.Get<IVModelRender*>("engine.dll", "VEngineModel016");

		I::VGuiPanel        = U::Interface.Get<IVGuiPanel*>("vgui2.dll", "VGUI_Panel009");
		I::VGuiSurface      = U::Interface.Get<IVGuiSurface*>("vgui2.dll", "VGUI_Surface031");

		I::MatSystemSurface = U::Interface.Get<IMatSystemSurface*>("vguimatsurface.dll", "VGUI_Surface031");

		I::MaterialSystem   = U::Interface.Get<IMaterialSystem*>("materialsystem.dll", "VMaterialSystem080");
		I::Cvar             = U::Interface.Get<ICvar*>("engine.dll", "VEngineCvar007");

		U::Log.Write("[+] Interfaces fetched (see XASSERT popups for any failures).");

		// Fail-closed: дальше идут прямые разыменования указателей и хуки.
		// На чужом билде игры лучше честный MessageBox и тихий выход,
		// чем вылет процесса без объяснений.
		struct Need_t { const char* m_szName; const void* m_pPtr; };
		const Need_t aNeed[] = {
			{ "VClient016", I::BaseClient },
			{ "VClientEntityList003", I::ClientEntityList },
			{ "VClientPrediction001", I::Prediction },
			{ "GameMovement001", I::GameMovement },
			{ "VEngineClient013", I::EngineClient },
			{ "EngineTraceClient003", I::EngineTrace },
			{ "VEngineVGui001", I::EngineVGui },
			{ "VEngineRenderView013", I::RenderView },
			{ "VDebugOverlay003", I::DebugOverlay },
			{ "VModelInfoClient004", I::ModelInfo },
			{ "VEngineModel016", I::ModelRender },
			{ "VGUI_Panel009", I::VGuiPanel },
			{ "VGUI_Surface031", I::VGuiSurface },
			{ "vguimatsurface/VGUI_Surface031", I::MatSystemSurface },
			{ "VMaterialSystem080", I::MaterialSystem },
			{ "VEngineCvar007", I::Cvar },
		};
		char szMissing[512] = { };
		for (size_t i = 0; i < sizeof(aNeed) / sizeof(aNeed[0]); i++)
		{
			U::Log.Write("[*] iface %-32s : %s", aNeed[i].m_szName, aNeed[i].m_pPtr ? "(ok)" : "(MISSING!)");
			if (!aNeed[i].m_pPtr)
			{
				strcat_s(szMissing, aNeed[i].m_szName);
				strcat_s(szMissing, "\n");
			}
		}

		if (!U::Offsets.m_dwClientMode || !U::Offsets.m_dwGlobalVars)
			strcat_s(szMissing, "ClientMode/GlobalVars patterns\n");
		if (!U::Offsets.m_dwMoveHelper)
			strcat_s(szMissing, "MoveHelper pattern\n");

		if (szMissing[0])
		{
			U::Log.Write("[!] Critical data missing, aborting init (game left untouched).");
			char szMsg[1024] = { };
			sprintf_s(szMsg, sizeof(szMsg),
				"ZenWare: initialization failed, game left untouched.\nMissing:\n%s\nSee ZenWare.log. Probably a game update - run Verify-Signatures.bat.",
				szMissing);
			MessageBoxA(HWND_DESKTOP, szMsg, "ZenWare", MB_ICONERROR);
			return;
		}

		I::ClientMode = **reinterpret_cast<void***>(U::Offsets.m_dwClientMode);
		U::Log.Write("[*] ClientMode    : 0x%08X %s", reinterpret_cast<DWORD>(I::ClientMode), I::ClientMode ? "(ok)" : "(NULL!)");
		XASSERT(I::ClientMode == nullptr);

		I::GlobalVars = **reinterpret_cast<CGlobalVarsBase***>(U::Offsets.m_dwGlobalVars);
		U::Log.Write("[*] GlobalVars    : 0x%08X %s", reinterpret_cast<DWORD>(I::GlobalVars), I::GlobalVars ? "(ok)" : "(NULL!)");
		XASSERT(I::GlobalVars == nullptr);

		I::MoveHelper = **reinterpret_cast<IMoveHelper***>(U::Offsets.m_dwMoveHelper);
		U::Log.Write("[*] MoveHelper    : 0x%08X %s", reinterpret_cast<DWORD>(I::MoveHelper), I::MoveHelper ? "(ok)" : "(NULL!)");
		XASSERT(I::MoveHelper == nullptr);
		U::Log.Write("[*] Before relocate.");

		U::Log.Write("[*] RelocateToGameDir -> %s.", U::Log.RelocateToGameDir() ? "moved" : "staying in TEMP");

		F::Config.Load();
		U::Log.Write("[*] Config loaded from \"%s\".", F::Config.FilePath());
	}

	U::Log.Write("[*] Initializing draw manager (fonts) ...");
	G::Draw.Init();
	U::Log.Write("[+] Draw manager ready.");

	U::Log.Write("[*] Installing hooks ...");
	G::Hooks.Init();
	U::Log.Write("[===] Initialization finished.");
}

void CGlobal_ModuleEntry::RequestUnload()
{
	if (InterlockedExchange(&m_bShuttingDown, 1))
		return;

	U::Log.Write("[*] PANIC requested (F11).");

	const HANDLE hThread = CreateThread(nullptr, 0, UnloadThread, nullptr, 0, nullptr);

	if (hThread)
		CloseHandle(hThread);
}

bool CGlobal_ModuleEntry::IsShuttingDown() const
{
	return m_bShuttingDown != 0;
}
