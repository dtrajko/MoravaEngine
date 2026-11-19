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
 * and a skinned model, its controller's state machines. GameSystems owns it, the same in the player, the editor's Play
 * and Simulate (with nothing setting the parameters there: the entry states play).
 *
 * Each frame (Update, after the scripts' OnUpdate set the parameters), in the base and in each layer: the first
 * transition that may be taken is (those from Any State first, then the current state's), the states' clips move on by
 * the frame's time, the current state's clip (or a blend state's clips, by its parameter) is sampled and, while a
 * crossfade runs, blended with the state it came from. The layers then take over the nodes of their masks from the pose
 * under them, each by its weight; the model takes the pose (ModelH2M::SetPose) and draws it until Stop gives it back. A
 * crossfade started during another one fades from the pose shown then, so there is no jump. A controller edited during
 * the game (the Animator panel) is taken at the next frame, its states and layers kept by name. The editor also runs one
 * outside the game (Sync each frame): its scene's animators in their entry states, as the game starts them.
 *
 * The events of the states' clips (AnimatorState::Events) that a frame's time passes are collected for the frame
 * (GetEvents; GameSystems gives them to the scripts), from the base and the layers. During a crossfade only the state
 * that weighs more fires its events, so a foot coming down in both clips is heard once.
 *
 * Then the bone attachments (BoneAttachmentComponent): each one's bone, where its parent's pose puts it (the bind pose
 * when the parent has no animator), so the entity follows it in the frame.
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

	enum class Result { Done, NoAnimator, NoParameter, NoState, NoLayer };
	// A parameter, by name and type (a trigger: set, or reset with value 0)
	Result SetParameter(EnvMapVulkanEntityID entity, const std::string& name, AnimatorParameterType type, float value);
	Result GetParameter(EnvMapVulkanEntityID entity, const std::string& name, AnimatorParameterType type, float& value) const;
	// To a state at once, crossfading for fade seconds (0: a cut), whatever the transitions say; in the base (layer "") or
	// in the layer of that name
	Result Play(EnvMapVulkanEntityID entity, const std::string& state, float fade, const std::string& layer = std::string());

	// A node of the entity's model (a bone: hand_r) in the world, as the frame's pose has it (its bind pose without an
	// animator). False: the entity has no model with a skeleton, or no such node
	bool GetNodeTransform(EnvMapVulkanEntityID entity, const std::string& node, glm::mat4& world) const;
	// The same in the model's bind pose, without a running animation (the Inspector placing an attachment)
	static bool GetBindNodeTransform(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, const std::string& node, glm::mat4& world);

	// What a layer does now
	struct LayerInfo
	{
		std::string Name;                    // "" for the base
		std::string State;                   // the current state ("" when it has none)
		float StateTime = 0.0f;              // seconds played in it (times its speed)
		float NormalizedTime = 0.0f;         // 1 at the clip's end (more than 1 after loops)
		std::string PreviousState;           // the state a crossfade comes from ("" when none, or from a pose)
		bool InTransition = false;
		float FadeProgress = 0.0f;           // 0 to 1
		float Weight = 1.0f;                 // as it blends now (with its weight parameter)
	};
	// What an animator does now: the Inspector shows it, scripts read it (Animator.CurrentState...). The base's state is
	// in the first fields, the layers' in Layers
	struct Info : LayerInfo
	{
		std::string Controller;              // its file
		uint64_t Transitions = 0;            // taken since the game started (the base's and the layers')
		uint64_t Events = 0;                 // fired since the game started
		std::string LastEvent;               // the last one's name and state, as "Footstep (Walk)" ("" when none yet)
		std::vector<float> Values;           // the parameters', in the controller's order
		std::vector<LayerInfo> Layers;       // the layers over the base, in order
		std::string Problem;                 // why it doesn't animate ("" when it does)
	};
	bool GetInfo(EnvMapVulkanEntityID entity, Info& info) const;
	std::shared_ptr<AnimatorController> GetController(EnvMapVulkanEntityID entity) const;

	// The events the last Update fired, in order (cleared by the next one)
	struct Event
	{
		EnvMapVulkanEntityID Entity = NoEntity;
		std::string Name;
		std::string State;
	};
	const std::vector<Event>& GetEvents() const { return m_Events; }
	uint32_t GetAnimatorCount() const { return (uint32_t)m_Animators.size(); }

private:
	struct StateClip
	{
		std::shared_ptr<const std::vector<AnimationClip>> File; // keeps Clip alive
		const AnimationClip* Clip = nullptr;
		AnimationBinding Binding;
	};
	// The base's or a layer's states as they play
	struct Machine
	{
		std::string Layer;                         // "" for the base
		std::vector<std::vector<StateClip>> Clips; // each state's clip (a blend state's: its clips, in threshold order)
		std::vector<std::string> StateNames;       // as bound: an edit is matched to them by name
		std::vector<uint8_t> Mask;                 // the nodes it moves (AnimationSkeleton::GetMask)
		int Current = -1;
		float CurrentTime = 0.0f;                  // normalized: 1 at the end of the state's clip (more after loops)
		int Previous = -1;                         // -1 while fading: from Snapshot
		float PreviousTime = 0.0f;
		float FadeDuration = 0.0f, FadeElapsed = 0.0f;
		bool Fading = false;
		float Weight = 1.0f;                       // the frame's (a layer's, with its parameter)
		AnimationPose Snapshot, From, To, Pose;
	};
	struct Animator
	{
		EnvMapVulkanEntityID Entity = NoEntity;
		H2M::RefH2M<H2M::ModelH2M> Model;
		const H2M::ModelH2M* SourceModel = nullptr; // the entity's model when added, animated or not (Sync)
		std::shared_ptr<AnimatorController> Controller;
		std::string ControllerFile;
		uint64_t Revision = 0;         // the controller's when bound (0: not yet)
		std::vector<float> Values;     // the parameters'
		std::vector<std::string> ParameterNames; // as bound: an edit is matched to them by name
		std::vector<Machine> Machines; // the base, then the controller's layers in order
		AnimationPose Pose;            // the frame's, every layer's
		uint64_t Transitions = 0;
		uint64_t Events = 0;
		std::string LastEvent;
		std::string Problem;
	};

	void AddEntity(EnvMapVulkanEntityID entity);
	// The controller's parameters, layers and states (again, after an edit): values, layers and states kept by name
	void Bind(Animator& animator);
	void BindMachine(Animator& animator, const AnimatorStateMachine& states, Machine& machine, bool rebinding);
	void UpdateMachine(Animator& animator, Machine& machine, const AnimatorStateMachine& states, float frameTime);
	void StartTransition(Animator& animator, Machine& machine, int to, float fade);
	bool CanTake(const Animator& animator, const Machine& machine, const AnimatorStateMachine& states, const AnimatorTransition& transition) const;
	// A state's length in seconds as it plays now (a blend state's: its clips' weighted by its parameter; 0: no clip)
	float GetStateLength(const Animator& animator, const Machine& machine, const AnimatorStateMachine& states, int state) const;
	float GetStateSpeed(const Animator& animator, const AnimatorStateMachine& states, int state) const;
	// A blend state's two clips at its parameter (of clips bound), and the second one's weight
	void GetBlend(const Animator& animator, const AnimatorState& state, size_t clips, size_t& a, size_t& b, float& weight) const;
	void SampleState(const Animator& animator, const Machine& machine, const AnimatorStateMachine& states, int state, float time, AnimationPose& pose);
	// The state's events between two of its normalized times (before and after the frame)
	void FireEvents(Animator& animator, const AnimatorStateMachine& states, int state, float from, float to);
	// The attachments' bones, where the frame's poses put them
	void UpdateAttachments();
	const AnimationPose* GetPose(EnvMapVulkanEntityID entity) const;

	EnvMapVulkanScene* m_Scene = nullptr;
	std::map<EnvMapVulkanEntityID, Animator> m_Animators;
	std::vector<Event> m_Events;
	std::vector<size_t> m_PassedEvents; // FireEvents' (kept: no allocation per frame)
	AnimationPose m_BlendA, m_BlendB;   // a blend state's two clips (kept)
};
