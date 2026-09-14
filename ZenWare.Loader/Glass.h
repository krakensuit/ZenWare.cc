#pragma once

// ZenWare Loader - Liquid Glass
// --------------------------------------------
// Эффект «жидкого стекла» для окна загрузчика:
//   * акриловый блюр рабочего стола под окном (Win10 1803+)
//   * мятный полупрозрачный тинт (по умолчанию #73FFCC, alpha 0.85)
//   * блик по краям окна, анимированный отблеск, подсветка под курсором
//   * закруглённые углы (DWM на Win11, регион-фолбэк на Win10)
// Реализовано на WinAPI + GDI. Внешние библиотеки не нужны.

#include <windows.h>

namespace Glass
{
	// Мятный цвет по умолчанию (#73FFCC) и его альфа (0.85).
	constexpr COLORREF kMint = RGB(0x73, 0xFF, 0xCC);
	constexpr BYTE       kMintAlpha = 217;
	constexpr int        kCornerRadius = 10;

	// Системный акрил доступен (SetWindowCompositionAttribute присутствует).
	// Зовётся ДО создания окна: от этого зависит стиль окна.
	bool IsAvailable();

	// Включает стекло. alpha = стартовая прозрачность тинта (0..255).
	// Возвращает true, если блюр включён.
	bool Enable(HWND hwnd, COLORREF tint = kMint, BYTE alpha = kMintAlpha);

	// Плавное изменение прозрачности тинта (заменяет layered fade-in).
	void SetAlpha(HWND hwnd, BYTE alpha);

	// Выключить блюр (перед уничтожением окна / при смене темы).
	void Disable(HWND hwnd);

	// Системный материал Windows 11 (DWMSBT_MAINWINDOW) - главный путь из ТЗ.
	// Возвращает true, если DWM принял материал (build >= 22000).
	bool EnableSystemBackdrop(HWND hwnd);
	// Номер сборки Windows через RtlGetVersion (GetVersionEx врёт при манифесте).
	DWORD OsBuild();

	// Закругление углов: DWM (Win11), иначе регион окна.
	void RoundCorners(HWND hwnd, int radius = kCornerRadius);

	// Рисует поверх готового кадра: рамка-блик, отблеск по времени,
	// мягкое пятно под курсором. hdc — DC окна, rc — клиентская область.
	void PaintGlass(HDC hdc, const RECT& rc, POINT mouse);

	// Освобождение кэшированных GDI-ресурсов.
	void Shutdown();
}
