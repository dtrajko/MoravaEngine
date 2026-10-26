#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>


/**
 * An animator controller (docs/engine/Game_Engine_Runtime_Guide.html, 6.4), a .manim file: the states a character's
 * animation can be in (each a clip, its speed, looped or not), the transitions between them (when, and how long the
 * crossfade), and the parameters the game sets to choose (Speed, Grounded, Jump). SceneAnimation plays it; the
 * Animator panel edits it. Plain data: states and parameters are named, and transitions name their states, so an edit
 * keeps the rest as it was.
 */

enum class AnimatorParameterType : int32_t { Float = 0, Bool = 1, Trigger = 2 };

// A value the game sets (Animator.SetFloat, SetBool, SetTrigger). A trigger is set until a transition takes it
struct AnimatorParameter
{
	std::string Name;
	AnimatorParameterType Type = AnimatorParameterType::Float;
	float Default = 0.0f; // Bool: 0 or 1; Trigger: not set
};

// Float: Greater or Less than Value. Bool: True or False. Trigger: set (Test isn't read)
enum class AnimatorConditionTest : int32_t { Greater = 0, Less = 1, True = 2, False = 3 };

struct AnimatorCondition
{
	std::string Parameter;
	AnimatorConditionTest Test = AnimatorConditionTest::Greater;
	float Value = 0.0f;
};

struct AnimatorState
{
	std::string Name;
	// The clip: a model file's (absolute here, relative to the project in the file; empty: the animated model's own file)
	// and its name there (empty: its first clip)
	std::string ClipFile;
	std::string ClipName;
	float Speed = 1.0f;          // 2: twice as fast; negative plays backwards
	std::string SpeedParameter;  // a Float parameter the speed is multiplied by (empty: none)
	bool Loop = true;            // else held on its last frame
	bool InPlace = false;        // the clip's root (the hips) doesn't move across the ground: a walk without root motion
};

// From one state (or from any: FromAny) to another, when every condition holds and, with an exit time, once the state
// has played that far (its normalized time: 1 is the clip's end; a looped state below 1: that far in each loop)
struct AnimatorTransition
{
	bool FromAny = false;
	std::string From;
	std::string To;
	float Fade = 0.15f; // seconds of crossfade
	bool HasExitTime = false;
	float ExitTime = 1.0f;
	std::vector<AnimatorCondition> Conditions;
};

struct AnimatorController
{
	static constexpr int Version = 1;
	static constexpr const char* FileExtension = ".manim";

	std::string Entry; // the state the animator starts in (empty or unknown: the first)
	std::vector<AnimatorParameter> Parameters;
	std::vector<AnimatorState> States;
	std::vector<AnimatorTransition> Transitions;

	// Changes with every edit (MarkChanged): a running animator takes the edited controller then
	uint64_t Revision = 1;
	void MarkChanged() { Revision++; }

	int FindState(const std::string& name) const;
	int FindParameter(const std::string& name) const;
	int GetEntryState() const; // -1: no states

	// The file as text, and back. Load: false with the reason in error when it isn't a controller file
	std::string ToText() const;
	static bool FromText(const std::string& text, const std::string& filepath, AnimatorController& controller, std::string& error);
	static bool Load(const std::string& filepath, AnimatorController& controller, std::string& error);
	bool Save(const std::string& filepath, std::string& error) const;

	static const char* GetParameterTypeName(AnimatorParameterType type);
	static const char* GetConditionTestName(AnimatorConditionTest test);
};

/**
 * The controllers of the open files, each loaded once and shared: the editor's Animator panel edits the same object the
 * running game plays, so a change (a fade, a speed) shows at once.
 */
namespace AnimatorControllerLibrary
{
	// The controller of the file (absolute), loaded the first time; nullptr when it can't be read (logged)
	std::shared_ptr<AnimatorController> Get(const std::string& filepath);
	// Read again from the file (the panel's Revert): the same object, with what the file has
	bool Reload(const std::string& filepath, std::string& error);
	// The controllers loaded, by file
	std::vector<std::pair<std::string, std::shared_ptr<AnimatorController>>> GetLoaded();
}
