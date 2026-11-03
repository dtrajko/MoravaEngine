#include "AnimationClip.h"

#include "Assets/ModelCook.h"
#include "Core/Log.h"

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

AnimationSkeleton::AnimationSkeleton(const std::vector<SourceNode>& nodes, const std::unordered_map<std::string, uint32_t>& boneMapping, const std::vector<glm::mat4>& boneOffsets)
	: m_BoneOffsets(boneOffsets)
{
	if (nodes.empty())
	{
		return;
	}
	m_InverseRoot = glm::inverse(nodes[0].Matrix);

	m_Nodes.reserve(nodes.size());
	for (const SourceNode& source : nodes)
	{
		Node entry;
		entry.Name = source.Name;
		entry.Parent = source.Parent;
		entry.BindMatrix = source.Matrix;
		entry.Bind = Decompose(entry.BindMatrix);
		auto bone = boneMapping.find(entry.Name);
		entry.Bone = bone != boneMapping.end() && bone->second < boneOffsets.size() ? (int)bone->second : -1;
		m_NodeByName.emplace(entry.Name, (int)m_Nodes.size()); // the first node of a name (names are unique in a rig)
		m_Nodes.push_back(std::move(entry));
	}
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
		ModelData data;
		ModelCook::LoadInfo info;
		std::string readError;
		if (!ModelCook::Load(path, data, info, readError))
		{
			Log::GetLogger()->error("Animation: '{0}' could not be read: {1}", path, readError);
		}
		else if (data.Clips.empty())
		{
			Log::GetLogger()->error("Animation: '{0}' has no animations", path);
		}
		else
		{
			*clips = std::move(data.Clips);
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
