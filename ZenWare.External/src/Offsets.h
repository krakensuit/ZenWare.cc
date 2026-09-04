#pragma once
#include <cstdint>

// External-адреса L4D2 (x86). Значения сверены с открытым PoC
// Yhzan95/Left4Dead2_External (коммит 2026-07-04, та же эпоха билда,
// что и текущий Steam-билд) + подтверждены схемой доступа в его коде:
//   ViewMatrix: engine.dll+dwViewMatrix -> ptr -> +inner -> 16 float
//   LocalPlayer: client.dll+dwLocalPlayer -> указатель на сущность
//   EntityList: client.dll+dwEntityList, шаг 0x10
// После крупного обновления игры перепроверь через Cheat Engine;
// при рассинхроне оверлей сам покажет "offsets stale" (см. main.cpp).
namespace Off
{
	inline const wchar_t* kProc = L"left4dead2.exe";
	inline const wchar_t* kClient = L"client.dll";
	inline const wchar_t* kEngine = L"engine.dll";

	// client.dll: указатель на локальную сущность
	inline uintptr_t dwLocalPlayer = 0x726BD8;

	// client.dll: база массива сущностей, ent = read(base + i*0x10)
	inline uintptr_t dwEntityList = 0x73A574;

	// engine.dll: указатель -> +inner -> матрица 4x4 (row-major)
	inline uintptr_t dwViewMatrix = 0x601FDC;
	inline uintptr_t dwViewMatrixInner = 0x2E4;

	// Нетвары (смещения от начала сущности)
	inline uintptr_t offTeam = 0xE4;       // int: 2 = survivors, 3 = infected
	inline uintptr_t offHealth = 0xEC;    // int
	inline uintptr_t offLifeState = 0x147; // uint8: 0 = alive
	inline uintptr_t offFlags = 0xF0;     // int: bit0 = onground
	inline uintptr_t offOrigin = 0x124;   // Vec3
	inline uintptr_t offGhost = 0x1C9A;   // bool: infected-призрак
	inline uintptr_t offZombieClass = 0x1C90; // int: особые (танк ~7-8)

	// Флаг дормантности (ветка Source 2009; если мусор — ESP просто
	// перестанет фильтровать, проверка ниже толерантна к ошибке чтения)
	inline uintptr_t offDormant = 0xE9;

	inline int kMaxEnts = 256;
	inline int kGroundFlag = 1;
}
