// Navigation in the editor (docs/games/Shooter_Plan.html, E1.2-E1.4): the scene's Nav Mesh entity, its Inspector section,
// the bake on a worker thread (the triangles gathered on the main thread, Recast's steps on the worker), the .mnav file
// next to the scene, and the Navigation debug view over the viewport with its two-click path and the running game's agents


#include "Core/Application.h"
#include "Core/Log.h"
#include "EnvMapVulkan/EnvMapVulkanSceneLoader.h"
#include "Navigation/NavMesh.h"
#include "Navigation/NavMeshBuilder.h"
#include "Navigation/NavMeshFile.h"
#include "Navigation/SceneNavigation.h"
#include "Physics/ScenePhysics.h"
#include "Project/Project.h"

#include "imgui.h"

#include <glm/gtc/constants.hpp>

#include <atomic>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>


#include "EditorState.h"


bool s_ShowNavMesh = true;
static float s_NavMeshDrawDistance = 120.0f; // meters from the camera: farther polygons aren't drawn (a large terrain's)

// A bake in progress: the worker writes Result, then Done
struct NavMeshBake
{
	std::thread Worker;
	NavMeshBuilder::Progress Progress;
	std::atomic<bool> Done { false };
	bool Running = false;
	NavMeshBuilder::Result Result;
	EnvMapVulkanEntityID Entity = NoEntity; // the Nav Mesh it is for: the result goes to it if it still exists
	uint64_t SourceHash = 0;
	uint32_t Sources = 0;                   // colliders and terrains
	size_t Triangles = 0;
	double GatherMilliseconds = 0.0;
	std::chrono::steady_clock::time_point Started;
};
static NavMeshBake s_Bake;

// The scene file the Nav Mesh's baked mesh is next to (its .mnav): saving the scene elsewhere writes it there too
static std::string s_NavMeshSceneFile;

// The debug view's two-click path (E1.3): two points clicked on the navmesh, the path between them and the straight walk,
// found again when a point or the navmesh changes
struct NavMeshTestPath
{
	bool Enabled = false;
	int Points = 0;                         // 0 none, 1 the start, 2 both
	glm::vec3 Start = glm::vec3(0.0f), End = glm::vec3(0.0f);
	std::weak_ptr<const NavMesh> FoundOn;   // the navmesh the result is of (expired: to be found)
	bool Found = false, Complete = false;
	std::vector<glm::vec3> Corners;
	float Length = 0.0f;
	bool StraightClear = false;
	glm::vec3 StraightHit = glm::vec3(0.0f);
	double Milliseconds = 0.0;
};
static NavMeshTestPath s_TestPath;

static std::shared_ptr<const NavMesh> GetSceneNavMesh()
{
	const EnvMapVulkanEntityID entity = s_Scene.FindFirst<NavMeshComponent>();
	return entity != NoEntity ? s_Scene.Get<NavMeshComponent>(entity).Mesh : nullptr;
}

// The debug view's triangles and edges, made again when the navmesh is another
static const NavMesh::DebugGeometry& GetDebugGeometryOf(const std::shared_ptr<const NavMesh>& mesh)
{
	static std::weak_ptr<const NavMesh> s_GeometryOf;
	static NavMesh::DebugGeometry s_Geometry;
	if (s_GeometryOf.lock() != mesh)
	{
		s_GeometryOf = mesh;
		s_Geometry = mesh ? mesh->GetDebugGeometry() : NavMesh::DebugGeometry();
	}
	return s_Geometry;
}

// The test path's result for its points on this navmesh (found once per change)
static void UpdateTestPath(const std::shared_ptr<const NavMesh>& mesh)
{
	NavMeshTestPath& t = s_TestPath;
	if (t.Points < 2 || !mesh)
	{
		t.Found = false;
		t.Corners.clear();
		t.FoundOn.reset();
		return;
	}
	if (t.FoundOn.lock() == mesh)
	{
		return;
	}
	const auto started = std::chrono::steady_clock::now();
	t.FoundOn = mesh;
	t.Found = mesh->FindPath(t.Start, t.End, t.Corners, &t.Complete);
	t.Length = 0.0f;
	for (size_t i = 1; i < t.Corners.size(); i++)
	{
		t.Length += glm::distance(t.Corners[i - 1], t.Corners[i]);
	}
	t.StraightClear = mesh->Raycast(t.Start, t.End, &t.StraightHit);
	t.Milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
}

// A click: the start, then the end; a third starts again
static void SetTestPathPoint(const glm::vec3& point)
{
	NavMeshTestPath& t = s_TestPath;
	if (t.Points != 1)
	{
		t.Start = point;
		t.Points = 1;
	}
	else
	{
		t.End = point;
		t.Points = 2;
	}
	t.FoundOn.reset();
}

