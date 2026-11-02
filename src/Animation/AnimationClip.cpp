#include "AnimationClip.h"

#include "Core/Log.h"
#include "Core/Math.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <functional>
#include <map>
#include <set>
#include <system_error>


glm::mat4 AnimationTransform::GetMatrix() const
{
	return glm::translate(glm::mat4(1.0f), Translation) * glm::mat4_cast(Rotation) * glm::scale(glm::mat4(1.0f), Scale);
}

static AnimationTransform Decompose(const glm::mat4& matrix)
{
	AnimationTransform transform;
	glm::vec3 skew;
	glm::vec4 perspective;
	if (!glm::decompose(matrix, transform.Scale, transform.Rotation, transform.Translation, skew, perspective))
	{
		transform = AnimationTransform();
		transform.Translation = glm::vec3(matrix[3]);
	}
	transform.Rotation = glm::normalize(transform.Rotation);
	return transform;
}

// ---- AnimationSkeleton

AnimationSkeleton::AnimationSkeleton(const aiScene* scene, const std::unordered_map<std::string, uint32_t>& boneMapping, const std::vector<glm::mat4>& boneOffsets)
	: m_BoneOffsets(boneOffsets)
{
	if (!scene || !scene->mRootNode)
	{
		return;
	}
	m_InverseRoot = glm::inverse(Math::Mat4FromAssimpMat4(scene->mRootNode->mTransformation));

	std::function<void(const aiNode*, int)> add = [&](const aiNode* node, int parent)
	{
		Node entry;
		entry.Name = node->mName.C_Str();
		entry.Parent = parent;
		entry.BindMatrix = Math::Mat4FromAssimpMat4(node->mTransformation);
		entry.Bind = Decompose(entry.BindMatrix);
		auto bone = boneMapping.find(entry.Name);
		entry.Bone = bone != boneMapping.end() && bone->second < boneOffsets.size() ? (int)bone->second : -1;
		const int index = (int)m_Nodes.size();
		m_NodeByName.emplace(entry.Name, index); // the first node of a name (names are unique in a rig)
		m_Nodes.push_back(std::move(entry));
		for (uint32_t i = 0; i < node->mNumChildren; i++)
		{
			add(node->mChildren[i], index);
		}
	};
	add(scene->mRootNode, -1);
}

int AnimationSkeleton::FindNode(const std::string& name) const
{
	auto it = m_NodeByName.find(name);
	return it != m_NodeByName.end() ? it->second : -1;
}

void AnimationSkeleton::GetBindPose(AnimationPose& pose) const
{
	pose.Nodes.resize(m_Nodes.size());
	pose.Animated.assign(m_Nodes.size(), 0);
	for (size_t i = 0; i < m_Nodes.size(); i++)
	{
		pose.Nodes[i] = m_Nodes[i].Bind;
	}
}

void AnimationSkeleton::ComputeBoneTransforms(const AnimationPose& pose, std::vector<glm::mat4>& boneTransforms) const
{
	boneTransforms.assign(m_BoneOffsets.size(), glm::mat4(1.0f));
	std::vector<glm::mat4> world(m_Nodes.size());
	for (size_t i = 0; i < m_Nodes.size(); i++)
	{
		const Node& node = m_Nodes[i];
		const bool animated = i < pose.Nodes.size() && pose.Animated[i] != 0;
		const glm::mat4 local = animated ? pose.Nodes[i].GetMatrix() : node.BindMatrix;
		world[i] = node.Parent >= 0 ? world[node.Parent] * local : local;
		if (node.Bone >= 0)
		{
			boneTransforms[node.Bone] = m_InverseRoot * world[i] * m_BoneOffsets[node.Bone];
		}
	}
}

// ---- AnimationClip

AnimationClip::AnimationClip(const aiAnimation* animation, uint32_t index)
{
	m_Name = animation->mName.C_Str();
	if (m_Name.empty())
	{
		m_Name = "Animation " + std::to_string(index);
	}
	const double ticksPerSecond = animation->mTicksPerSecond != 0.0 ? animation->mTicksPerSecond : 25.0;
	m_Duration = (float)(animation->mDuration / ticksPerSecond);

	m_Channels.reserve(animation->mNumChannels);
	for (uint32_t c = 0; c < animation->mNumChannels; c++)
	{
		const aiNodeAnim* source = animation->mChannels[c];
		Channel channel;
		channel.Node = source->mNodeName.C_Str();
		for (uint32_t k = 0; k < source->mNumPositionKeys; k++)
		{
			const aiVectorKey& key = source->mPositionKeys[k];
			channel.PositionTimes.push_back((float)(key.mTime / ticksPerSecond));
			channel.Positions.emplace_back(key.mValue.x, key.mValue.y, key.mValue.z);
		}
		for (uint32_t k = 0; k < source->mNumRotationKeys; k++)
		{
			const aiQuatKey& key = source->mRotationKeys[k];
			channel.RotationTimes.push_back((float)(key.mTime / ticksPerSecond));
			channel.Rotations.push_back(glm::normalize(glm::quat(key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z)));
		}
		for (uint32_t k = 0; k < source->mNumScalingKeys; k++)
		{
			const aiVectorKey& key = source->mScalingKeys[k];
			channel.ScaleTimes.push_back((float)(key.mTime / ticksPerSecond));
			channel.Scales.emplace_back(key.mValue.x, key.mValue.y, key.mValue.z);
		}
		m_Channels.push_back(std::move(channel));
	}
}

