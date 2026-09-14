#pragma once

// ZenWare Loader - палитра и типографика (Direct2D-слой).
// РЕШЕНИЕ: существующий Theme_t в Main.cpp не переименовываю - рабочий код
// не должен ломаться; новая палитра живёт под своим именем и используется
// только D2D-рендерером.

#include <windows.h>

namespace Zen2D
{
	// Одна финальная палитра (то, что просили вместо кислотно-зелёного).
	struct Theme_t
	{
		DWORD background;    // #0A0E0D
		DWORD surface;       // #121815
		DWORD border;        // #1E2A25
		DWORD textPrimary;   // #E8FFF5
		DWORD textSecondary; // #7A948A
		DWORD accent;        // #7FFFD4 настоящий мятный
		DWORD accentDim;     // #3DB88F
		DWORD danger;        // #FF5C5C
	};

	inline DWORD Rgb(BYTE r, BYTE g, BYTE b) { return (static_cast<DWORD>(r) << 16) | (static_cast<DWORD>(g) << 8) | b; }

	inline Theme_t Dark()
	{
		Theme_t t{};
		t.background = Rgb(0x0A, 0x0E, 0x0D);
		t.surface = Rgb(0x12, 0x18, 0x15);
		t.border = Rgb(0x1E, 0x2A, 0x25);
		t.textPrimary = Rgb(0xE8, 0xFF, 0xF5);
		t.textSecondary = Rgb(0x7A, 0x94, 0x8A);
		t.accent = Rgb(0x7F, 0xFF, 0xD4);
		t.accentDim = Rgb(0x3D, 0xB8, 0x8F);
		t.danger = Rgb(0xFF, 0x5C, 0x5C);
		return t;
	}

	// Светлая тема системного оформления: тот же характер, инвертированная база.
	inline Theme_t Light()
	{
		Theme_t t{};
		t.background = Rgb(0xF4, 0xFA, 0xF7);
		t.surface = Rgb(0xFF, 0xFF, 0xFF);
		t.border = Rgb(0xD3, 0xE4, 0xDC);
		t.textPrimary = Rgb(0x11, 0x1A, 0x17);
		t.textSecondary = Rgb(0x5E, 0x74, 0x6B);
		t.accent = Rgb(0x1F, 0xA9, 0x7F);
		t.accentDim = Rgb(0x14, 0x7A, 0x5B);
		t.danger = Rgb(0xD8, 0x3A, 0x3A);
		return t;
	}

	// Состояние кадра, которое рендер получает каждый кадр.
	struct FrameState_t
	{
		float dt = 0.016f;        // шаг времени, с
		float elapsed = 0.0f;     // время с запуска, с
		bool  busy = false;       // идёт инжект/запуск
		float progress = 0.0f;    // 0..1 для определённого прогресса (splash)
		bool  external = false;   // режим EXTERNAL
		float hoverLaunch = 0.0f; // 0..1
		float hoverInject = 0.0f; // 0..1
		float modeT = 0.0f;       // 0 = internal, 1 = external (плавно)
		POINT cursor = { 0, 0 };  // курсор в клиентских координатах
		bool  dark = true;        // системная тема
	};
}
