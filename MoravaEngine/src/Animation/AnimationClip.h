#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>


struct aiAnimation;
struct aiScene;

/**
 * Skeletal animation apart from the model that shows it (docs/engine/Game_Engine_Runtime_Guide.html, 6.4): a clip is
 * sampled at a time into a pose, two poses are blended, and a pose becomes the bone matrices the skinning shaders read.
 * ModelH2M plays its own clips with it (the Inspector's Animation section), the animator (SceneAnimation) its states'.
 *
 * The skeleton is the model file's node tree; a clip keys nodes by name, so a clip from another file of the same rig (a
 * character's idle.dae next to its walking.dae, as Mixamo exports them) drives the same skeleton. Times are in seconds.
 */

// A node's place relative to its parent, in parts that blend: lerp for translation and scale, slerp for rotation
struct AnimationTransform
{
	glm::vec3 Translation = glm::vec3(0.0f);
	glm::quat Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 Scale = glm::vec3(1.0f);

	glm::mat4 GetMatrix() const;
};

// Each node of a skeleton where a clip (or a blend of two) puts it. Animated false: the node is in its bind place, and its
// matrix is taken as the file has it (exact, also for what translation, rotation and scale can't express)
struct AnimationPose
{
	std::vector<AnimationTransform> Nodes;
	std::vector<uint8_t> Animated;
};

// A model file's node tree, flattened with each parent before its children, and the bones its skinned meshes are bound to
class AnimationSkeleton
{
public:
	struct Node
	{
		std::string Name;
		int Parent = -1;                       // -1: the root
		glm::mat4 BindMatrix = glm::mat4(1.0f); // relative to the parent, as in the file
		AnimationTransform Bind;               // the same in parts
		int Bone = -1;                         // the bone matrix it gives, -1: none
	};

	// boneMapping: bone name to bone index; boneOffsets: each bone's offset (mesh space to bone space)
	AnimationSkeleton(const aiScene* scene, const std::unordered_map<std::string, uint32_t>& boneMapping, const std::vector<glm::mat4>& boneOffsets);

	const std::vector<Node>& GetNodes() const { return m_Nodes; }
	int FindNode(const std::string& name) const;
	uint32_t GetBoneCount() const { return (uint32_t)m_BoneOffsets.size(); }

	// Every node in its bind place
	void GetBindPose(AnimationPose& pose) const;
	// The skinning shaders' matrices for the pose: the inverse root transform * the bone's world in the file * its offset
	void ComputeBoneTransforms(const AnimationPose& pose, std::vector<glm::mat4>& boneTransforms) const;

private:
	std::vector<Node> m_Nodes;
	std::unordered_map<std::string, int> m_NodeByName;
	std::vector<glm::mat4> m_BoneOffsets;
	glm::mat4 m_InverseRoot = glm::mat4(1.0f);
};

// One animation of a file: keys for some of its nodes, by name
class AnimationClip
{
public:
	struct Channel
	{
		std::string Node;
		std::vector<float> PositionTimes, RotationTimes, ScaleTimes;
		std::vector<glm::vec3> Positions, Scales;
		std::vector<glm::quat> Rotations;
	};

	AnimationClip() = default;
	// The file's ticks become seconds (a file without a tick rate: 25 per second, as Assimp's)
	explicit AnimationClip(const aiAnimation* animation, uint32_t index);

	const std::string& GetName() const { return m_Name; }
	float GetDuration() const { return m_Duration; } // seconds
	const std::vector<Channel>& GetChannels() const { return m_Channels; }

	// For tests: a clip made in code
	void SetName(const std::string& name) { m_Name = name; }
	void SetDuration(float duration) { m_Duration = duration; }
	std::vector<Channel>& GetChannels() { return m_Channels; }

private:
	std::string m_Name;
	float m_Duration = 0.0f;
	std::vector<Channel> m_Channels;
};

// A clip bound to a skeleton: the node each channel drives (-1: the skeleton has no such node), and its root channel (the
// animated node nearest the skeleton's root: a walk's hips), which In Place holds still across the ground
struct AnimationBinding
{
	std::vector<int> ChannelNodes;
	int RootChannel = -1;
	glm::vec3 RootUp = glm::vec3(0.0f, 1.0f, 0.0f); // the world's up in the root channel's parent space (bind pose)
	uint32_t BoundChannels = 0;                     // channels whose node the skeleton has

	static AnimationBinding Bind(const AnimationSkeleton& skeleton, const AnimationClip& clip);
};

namespace AnimationSampling
{
	// The clip at time (seconds; looped: wrapped into the clip, else held at its ends) into pose, over its nodes: the
	// nodes the clip has no keys for are left as pose had them. inPlace: the root channel keeps its first key's place
	// across the ground (its height still moves): a walk without root motion
	void SampleClip(const AnimationClip& clip, const AnimationBinding& binding, float time, bool loop, bool inPlace, AnimationPose& pose);

	// from towards to by weight (0: from, 1: to), into out (may be from or to)
	void BlendPoses(const AnimationPose& from, const AnimationPose& to, float weight, const AnimationSkeleton& skeleton, AnimationPose& out);

	// The time a clip shows: wrapped into it when looped, else held at its ends
	float GetClipTime(float time, float duration, bool loop);
}

// The clips of model files, each file read once and shared (the animator's states name clips by file and clip name)
namespace AnimationClipLibrary
{
	// The file's clips; an empty list when it has none or can't be read (logged once)
	std::shared_ptr<const std::vector<AnimationClip>> Load(const std::string& path);
	// The clip of that name in the file's clips (an empty name: the first); nullptr when there is none
	const AnimationClip* Find(const std::vector<AnimationClip>& clips, const std::string& name);
}
