#pragma once

// Game-independent overlay window.
//
// Stage 1 of moving the in-game GUI off the game renderer. The window, its message
// loop and its clock belong to the DLL alone: there is no game hook here, so a pause,
// a stalled game loop or a low frame rate cannot freeze this surface, and a fault in
// it cannot take the game down with it.
//
// Right now the overlay draws the product watermark and its own frame rate, which is
// the measurable proof that the surface really is independent. Later stages move the
// menu drawing here.
namespace Overlay
{
	// Creates the window and the render thread. Safe to call once; returns false when
	// the feature is switched off or the window could not be created.
	bool Init();

	// Signals the thread to stop and waits for it to leave.
	void Shutdown();
}
