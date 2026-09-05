#include "Entry.h"

#include "../Util/Logger/Logger.h"
#include "../Features/Config/Config.h"

namespace
{
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

		const DWORD dwAddr = reinterpret_cast<DWORD>(pInfo->ExceptionRecord->ExceptionAddress);

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

		U::Log.Write("[!!!] EXCEPTION 0x%08X at 0x%08X (%s)", dwCode, dwAddr, szWhere);
		U::Log.Write("[!!!] Process is dying. Send the last lines of this file to the developer.");

		return EXCEPTION_CONTINUE_SEARCH;
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

		U::Log.Write("[+] Interfaces fetched (see XASSERT popups for any failures).");

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
