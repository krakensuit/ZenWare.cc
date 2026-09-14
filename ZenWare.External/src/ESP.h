#pragma once
#include "Memory.h"
#include "Overlay.h"
#include <vector>

struct Vec3 { float x = 0, y = 0, z = 0; };

// ESP via memory reads only: boxes + HP + distance + team color.
// Player names are intentionally absent: external cannot get them
// (GetPlayerInfo is an engine call).
class ESP
{
public:
	struct RawEnt
	{
		Vec3 org;
		int hp = 0, dist = 0;
		COLORREF color = RGB(255, 255, 255);
	};

	struct Snap
	{
		uintptr_t local = 0;
		int localTeam = 0;
		Vec3 localOrg;
		float mat[16] = { };
		bool matOk = false;
	};

	// Reads local + matrix, sanity-checks them. false = the data is corrupt
	// (offsets are stale) - nothing may be drawn from it.
	bool Snapshot(const Memory& mem, uintptr_t client, uintptr_t engine, Snap& out);

	// Collects raw data of visible entities (world coordinates).
	void Collect(const Memory& mem, uintptr_t client, const Snap& snap, std::vector<RawEnt>& out);

	// Projects and draws.
	void Draw(Overlay& o, const Snap& snap, const std::vector<RawEnt>& ents);

private:
	bool WorldToScreen(const Vec3& in, const float* m, int sw, int sh, int& x, int& y) const;
	bool Finite3(const Vec3& v) const;
};