// The nearest of the navmesh's (detail) triangles a ray hits (Moller-Trumbore): the debug view's surface under the cursor
static bool RaycastNavMeshTriangles(const NavMesh::DebugGeometry& geometry, const glm::vec3& origin, const glm::vec3& direction, glm::vec3& hit)
{
	float nearest = FLT_MAX;
	for (const NavMesh::DebugGeometry::Triangle& tri : geometry.Triangles)
	{
		const glm::vec3 e1 = tri.B - tri.A, e2 = tri.C - tri.A;
		const glm::vec3 p = glm::cross(direction, e2);
		const float det = glm::dot(e1, p);
		if (std::fabs(det) < 1e-9f)
		{
			continue;
		}
		const float inverse = 1.0f / det;
		const glm::vec3 s = origin - tri.A;
		const float u = glm::dot(s, p) * inverse;
		if (u < 0.0f || u > 1.0f)
		{
			continue;
		}
		const glm::vec3 q = glm::cross(s, e1);
		const float v = glm::dot(direction, q) * inverse;
		if (v < 0.0f || u + v > 1.0f)
		{
			continue;
		}
		const float distance = glm::dot(e2, q) * inverse;
		if (distance > 0.0f && distance < nearest)
		{
			nearest = distance;
		}
	}
	if (nearest == FLT_MAX)
	{
		return false;
	}
	hit = origin + direction * nearest;
	return true;
}

bool HandleNavMeshClick(const glm::vec3& rayOrigin, const glm::vec3& rayDirection)
{
	const EnvMapVulkanEntityID entity = s_Scene.FindFirst<NavMeshComponent>();
	if (!s_TestPath.Enabled || !s_ShowNavMesh || entity == NoEntity || s_EditorSelection.GetPrimary() != entity)
	{
		return false;
	}
	const std::shared_ptr<const NavMesh> mesh = GetSceneNavMesh();
	glm::vec3 hit;
	if (mesh && RaycastNavMeshTriangles(GetDebugGeometryOf(mesh), rayOrigin, glm::normalize(rayDirection), hit))
	{
		SetTestPathPoint(hit);
	}
	return true; // the click is the path's, also beside the navmesh: the Nav Mesh stays selected
}

bool SetNavMeshTestPath(const glm::vec3& start, const glm::vec3& end)
{
	s_TestPath.Enabled = true;
	s_TestPath.Start = start;
	s_TestPath.End = end;
	s_TestPath.Points = 2;
	s_TestPath.FoundOn.reset();
	const std::shared_ptr<const NavMesh> mesh = GetSceneNavMesh();
	if (!mesh)
	{
		Log::GetLogger()->warn("Navigation: the test path needs a baked navmesh");
		return false;
	}
	UpdateTestPath(mesh);
	const NavMeshTestPath& t = s_TestPath;
	if (!t.Found)
	{
		Log::GetLogger()->info("Navigation: test path from ({0:.2f}, {1:.2f}, {2:.2f}) to ({3:.2f}, {4:.2f}, {5:.2f}): none", start.x, start.y,
			start.z, end.x, end.y, end.z);
		return true;
	}
	Log::GetLogger()->info("Navigation: test path from ({0:.2f}, {1:.2f}, {2:.2f}) to ({3:.2f}, {4:.2f}, {5:.2f}): {6} corners, {7:.2f} m, {8}; "
		"straight walk {9}", start.x, start.y, start.z, end.x, end.y, end.z, t.Corners.size(), t.Length, t.Complete ? "complete" : "partial",
		t.StraightClear ? "clear" : "blocked");
	return true;
}

// What a navmesh is baked from: the static colliders and terrains, and the settings
static uint64_t GetNavMeshSourceHash(const EnvMapVulkanScene& scene, const NavMeshSettings& settings)
{
	const uint64_t geometry = ScenePhysics::GetStaticGeometryHash(scene);
	const uint64_t values = settings.GetHash();
	return geometry ^ (values + 0x9e3779b97f4a7c15ull + (geometry << 6) + (geometry >> 2));
}

static const char* GetNavMeshFileName(const std::string& sceneFile, std::string& storage)
{
	storage = std::filesystem::u8path(NavMeshFile::GetPathForScene(sceneFile)).filename().u8string();
	return storage.c_str();
}

void SetNavMeshSceneFile(const std::string& scenePath)
{
	s_NavMeshSceneFile = scenePath;
	s_TestPath.Points = 0; // another scene's points
	s_TestPath.FoundOn.reset();
}

void AddNavMesh()
{
	EnvMapVulkanEntityID entity = s_Scene.FindFirst<NavMeshComponent>();
	if (entity == NoEntity)
	{
		entity = s_Scene.CreateEntity("Nav Mesh");
		s_Scene.Add<NavMeshComponent>(entity);
		// A navmesh baked for this scene before (its Nav Mesh deleted and added again): back, the Inspector says whether it
		// is still current
		if (!s_SceneFilePath.empty() && EnvMapVulkanSceneLoader::LoadNavMesh(s_SceneFilePath, s_Scene))
		{
			s_NavMeshSceneFile = s_SceneFilePath;
		}
	}
	s_EditorSelection.SelectOnly(entity);
}

