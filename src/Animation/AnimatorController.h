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

// A moment of a state's clip the game hears of (Entity.OnAnimationEvent): a foot down (Footstep), a take-off. Time is
// normalized, 0 the clip's start and 1 its end; in a looped state it comes again in each loop
struct AnimatorEvent
{
	std::string Name;
	float Time = 0.0f;
};

// One clip of a blend state, and the parameter's value where it alone shows
struct AnimatorBlendClip
{
	std::string ClipFile; // as a state's (empty: the animated model's own file)
	std::string ClipName;
	float Threshold = 0.0f;
};

struct AnimatorState
{
	std::string Name;
	// The clip: a model file's (absolute here, relative to the project in the file; empty: the animated model's own file)
	// and its name there (empty: its first clip)
	std::string ClipFile;
	std::string ClipName;
	// A blend state (Blend not empty: Clip isn't used): its clips blended by a Float parameter, between the two whose
	// thresholds it lies between (below the first or above the last: that one alone). Sorted by threshold. The clips play in
	// step, each at the same fraction of its length (a walk's feet meet the ground together): the state's length is theirs
	// weighted as they show. An aim (up, ahead, down by the look's pitch), a walk by its direction
	std::vector<AnimatorBlendClip> Blend;
	std::string BlendParameter;
	float Speed = 1.0f;          // 2: twice as fast; negative plays backwards
	std::string SpeedParameter;  // a Float parameter the speed is multiplied by (empty: none)
	bool Loop = true;            // else held on its last frame
	bool InPlace = false;        // the clip's root (the hips) doesn't move across the ground: a walk without root motion
	std::vector<AnimatorEvent> Events; // sorted by time
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

// States and the transitions between them: the controller's own (its base layer) and each layer's
struct AnimatorStateMachine
{
	std::string Entry; // the state it starts in (empty or unknown: the first)
	std::vector<AnimatorState> States;
	std::vector<AnimatorTransition> Transitions;

	int FindState(const std::string& name) const;
	int GetEntryState() const; // -1: no states
};

// A layer over the base (and the layers before it): its own state machine, whose pose takes over the nodes of its mask by
// its weight. An upper body that aims while the legs walk: the mask from the spine up, the weight 1
struct AnimatorLayer : AnimatorStateMachine
{
	std::string Name;
	// The nodes it moves: each named node with every node under it (empty: all of them)
	std::vector<std::string> Mask;
	float Weight = 1.0f;          // 0 to 1
	std::string WeightParameter;  // a Float parameter the weight is multiplied by (empty: none): a layer faded in and out
};

// The controller is its base layer (all nodes, weight 1), the parameters every layer reads, and the layers over it in order
struct AnimatorController : AnimatorStateMachine
{
	static constexpr int Version = 1;
	static constexpr const char* FileExtension = ".manim";

	std::vector<AnimatorParameter> Parameters;
	std::vector<AnimatorLayer> Layers;

	// Changes with every edit (MarkChanged): a running animator takes the edited controller then
	uint64_t Revision = 1;
	void MarkChanged() { Revision++; }

	int FindParameter(const std::string& name) const;
	int FindLayer(const std::string& name) const; // -1: none (the base has no name)
	// The base (layer -1) or a layer's state machine; nullptr when there is no such layer
	const AnimatorStateMachine* GetMachine(int layer) const;
	AnimatorStateMachine* GetMachine(int layer);

	// The file as text, and back. Load: false with the reason in error when it isn't a controller file
	std::string ToText() const;
	static bool FromText(const std::string& text, const std::string& filepath, AnimatorController& controller, std::string& error);
	static bool Load(const std::string& filepath, AnimatorController& controller, std::string& error);
	bool Save(const std::string& filepath, std::string& error) const;

	// The state's events in time order (after one was added or its time edited)
	static void SortEvents(AnimatorState& state);
	// A blend state's clips in threshold order (after one was added or its threshold edited)
	static void SortBlend(AnimatorState& state);
	// The state's events a frame passed, from one normalized time of its clip to another (backwards: to below from), in the
	// order passed: [from, to), the clip's end included when not looped; a looped state's in each loop passed (a frame
	// longer than a loop: each event once)
	static void GetPassedEvents(const AnimatorState& state, float from, float to, std::vector<size_t>& events);

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
