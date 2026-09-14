#pragma once

#include "../Interface/Interface.h"

class CUtil_NetVarManager
{
public:
	int Get(const char* const szClass, const char* const szVar);
	// nExpectSize = sizeof(type) from M_NETVAR: checked against the real prop
	// size from RECVINFO; a mismatch is written to the log (garbage reads of neighbors).
	int Get(const char* const szClass, const char* const szVar, int nExpectSize);
};

namespace U { inline CUtil_NetVarManager NetVar; }

#define M_NETVAR(_name, type, table, name) inline type &_name() \
{ \
	static const int nOff = U::NetVar.Get(_(table), _(name), (int)sizeof(type)); \
	return *reinterpret_cast<type*>(reinterpret_cast<unsigned long>(this) + nOff); \
}