// ------------------------------------------------------------------------------------------------------------- the bake

bool StartNavMeshBake()
{
	if (s_Bake.Running)
	{
		Log::GetLogger()->warn("Navigation: a bake is running already");
		return false;
	}
	if (IsPlaying())
	{
		Log::GetLogger()->warn("Navigation: the navmesh is baked in Edit, not while the game runs");
		return false;
	}
	const EnvMapVulkanEntityID entity = s_Scene.FindFirst<NavMeshComponent>();
	if (entity == NoEntity)
	{
		Log::GetLogger()->warn("Navigation: the scene has no Nav Mesh to bake (Scene Hierarchy: Add > Nav Mesh)");
		return false;
	}
	if (s_SceneFilePath.empty())
	{
		Log::GetLogger()->warn("Navigation: save the scene first: its navmesh is saved next to it");
		ShowNotification("Save the scene first: its navmesh is saved next to it", true);
		return false;
	}
	NavMeshComponent& component = s_Scene.Get<NavMeshComponent>(entity);
	component.Settings.Clamp();
	const NavMeshSettings settings = component.Settings;

	// The triangles on the main thread (the models and the terrains' heights are the scene's), the bake on the worker
	const auto start = std::chrono::steady_clock::now();
	auto input = std::make_shared<NavMeshBuilder::Input>();
	ScenePhysics::GetStaticGeometry(s_Scene, input->Vertices, input->Indices, &s_Bake.Sources);
	s_Bake.GatherMilliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
	s_Bake.Triangles = input->Indices.size() / 3;
	s_Bake.SourceHash = GetNavMeshSourceHash(s_Scene, settings);
	s_Bake.Entity = entity;
	s_Bake.Progress.Cancel = false;
	s_Bake.Progress.TilesDone = 0;
	s_Bake.Progress.TilesTotal = 0;
	s_Bake.Done = false;
	s_Bake.Running = true;
	s_Bake.Started = start;
	if (s_Bake.Worker.joinable())
	{
		s_Bake.Worker.join();
	}
	Log::GetLogger()->info("Navigation: baking '{0}' from {1} static colliders and terrains ({2} triangles, gathered in {3:.0f} ms)",
		s_Scene.Get<NameComponent>(entity).Name, s_Bake.Sources, s_Bake.Triangles, s_Bake.GatherMilliseconds);
	s_Bake.Worker = std::thread([input, settings]()
	{
		s_Bake.Result = NavMeshBuilder::Build(*input, settings, &s_Bake.Progress);
		s_Bake.Done = true;
	});
	return true;
}

// A finished bake: its navmesh to the Nav Mesh and to the .mnav next to the scene
static void FinishNavMeshBake()
{
	if (s_Bake.Worker.joinable())
	{
		s_Bake.Worker.join();
	}
	s_Bake.Running = false;
	s_Bake.Done = false;
	NavMeshBuilder::Result result = std::move(s_Bake.Result);
	s_Bake.Result = NavMeshBuilder::Result();
	const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - s_Bake.Started).count();

	if (result.Canceled)
	{
		Log::GetLogger()->info("Navigation: the bake was canceled");
		ShowNotification("The navmesh's bake was canceled");
		return;
	}
	if (!result.Error.empty())
	{
		Log::GetLogger()->error("Navigation: the navmesh couldn't be baked: {0}", result.Error);
		ShowNotification("The navmesh couldn't be baked: " + result.Error, true);
		return;
	}
	if (!s_Scene.Exists(s_Bake.Entity) || !s_Scene.Has<NavMeshComponent>(s_Bake.Entity))
	{
		Log::GetLogger()->warn("Navigation: the Nav Mesh went while it was baked (another scene, or deleted): the bake isn't used");
		return;
	}
	result.Data.SourceHash = s_Bake.SourceHash;
	auto data = std::make_shared<NavMeshData>(std::move(result.Data));
	auto mesh = std::make_shared<NavMesh>();
	if (!mesh->Load(*data))
	{
		ShowNotification("The baked navmesh couldn't be loaded (the Console says why)", true);
		return;
	}
	NavMeshComponent& component = s_Scene.Get<NavMeshComponent>(s_Bake.Entity);
	component.Data = data;
	component.Mesh = mesh;

	const NavMesh::Islands islands = mesh->GetIslands();
	// The islands, largest first, with the heights each spans: two floors joined by stairs are one island from floor to floor
	std::string list;
	for (size_t i = 0; i < islands.List.size() && i < 8; i++)
	{
		char item[96];
		snprintf(item, sizeof(item), "%s%.0f m2 at %.2f to %.2f m", i == 0 ? "" : "; ", islands.List[i].Area, islands.List[i].MinY, islands.List[i].MaxY);
		list += item;
	}
	if (islands.List.size() > 8)
	{
		list += "; " + std::to_string(islands.List.size() - 8) + " more";
	}
	Log::GetLogger()->info("Navigation: baked in {0:.1f} s: {1} tiles, {2} polygons, {3:.0f} m2 walkable in {4} island{5}: {6}", seconds,
		data->Tiles.size(), data->Polygons, islands.TotalArea, islands.Count, islands.Count == 1 ? "" : "s", list);

	// Saved at once (not with the scene): the scene's file doesn't change with a bake
	std::string error, name;
	if (s_SceneFilePath.empty() || !NavMeshFile::Write(NavMeshFile::GetPathForScene(s_SceneFilePath), *data, error))
	{
		Log::GetLogger()->error("Navigation: the navmesh couldn't be saved: {0}", s_SceneFilePath.empty() ? std::string("the scene has no file") : error);
		ShowNotification("The navmesh is baked but couldn't be saved (the Console says why)", true);
		return;
	}
	s_NavMeshSceneFile = s_SceneFilePath;
	Log::GetLogger()->info("Navigation: saved to '{0}'", ProjectPaths::ToStored(NavMeshFile::GetPathForScene(s_SceneFilePath)));
	ShowNotification("Navmesh baked: " + std::to_string(data->Polygons) + " polygons in " + std::to_string(islands.Count) +
		(islands.Count == 1 ? " island, saved to " : " islands, saved to ") + GetNavMeshFileName(s_SceneFilePath, name));
}

