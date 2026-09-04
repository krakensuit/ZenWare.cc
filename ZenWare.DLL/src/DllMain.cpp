#include "Entry/Entry.h"

namespace
{
	//The whole init runs on a dedicated thread: DllMain must not block
	//(loader-lock) and manual mapping calls DllMain via a remote thread.
	DWORD WINAPI InitThread(LPVOID lpParam)
	{
		DisableThreadLibraryCalls(static_cast<HMODULE>(lpParam));
		G::ModuleEntry.Load();
		return 0;
	}
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(lpvReserved);

	static bool s_bAttached = false;

	if ((fdwReason == DLL_PROCESS_ATTACH) && !s_bAttached)
	{
		s_bAttached = true;

		const HANDLE hThread = CreateThread(nullptr, 0, InitThread, hinstDLL, 0, nullptr);

		if (hThread)
			CloseHandle(hThread);
	}

	return TRUE;
}
