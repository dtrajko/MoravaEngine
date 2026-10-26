#pragma once


/**
 * A check of the animation code at the renderer's start (docs/engine/Game_Engine_Runtime_Guide.html, 6.4), as the scene's
 * and the prefabs' self-tests: a skeleton and a clip made in code, sampled (between keys, looped, held, In Place),
 * blended and turned into bone matrices, and an animator controller's file read, written and read again. Logs "animation
 * self-test failed: ..." (CI fails on it) or that it passed.
 */
namespace AnimationSelfTest
{
	bool Run();
}
