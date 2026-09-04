#pragma once
#include "Memory.h"
#include "Overlay.h"
#include <cstdint>

struct Vec3m { float x = 0, y = 0, z = 0; };

// Только ввод (SendInput) + чтение флагов/скорости. Никаких записей в игру.
// Внешний стрейф слабее internal по построению: mousedx недоступен снаружи,
// поэтому доступен только таймерный режим (помечен experimental).
class Movement
{
public:
	bool bBhop = false;
	bool bStrafe = false;
	bool bStats = true;

	// Вызывать ~каждые 2мс. Возвращает текущую скорость для оверлея.
	float OnLogic(const Memory& mem, uintptr_t localAddr);
	void DrawStats(Overlay& o);
	// Отпустить все удерживаемые клавиши (при выходе/смерти игры).
	void Reset();

private:
	void SetSpace(bool down);

	bool m_spaceDown = false;
	bool m_wasGround = true;

	// JumpStats observer
	bool m_air = false;
	Vec3m m_takeoff;
	float m_takeSpeed = 0, m_maxSpeed = 0;
	uint64_t m_takeMs = 0;
	bool m_duckAtLand = false;
	struct Res { float dist = 0, pre = 0, mx = 0; uint64_t airMs = 0, until = 0; bool ok = false; } m_res;

	// Strafe timer
	bool m_side = false;
	uint64_t m_nextFlip = 0;
};

inline uint64_t NowMs();
