#pragma once

#include "../Hooks/Hooks.h"

class CGlobal_ModuleEntry
{
public:
	void Load();

	//Panic unload (F11): passivates all detours first, then removes them from
	//a detached thread while the game keeps running.
	void RequestUnload();
	bool IsShuttingDown() const;

private:
	static void UnloadWorker();

	volatile long m_bShuttingDown = 0;
};

namespace G { inline CGlobal_ModuleEntry ModuleEntry; }
