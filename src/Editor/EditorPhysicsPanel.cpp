// The Physics panel (docs/engine/Game_Engine_Runtime_Guide.html, 3.6): the running game's bodies, steps and events, the
// debug views drawn into the viewport (colliders, contacts), a query test from the view, and the project's collision layers


#include "Physics/ScenePhysics.h"
#include "Project/Project.h"

#include "H2M/Renderer/CameraH2M.h"

#include "imgui.h"

#include <glm/gtc/constants.hpp>

#include <cmath>
#include <cstdio>


#include "EditorState.h"


// The query test: a ray, a moving sphere or a sphere's overlap, along the view's center (what is in the middle of the
// viewport), each frame while it is on. Its result, for the panel; drawn into the viewport with the debug lines
enum class PhysicsQueryMode { Ray, SphereCast, OverlapSphere };
static bool s_QueryEnabled = false;
static PhysicsQueryMode s_QueryMode = PhysicsQueryMode::Ray;
static float s_QueryRadius = 0.5f;
static float s_QueryDistance = 50.0f;       // how far the ray and the cast go; the overlap's center: half of it, at most 5 m
static uint32_t s_QueryLayers = ObjectLayers::Solid;
static bool s_QueryHadHit = false;
static ScenePhysics::Hit s_QueryHit;
static std::vector<EnvMapVulkanEntityID> s_QueryOverlaps;
static glm::vec3 s_QueryOrigin = glm::vec3(0.0f), s_QueryDirection = glm::vec3(0.0f, 0.0f, -1.0f);

static const char* GetEntityName(EnvMapVulkanEntityID entity)
{
	const NameComponent* name = s_Scene.TryGet<NameComponent>(entity);
	return name ? name->Name.c_str() : "(gone)";
}

static glm::vec3 GetOverlapCenter()
{
	return s_QueryOrigin + s_QueryDirection * std::min(s_QueryDistance * 0.5f, 5.0f);
}

static ImU32 GetDebugLineColor(ScenePhysics::DebugLineKind kind)
{
	switch (kind)
	{
		case ScenePhysics::DebugLineKind::Static:    return IM_COL32(160, 160, 160, 200);
		case ScenePhysics::DebugLineKind::Active:    return IM_COL32(70, 230, 70, 230);
		case ScenePhysics::DebugLineKind::Sleeping:  return IM_COL32(80, 140, 255, 230);
		case ScenePhysics::DebugLineKind::Trigger:   return IM_COL32(255, 220, 0, 230);
		case ScenePhysics::DebugLineKind::Character: return IM_COL32(255, 140, 40, 230);
		case ScenePhysics::DebugLineKind::Contact:   return IM_COL32(255, 50, 50, 255);
		default:                                     return IM_COL32(255, 255, 255, 255);
	}
}

static const ImU32 QueryColor = IM_COL32(255, 60, 255, 255);

// A circle of lines around center, in the plane of the unit vectors u and v
static void AddCircle(std::vector<EditorWorldLine>& lines, const glm::vec3& center, const glm::vec3& u, const glm::vec3& v, float radius, ImU32 color)
{
	const int segments = 32;
	const float step = glm::two_pi<float>() / segments;
	for (int i = 0; i < segments; i++)
	{
		lines.push_back({ center + radius * (std::cos(i * step) * u + std::sin(i * step) * v),
			center + radius * (std::cos((i + 1) * step) * u + std::sin((i + 1) * step) * v), color });
	}
}

static void AddSphere(std::vector<EditorWorldLine>& lines, const glm::vec3& center, float radius, ImU32 color)
{
	const glm::vec3 x(1.0f, 0.0f, 0.0f), y(0.0f, 1.0f, 0.0f), z(0.0f, 0.0f, 1.0f);
	AddCircle(lines, center, x, y, radius, color);
	AddCircle(lines, center, x, z, radius, color);
	AddCircle(lines, center, y, z, radius, color);
}