// ---- AnimationBinding

AnimationBinding AnimationBinding::Bind(const AnimationSkeleton& skeleton, const AnimationClip& clip)
{
	AnimationBinding binding;
	const std::vector<AnimationSkeleton::Node>& nodes = skeleton.GetNodes();
	binding.ChannelNodes.reserve(clip.GetChannels().size());
	int rootDepth = INT32_MAX;
	for (size_t c = 0; c < clip.GetChannels().size(); c++)
	{
		const int node = skeleton.FindNode(clip.GetChannels()[c].Node);
		binding.ChannelNodes.push_back(node);
		if (node < 0)
		{
			continue;
		}
		binding.BoundChannels++;
		// The root channel: the one with translation keys nearest the skeleton's root
		if (clip.GetChannels()[c].Positions.size() > 1)
		{
			int depth = 0;
			for (int parent = nodes[node].Parent; parent >= 0; parent = nodes[parent].Parent)
			{
				depth++;
			}
			if (depth < rootDepth)
			{
				rootDepth = depth;
				binding.RootChannel = (int)c;
			}
		}
	}

	// The world's up (Y) seen from the root channel's parent, in the bind pose: In Place holds the root still across it
	if (binding.RootChannel >= 0)
	{
		glm::mat4 parentWorld(1.0f);
		std::vector<int> chain;
		for (int parent = nodes[binding.ChannelNodes[binding.RootChannel]].Parent; parent >= 0; parent = nodes[parent].Parent)
		{
			chain.push_back(parent);
		}
		for (auto it = chain.rbegin(); it != chain.rend(); ++it)
		{
			parentWorld = parentWorld * nodes[*it].BindMatrix;
		}
		const glm::vec3 up = glm::inverse(glm::mat3(parentWorld)) * glm::vec3(0.0f, 1.0f, 0.0f);
		if (glm::length(up) > 1e-6f)
		{
			binding.RootUp = glm::normalize(up);
		}
	}
	return binding;
}

// ---- Sampling

namespace AnimationSampling
{
	float GetClipTime(float time, float duration, bool loop)
	{
		if (duration <= 0.0f)
		{
			return 0.0f;
		}
		if (loop)
		{
			const float wrapped = std::fmod(time, duration);
			return wrapped < 0.0f ? wrapped + duration : wrapped;
		}
		return glm::clamp(time, 0.0f, duration);
	}

	// The key before time and how far it is towards the next (0..1)
	static void FindKey(const std::vector<float>& times, float time, size_t& index, float& factor)
	{
		if (times.size() < 2 || time <= times.front())
		{
			index = 0;
			factor = 0.0f;
			return;
		}
		if (time >= times.back())
		{
			index = times.size() - 1;
			factor = 0.0f;
			return;
		}
		index = (size_t)(std::upper_bound(times.begin(), times.end(), time) - times.begin()) - 1;
		const float span = times[index + 1] - times[index];
		factor = span > 0.0f ? (time - times[index]) / span : 0.0f;
	}

	static glm::vec3 SampleVector(const std::vector<float>& times, const std::vector<glm::vec3>& values, float time)
	{
		size_t index;
		float factor;
		FindKey(times, time, index, factor);
		return factor > 0.0f ? glm::mix(values[index], values[index + 1], factor) : values[index];
	}

	static glm::quat SampleRotation(const std::vector<float>& times, const std::vector<glm::quat>& values, float time)
	{
		size_t index;
		float factor;
		FindKey(times, time, index, factor);
		return factor > 0.0f ? glm::normalize(glm::slerp(values[index], values[index + 1], factor)) : values[index];
	}

