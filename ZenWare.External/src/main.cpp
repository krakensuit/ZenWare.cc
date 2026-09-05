#include "Memory.h"
#include "Overlay.h"
#include "ESP.h"
#include "Movement.h"
#include "Offsets.h"
#include <vector>

// ZenWare.External: отдельный процесс, только чтение памяти (RPM) +
// прозрачный оверлей + эмуляция ввода (SendInput). В игру ничего не
// пишется, не инжектится и не хукается.
//
// Управление: INS = ESP вкл/выкл, F7 = язык RU/EN, F8 = bhop, F10 = strafe assist,
// F9 = панель статистики, END = выход.

static bool KeyPressed(int vk)
{
	static bool prev[256] = { };
	bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
	bool hit = down && !prev[vk & 0xFF];
	prev[vk & 0xFF] = down;
	return hit;
}

static void Pump()
{
	MSG m;
	while (PeekMessageW(&m, nullptr, 0, 0, PM_REMOVE))
	{
		if (m.message == WM_QUIT)
			ExitProcess(0);
		TranslateMessage(&m);
		DispatchMessageW(&m);
	}
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	Memory mem;
	Overlay o;
	ESP esp;
	Movement mv;
	std::vector<ESP::RawEnt> drawList;

	bool bEsp = true;
	int staleFrames = 0;
	bool bWasInGame = false;
	bool bRu = (PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_RUSSIAN); // F7 переключает

	for (;;)
	{
		// --- ждём игру ---
		while (!mem.Attach(Off::kProc)) { Pump(); Sleep(500); }

		uintptr_t client = 0, engine = 0;
		uint32_t cb = 0, eb = 0;
		if (!mem.Module(Off::kClient, client, cb) || !mem.Module(Off::kEngine, engine, eb))
		{
			mem.Close();
			Sleep(500);
			continue;
		}

		if (!o.IsAlive() && !o.Create())
			return 1;

		uint64_t lastDraw = 0, lastSnap = 0;
		int followTick = 0;
		ESP::Snap snap;

		// --- главный цикл, пока живо окно игры ---
		bWasInGame = true;
		while (FindWindowW(L"Valve001", nullptr))
		{
			Pump();

			if (KeyPressed(VK_INSERT)) bEsp = !bEsp;
			if (KeyPressed(VK_F7)) bRu = !bRu;
			if (KeyPressed(VK_F8)) mv.bBhop = !mv.bBhop;
			if (KeyPressed(VK_F10)) mv.bStrafe = !mv.bStrafe;
			if (KeyPressed(VK_F9)) mv.bStats = !mv.bStats;
			if (GetAsyncKeyState(VK_END) & 0x8000) { mv.Reset(); return 0; }

			uint64_t now = GetTickCount64();

			uintptr_t localAddr = 0;
			mem.Read(client + Off::dwLocalPlayer, localAddr);

			float speed = mv.OnLogic(mem, localAddr);

			// Снапшот сущностей ~20 раз/сек (полный обход списка дорогой).
			if (now - lastSnap >= 50)
			{
				lastSnap = now;
				if (esp.Snapshot(mem, client, engine, snap))
				{
					staleFrames = 0;
					esp.Collect(mem, client, snap, drawList);
				}
				else
				{
					staleFrames++;
				}
			}

			if (++followTick % 30 == 0)
				o.FollowGame();

			if (now - lastDraw >= 16 && o.GameVisible())
			{
				lastDraw = now;
				o.BeginFrame();

				wchar_t st[224];
				const wchar_t* on = bRu ? L"вкл" : L"on";
				const wchar_t* off = bRu ? L"выкл" : L"off";
				swprintf_s(st, L"ZenWare.External | ESP[INS]:%s BHOP[F8]:%s STRAFE[F10]:%s STATS[F9]:%s LANG[F7]:%s | END=%s",
					bEsp ? on : off, mv.bBhop ? on : off,
					mv.bStrafe ? on : off, mv.bStats ? on : off,
					bRu ? L"RU" : L"EN", bRu ? L"выход" : L"exit");
				o.Text(10, 8, RGB(0, 255, 171), L"%s", st);

				if (staleFrames > 20)
				{
					o.Text(10, 30, RGB(255, 90, 90),
						bRu ? L"офсеты протухли - обнови Offsets.h (см. комментарии)"
						    : L"offsets stale - update Offsets.h (see comments)");
				}
				else
				{
					esp.Draw(o, snap, drawList);
					if (mv.bStats)
					{
						wchar_t sp[48];
						swprintf_s(sp, L"%.0f u/s", speed);
						o.Text(o.Width() / 2 - 40, o.Height() / 2 - 120, RGB(0, 255, 171), L"%s", sp);
					}
				}

				mv.DrawStats(o);
				o.EndFrame();
			}

			Sleep(2);
		}

		// Игра закрылась: отпустить все зажатые нами клавиши.
		mv.Reset();
		drawList.clear();
		staleFrames = 0;
		mem.Close();
		// Была игра и пропала — выходим вслед за ней (с паузой на переходные состояния).
		if (bWasInGame)
		{
			Sleep(1500);
			Memory probe;
			if (!FindWindowW(L"Valve001", nullptr) && !probe.Attach(Off::kProc))
				return 0;
			probe.Close();
		}
		Sleep(500);
	}
}
