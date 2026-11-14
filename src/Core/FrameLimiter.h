#pragma once

// The frame rate limit (docs/engine/Game_Engine_Runtime_Guide.html, #frame-rate-limit): the game loop waits at the end of a
// frame until the next one is due, so the CPU and the GPU rest instead of drawing frames nobody needs. On battery a lower
// limit can apply (a laptop that runs on its battery: less power drawn, less heat, and a battery or charger that can't give
// the GPU's full power doesn't turn the computer off).
//
// The player sets them from the project and the player's choice (PlayerSettings); the editor sets none (its frame times are
// what a frame really costs).

namespace FrameLimiter
{
	// Frames a second at most; 0: no limit
	void SetLimit(int framesPerSecond);
	int GetLimit();
	// On battery: frames a second at most (the lower of this and SetLimit's); 0: as plugged in
	void SetBatteryLimit(int framesPerSecond);
	int GetBatteryLimit();

	// The computer runs on its battery (asked of the system every two seconds; false when it can't tell). --on-battery
	// pretends it does (ForceOnBattery), to see the battery limit on a desktop
	bool IsOnBattery();
	void ForceOnBattery(bool onBattery);

	// VSync's refresh rate (0: VSync off): a limit at or above it is left to VSync, which keeps the frames even
	void SetVSyncRate(int hertz);

	// The limit in force now (0: none), and why ("on battery", "VSync holds it"): the debug overlay's row
	int GetActiveLimit();
	const char* GetActiveReason();

	// At the end of a frame, after it was presented: waits until the next frame is due
	void Wait();
}
