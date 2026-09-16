#pragma once

#include <windows.h>

// Custom fonts embedded in the DLL as RCDATA resources.
//
// The .ttf files are not part of the repository yet: the loader is written so
// that a missing resource is not an error. When the files are added under
// ZenWare.DLL\res\fonts\ and listed in resource.rc (see the resource table in
// the notes), Load() starts returning a handle and the renderer picks the real
// faces up automatically.
namespace Fonts
{
	enum EResource : int
	{
		INTER_REGULAR = 300,
		INTER_MEDIUM = 301,
		INTER_SEMIBOLD = 302,
		JETBRAINS_MONO = 303,
		FONT_AWESOME6_SOLID = 304,
	};

	// Registers one font from its RCDATA resource in the current process through
	// AddFontMemResourceEx. Returns the handle to keep alive, or nullptr when the
	// resource is missing or the API refused the file. szName is used for logs only.
	HANDLE Load(int nResourceId, const char* szName);

	// Registers every font listed in EResource. Safe to call when resources are
	// absent: it logs and continues.
	void LoadAll();

	// Releases everything registered by Load/LoadAll. Call from the module unload.
	void FreeAll();

	// Number of fonts currently registered (for logs and diagnostics).
	int Count();
}