	void SampleClip(const AnimationClip& clip, const AnimationBinding& binding, float time, bool loop, bool inPlace, AnimationPose& pose)
	{
		const float clipTime = GetClipTime(time, clip.GetDuration(), loop);
		const std::vector<AnimationClip::Channel>& channels = clip.GetChannels();
		for (size_t c = 0; c < channels.size() && c < binding.ChannelNodes.size(); c++)
		{
			const int node = binding.ChannelNodes[c];
			if (node < 0 || node >= (int)pose.Nodes.size())
			{
				continue;
			}
			const AnimationClip::Channel& channel = channels[c];
			AnimationTransform& transform = pose.Nodes[node];
			if (!channel.Positions.empty())
			{
				transform.Translation = SampleVector(channel.PositionTimes, channel.Positions, clipTime);
				if (inPlace && (int)c == binding.RootChannel)
				{
					// Only the part along the world's up is left of the move since the first key
					const glm::vec3 start = channel.Positions.front();
					transform.Translation = start + binding.RootUp * glm::dot(transform.Translation - start, binding.RootUp);
				}
			}
			if (!channel.Rotations.empty())
			{
				transform.Rotation = SampleRotation(channel.RotationTimes, channel.Rotations, clipTime);
			}
			if (!channel.Scales.empty())
			{
				transform.Scale = SampleVector(channel.ScaleTimes, channel.Scales, clipTime);
			}
			pose.Animated[node] = 1;
		}
	}

	void BlendPoses(const AnimationPose& from, const AnimationPose& to, float weight, const AnimationSkeleton& skeleton, AnimationPose& out)
	{
		const std::vector<AnimationSkeleton::Node>& nodes = skeleton.GetNodes();
		const size_t count = nodes.size();
		out.Nodes.resize(count);
		out.Animated.resize(count, 0);
		weight = glm::clamp(weight, 0.0f, 1.0f);
		for (size_t i = 0; i < count; i++)
		{
			const bool fromAnimated = i < from.Nodes.size() && from.Animated[i] != 0;
			const bool toAnimated = i < to.Nodes.size() && to.Animated[i] != 0;
			if (!fromAnimated && !toAnimated)
			{
				out.Nodes[i] = nodes[i].Bind;
				out.Animated[i] = 0;
				continue;
			}
			const AnimationTransform& a = fromAnimated ? from.Nodes[i] : nodes[i].Bind;
			const AnimationTransform& b = toAnimated ? to.Nodes[i] : nodes[i].Bind;
			AnimationTransform blended;
			blended.Translation = glm::mix(a.Translation, b.Translation, weight);
			blended.Scale = glm::mix(a.Scale, b.Scale, weight);
			blended.Rotation = glm::normalize(glm::slerp(a.Rotation, b.Rotation, weight)); // glm's slerp takes the short way
			out.Nodes[i] = blended;
			out.Animated[i] = 1;
		}
	}
}

// ---- AnimationClipLibrary

namespace AnimationClipLibrary
{
	static std::map<std::string, std::shared_ptr<const std::vector<AnimationClip>>> s_Files;

	std::shared_ptr<const std::vector<AnimationClip>> Load(const std::string& path)
	{
		std::error_code error;
		const std::string key = std::filesystem::weakly_canonical(std::filesystem::u8path(path), error).generic_u8string();
		auto it = s_Files.find(key);
		if (it != s_Files.end())
		{
			return it->second;
		}

		auto clips = std::make_shared<std::vector<AnimationClip>>();
		Assimp::Importer importer;
		// The nodes as the model's import has them (no flag of ModelH2M's changes the node tree): only the clips are kept
		const bool exists = std::filesystem::exists(std::filesystem::u8path(path), error);
		const aiScene* scene = exists ? importer.ReadFile(path, aiProcess_ValidateDataStructure) : nullptr;
		if (!scene)
		{
			Log::GetLogger()->error("Animation: '{0}' {1}", path, exists ? std::string("could not be read: ") + importer.GetErrorString() : "doesn't exist");
		}
		else if (scene->mNumAnimations == 0)
		{
			Log::GetLogger()->error("Animation: '{0}' has no animations", path);
		}
		else
		{
			for (uint32_t a = 0; a < scene->mNumAnimations; a++)
			{
				clips->emplace_back(scene->mAnimations[a], a);
			}
			Log::GetLogger()->info("Animation: {0} clip(s) from '{1}'", clips->size(), path);
		}
		s_Files[key] = clips;
		return clips;
	}

	const AnimationClip* Find(const std::vector<AnimationClip>& clips, const std::string& name)
	{
		if (name.empty())
		{
			return clips.empty() ? nullptr : &clips.front();
		}
		for (const AnimationClip& clip : clips)
		{
			if (clip.GetName() == name)
			{
				return &clip;
			}
		}
		return nullptr;
	}
}