void GetPhysicsOverlayLines(std::vector<EditorWorldLine>& lines)
{
	ScenePhysics* physics = GetGamePhysics();
	if (physics)
	{
		physics->SetCollectContacts(s_ShowPhysicsContacts);
	}

	std::vector<ScenePhysics::DebugLine> debugLines;
	if (physics)
	{
		physics->GetDebugLines(s_ShowPhysicsColliders, s_ShowPhysicsContacts, debugLines);
	}
	else if (s_ShowPhysicsColliders)
	{
		ScenePhysics::GetColliderLines(s_Scene, debugLines);
	}
	for (const ScenePhysics::DebugLine& line : debugLines)
	{
		lines.push_back({ line.A, line.B, GetDebugLineColor(line.Kind) });
	}

	// The query test, from the view's center
	s_QueryHadHit = false;
	s_QueryOverlaps.clear();
	if (!s_QueryEnabled || !physics)
	{
		return;
	}
	const glm::mat4 cameraWorld = glm::inverse((*s_Camera).GetViewMatrix());
	s_QueryOrigin = glm::vec3(cameraWorld[3]);
	s_QueryDirection = -glm::normalize(glm::vec3(cameraWorld[2]));
	switch (s_QueryMode)
	{
		case PhysicsQueryMode::Ray:
			s_QueryHadHit = physics->Raycast(s_QueryOrigin, s_QueryDirection, s_QueryDistance, s_QueryHit, s_QueryLayers);
			break;
		case PhysicsQueryMode::SphereCast:
			s_QueryHadHit = physics->SphereCast(s_QueryOrigin, s_QueryRadius, s_QueryDirection, s_QueryDistance, s_QueryHit, s_QueryLayers);
			if (s_QueryHadHit)
			{
				// The sphere where it stopped
				AddSphere(lines, s_QueryOrigin + s_QueryDirection * s_QueryHit.Distance, s_QueryRadius, QueryColor);
			}
			break;
		case PhysicsQueryMode::OverlapSphere:
			s_QueryOverlaps = physics->OverlapSphere(GetOverlapCenter(), s_QueryRadius, s_QueryLayers);
			AddSphere(lines, GetOverlapCenter(), s_QueryRadius, QueryColor);
			break;
	}
	if (s_QueryHadHit)
	{
		// A cross at the hit, and the surface's normal
		const float size = 0.08f;
		const glm::vec3 axes[3] = { glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) };
		for (const glm::vec3& axis : axes)
		{
			lines.push_back({ s_QueryHit.Point - axis * size, s_QueryHit.Point + axis * size, QueryColor });
		}
		lines.push_back({ s_QueryHit.Point, s_QueryHit.Point + s_QueryHit.Normal * 0.5f, QueryColor });
	}
}

static void ColorLegend(ScenePhysics::DebugLineKind kind, const char* label)
{
	ImGui::ColorButton(label, ImGui::ColorConvertU32ToFloat4(GetDebugLineColor(kind)), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
		ImVec2(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()));
	ImGui::SameLine(0.0f, 4.0f);
	ImGui::TextUnformatted(label);
}

static void DebugViewControls(const ScenePhysics* physics)
{
	ImGui::SeparatorText("Debug Views");
	ImGui::Checkbox("Colliders", &s_ShowPhysicsColliders);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Every collider's shape as lines in the viewport, through what is in front of it.\n"
			"Without Play the scene's Box, Sphere and Capsule colliders and the characters (mesh colliders are made at Play)");
	}
	ImGui::SameLine();
	ImGui::Checkbox("Contacts", &s_ShowPhysicsContacts);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("While the game runs: where bodies touched in the last step (a cross), with the contact's normal");
	}
	if (s_ShowPhysicsColliders)
	{
		ColorLegend(ScenePhysics::DebugLineKind::Static, "Static");
		ImGui::SameLine();
		ColorLegend(ScenePhysics::DebugLineKind::Active, physics ? "Awake" : "Moving");
		if (physics)
		{
			ImGui::SameLine();
			ColorLegend(ScenePhysics::DebugLineKind::Sleeping, "Sleeping");
		}
		ImGui::SameLine();
		ColorLegend(ScenePhysics::DebugLineKind::Trigger, "Trigger");
		ImGui::SameLine();
		ColorLegend(ScenePhysics::DebugLineKind::Character, "Character");
	}
}

