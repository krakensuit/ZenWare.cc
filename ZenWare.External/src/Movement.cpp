#include "Movement.h"
#include "Offsets.h"
#include <cmath>
#include <cstdio>

inline uint64_t NowMs() { return GetTickCount64(); }

void Movement::SetSpace(bool down)
{
	if (down == m_spaceDown)
		return;
	m_spaceDown = down;
	INPUT in = { };
	in.type = INPUT_KEYBOARD;
	in.ki.wVk = VK_SPACE;
	in.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
	SendInput(1, &in, sizeof(in));
}

static void TapKey(WORD vk, bool down)
{
	INPUT in = { };
	in.type = INPUT_KEYBOARD;
	in.ki.wVk = vk;
	in.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
	SendInput(1, &in, sizeof(in));
}

float Movement::OnLogic(const Memory& mem, uintptr_t localAddr)
{
	float speed = 0.0f;
	if (!localAddr)
	{
		SetSpace(false);
		return 0.0f;
	}

	int hp = 0, flags = 0;
	Vec3m org;
	mem.Read(localAddr + Off::offHealth, hp);
	mem.Read(localAddr + Off::offFlags, flags);
	if (Off::offOrigin) mem.Read(localAddr + Off::offOrigin, org);

	// Скорость по дельте позиций со сглаживанием (offVelocity нет в референсе).
	// Вызывается ~каждые 2мс.
	{
		static Vec3m s_prev = { };
		static uint64_t s_prevMs = 0;
		static float s_smooth = 0.0f;
		static bool s_init = false;
		uint64_t now = NowMs();
		if (s_init && now > s_prevMs)
		{
			float dt = (now - s_prevMs) / 1000.0f;
			if (dt > 0.0f && dt < 0.5f)
			{
				float dx = org.x - s_prev.x, dy = org.y - s_prev.y;
				float inst = sqrtf(dx * dx + dy * dy) / dt;
				if (inst < 3000.0f)
					s_smooth = s_smooth * 0.7f + inst * 0.3f;
			}
		}
		s_prev = org; s_prevMs = now; s_init = true;
		speed = s_smooth;
	}

	bool ground = (flags & Off::kGroundFlag) != 0;
	bool alive = (hp > 0);

	// --- BunnyHop: только чтение флагов + эмуляция пробела ---
	bool wantJump = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
	if (bBhop && alive && wantJump && ground)
		SetSpace(true);
	else
		SetSpace(false);

	// --- EdgeJump: сошли с края с зажатым пробелом -> дожать прыжок ---
	if (bBhop && alive && m_wasGround && !ground && wantJump)
		SetSpace(true);
	m_wasGround = ground;

	// --- Strafe assist (experimental): чередование A/D в воздухе ---
	if (bStrafe && alive && !ground)
	{
		uint64_t now = NowMs();
		if (now >= m_nextFlip)
		{
			m_side = !m_side;
			// интервал короче на высокой скорости
			int ms = speed > 500.0f ? 90 : 130;
			m_nextFlip = now + ms;
			TapKey(m_side ? 'D' : 'A', true);
			TapKey(m_side ? 'A' : 'D', false);
		}
	}
	else if (ground)
	{
		TapKey('A', false);
		TapKey('D', false);
		m_nextFlip = 0;
	}

	// --- JumpStats observer ---
	if (bStats && alive)
	{
		if (!m_air && !ground)
		{
			m_air = true;
			m_takeoff = org;
			m_takeSpeed = speed;
			m_maxSpeed = speed;
			m_takeMs = NowMs();
			m_duckAtLand = false;
			m_res.ok = false;
		}
		if (m_air)
		{
			if (speed > m_maxSpeed) m_maxSpeed = speed;
			bool duck = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
			if (duck) m_duckAtLand = true;
			if (ground)
			{
				m_air = false;
				float dx = org.x - m_takeoff.x, dy = org.y - m_takeoff.y;
				float dist = sqrtf(dx * dx + dy * dy);
				uint64_t airMs = NowMs() - m_takeMs;
				if (airMs > 250 && dist > 40.0f && dist < 2000.0f)
				{
					m_res.dist = dist; m_res.pre = m_takeSpeed; m_res.mx = m_maxSpeed;
					m_res.airMs = airMs;
					m_res.until = NowMs() + 4000;
					m_res.ok = true;
				}
			}
		}
	}
	else
	{
		m_air = false;
		m_res.ok = false;
	}

	return speed;
}

void Movement::Reset()
{
	SetSpace(false);
	TapKey('A', false);
	TapKey('D', false);
	m_air = false;
	m_res.ok = false;
	m_wasGround = true;
	m_nextFlip = 0;
}

void Movement::DrawStats(Overlay& o)
{
	if (!bStats || !m_res.ok || NowMs() > m_res.until)
		return;
	int cx = o.Width() / 2;
	int cy = o.Height() / 2 + 76;
	wchar_t a[64], b[64];
	swprintf_s(a, L"%.0fu  pre %.0f  max %.0f", m_res.dist, m_res.pre, m_res.mx);
	o.Text(cx - 110, cy, RGB(235, 245, 240), L"%s", a);
	swprintf_s(b, L"air %llums%s", m_res.airMs, m_duckAtLand ? L"  [duck]" : L"");
	o.Text(cx - 60, cy + 16, RGB(0, 255, 171), L"%s", b);
}
