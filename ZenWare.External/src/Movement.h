#pragma once
#include "Memory.h"
#include "Overlay.h"
#include <cstdint>

struct Vec3m { float x = 0, y = 0, z = 0; };

// Input only (SendInput) + reading flags/speed. Nothing is written to the game.
// External strafe is weaker than internal by design: mousedx is unavailable
// from outside, so only the timer mode is possible (marked experimental).
class Movement
{
public:
	bool bBhop = false;
	bool bStrafe = false;
	bool bStats = true;

	// Call ~every 2ms. Returns the current speed for the overlay.
	float OnLogic(const Memory& mem, uintptr_t localAddr);
	void DrawStats(Overlay& o);
	// Release every held key (on exit/game death).
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
	// EdgeJump: hold the emulated space so a game tick sees the press
	uint64_t m_holdSpaceUntil = 0;
};

inline uint64_t NowMs();