void UpdateNavMeshBake()
{
	// While the game runs the edited scene is put away: a finished bake waits for Stop
	if (s_Bake.Running && s_Bake.Done && !IsPlaying())
	{
		FinishNavMeshBake();
	}
}

void ShutdownNavMeshBake()
{
	if (!s_Bake.Running)
	{
		return;
	}
	// --bake-navmesh: the run was for the bake, which finishes and is saved; else it stops
	if (!Application::Get()->GetCommandLine().BakeNavMesh || IsPlaying())
	{
		s_Bake.Progress.Cancel = true;
	}
	else
	{
		Log::GetLogger()->info("Navigation: waiting for the bake to finish (--bake-navmesh)");
	}
	s_Bake.Worker.join();
	if (s_Bake.Progress.Cancel)
	{
		s_Bake.Running = false;
		s_Bake.Done = false;
		return;
	}
	FinishNavMeshBake();
}

void SaveNavMeshWithScene(const std::string& scenePath)
{
	const EnvMapVulkanEntityID entity = s_Scene.FindFirst<NavMeshComponent>();
	if (entity == NoEntity || scenePath.empty() || scenePath == s_NavMeshSceneFile)
	{
		return;
	}
	const NavMeshComponent& component = s_Scene.Get<NavMeshComponent>(entity);
	if (!component.Data)
	{
		return;
	}
	std::string error;
	const std::string file = NavMeshFile::GetPathForScene(scenePath);
	if (!NavMeshFile::Write(file, *component.Data, error))
	{
		Log::GetLogger()->error("Navigation: the navmesh couldn't be saved with the scene: {0}", error);
		return;
	}
	s_NavMeshSceneFile = scenePath;
	Log::GetLogger()->info("Navigation: the navmesh saved with the scene to '{0}'", ProjectPaths::ToStored(file));
}

// -------------------------------------------------------------------------------------------------------- the Inspector

static void Tooltip(const char* text)
{
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("%s", text);
	}
}

// The islands of the shown mesh, found again only when it changes
static NavMesh::Islands GetIslandsOf(const std::shared_ptr<const NavMesh>& mesh)
{
	static std::weak_ptr<const NavMesh> s_Of;
	static NavMesh::Islands s_Islands;
	if (s_Of.lock() != mesh)
	{
		s_Of = mesh;
		s_Islands = mesh ? mesh->GetIslands() : NavMesh::Islands();
	}
	return s_Islands;
}

