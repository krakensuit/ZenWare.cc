#include "ESP.h"
#include "Offsets.h"
#include <cmath>
#include <cstdio>
#include <string>

bool ESP::WorldToScreen(const Vec3& in, const float* m, int sw, int sh, int& x, int& y) const
{
	float w = m[12] * in.x + m[13] * in.y + m[14] * in.z + m[15];
	if (w < 0.01f)
		return false;
	float nx = (m[0] * in.x + m[1] * in.y + m[2] * in.z + m[3]) / w;
	float ny = (m[4] * in.x + m[5] * in.y + m[6] * in.z + m[7]) / w;
	x = (int)(sw * 0.5f + nx * sw * 0.5f);
	y = (int)(sh * 0.5f - ny * sh * 0.5f);
	return true;
}

bool ESP::Finite3(const Vec3& v) const
{
	return isfinite(v.x) && isfinite(v.y) && isfinite(v.z)
		&& fabsf(v.x) < 20000.0f && fabsf(v.y) < 20000.0f && fabsf(v.z) < 20000.0f;
}

bool ESP::Snapshot(const Memory& mem, uintptr_t client, uintptr_t engine, Snap& out)
{
	out = Snap();
	uintptr_t local = 0;
	if (!mem.Read(client + Off::dwLocalPlayer, local) || !local)
		return false;

	int team = 0, hp = 0;
	uint8_t life = 1;
	Vec3 org;
	if (!mem.Read(local + Off::offTeam, team) || (team != 2 && team != 3))
		return false;
	if (!mem.Read(local + Off::offHealth, hp) || hp <= 0 || hp > 20000)
		return false;
	if (!mem.Read(local + Off::offLifeState, life) || life > 1)
		return false;
	if (!mem.Read(local + Off::offOrigin, org) || !Finite3(org))
		return false;

	uintptr_t matPtr = 0;
	float mat[16] = { };
	if (!mem.Read(engine + Off::dwViewMatrix, matPtr) || !matPtr)
		return false;
	if (!mem.ReadRaw(matPtr + Off::dwViewMatrixInner, mat, sizeof(mat)))
		return false;
	for (int i = 0; i < 16; ++i)
	{
		if (!isfinite(mat[i]) || fabsf(mat[i]) > 100000.0f)
			return false;
	}

	out.local = local;
	out.localTeam = team;
	out.localOrg = org;
	memcpy(out.mat, mat, sizeof(mat));
	out.matOk = true;
	return true;
}

void ESP::Collect(const Memory& mem, uintptr_t client, const Snap& snap, std::vector<RawEnt>& out)
{
	out.clear();
	if (!snap.matOk)
		return;

	uintptr_t list = client + Off::dwEntityList;
	for (int i = 1; i <= Off::kMaxEnts; ++i)
	{
		uintptr_t ent = 0;
		if (!mem.Read(list + (uintptr_t)i * 0x10, ent) || !ent || ent == snap.local)
			continue;

		uint8_t dorm = 0;
		if (mem.Read(ent + Off::offDormant, dorm) && dorm)
			continue;

		int team = 0, hp = 0;
		uint8_t life = 1;
		if (!mem.Read(ent + Off::offTeam, team))
			continue;
		if (!mem.Read(ent + Off::offHealth, hp) || hp <= 0 || hp > 20000)
			continue;
		if (!mem.Read(ent + Off::offLifeState, life) || life != 0)
			continue;

		bool ghost = false;
		if (Off::offGhost && mem.Read(ent + Off::offGhost, ghost) && ghost)
			continue;

		Vec3 org;
		if (!mem.Read(ent + Off::offOrigin, org) || !Finite3(org))
			continue;

		COLORREF color;
		if (team == 2 || team == 3)
		{
			int cls = 0;
			// Танк красим отдельно (zombieClass 7-8 по данным референса).
			if (team == 3 && Off::offZombieClass && mem.Read(ent + Off::offZombieClass, cls) && cls >= 7)
				color = RGB(170, 60, 255);
			else
				color = (team == snap.localTeam) ? RGB(0, 255, 171) : RGB(255, 80, 80);
		}
		else
		{
			// team 0 + живое: возможно обычная заражённая / ведьма.
			// Проверяем имя модели (ent+0x10 -> строка), как в референсе.
			uintptr_t modelPtr = 0;
			if (!mem.Read(ent + 0x10, modelPtr) || !modelPtr)
				continue;
			char name[96] = { };
			if (!mem.ReadRaw(modelPtr, name, sizeof(name) - 1))
				continue;
			for (char* p = name; *p; ++p) *p = (char)tolower((unsigned char)*p);
			std::string s(name);
			if (s.find("witch") != std::string::npos)
				color = RGB(200, 0, 255);
			else if (s.find("common") != std::string::npos || s.find("infected") != std::string::npos || s.find("zombie") != std::string::npos)
				color = RGB(255, 255, 0);
			else
				continue;
		}

		RawEnt d;
		d.org = org;
		d.hp = hp;
		d.color = color;
		d.dist = 0;
		out.push_back(d);
	}

	// Дистанция отдельным проходом (дешевле, чем внутри фильтра).
	for (auto& d : out)
	{
		float dx = d.org.x - snap.localOrg.x, dy = d.org.y - snap.localOrg.y;
		d.dist = (int)(sqrtf(dx * dx + dy * dy) / 52.5f);
	}
}

void ESP::Draw(Overlay& o, const Snap& snap, const std::vector<RawEnt>& ents)
{
	if (!snap.matOk)
		return;

	const int sw = o.Width(), sh = o.Height();
	if (sw < 100 || sh < 100)
		return;

	for (const auto& e : ents)
	{
		Vec3 head = e.org; head.z += 72.0f;
		int x0, y0, x1, y1;
		if (!WorldToScreen(e.org, snap.mat, sw, sh, x0, y0))
			continue;
		if (!WorldToScreen(head, snap.mat, sw, sh, x1, y1))
			continue;

		int h = y0 - y1;
		if (h <= 4 || h > sh)
			continue;
		int w = h / 3;
		int x = x1 - w / 2;

		o.Rect(x, y1, w, h, e.color);

		int frac = e.hp > 100 ? 100 : e.hp;
		int bh = h * frac / 100;
		o.Rect(x - 6, y1, 3, h, RGB(20, 20, 20));
		o.Rect(x - 6, y0 - bh, 3, bh, RGB(255 - 255 * frac / 100, 255 * frac / 100, 0));

		wchar_t buf[48];
		swprintf_s(buf, L"%dhp %dm", e.hp, e.dist);
		o.Text(x, y1 - 16, RGB(230, 255, 245), L"%s", buf);
	}
}
