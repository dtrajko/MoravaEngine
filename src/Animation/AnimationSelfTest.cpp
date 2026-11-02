#include "AnimationSelfTest.h"

#include "AnimationClip.h"
#include "AnimatorController.h"

#include "Core/Log.h"

#include <assimp/scene.h>

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>


static bool Near(const glm::vec3& a, const glm::vec3& b)
{
	return glm::all(glm::lessThan(glm::abs(a - b), glm::vec3(1e-4f)));
}

static bool Near(const glm::quat& a, const glm::quat& b)
{
	return std::abs(std::abs(glm::dot(a, b)) - 1.0f) < 1e-4f; // q and -q are the same rotation
}

namespace AnimationSelfTest
{
	bool Run()
	{
		auto fail = [](const char* what)
		{
			Log::GetLogger()->error("Animation self-test failed: {0}", what);
			return false;
		};

		// A skeleton: Root, and Bone one meter above it (the bone the skin is bound to, offset identity)
		aiScene scene;
		scene.mRootNode = new aiNode("Root");
		aiNode* boneNode = new aiNode("Bone");
		boneNode->mTransformation = aiMatrix4x4(1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1);
		scene.mRootNode->addChildren(1, &boneNode);
		const std::unordered_map<std::string, uint32_t> boneMapping = { { "Bone", 0 } };
		const AnimationSkeleton skeleton(&scene, boneMapping, { glm::mat4(1.0f) });
		if (skeleton.GetNodes().size() != 2 || skeleton.FindNode("Bone") != 1 || skeleton.GetNodes()[1].Parent != 0 || skeleton.GetBoneCount() != 1)
		{
			return fail("the skeleton's nodes");
		}
		AnimationPose pose;
		std::vector<glm::mat4> bones;
		skeleton.GetBindPose(pose);
		skeleton.ComputeBoneTransforms(pose, bones);
		if (bones.size() != 1 || !Near(glm::vec3(bones[0][3]), glm::vec3(0.0f, 1.0f, 0.0f)))
		{
			return fail("the bind pose's bone matrix");
		}

		// A clip of one second: the bone moves 2 m along X and 2 m up, and turns 90 degrees around Y; a channel of a node the
		// skeleton doesn't have is left out
		AnimationClip clip;
		clip.SetName("Move");
		clip.SetDuration(1.0f);
		AnimationClip::Channel channel;
		channel.Node = "Bone";
		channel.PositionTimes = { 0.0f, 1.0f };
		channel.Positions = { glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(2.0f, 3.0f, 0.0f) };
		channel.RotationTimes = { 0.0f, 1.0f };
		channel.Rotations = { glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)) };
		AnimationClip::Channel missing;
		missing.Node = "Tail";
		missing.PositionTimes = { 0.0f };
		missing.Positions = { glm::vec3(5.0f) };
		clip.GetChannels() = { missing, channel };
		const AnimationBinding binding = AnimationBinding::Bind(skeleton, clip);
		if (binding.BoundChannels != 1 || binding.ChannelNodes[0] != -1 || binding.ChannelNodes[1] != 1 || binding.RootChannel != 1)
		{
			return fail("the clip's binding to the skeleton");
		}

		auto sample = [&](float time, bool loop, bool inPlace)
		{
			AnimationPose result;
			skeleton.GetBindPose(result);
			AnimationSampling::SampleClip(clip, binding, time, loop, inPlace, result);
			return result;
		};
		const AnimationPose half = sample(0.5f, false, false);
		if (!half.Animated[1] || half.Animated[0] || !Near(half.Nodes[1].Translation, glm::vec3(1.0f, 2.0f, 0.0f)) ||
			!Near(half.Nodes[1].Rotation, glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f))))
		{
			return fail("sampling between two keys");
		}
		if (!Near(sample(1.25f, true, false).Nodes[1].Translation, glm::vec3(0.5f, 1.5f, 0.0f)) ||
			!Near(sample(1.25f, false, false).Nodes[1].Translation, glm::vec3(2.0f, 3.0f, 0.0f)))
		{
			return fail("a looped clip wraps, another holds its last key");
		}
		if (!Near(sample(0.5f, false, true).Nodes[1].Translation, glm::vec3(0.0f, 2.0f, 0.0f)))
		{
			return fail("In Place keeps the root across the ground, not its height");
		}

		// Blending: the ends are the poses, half way is half way
		const AnimationPose start = sample(0.0f, false, false), end = sample(1.0f, false, false);
		AnimationPose blended;
		AnimationSampling::BlendPoses(start, end, 0.5f, skeleton, blended);
		if (!Near(blended.Nodes[1].Translation, half.Nodes[1].Translation) || !Near(blended.Nodes[1].Rotation, half.Nodes[1].Rotation))
		{
			return fail("a blend half way");
		}
		AnimationSampling::BlendPoses(start, end, 0.0f, skeleton, blended);
		if (!Near(blended.Nodes[1].Translation, start.Nodes[1].Translation) || !Near(blended.Nodes[1].Rotation, start.Nodes[1].Rotation))
		{
			return fail("a blend at weight 0 is the first pose");
		}
		skeleton.ComputeBoneTransforms(end, bones);
		const glm::mat4 expected = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 3.0f, 0.0f)) *
			glm::mat4_cast(glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
		for (int c = 0; c < 4; c++)
		{
			if (!Near(glm::vec3(bones[0][c]), glm::vec3(expected[c])))
			{
				return fail("a posed bone's matrix");
			}
		}

		// A controller's file: read, written, read again the same
		const char* text =
			"Animator:\n"
			"  Version: 1\n"
			"  Entry: Walk\n"
			"  Parameters:\n"
			"    - {Name: Speed, Type: Float, Default: 0.5}\n"
			"    - {Name: Grounded, Type: Bool, Default: true}\n"
			"    - {Name: Jump, Type: Trigger}\n"
			"  States:\n"
			"    - {Name: Idle, Clip: {File: '', Name: ''}, Speed: 1, Loop: true, InPlace: false}\n"
			"    - {Name: Walk, Clip: {File: '', Name: Walk}, Speed: 0.6, SpeedParameter: Speed, Loop: true, InPlace: true,\n"
			"       Events: [{Name: Footstep, Time: 0.6}, {Name: Footstep, Time: 0.1}]}\n"
			"  Transitions:\n"
			"    - {From: Idle, To: Walk, Fade: 0.2, Conditions: [{Parameter: Speed, Test: Greater, Value: 0.1}]}\n"
			"    - {FromAny: true, To: Idle, Fade: 0.1, ExitTime: 0.9, Conditions: [{Parameter: Jump}, {Parameter: Grounded, Test: False}]}\n";
		AnimatorController controller;
		std::string error;
		if (!AnimatorController::FromText(text, "(self-test)", controller, error))
		{
			return fail("a controller's text can't be read");
		}
		if (controller.States.size() != 2 || controller.Parameters.size() != 3 || controller.Transitions.size() != 2 ||
			controller.GetEntryState() != 1 || controller.Parameters[1].Default != 1.0f || controller.States[1].SpeedParameter != "Speed" ||
			!controller.States[1].InPlace || !controller.Transitions[1].FromAny || !controller.Transitions[1].HasExitTime ||
			controller.Transitions[1].Conditions.size() != 2 || controller.Transitions[1].Conditions[1].Test != AnimatorConditionTest::False ||
			controller.Transitions[0].HasExitTime || controller.States[1].Events.size() != 2 || controller.States[1].Events[0].Time != 0.1f)
		{
			return fail("a controller's states, parameters, events and transitions as read");
		}
		const std::string written = controller.ToText();
		AnimatorController again;
		const uint64_t revision = again.Revision;
		if (!AnimatorController::FromText(written, "(self-test)", again, error) || again.ToText() != written)
		{
			return fail("a controller written and read again isn't the same");
		}
		if (again.Revision <= revision)
		{
			return fail("a controller read again counts as a change (its revision)");
		}

		// The events a frame passes: [from, to) in each loop of a looped state, once in a held one, its end included
		std::vector<size_t> passed;
		auto events = [&](const AnimatorState& state, float from, float to)
		{
			AnimatorController::GetPassedEvents(state, from, to, passed);
			std::string names;
			for (size_t e : passed)
			{
				names += std::to_string(e);
			}
			return names;
		};
		AnimatorState walk = controller.States[1]; // events 0 at 0.1, 1 at 0.6
		if (events(walk, 0.0f, 0.1f) != "" || events(walk, 0.0f, 0.11f) != "0" || events(walk, 0.1f, 0.6f) != "0" ||
			events(walk, 0.5f, 1.2f) != "10" || events(walk, 3.9f, 4.65f) != "01" || events(walk, 0.0f, 5.0f) != "01" ||
			events(walk, 0.7f, 0.05f) != "10" || events(walk, 0.2f, 0.2f) != "")
		{
			return fail("the events a looped state's frame passes");
		}
		AnimatorState once = walk;
		once.Loop = false;
		once.Events.push_back({ "End", 1.0f });
		if (events(once, 0.0f, 0.5f) != "0" || events(once, 0.5f, 1.3f) != "12" || events(once, 1.3f, 1.6f) != "" ||
			events(once, 0.95f, 1.0f) != "2")
		{
			return fail("the events a held state's frame passes");
		}

		Log::GetLogger()->info("Animation self-test passed");
		return true;
	}
}
