#pragma once

#include <cstring>


// How a picture's edges are smoothed (docs/engine/Game_Engine_Runtime_Guide.html, 8.1, 8.2): the scene's render settings for
// the main view, a camera's choice for a render texture (not TAA: texture views have no history). Saved by name.
enum class AntialiasingMode
{
	Off = 0,
	FXAA, // one pass, cheaper, softer
	SMAA, // three passes, sharper
	TAA,  // over the frames: also calms what shimmers when the camera moves. Main view only
};

inline const char* GetAntialiasingModeName(AntialiasingMode mode)
{
	switch (mode)
	{
	case AntialiasingMode::FXAA: return "FXAA";
	case AntialiasingMode::SMAA: return "SMAA";
	case AntialiasingMode::TAA: return "TAA";
	default: return "Off";
	}
}

// An unknown name: Off
inline AntialiasingMode GetAntialiasingModeFromName(const char* name)
{
	if (std::strcmp(name, "FXAA") == 0)
	{
		return AntialiasingMode::FXAA;
	}
	if (std::strcmp(name, "TAA") == 0)
	{
		return AntialiasingMode::TAA;
	}
	return std::strcmp(name, "SMAA") == 0 ? AntialiasingMode::SMAA : AntialiasingMode::Off;
}
