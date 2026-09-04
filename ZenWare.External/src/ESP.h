#pragma once
#include "Memory.h"
#include "Overlay.h"
#include <vector>

struct Vec3 { float x = 0, y = 0, z = 0; };

// ESP только чтением памяти: боксы + HP + дистанция + цвет команды.
// Имена игроков external получить не может (GetPlayerInfo — вызов движка),
// поэтому их нет осознанно.
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

	// Читает local + матрицу, проверяет их sanity. false = данные битые
	// (оффсеты устарели) — рисовать по ним нельзя.
	bool Snapshot(const Memory& mem, uintptr_t client, uintptr_t engine, Snap& out);

	// Собирает сырые данные видимых сущностей (мировые координаты).
	void Collect(const Memory& mem, uintptr_t client, const Snap& snap, std::vector<RawEnt>& out);

	// Проецирует и рисует.
	void Draw(Overlay& o, const Snap& snap, const std::vector<RawEnt>& ents);

private:
	bool WorldToScreen(const Vec3& in, const float* m, int sw, int sh, int& x, int& y) const;
	bool Finite3(const Vec3& v) const;
};
