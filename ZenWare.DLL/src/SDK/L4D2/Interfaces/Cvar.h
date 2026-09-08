#pragma once

// Minimal ICvar/ConVar for L4D2 (VEngineCvar007, engine.dll).
// Only the slots we use are modelled; indices are verified empirically
// against a working L4D2 build: ICvar::FindVar sits at +0x34 (index 13),
// ConVar::SetValue(int) at +0x3C (index 15) - the callee passes 0/-100 there.
// Full vtable layouts differ per engine branch, so never extend by guessing.

class ConVar
{
public:
	virtual void _pad00() = 0;
	virtual void _pad01() = 0;
	virtual void _pad02() = 0;
	virtual void _pad03() = 0;
	virtual void _pad04() = 0;
	virtual void _pad05() = 0;
	virtual void _pad06() = 0;
	virtual void _pad07() = 0;
	virtual void _pad08() = 0;
	virtual void _pad09() = 0;
	virtual void _pad10() = 0;
	virtual void _pad11() = 0;
	virtual void _pad12() = 0;
	virtual void _pad13() = 0;
	virtual void _pad14() = 0;
	virtual void SetValue(int nValue) = 0; //15
};

class ICvar
{
public:
	virtual void _pad00() = 0;
	virtual void _pad01() = 0;
	virtual void _pad02() = 0;
	virtual void _pad03() = 0;
	virtual void _pad04() = 0;
	virtual void _pad05() = 0;
	virtual void _pad06() = 0;
	virtual void _pad07() = 0;
	virtual void _pad08() = 0;
	virtual void _pad09() = 0;
	virtual void _pad10() = 0;
	virtual void _pad11() = 0;
	virtual void _pad12() = 0;
	virtual ConVar* FindVar(const char* szName) = 0; //13
};

namespace I { inline ICvar* Cvar = nullptr; }
