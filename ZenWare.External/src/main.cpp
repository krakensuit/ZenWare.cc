#include "Memory.h"
#include "Overlay.h"
#include "ESP.h"
#include "Movement.h"
#include "Offsets.h"
#include "Resolve.h"
#include <vector>

// ZenWare.External: a separate process, memory read-only (RPM) +
// a transparent overlay + input emulation (SendInput). Nothing is
// written to, injected into or hooked in the game.
//
// Controls: INS = ESP on/off, F7 = RU/EN language, F8 = bhop, F10 = strafe assist,
// F9 = stats panel, END = quit.

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
	bool bRu = (PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_RUSSIAN); // F7 toggles it

	for (;;)
	{
		// --- waiting for the game ---
		while (!mem.Attach(Off::kProc)) { Pump(); Sleep(500); }

		uintptr_t client = 0, engine = 0;
		uint32_t cb = 0, eb = 0;
		if (!mem.Module(Off::kClient, client, cb) || !mem.Module(Off::kEngine, engine, eb))
		{
			mem.Close();
			Sleep(500);
			continue;
		}

		// Offsets for the specific game build: signatures/anchor, otherwise hardcode.
		Resolved_t res;
		ResolveOffsets(mem, client, cb, engine, eb, res);
		Off::dwLocalPlayer = res.local;
		Off::dwEntityList = res.list;
		Off::dwViewMatrix = res.mat;

		if (!o.IsAlive() && !o.Create())
			return 1;

		uint64_t lastDraw = 0, lastSnap = 0;
		int followTick = 0;
		ESP::Snap snap;

		// --- main loop, while the game window is alive ---
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

			// Entity snapshot ~20 times/sec (a full list walk is expensive).
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

				// Resolve diagnostics: which offsets came from where (sig/anchor/hard).
				{
					wchar_t dg[160];
					swprintf_s(dg, L"LP:%05X(%hs) ENT:%05X(%hs) MAT:%05X(%hs mc=%d) stale=%d",
						(unsigned)(res.local & 0xFFFFF), res.srcLocal,
						(unsigned)(res.list & 0xFFFFF), res.srcList,
						(unsigned)(res.mat & 0xFFFFF), res.srcMat, res.matCands, staleFrames);
					o.Text(10, 24, RGB(120, 140, 132), L"%s", dg);
					wchar_t dg2[192];
					swprintf_s(dg2, L"rd:cli=%dKB eng=%dKB lphits=%d thunks=%d datasrc=%d ch=%08X eh=%08X sz=%X/%X",
						res.dbgCliKB, res.dbgEngKB, res.dbgLpHits, res.dbgThunks, res.dbgDataSec,
						res.dbgCliHead, res.dbgEngHead, res.dbgCliSize, res.dbgEngSize);
					o.Text(10, 56, RGB(120, 140, 132), L"%s", dg2);
				}

				if (staleFrames > 20)
				{
					o.Text(10, 40, RGB(255, 90, 90),
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

		// The game closed: release every key we are holding down.
		mv.Reset();
		drawList.clear();
		staleFrames = 0;
		mem.Close();
		// The game was here and disappeared - exit after it (with a pause for transitional states).
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
