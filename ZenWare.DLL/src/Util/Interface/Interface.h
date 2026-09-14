#pragma once

#include "../VFunc/VFunc.h"

class CUtil_Interface
{
public:
	template<typename T>
	inline T Get(const char* const szModule, const char* const szObject)
	{
		const HMODULE hMod = GetModuleHandleA(szModule);

		if (!hMod)
			return NULL;

		void* pRet = GetInterface(hMod, szObject);

		if (pRet)
		{
			return static_cast<T>(pRet);
		}
		else
		{
			MessageBoxA(HWND_DESKTOP, szObject, "Failed to find interface:", MB_ICONERROR);
			return NULL;
		}
	}

	// Silent grab without a popup: for probing several modules (cvar lives
	// sometimes in engine, sometimes in vstdlib depending on the game build).
	template<typename T>
	inline T TryGet(const char* const szModule, const char* const szObject)
	{
		const HMODULE hMod = GetModuleHandleA(szModule);

		if (!hMod)
			return NULL;

		return static_cast<T>(GetInterface(hMod, szObject));
	}

private:
	void* GetInterface(const HMODULE hModule, const char* const szObject);
};

namespace U { inline CUtil_Interface Interface; }