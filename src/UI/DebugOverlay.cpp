#include "DebugOverlay.h"

#include "UIFont.h"
#include "UILayout.h"

#include "EnvMapVulkan/EnvMapVulkanScene.h"

#include "Game/GameLoop.h"
#include "Game/GameSystems.h"

#include "H2M/Renderer/ModelH2M.h" // the scene's components hold models

#include "Scripting/ScriptHost.h"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>


DebugOverlayGame DebugOverlayGame::Gather(const GameLoop& loop, GameSystems& systems, const EnvMapVulkanScene& scene, const std::string& sceneName)
{
	DebugOverlayGame game;
	game.SceneName = sceneName;
	game.Entities = (uint32_t)scene.GetEntityCount();
	game.StepsPerSecond = loop.GetStepsPerSecond();
	game.TimeScale = loop.GetTimeScale();
	game.GameTime = loop.GetGameTime();
	game.Physics = systems.GetPhysics().GetStats();
	game.ScriptInstances = (uint32_t)systems.GetScripts().GetInstanceCount();
	game.ScriptExceptions = systems.GetScripts().GetExceptionCount();
	game.Managed = ScriptHost::GetMemory(game.ManagedBytes, game.Collections);
	game.Audio = systems.GetAudio().GetStats();
	return game;
}

namespace
{
	// The overlay's look, in pixels before the content scale
	constexpr float FontPixels = 14.0f;
	constexpr float Margin = 8.0f;      // from the view's corner
	constexpr float Padding = 8.0f;     // inside the panel
	constexpr float ColumnGap = 16.0f;  // between a row's name and its value
	constexpr float GraphHeight = 40.0f;
	constexpr float GraphMinWidth = 240.0f;
	constexpr float GraphTopMs = 50.0f; // a bar this long fills the graph

	const glm::vec4 TextColor(0.92f, 0.92f, 0.92f, 1.0f);
	const glm::vec4 DimColor(0.62f, 0.62f, 0.62f, 1.0f);
	const glm::vec4 HeaderColor(1.0f, 0.78f, 0.35f, 1.0f);
	const glm::vec4 WarningColor(1.0f, 0.4f, 0.35f, 1.0f);
	const glm::vec4 PanelColor(0.0f, 0.0f, 0.0f, 0.62f);

	struct Row
	{
		enum class Kind { Text, Graph, Space } Type = Kind::Text;
		std::string Name;
		std::string Value;
		glm::vec4 NameColor = DimColor;
		glm::vec4 ValueColor = TextColor;
	};

	std::string Format(const char* format, ...)
	{
		char buffer[256];
		va_list args;
		va_start(args, format);
		std::vsnprintf(buffer, sizeof(buffer), format, args);
		va_end(args);
		return buffer;
	}

	std::string FormatCount(uint64_t count)
	{
		if (count >= 1000000)
		{
			return Format("%.2f M", (double)count / 1000000.0);
		}
		if (count >= 10000)
		{
			return Format("%.1f k", (double)count / 1000.0);
		}
		return Format("%llu", (unsigned long long)count);
	}

	void AddRow(std::vector<Row>& rows, const std::string& name, const std::string& value, const glm::vec4& valueColor = TextColor)
	{
		Row row;
		row.Name = name;
		row.Value = value;
		row.ValueColor = valueColor;
		rows.push_back(row);
	}

	void AddHeader(std::vector<Row>& rows, const std::string& name, const std::string& value = "")
	{
		rows.push_back({ Row::Kind::Space });
		Row row;
		row.Name = name;
		row.Value = value;
		row.NameColor = HeaderColor;
		row.ValueColor = HeaderColor;
		rows.push_back(row);
	}

	// Green for 60 frames per second or more, yellow down to 30, red below
	glm::vec4 FrameTimeColor(float ms)
	{
		if (ms <= 17.5f)
		{
			return glm::vec4(0.35f, 0.85f, 0.4f, 0.9f);
		}
		if (ms <= 34.0f)
		{
			return glm::vec4(0.95f, 0.8f, 0.25f, 0.9f);
		}
		return glm::vec4(0.95f, 0.3f, 0.25f, 0.9f);
	}

