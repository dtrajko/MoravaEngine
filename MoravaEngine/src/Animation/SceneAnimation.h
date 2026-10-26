#pragma once

#include "AnimationClip.h"
#include "AnimatorController.h"

#include "EnvMapVulkan/EnvMapVulkanComponents.h"

#include "H2M/Core/RefH2M.h"

#include <map>
#include <memory>
#include <string>
#include <vector>


class EnvMapVulkanScene;

/**
 * A running game's animators (docs/engine/Game_Engine_Runtime_Guide.html, 6.4): for each entity with an AnimatorComponent
 * and a skinned model, its controller's state machine. GameSystems owns it, the same in the player, the editor's Play
 * and Simulate (with nothing setting the parameters there: the entry state plays).
 *
 * Each frame (Update, after the scripts' OnUpdate set the parameters): the first transition that may be taken is
 * (those from Any State first, then the current state's), the states' clips move on by the frame's time, the current
 * state's clip is sampled and, while a crossfade runs, blended with the state it came from; the model takes the pose
 * (ModelH2M::SetPose) and draws it until Stop gives it back. A crossfade started during another one fades from the pose
 * shown then, so there is no jump. A controller edited during the game (the Animator panel) is taken at the next frame,
 * its states kept by name. The editor also runs one outside the game (Sync each frame): its scene's animators in their
 * entry states, as the game starts them.
 */
class SceneAnimation
{
public:
	void Start(EnvMapVulkanScene& scene);
	// Entities new in the running game (a prefab placed, 6.2): their animators, in their entry states
	void AddEntities(const std::vector<EnvMapVulkanEntityID>& entities);
	// The editor's preview (edit mode: the entry states): the scene as edited since the last frame. Entities that got an
	// Animator come in, and those that lost it, or whose controller file or model changed, go (or start again)
	void Sync();
	void Update(float frameTime);
	void Stop();
	bool IsRunning() const { return m_Scene != nullptr; }

	enum class Result { Done, NoAnimator, NoParameter, NoState };
	// A parameter, by name and type (a trigger: set, or reset with value 0)
	Result SetParameter(EnvMapVulkanEntityID entity, const std::string& name, AnimatorParameterType type, float value);
	Result GetParameter(EnvMapVulkanEntityID entity, const std::string& name, AnimatorParameterType type, float& value) const;
	// To a state at once, crossfading for fade seconds (0: a cut), whatever the transitions say
	Result Play(EnvMapVulkanEntityID entity, const std::string& state, float fade);

	// What an animator does now: the Inspector shows it, scripts read it (Animator.CurrentState...)
	struct Info
	{
		std::string Controller;              // its file
		std::string State;                   // the current state ("" when the controller has none)
		float StateTime = 0.0f;              // seconds played in it (times its speed)
		float NormalizedTime = 0.0f;         // 1 at the clip's end (more than 1 after loops)
		std::string PreviousState;           // the state a crossfade comes from ("" when none, or from a pose)
		bool InTransition = false;
		float FadeProgress = 0.0f;           // 0 to 1
		uint64_t Transitions = 0;            // taken since the game started
		std::vector<float> Values;           // the parameters', in the controller's order
		std::string Problem;                 // why it doesn't animate ("" when it does)
	};
	bool GetInfo(EnvMapVulkanEntityID entity, Info& info) const;
	std::shared_ptr<AnimatorController> GetController(EnvMapVulkanEntityID entity) const;
	uint32_t GetAnimatorCount() const { return (uint32_t)m_Animators.size(); }

private:
	struct StateClip
	{
		std::shared_ptr<const std::vector<AnimationClip>> File; // keeps Clip alive
		const AnimationClip* Clip = nullptr;
		AnimationBinding Binding;
	};
	struct Animator
	{
		EnvMapVulkanEntityID Entity = NoEntity;
		H2M::RefH2M<H2M::ModelH2M> Model;
		const H2M::ModelH2M* SourceModel = nullptr; // the entity's model when added, animated or not (Sync)
		std::shared_ptr<AnimatorController> Controller;
		std::string ControllerFile;
		uint64_t Revision = 0;         // the controller's when bound (0: not yet)
		std::vector<StateClip> Clips;  // the states'
		std::vector<float> Values;     // the parameters'
		std::vector<std::string> StateNames, ParameterNames; // as bound: an edit is matched to them by name
		int Current = -1;
		float CurrentTime = 0.0f;
		int Previous = -1;             // -1 while fading: from Snapshot
		float PreviousTime = 0.0f;
		float FadeDuration = 0.0f, FadeElapsed = 0.0f;
		bool Fading = false;
		AnimationPose Snapshot, From, To, Pose;
		uint64_t Transitions = 0;
		std::string Problem;
	};

	void AddEntity(EnvMapVulkanEntityID entity);
	// The controller's states and parameters (again, after an edit): states and values kept by name
	void Bind(Animator& animator);
	void StartTransition(Animator& animator, int to, float fade);
	bool CanTake(const Animator& animator, const AnimatorTransition& transition) const;
	float GetNormalizedTime(const Animator& animator, int state, float time) const;
	float GetStateSpeed(const Animator& animator, int state) const;
	void SampleState(const Animator& animator, int state, float time, AnimationPose& pose) const;

	EnvMapVulkanScene* m_Scene = nullptr;
	std::map<EnvMapVulkanEntityID, Animator> m_Animators;
};