static void StatsControls(const ScenePhysics& physics)
{
	const ScenePhysics::Stats stats = physics.GetStats();
	ImGui::SeparatorText("World");
	ImGui::Text("Bodies: %u dynamic, %u kinematic, %u static, %u trigger%s", stats.Dynamic, stats.Kinematic, stats.Static, stats.Triggers,
		stats.Triggers == 1 ? "" : "s");
	ImGui::Text("Characters: %u", stats.Characters);
	ImGui::Text("Awake: %u (the rest sleep until something touches them)", stats.Awake);
	ImGui::Text("Touching pairs: %u", stats.TouchingPairs);
	ImGui::Text("Steps: %s, last %.2f ms, average %.2f ms", FormatCount(stats.Steps).c_str(), stats.LastStepMs, stats.AverageStepMs);
}

static void EventControls(const ScenePhysics& physics)
{
	const std::vector<ScenePhysics::Event>& events = physics.GetRecentEvents();
	char header[64];
	snprintf(header, sizeof(header), "Events (the last %zu)###Events", events.size());
	ImGui::SeparatorText(header);
	if (events.empty())
	{
		ImGui::TextDisabled("Nothing has touched yet");
		return;
	}
	const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;
	if (ImGui::BeginTable("PhysicsEvents", 4, flags, ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * 10.0f)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Step");
		ImGui::TableSetupColumn("Event");
		ImGui::TableSetupColumn("Entity");
		ImGui::TableSetupColumn("Other", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableHeadersRow();
		// The newest first
		for (auto event = events.rbegin(); event != events.rend(); ++event)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("%llu", (unsigned long long)event->Step);
			ImGui::TableNextColumn();
			const bool trigger = event->Type == ScenePhysics::EventType::TriggerEnter || event->Type == ScenePhysics::EventType::TriggerExit;
			if (trigger)
			{
				ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(GetDebugLineColor(ScenePhysics::DebugLineKind::Trigger)), "%s",
					ScenePhysics::GetEventName(event->Type));
			}
			else
			{
				ImGui::TextUnformatted(ScenePhysics::GetEventName(event->Type));
			}
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(GetEntityName(event->Entity));
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(GetEntityName(event->Other));
		}
		ImGui::EndTable();
	}
}

static void LayerCheckboxes(const char* id, uint32_t& layers)
{
	ImGui::PushID(id);
	for (uint32_t layer = 0; layer < PhysicsCollisionMatrix::LayerCount; layer++)
	{
		if (layer > 0)
		{
			ImGui::SameLine();
		}
		bool on = (layers >> layer) & 1u;
		if (ImGui::Checkbox(PhysicsCollisionMatrix::GetLayerName(layer), &on))
		{
			layers = on ? layers | (1u << layer) : layers & ~(1u << layer);
		}
	}
	ImGui::PopID();
}

static void QueryControls(bool running)
{
	ImGui::SeparatorText("Query Test");
	ImGui::Checkbox("From the view's center", &s_QueryEnabled);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("While the game runs, each frame: what is in the middle of the viewport, as a game's script would ask (Raycast,\n"
			"SphereCast, OverlapSphere). Drawn in magenta: the hit point and its normal, the sphere");
	}
	if (!s_QueryEnabled)
	{
		return;
	}
	const char* modes[] = { "Ray", "Sphere Cast", "Overlap Sphere" };
	int mode = (int)s_QueryMode;
	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
	if (ImGui::Combo("Query", &mode, modes, IM_ARRAYSIZE(modes)))
	{
		s_QueryMode = (PhysicsQueryMode)mode;
	}
	if (s_QueryMode != PhysicsQueryMode::Ray)
	{
		ImGui::SliderFloat("Radius", &s_QueryRadius, 0.05f, 3.0f, "%.2f m");
	}
	ImGui::SliderFloat("Distance", &s_QueryDistance, 1.0f, 200.0f, "%.0f m", ImGuiSliderFlags_Logarithmic);
	ImGui::PopItemWidth();
	ImGui::TextUnformatted("Hits:");
	ImGui::SameLine();
	LayerCheckboxes("QueryLayers", s_QueryLayers);

	if (!running)
	{
		ImGui::TextDisabled("Queries ask the running game: press Play");
		return;
	}
	if (s_QueryMode == PhysicsQueryMode::OverlapSphere)
	{
		if (s_QueryOverlaps.empty())
		{
			ImGui::TextDisabled("Nothing in the sphere");
		}
		for (EnvMapVulkanEntityID entity : s_QueryOverlaps)
		{
			ImGui::BulletText("%s", GetEntityName(entity));
		}
	}
	else if (s_QueryHadHit)
	{
		ImGui::Text("Hit: %s", GetEntityName(s_QueryHit.Entity));
		ImGui::Text("Distance: %.2f m", s_QueryHit.Distance);
		ImGui::Text("Point: %.2f, %.2f, %.2f", s_QueryHit.Point.x, s_QueryHit.Point.y, s_QueryHit.Point.z);
		ImGui::Text("Normal: %.2f, %.2f, %.2f", s_QueryHit.Normal.x, s_QueryHit.Normal.y, s_QueryHit.Normal.z);
	}
	else
	{
		ImGui::TextDisabled("Nothing within %.0f m", s_QueryDistance);
	}
}

