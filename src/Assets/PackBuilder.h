#pragma once

#include <string>


/**
 * Builds a game's pack file (docs/engine/Game_Engine_Runtime_Guide.html, 10.3): MoravaEngine --build-pack <file.mproject>
 * [--pack-output <folder>]. In the editor's part of the engine: it imports models and compiles shaders. Runs without a window.
 *
 * What goes in: the project's asset, scene and settings folders, everything their files refer to (found by walking them, so a
 * missing asset is an error here and not a surprise in the game), the renderer's own files and all its shaders. Models and
 * material maps go in cooked, through the cook cache: what was cooked before and hasn't changed isn't cooked again.
 *
 * What a script names only in its code can't be found by walking: a project's own files are in the pack anyway, an engine file
 * named only in C# has to be used by a scene as well.
 *
 * The output folder gets Game.mpak and the game's script assembly beside it (default: <Project>/Build).
 */
namespace PackBuilder
{
	// The program's exit code: 0 when the pack was written, 1 otherwise (each reason is logged, and no pack is written)
	int Run(const std::string& projectFile, const std::string& outputFolder);
}
