#pragma once
#include "Memory.h"

// Runtime-резолв оффсетов под билд игры друга: хардкод в Offsets.h совпадает
// только с нашим билдом. Сигнатуры находят RVA заново, якорный скан ищет
// массив сущностей по известному указателю локального игрока.
// Не нашлось — остаётся хардкод, оверлей как раньше покажет "stale".
struct Resolved_t
{
	uintptr_t local = 0; // RVA в client.dll
	uintptr_t list = 0;  // RVA в client.dll
	uintptr_t mat = 0;   // RVA в engine.dll
	char srcLocal[8] = { };
	char srcList[8] = { };
	char srcMat[8] = { };
	int matCands = 0; // сколько thunk-кандидатов прошло фильтры (диагностика)
	int dbgCliKB = -1; // сколько КБ client.dll реально прочиталось
	int dbgLpHits = -1; // совпадений LP-сигнатуры
	int dbgEngKB = -1; // сколько КБ engine.dll реально прочиталось
	int dbgThunks = -1; // всего B9/E9 thunk'ов
	int dbgDataSec = 0; // .data engine найдена (1) или нет (0)
	uint32_t dbgCliHead = 0; // первые 4 байта прочитанного client.dll (должен быть MZ)
	uint32_t dbgEngHead = 0; // первые 4 байта прочитанного engine.dll
	uint32_t dbgCliSize = 0; // SizeOfImage client (modBaseSize)
	uint32_t dbgEngSize = 0; // SizeOfImage engine
};

bool ResolveOffsets(const Memory& mem, uintptr_t client, uint32_t clientSize,
	uintptr_t engine, uint32_t engineSize, Resolved_t& out);