// Which layers collide: a triangle of checkboxes (a pair is one box), saved in the project
static void CollisionMatrixControls(bool running)
{
	ImGui::SeparatorText("Collision Layers");
	Project* project = Project::GetOpen();
	PhysicsCollisionMatrix engineMatrix;
	PhysicsCollisionMatrix& matrix = project ? project->Collisions : engineMatrix;
	if (!project)
	{
		ImGui::TextDisabled("No project: the engine's choice (a project keeps its own)");
	}

	bool changed = false;
	ImGui::BeginDisabled(!project);
	const ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV;
	if (ImGui::BeginTable("CollisionMatrix", PhysicsCollisionMatrix::LayerCount + 1, flags))
	{
		ImGui::TableSetupColumn("");
		// The columns in reverse, so the boxes form a triangle with the rows (as in Unity's matrix)
		for (uint32_t column = PhysicsCollisionMatrix::LayerCount; column-- > 0;)
		{
			ImGui::TableSetupColumn(PhysicsCollisionMatrix::GetLayerName(column));
		}
		ImGui::TableHeadersRow();
		for (uint32_t row = 0; row < PhysicsCollisionMatrix::LayerCount; row++)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(PhysicsCollisionMatrix::GetLayerName(row));
			for (uint32_t column = PhysicsCollisionMatrix::LayerCount; column-- > 0;)
			{
				ImGui::TableNextColumn();
				if (column < row)
				{
					continue; // the same pair is in the other half
				}
				ImGui::PushID((int)(row * PhysicsCollisionMatrix::LayerCount + column));
				bool collide = matrix.Collides(row, column);
				const bool possible = PhysicsCollisionMatrix::CanCollide(row, column);
				ImGui::BeginDisabled(!possible);
				if (ImGui::Checkbox("##collide", &collide))
				{
					matrix.Set(row, column, collide);
					changed = true;
				}
				ImGui::EndDisabled();
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
				{
					ImGui::SetTooltip(possible ? "%s with %s" : "%s with %s: never (neither moves, or sensors don't sense sensors)",
						PhysicsCollisionMatrix::GetLayerName(row), PhysicsCollisionMatrix::GetLayerName(column));
				}
				ImGui::PopID();
			}
		}
		ImGui::EndTable();
	}
	ImGui::EndDisabled();
	if (project && ImGui::Button("Engine's Choice"))
	{
		changed = changed || project->Collisions != PhysicsCollisionMatrix();
		project->Collisions = PhysicsCollisionMatrix();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Everything that can collide does, but a trigger senses no static body");
	}
	if (running)
	{
		ImGui::TextDisabled("A change takes effect at the next Play");
	}
	if (changed)
	{
		SaveProject();
	}
}

void OnImGuiRenderPhysicsPanel()
{
	if (!s_ShowPhysicsPanel)
	{
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(440.0f, 560.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Physics", &s_ShowPhysicsPanel))
	{
		ImGui::End();
		return;
	}
	const ScenePhysics* physics = GetGamePhysics();
	if (!physics)
	{
		ImGui::TextWrapped("The game isn't running: the bodies are made at Play, from the entities' colliders, rigid bodies and characters.");
	}
	DebugViewControls(physics);
	if (physics)
	{
		StatsControls(*physics);
		EventControls(*physics);
	}
	QueryControls(physics != nullptr);
	CollisionMatrixControls(physics != nullptr);
	ImGui::End();
}
