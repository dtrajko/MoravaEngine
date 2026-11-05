#pragma once


/**
 * GLSL to SPIR-V through shaderc: in the editor's part of the engine, not in the core. A player reads the shaders a pack holds
 * (docs/engine/Game_Engine_Runtime_Guide.html, 10.3) and never compiles, and doesn't link shaderc.
 */
namespace ShaderCompile
{
	// Gives VulkanShaderH2M the compiler, at the editor's startup
	void Install();
}
