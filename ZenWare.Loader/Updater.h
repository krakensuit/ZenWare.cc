#pragma once
#include <windows.h>

// Фоновая проверка обновлений с GitHub Releases (публичный репозиторий).
// Потокобезопасно: в UI пишет только через LoaderUtil::Status/Log.
// Не нашёл новее / нет сети — молча ничего не делает (строка в лог).
namespace Updater
{
	// hMain нужен для MessageBox и WM_CLOSE после обновления.
	void CheckAsync(HWND hMain);
}