static void SettingsControls(NavMeshSettings& s)
{
	ImGui::SeparatorText("Agent");
	ImGui::DragFloat("Radius", &s.AgentRadius, 0.01f, 0.05f, 10.0f, "%.2f m");
	Tooltip("How far the walkable area keeps from walls and ledges: the character's radius (its Character Controller's)");
	ImGui::DragFloat("Height", &s.AgentHeight, 0.01f, 0.1f, 20.0f, "%.2f m");
	Tooltip("The room under a ceiling the character needs: lower places (under a table, a low pipe) aren't walkable");
	ImGui::DragFloat("Step Height", &s.AgentClimb, 0.01f, 0.0f, 5.0f, "%.2f m");
	Tooltip("The highest step it walks up: a stair's riser, a curb (the Character Controller's Step Height)");
	ImGui::DragFloat("Max Slope", &s.MaxSlopeDegrees, 0.5f, 0.0f, 89.0f, "%.1f deg");
	Tooltip("Steeper ground isn't walkable (the Character Controller's Max Slope)");

	if (ImGui::TreeNode("Bake Settings"))
	{
		ImGui::DragFloat("Cell Size", &s.CellSize, 0.005f, 0.02f, 2.0f, "%.3f m");
		Tooltip("The bake's grid across: smaller follows the walls closer and finds narrower gaps, and bakes slower\n"
			"(a third of the agent's radius or less: 0.1 to 0.15 m for people)");
		ImGui::DragFloat("Cell Height", &s.CellHeight, 0.005f, 0.02f, 2.0f, "%.3f m");
		Tooltip("The bake's grid up: smaller steps and heights are told apart better");
		ImGui::DragFloat("Tile Size", &s.TileSize, 0.5f, 4.0f, 256.0f, "%.1f m");
		Tooltip("The navmesh is baked in square tiles of this width: large outdoor levels fit, and a tile can be baked again\n"
			"alone (later). Smaller tiles have fewer polygons each");
		ImGui::DragFloat("Min Region Area", &s.RegionMinArea, 0.1f, 0.0f, 10000.0f, "%.1f m2");
		Tooltip("Walkable islands smaller than this are dropped: a table's top, a crate's lid");
		ImGui::DragFloat("Edge Max Error", &s.EdgeMaxError, 0.05f, 0.1f, 10.0f, "%.2f cells");
		Tooltip("How far a polygon's edge may leave the walkable area's outline: larger, fewer polygons");
		ImGui::DragFloat("Detail Spacing", &s.DetailSampleDistance, 0.1f, 0.0f, 32.0f, "%.1f cells");
		Tooltip("The detail mesh's height samples, in cells: the polygons follow the ground's real height (a hill, a ramp)\n"
			"within Detail Error. 0: no detail mesh");
		ImGui::DragFloat("Detail Error", &s.DetailSampleMaxError, 0.05f, 0.0f, 16.0f, "%.2f cells");
		Tooltip("How far, in cells of height, the detail mesh may be from the ground");
		ImGui::TreePop();
	}
	s.Clamp();
}

