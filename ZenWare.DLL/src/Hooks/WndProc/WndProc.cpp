#include "WndProc.h"

using namespace Hooks;

#include "../../Features/Menu/Menu.h"
#include "../../Util/Logger/Logger.h"

LRESULT CALLBACK WndProc::Detour(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (F::Menu.ShouldBlockInput(uMsg))
	return 0;

	return CallWindowProcW(oWndProc, hwnd, uMsg, wParam, lParam);
}

void WndProc::Init()
{
	//Bounded search: an unbounded loop here would silently hang the init
	//thread if the window class ever changes.
	int nWaitedMs = 0;

	while (!hwGame)
	{
		hwGame = FindWindowW(L"Valve001", nullptr);

		if (!hwGame)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			nWaitedMs += 100;

			if ((nWaitedMs % 5000) == 0)
				U::Log.Write("[*] WndProc: game window not found yet (%d s) ...", nWaitedMs / 1000);

			if (nWaitedMs >= 60'000)
			{
				U::Log.Write("[!] WndProc: window class L\"Valve001\" not found in 60s, hook skipped.");
				return;
			}
		}
	}

	oWndProc = reinterpret_cast<WNDPROC>(SetWindowLongW(hwGame, GWL_WNDPROC, reinterpret_cast<LONG>(Detour)));

	if (oWndProc)
		U::Log.Write("[+] WndProc hooked (HWND 0x%08X).", reinterpret_cast<DWORD>(hwGame));
	else
		U::Log.Write("[!] SetWindowLongW failed, error %lu.", GetLastError());
}

