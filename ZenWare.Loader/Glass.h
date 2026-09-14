#pragma once

// ZenWare Loader - системный бэкдроп окна (frosted glass).
// Реализация по спецификации: Win11 22H2+ -> Acrylic (DWMSBT_TRANSIENTWINDOW),
// Win10 1803+ -> SetWindowCompositionAttribute + ACCENT_ENABLE_ACRYLICBLURBEHIND,
// Win10 < 1803 -> ACCENT_ENABLE_BLURBEHIND. Если ничего не применилось —
// возвращаем false, и лоадер рисует обычный тёмный фон.

#include <windows.h>

namespace Glass
{
	// Мятный акцент (совпадает с палитрой Zen2D).
	constexpr COLORREF kMint = RGB(0x6E, 0xE7, 0xB7);
	constexpr BYTE     kMintAlpha = 0xCC;
	constexpr int      kCornerRadius = 10;

	// Номер сборки Windows через RtlGetVersion (ntdll). GetVersionEx врёт при манифесте.
	DWORD OsBuild();

	// Главный вход: поднимает бэкдроп и растягивает рамку DWM на клиентскую область.
	// true = бэкдроп реально применён (Win11 Acrylic или Win10 acrylic/blur).
	bool EnableSystemBackdrop(HWND hwnd);

	// Фолбэк-совместимость с прежним вызовом лоадера.
	bool Enable(HWND hwnd, COLORREF tint = kMint, BYTE alpha = 0);

	// true, если бэкдроп применён (окно НЕ должно быть layered).
	bool IsAvailable();

	// Прозрачность тинта (для fade-in; на системном материале — no-op).
	void SetAlpha(HWND hwnd, BYTE alpha);

	// Снять бэкдроп (перед уничтожением окна).
	void Disable(HWND hwnd);

	// Скругление углов: DWM на Win11, регион окна на Win10.
	void RoundCorners(HWND hwnd, int radius = kCornerRadius);

	// Освобождение внутренних ресурсов.
	void Shutdown();
}
