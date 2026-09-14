#pragma once
#include <cstdint>

// External L4D2 addresses (x86). Values cross-checked against the open PoC
// Yhzan95/Left4Dead2_External (commit 2026-07-04, same build era as the
// current Steam build) + confirmed by the access scheme in its code:
//   ViewMatrix: engine.dll+dwViewMatrix -> ptr -> +inner -> 16 float
//   LocalPlayer: client.dll+dwLocalPlayer -> pointer to the entity
//   EntityList: client.dll+dwEntityList, stride 0x10
// After a major game update re-check with Cheat Engine;
// on a mismatch the overlay itself shows "offsets stale" (see main.cpp).
namespace Off
{
	inline const wchar_t* kProc = L"left4dead2.exe";
	inline const wchar_t* kClient = L"client.dll";
	inline const wchar_t* kEngine = L"engine.dll";

	// client.dll: pointer to the local entity
	inline uintptr_t dwLocalPlayer = 0x726BD8;

	// client.dll: entity array base, ent = read(base + i*0x10)
	inline uintptr_t dwEntityList = 0x73A574;

	// engine.dll: pointer -> +inner -> 4x4 matrix (row-major)
	inline uintptr_t dwViewMatrix = 0x601FDC;
	inline uintptr_t dwViewMatrixInner = 0x2E4;

	// Netvars (offsets from the entity base)
	inline uintptr_t offTeam = 0xE4;       // int: 2 = survivors, 3 = infected
	inline uintptr_t offHealth = 0xEC;    // int
	inline uintptr_t offLifeState = 0x147; // uint8: 0 = alive
	inline uintptr_t offFlags = 0xF0;     // int: bit0 = onground
	inline uintptr_t offOrigin = 0x124;   // Vec3
	inline uintptr_t offGhost = 0x1C9A;   // bool: infected ghost
	inline uintptr_t offZombieClass = 0x1C90; // int: specials (tank ~7-8)

	// Dormant flag (Source 2009 branch; if it is garbage the ESP simply
	// stops filtering, the check below tolerates a failed read)
	inline uintptr_t offDormant = 0xE9;

	inline int kMaxEnts = 256;
	inline int kGroundFlag = 1;
}