void NavMeshControls(EnvMapVulkanEntityID entity)
{
	NavMeshComponent* component = s_Scene.TryGet<NavMeshComponent>(entity);
	if (!component)
	{
		return;
	}
	ImGui::PushID("NavMesh");
	ImGui::TextWrapped("Where characters can walk: baked from the scene's static colliders and terrains (not triggers, moving "
		"bodies or characters), for one size of character.");

	ImGui::BeginDisabled(s_Bake.Running);
	SettingsControls(component->Settings);
	ImGui::EndDisabled();

	ImGui::SeparatorText("Bake");
	if (s_Bake.Running)
	{
		const int total = s_Bake.Progress.TilesTotal, done = s_Bake.Progress.TilesDone;
		char overlay[64];
		snprintf(overlay, sizeof(overlay), total > 0 ? "Tile %d of %d" : "Starting", done, total);
		ImGui::ProgressBar(total > 0 ? (float)done / (float)total : 0.0f, ImVec2(-80.0f, 0.0f), overlay);
		ImGui::SameLine();
		ImGui::BeginDisabled(s_Bake.Progress.Cancel);
		if (ImGui::Button("Cancel"))
		{
			s_Bake.Progress.Cancel = true;
		}
		ImGui::EndDisabled();
	}
	else
	{
		const bool canBake = !IsPlaying() && !s_SceneFilePath.empty();
		ImGui::BeginDisabled(!canBake);
		if (ImGui::Button("Bake"))
		{
			StartNavMeshBake();
		}
		ImGui::EndDisabled();
		Tooltip(IsPlaying() ? "Stop the game first: the navmesh is baked from the edited scene" :
			s_SceneFilePath.empty() ? "Save the scene first: its navmesh is saved next to it (<Scene>.mnav)" :
			"Bakes the navmesh from the static colliders and terrains, and saves it next to the scene (<Scene>.mnav).\n"
			"Baking is by hand: after moving walls or floors, bake again (the Inspector says when it is out of date)");
	}

	if (component->Data && component->Mesh)
	{
		const NavMeshData& data = *component->Data;
		const NavMesh::Islands islands = GetIslandsOf(component->Mesh);
		ImGui::Text("%d tiles, %d polygons", (int)data.Tiles.size(), data.Polygons);
		ImGui::Text("%.0f m2 walkable, %u island%s", islands.TotalArea, islands.Count, islands.Count == 1 ? "" : "s");
		Tooltip("Islands are parts of the navmesh that aren't joined: a character can't walk from one to another.\n"
			"In the viewport each has its color (the largest blue-green); two floors joined by stairs are one island");

		// Out of date: what it was baked from changed (looked at four times a second: the hash goes over every collider)
		static double s_CheckedAt = -1.0;
		static uint64_t s_CurrentHash = 0;
		static EnvMapVulkanEntityID s_CheckedEntity = NoEntity;
		if (ImGui::GetTime() - s_CheckedAt > 0.25 || s_CheckedEntity != entity)
		{
			s_CheckedAt = ImGui::GetTime();
			s_CheckedEntity = entity;
			s_CurrentHash = GetNavMeshSourceHash(s_Scene, component->Settings);
		}
		if (s_CurrentHash != data.SourceHash && !s_Bake.Running)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "Out of date: bake again");
			Tooltip("The static colliders, the terrains or the settings changed since it was baked (or a model was edited).\n"
				"The game uses the navmesh as it was baked");
		}
		else if (!s_Bake.Running)
		{
			ImGui::TextColored(ImVec4(0.55f, 0.9f, 0.55f, 1.0f), "Up to date");
		}
		std::string name;
		if (!s_SceneFilePath.empty())
		{
			ImGui::TextDisabled("Saved as %s", GetNavMeshFileName(s_SceneFilePath, name));
		}
	}
	else if (!s_Bake.Running)
	{
		ImGui::TextDisabled("Not baked yet");
	}

	ImGui::SeparatorText("Debug View");
	ImGui::Checkbox("Show in Viewport", &s_ShowNavMesh);
	Tooltip("The navmesh over the scene (through what is in front of it): its polygons, colored by island, their outline\n"
		"bright (a wall or a ledge beyond) and the edges between them faint. In Edit and while the game runs");
	ImGui::BeginDisabled(!s_ShowNavMesh);
	ImGui::SliderFloat("Draw Distance", &s_NavMeshDrawDistance, 10.0f, 1000.0f, "%.0f m", ImGuiSliderFlags_Logarithmic);
	Tooltip("Polygons farther from the camera aren't drawn (a large terrain's navmesh is many polygons)");

	// The two-click path: what Navigation.FindPath and Navigation.Raycast give a script, between two points
	ImGui::BeginDisabled(!component->Mesh || IsPlaying());
	ImGui::Checkbox("Test Path", &s_TestPath.Enabled);
	ImGui::EndDisabled();
	Tooltip("Click two points on the navmesh in the viewport, the start and then the end (while this is on and the Nav Mesh\n"
		"is selected, clicks don't select): the path between them (yellow; orange when it can't reach the end) and the\n"
		"straight walk (green when clear, red to where it leaves the navmesh), as Navigation.FindPath and Raycast give them");
	if (s_TestPath.Enabled && component->Mesh)
	{
		UpdateTestPath(component->Mesh);
		const NavMeshTestPath& t = s_TestPath;
		if (t.Points == 0)
		{
			ImGui::TextDisabled("Click the start on the navmesh");
		}
		else if (t.Points == 1)
		{
			ImGui::TextDisabled("Click the end");
		}
		else if (!t.Found)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1.0f), "No path: a point isn't near the navmesh");
		}
		else
		{
			ImGui::Text("%d corners, %.2f m (%.2f ms)", (int)t.Corners.size(), t.Length, t.Milliseconds);
			if (t.Complete)
			{
				ImGui::TextColored(ImVec4(0.55f, 0.9f, 0.55f, 1.0f), "Reaches the end");
			}
			else
			{
				ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.3f, 1.0f), "Partial: stops %.2f m from the end", glm::distance(t.Corners.back(), t.End));
				Tooltip("Nothing joins the end to the start (another island): the path goes to the reachable point nearest it");
			}
			if (t.StraightClear)
			{
				ImGui::Text("Straight walk: clear");
			}
			else
			{
				ImGui::Text("Straight walk: leaves it after %.2f m",
					glm::distance(glm::vec2(t.Start.x, t.Start.z), glm::vec2(t.StraightHit.x, t.StraightHit.z)));
			}
		}
		if (t.Points > 0 && ImGui::SmallButton("Clear Points"))
		{
			s_TestPath.Points = 0;
			s_TestPath.FoundOn.reset();
		}
	}
	ImGui::EndDisabled();
	ImGui::PopID();
}

// ------------------------------------------------------------------------------------------------------ the debug view

// An island's color: the largest blue-green, the others each their own
static ImU32 GetIslandColor(uint32_t island, int alpha)
{
	static const ImU32 colors[] = {
		IM_COL32(40, 190, 230, 0), IM_COL32(255, 140, 40, 0), IM_COL32(230, 80, 220, 0), IM_COL32(240, 220, 50, 0),
		IM_COL32(90, 220, 90, 0), IM_COL32(240, 70, 70, 0), IM_COL32(150, 110, 255, 0), IM_COL32(180, 255, 120, 0) };
	return colors[island % (sizeof(colors) / sizeof(colors[0]))] | ((ImU32)alpha << IM_COL32_A_SHIFT);
}

// A polygon in clip space cut at the near plane (w = MinW): what is behind the camera goes
static int ClipPolygon(const glm::vec4* in, int count, glm::vec4* out)
{
	const float MinW = 0.01f;
	int n = 0;
	for (int i = 0; i < count; i++)
	{
		const glm::vec4& a = in[i];
		const glm::vec4& b = in[(i + 1) % count];
		const bool aIn = a.w >= MinW, bIn = b.w >= MinW;
		if (aIn)
		{
			out[n++] = a;
		}
		if (aIn != bIn)
		{
			out[n++] = glm::mix(a, b, (MinW - a.w) / (b.w - a.w));
		}
	}
	return n;
}

