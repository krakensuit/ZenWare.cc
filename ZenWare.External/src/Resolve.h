#pragma once
#include "Memory.h"

// Runtime offset resolution for the friend's game build: the hardcode in
// Offsets.h only matches our build. Signatures re-find the RVAs, the anchor
// scan looks for the entity array via the known local player pointer.
// If not found - the hardcode stays and the overlay shows "stale" as before.
struct Resolved_t
{
	uintptr_t local = 0; // RVA in client.dll
	uintptr_t list = 0;  // RVA in client.dll
	uintptr_t mat = 0;   // RVA in engine.dll
	char srcLocal[8] = { };
	char srcList[8] = { };
	char srcMat[8] = { };
	int matCands = 0; // how many thunk candidates passed the filters (diagnostics)
	int dbgCliKB = -1; // how many KB of client.dll were actually read
	int dbgLpHits = -1; // LP signature hits
	int dbgEngKB = -1; // how many KB of engine.dll were actually read
	int dbgThunks = -1; // total B9/E9 thunks
	int dbgDataSec = 0; // engine .data found (1) or not (0)
	uint32_t dbgCliHead = 0; // first 4 bytes of the read client.dll (must be MZ)
	uint32_t dbgEngHead = 0; // first 4 bytes of the read engine.dll
	uint32_t dbgCliSize = 0; // SizeOfImage client (modBaseSize)
	uint32_t dbgEngSize = 0; // SizeOfImage engine
};

bool ResolveOffsets(const Memory& mem, uintptr_t client, uint32_t clientSize,
	uintptr_t engine, uint32_t engineSize, Resolved_t& out);
