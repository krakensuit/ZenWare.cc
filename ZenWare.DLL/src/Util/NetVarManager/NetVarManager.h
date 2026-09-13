#pragma once

#include "../Interface/Interface.h"

class CUtil_NetVarManager
{
public:
	int Get(const char* const szClass, const char* const szVar);
	// nExpectSize = sizeof(типа) из M_NETVAR: сверяется с реальным размером
	// пропа из RECVINFO, расхождение пишется в лог (мусорное чтение соседей).
	int Get(const char* const szClass, const char* const szVar, int nExpectSize);
};

namespace U { inline CUtil_NetVarManager NetVar; }

#define M_NETVAR(_name, type, table, name) inline type &_name() \
{ \
	static const int nOff = U::NetVar.Get(_(table), _(name), (int)sizeof(type)); \
	return *reinterpret_cast<type*>(reinterpret_cast<unsigned long>(this) + nOff); \
}