void DrawNavMeshOverlay(ImDrawList* drawList, const glm::mat4& viewProjection, const glm::vec3& cameraPosition, const ImVec2& viewportMin,
	const ImVec2& viewportSize)
{
	const std::shared_ptr<const NavMesh> mesh = GetSceneNavMesh();
	const NavMesh::DebugGeometry& geometry = GetDebugGeometryOf(mesh);
	if (!mesh)
	{
		return;
	}

	const float maxDistance2 = s_NavMeshDrawDistance * s_NavMeshDrawDistance;
	auto toPixel = [&](const glm::vec4& clip)
	{
		return ImVec2(viewportMin.x + (clip.x / clip.w + 1.0f) * 0.5f * viewportSize.x, viewportMin.y + (1.0f - clip.y / clip.w) * 0.5f * viewportSize.y);
	};
	// Entirely beyond one side of the view: not drawn
	auto outside = [](const glm::vec4* v, int count)
	{
		for (int axis = 0; axis < 2; axis++)
		{
			bool allBelow = true, allAbove = true;
			for (int i = 0; i < count; i++)
			{
				allBelow &= v[i][axis] < -v[i].w;
				allAbove &= v[i][axis] > v[i].w;
			}
			if (allBelow || allAbove)
			{
				return true;
			}
		}
		return false;
	};

	drawList->PushClipRect(viewportMin, ImVec2(viewportMin.x + viewportSize.x, viewportMin.y + viewportSize.y), true);
	// The polygons without anti-aliased edges: neighbors would overlap at their fringes, as seams
	const ImDrawListFlags flags = drawList->Flags;
	drawList->Flags &= ~ImDrawListFlags_AntiAliasedFill;
	for (const NavMesh::DebugGeometry::Triangle& t : geometry.Triangles)
	{
		const glm::vec3 center = (t.A + t.B + t.C) / 3.0f;
		const glm::vec3 offset = center - cameraPosition;
		if (glm::dot(offset, offset) > maxDistance2)
		{
			continue;
		}
		const glm::vec4 clip[3] = { viewProjection * glm::vec4(t.A, 1.0f), viewProjection * glm::vec4(t.B, 1.0f), viewProjection * glm::vec4(t.C, 1.0f) };
		glm::vec4 cut[4];
		const int count = ClipPolygon(clip, 3, cut);
		if (count < 3 || outside(cut, count))
		{
			continue;
		}
		ImVec2 points[4];
		for (int i = 0; i < count; i++)
		{
			points[i] = toPixel(cut[i]);
		}
		drawList->AddConvexPolyFilled(points, count, GetIslandColor(t.Island, 70));
	}
	drawList->Flags = flags;
	for (const NavMesh::DebugGeometry::Edge& e : geometry.Edges)
	{
		const glm::vec3 offset = (e.A + e.B) * 0.5f - cameraPosition;
		if (glm::dot(offset, offset) > maxDistance2)
		{
			continue;
		}
		glm::vec4 a = viewProjection * glm::vec4(e.A, 1.0f), b = viewProjection * glm::vec4(e.B, 1.0f);
		const float MinW = 0.01f;
		if (a.w < MinW && b.w < MinW)
		{
			continue;
		}
		if (a.w < MinW)
		{
			a = glm::mix(a, b, (MinW - a.w) / (b.w - a.w));
		}
		else if (b.w < MinW)
		{
			b = glm::mix(b, a, (MinW - b.w) / (a.w - b.w));
		}
		const glm::vec4 ends[2] = { a, b };
		if (outside(ends, 2))
		{
			continue;
		}
		drawList->AddLine(toPixel(a), toPixel(b), e.Outline ? IM_COL32(230, 250, 255, 220) : IM_COL32(230, 250, 255, 45), e.Outline ? 1.5f : 1.0f);
	}

	// Lines and dots on the mesh, just above the polygons they lie on
	const glm::vec3 lift(0.0f, 0.05f, 0.0f);
	auto line = [&](const glm::vec3& from, const glm::vec3& to, ImU32 color, float thickness)
	{
		glm::vec4 a = viewProjection * glm::vec4(from + lift, 1.0f), b = viewProjection * glm::vec4(to + lift, 1.0f);
		const float MinW = 0.01f;
		if (a.w < MinW && b.w < MinW)
		{
			return;
		}
		if (a.w < MinW)
		{
			a = glm::mix(a, b, (MinW - a.w) / (b.w - a.w));
		}
		else if (b.w < MinW)
		{
			b = glm::mix(b, a, (MinW - b.w) / (a.w - b.w));
		}
		drawList->AddLine(toPixel(a), toPixel(b), color, thickness);
	};
	auto dot = [&](const glm::vec3& at, ImU32 color, float radius)
	{
		const glm::vec4 clip = viewProjection * glm::vec4(at + lift, 1.0f);
		if (clip.w >= 0.01f)
		{
			drawList->AddCircleFilled(toPixel(clip), radius, color);
			drawList->AddCircle(toPixel(clip), radius, IM_COL32(0, 0, 0, 200), 0, 1.5f);
		}
	};
	auto ring = [&](const glm::vec3& center, float radius, ImU32 color, float thickness)
	{
		const int Segments = 20;
		for (int i = 0; i < Segments; i++)
		{
			const float a0 = glm::two_pi<float>() * (float)i / Segments, a1 = glm::two_pi<float>() * (float)(i + 1) / Segments;
			line(center + glm::vec3(std::cos(a0), 0.0f, std::sin(a0)) * radius, center + glm::vec3(std::cos(a1), 0.0f, std::sin(a1)) * radius,
				color, thickness);
		}
	};

	// The running game's agents (E1.4): each one's path (light blue; orange when partial) and corners, its destination,
	// its width on the ground (cyan; red when stuck; gray: a character without a Nav Agent, which the agents walk around),
	// the velocity it walks with (green, half a second of it) and the one along its path (faint)
	if (const SceneNavigation* navigation = GetGameNavigation())
	{
		std::vector<SceneNavigation::DebugAgent> agents;
		navigation->GetDebugAgents(agents);
		for (const SceneNavigation::DebugAgent& agent : agents)
		{
			const SceneNavigation::AgentState& s = agent.State;
			const glm::vec3 offset = s.Position - cameraPosition;
			if (glm::dot(offset, offset) > maxDistance2)
			{
				continue;
			}
			if (agent.Passive)
			{
				ring(s.Position, s.Radius, IM_COL32(200, 200, 200, 160), 1.5f);
				line(s.Position, s.Position + s.Velocity * 0.5f, IM_COL32(200, 200, 200, 160), 1.5f);
				continue;
			}
			const ImU32 pathColor = s.PathPartial ? IM_COL32(255, 150, 40, 220) : IM_COL32(110, 190, 255, 220);
			for (size_t i = 1; i < agent.Path.size(); i++)
			{
				line(agent.Path[i - 1], agent.Path[i], pathColor, 2.0f);
			}
			for (size_t i = 1; i + 1 < agent.Path.size(); i++)
			{
				dot(agent.Path[i], pathColor, 3.0f);
			}
			if (s.HasDestination)
			{
				dot(s.Destination, pathColor, 4.0f);
				ring(s.Destination, std::max(s.Settings.StoppingDistance, 0.1f), (pathColor & ~IM_COL32_A_MASK) | IM_COL32(0, 0, 0, 60), 1.0f);
			}
			ring(s.Position, s.Radius, s.Stuck ? IM_COL32(240, 70, 60, 255) : IM_COL32(60, 230, 230, 255), 2.0f);
			line(s.Position, s.Position + s.DesiredVelocity * 0.5f, IM_COL32(90, 230, 90, 90), 1.5f);
			line(s.Position, s.Position + s.Velocity * 0.5f, IM_COL32(90, 230, 90, 255), 2.5f);
		}
	}

	// The test path (two clicks): the straight walk under it, the path and its corners, the start and the end
	if (s_TestPath.Enabled && s_TestPath.Points > 0)
	{
		UpdateTestPath(mesh);
		const NavMeshTestPath& t = s_TestPath;
		if (t.Points == 2 && t.Found)
		{
			const ImU32 red = IM_COL32(240, 70, 60, 230);
			line(t.Start, t.StraightClear ? t.End : t.StraightHit, t.StraightClear ? IM_COL32(90, 230, 90, 200) : red, 1.5f);
			if (!t.StraightClear)
			{
				const glm::vec4 clip = viewProjection * glm::vec4(t.StraightHit + lift, 1.0f);
				if (clip.w >= 0.01f)
				{
					const ImVec2 c = toPixel(clip);
					drawList->AddLine(ImVec2(c.x - 5.0f, c.y - 5.0f), ImVec2(c.x + 5.0f, c.y + 5.0f), red, 2.0f);
					drawList->AddLine(ImVec2(c.x - 5.0f, c.y + 5.0f), ImVec2(c.x + 5.0f, c.y - 5.0f), red, 2.0f);
				}
			}
			const ImU32 pathColor = t.Complete ? IM_COL32(255, 225, 60, 255) : IM_COL32(255, 150, 40, 255);
			for (size_t i = 1; i < t.Corners.size(); i++)
			{
				line(t.Corners[i - 1], t.Corners[i], pathColor, 3.0f);
			}
			for (size_t i = 1; i + 1 < t.Corners.size(); i++)
			{
				dot(t.Corners[i], pathColor, 3.5f);
			}
			if (!t.Complete)
			{
				line(t.Corners.back(), t.End, IM_COL32(255, 150, 40, 110), 1.0f); // what it couldn't reach
			}
		}
		if (t.Points == 2)
		{
			dot(t.End, IM_COL32(240, 70, 60, 255), 6.0f);
		}
		dot(t.Start, IM_COL32(90, 230, 90, 255), 6.0f);
	}
	drawList->PopClipRect();
}