	std::vector<Row> MakeRows(const DebugOverlayFrame& frame, const DebugOverlayGame* game)
	{
		std::vector<Row> rows;
		AddRow(rows, "FPS", Format("%.0f   %.2f ms", frame.Fps, frame.CpuMs));
		if (!frame.VSync.empty())
		{
			AddRow(rows, "VSync", frame.VSync);
		}
		AddRow(rows, "GPU", frame.GpuValid ? Format("%.2f ms", frame.GpuMs) : "not measured");
		rows.push_back({ Row::Kind::Graph });
		AddRow(rows, "Draw calls", FormatCount(frame.Draws) + ", " + FormatCount(frame.Triangles) + " triangles");
		AddRow(rows, "Meshes", Format("%u, %u culled", frame.Meshes, frame.Culled));

		// The passes that cost most
		if (frame.GpuValid && !frame.Passes.empty())
		{
			std::vector<DebugOverlayFrame::Pass> passes = frame.Passes;
			std::stable_sort(passes.begin(), passes.end(), [](const auto& a, const auto& b) { return a.GpuMs > b.GpuMs; });
			AddHeader(rows, "GPU passes");
			for (size_t i = 0; i < passes.size() && i < 5; i++)
			{
				AddRow(rows, passes[i].Name, Format("%.2f ms", passes[i].GpuMs));
			}
		}

		// What each camera costs, with render texture cameras (7.5): in ms and in percent of the GPU frame
		if (frame.GpuValid && !frame.Cameras.empty() && frame.GpuMs > 0.0)
		{
			AddHeader(rows, "Cameras");
			for (const DebugOverlayFrame::Camera& camera : frame.Cameras)
			{
				AddRow(rows, camera.Name, Format("%.2f ms  %3.0f %%", camera.GpuMs, 100.0 * camera.GpuMs / frame.GpuMs));
			}
		}

		if (game)
		{
			AddHeader(rows, "Game", game->SceneName);
			AddRow(rows, "Entities", Format("%u", game->Entities));
			// Both change once a second, so they can be read
			AddRow(rows, "Game time", Format("%.0f s, %.0f steps/s", std::floor(game->GameTime), game->StepsPerSecond));
			AddRow(rows, "Time scale", Format("%.2f", game->TimeScale), game->TimeScale != 1.0f ? HeaderColor : TextColor);

			const ScenePhysics::Stats& physics = game->Physics;
			const uint32_t bodies = physics.Static + physics.Kinematic + physics.Dynamic + physics.Triggers;
			AddRow(rows, "Bodies", Format("%u, %u awake, %u dynamic", bodies, physics.Awake, physics.Dynamic));
			AddRow(rows, "Physics step", Format("%.2f ms, %u contact%s", physics.AverageStepMs, physics.TouchingPairs, physics.TouchingPairs == 1 ? "" : "s"));
			AddRow(rows, "Scripts", Format("%u, %llu exception%s", game->ScriptInstances, (unsigned long long)game->ScriptExceptions, game->ScriptExceptions == 1 ? "" : "s"),
				game->ScriptExceptions > 0 ? WarningColor : TextColor);
			if (game->Managed)
			{
				AddRow(rows, "Managed heap", Format("%.1f MB, %d collection%s", (double)game->ManagedBytes / (1024.0 * 1024.0), game->Collections, game->Collections == 1 ? "" : "s"));
			}
			AddRow(rows, "Sounds", Format("%u playing, %u one-shot%s", game->Audio.Playing, game->Audio.OneShots, game->Audio.OneShots == 1 ? "" : "s"));
		}

		rows.push_back({ Row::Kind::Space });
		Row hint;
		hint.Name = "F3 hides this";
		rows.push_back(hint);
		return rows;
	}
}

