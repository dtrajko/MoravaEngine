#include "CpuStats.h"

#include "Core/Log.h"

#include <algorithm>
#include <cstring>


namespace CpuStats
{
	namespace
	{
		std::vector<Stage> s_Stages;
		std::vector<double> s_ThisFrame; // per stage, this frame's time so far
		uint64_t s_CollectedFrames = 0;
		constexpr double Smoothing = 0.05; // as the GPU's times (EnvMapVulkanProfiler)
	}

	void Add(const char* name, double ms, const char* parent)
	{
		const char* parentName = parent ? parent : "";
		for (size_t i = 0; i < s_Stages.size(); i++)
		{
			if (s_Stages[i].Name == name && s_Stages[i].Parent == parentName)
			{
				s_ThisFrame[i] += ms;
				return;
			}
		}
		Stage stage;
		stage.Name = name;
		stage.Parent = parentName;
		s_Stages.push_back(stage);
		s_ThisFrame.push_back(ms);
	}

	void EndFrame(bool collecting)
	{
		for (size_t i = 0; i < s_Stages.size(); i++)
		{
			Stage& stage = s_Stages[i];
			stage.FrameMs = s_ThisFrame[i];
			stage.SmoothedMs += (stage.FrameMs - stage.SmoothedMs) * Smoothing;
			if (collecting)
			{
				stage.TotalMs += stage.FrameMs;
			}
			s_ThisFrame[i] = 0.0;
		}
		s_CollectedFrames += collecting ? 1 : 0;
	}

	const std::vector<Stage>& GetStages()
	{
		return s_Stages;
	}

	uint64_t GetCollectedFrames()
	{
		return s_CollectedFrames;
	}

	void LogTotals()
	{
		if (s_CollectedFrames == 0)
		{
			return;
		}
		const double frames = (double)s_CollectedFrames;
		// The main loop's stages, each followed by its own (at any depth, indented)
		auto log = [&](auto&& self, const Stage& stage, const std::string& indent) -> void
		{
			Log::GetLogger()->info("CPU: {0}{1}: {2:.2f} ms a frame", indent, stage.Name, stage.TotalMs / frames);
			for (const Stage& child : s_Stages)
			{
				if (child.Parent == stage.Name)
				{
					self(self, child, indent + "  ");
				}
			}
		};
		for (const Stage& top : s_Stages)
		{
			if (top.Parent.empty())
			{
				log(log, top, "");
			}
		}
	}
}
