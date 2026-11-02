#pragma once

#include <cstring>


// How a picture's edges are smoothed (docs/engine/Game_Engine_Runtime_Guide.html, 8.1): the scene's render settings for
// the main view, a camera's choice for a render texture. Saved by name.
enum class AntialiasingMode
{
	Off = 0,
	FXAA, // one pass, cheaper, softer
	SMAA, // three passes, sharper
};

inline const char* GetAntialiasingModeName(AntialiasingMode mode)
{
	switch (mode)
	{
	case AntialiasingMode::FXAA: return "FXAA";
	case AntialiasingMode::SMAA: return "SMAA";
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
	return std::strcmp(name, "SMAA") == 0 ? AntialiasingMode::SMAA : AntialiasingMode::Off;
}