void DebugOverlay::Build(UIDrawList& list, const glm::vec2& viewSize, float scale, const DebugOverlayFrame& frame, const DebugOverlayGame* game)
{
	const std::shared_ptr<UIFont> font = UIFont::Get("");
	if (!font || viewSize.x <= 0.0f || viewSize.y <= 0.0f)
	{
		return;
	}
	scale = std::max(scale, 0.5f);
	const float fontPixels = std::round(FontPixels * scale);
	const float lineHeight = std::round(font->GetLineHeight() * fontPixels);
	const float padding = std::round(Padding * scale);
	const float graphHeight = std::round(GraphHeight * scale);
	const std::vector<Row> rows = MakeRows(frame, game);

	// The panel's size: the widest name and the widest value side by side, at least the graph's width
	float nameWidth = 0.0f, valueWidth = 0.0f, height = 0.0f;
	for (const Row& row : rows)
	{
		if (row.Type == Row::Kind::Text)
		{
			nameWidth = std::max(nameWidth, UILayout::MeasureText(*font, row.Name) * fontPixels);
			valueWidth = std::max(valueWidth, UILayout::MeasureText(*font, row.Value) * fontPixels);
		}
		height += row.Type == Row::Kind::Text ? lineHeight : row.Type == Row::Kind::Graph ? graphHeight + padding : std::round(lineHeight * 0.4f);
	}
	const float width = std::ceil(std::max(nameWidth + ColumnGap * scale + valueWidth, GraphMinWidth * scale));
	// Top-right: a HUD's score or health is usually top-left
	const float margin = std::round(Margin * scale);
	const glm::vec2 origin(std::max(viewSize.x - margin - width - padding * 2.0f, 0.0f), margin);
	const H2M::RefH2M<H2M::Texture2D_H2M> none;
	list.AddQuad(origin, origin + glm::vec2(width, height) + glm::vec2(padding * 2.0f), glm::vec2(0.0f), glm::vec2(1.0f), PanelColor, none);

	UITextComponent text;
	text.Size = fontPixels;
	text.VerticalAlign = UITextVerticalAlign::Top;
	float y = origin.y + padding;
	const float left = origin.x + padding;
	for (const Row& row : rows)
	{
		if (row.Type == Row::Kind::Space)
		{
			y += std::round(lineHeight * 0.4f);
			continue;
		}
		if (row.Type == Row::Kind::Graph)
		{
			// The last frames' times, one bar each (the newest on the right), with lines at 60 and 30 frames per second
			const glm::vec2 graphMin(left, y + padding * 0.5f);
			const glm::vec2 graphMax(left + width, graphMin.y + graphHeight);
			list.AddQuad(graphMin, graphMax, glm::vec2(0.0f), glm::vec2(1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 0.06f), none);
			if (frame.History && frame.HistorySize > 0)
			{
				const float barWidth = width / (float)frame.HistorySize;
				for (uint32_t i = 0; i < frame.HistorySize; i++)
				{
					const float ms = frame.History[(frame.HistoryOffset + i) % frame.HistorySize];
					if (ms <= 0.0f)
					{
						continue; // not measured yet
					}
					const float barHeight = std::max(std::min(ms / GraphTopMs, 1.0f) * graphHeight, 1.0f);
					const float x0 = graphMin.x + barWidth * (float)i;
					list.AddQuad(glm::vec2(x0, graphMax.y - barHeight), glm::vec2(x0 + std::max(barWidth, 1.0f), graphMax.y), glm::vec2(0.0f),
						glm::vec2(1.0f), FrameTimeColor(ms), none);
				}
			}
			for (const float ms : { 1000.0f / 60.0f, 1000.0f / 30.0f })
			{
				const float lineY = std::round(graphMax.y - ms / GraphTopMs * graphHeight);
				list.AddQuad(glm::vec2(graphMin.x, lineY), glm::vec2(graphMax.x, lineY + std::max(std::round(scale), 1.0f)), glm::vec2(0.0f),
					glm::vec2(1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 0.3f), none);
			}
			y += graphHeight + padding;
			continue;
		}
		const glm::vec2 min(left, y), max(left + width, y + lineHeight);
		text.Text = row.Name;
		text.Color = row.NameColor;
		text.Align = UITextAlign::Left;
		UILayout::AddText(list, text, min, max);
		if (!row.Value.empty())
		{
			text.Text = row.Value;
			text.Color = row.ValueColor;
			text.Align = UITextAlign::Right;
			UILayout::AddText(list, text, min, max);
		}
		y += lineHeight;
	}
}
