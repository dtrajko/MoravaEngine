// Navigation in the editor (docs/games/Shooter_Plan.html, E1.2-E1.5): the scene's Nav Mesh entity, its Inspector section,
// the bake on a worker thread (the triangles gathered on the main thread, Recast's steps on the worker), the .mnav file
// next to the scene, the Nav Areas and Nav Links, and the Navigation debug view over the viewport with its two-click path
// and the running game's agents


#include "Core/Application.h"
#include "Core/Log.h"
#include "EnvMapVulkan/EnvMapVulkanDebugDraw.h"
#include "EnvMapVulkan/EnvMapVulkanSceneLoader.h"
#include "Navigation/NavMesh.h"
#include "Navigation/NavMeshBuilder.h"
#include "Navigation/NavMeshFile.h"
#include "Navigation/SceneNavigation.h"
#include "Physics/ScenePhysics.h"
#include "Project/Project.h"

#include "imgui.h"

#include <glm/gtc/constants.hpp>

#include <algorithm>

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
bool s_ShowNavMeshInPlay = false; // while the game runs (Play, Simulate): off, the game seen as it is; on, its agents and paths
static float s_NavMeshDrawDistance = 120.0f; // meters from the camera: farther, the debug view draws nothing (a large terrain's navmesh)
static bool s_NavMeshShowHidden = true;      // what the scene hides, drawn faintly

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
	std::vector<uint64_t> AreaEntities, LinkEntities; // the Nav Areas and Nav Links baked in (NavMeshData's)
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
	uint32_t FoundVersion = 0;              // and its doors' and links' states (NavMesh::GetStateVersion)
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
	if (t.FoundOn.lock() == mesh && t.FoundVersion == mesh->GetStateVersion())
	{
		return;
	}
	const auto started = std::chrono::steady_clock::now();
	t.FoundOn = mesh;
	t.FoundVersion = mesh->GetStateVersion();
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
	if (mesh && RaycastNavMeshTriangles(mesh->GetDebugGeometry(), rayOrigin, glm::normalize(rayDirection), hit))
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

// The Nav Areas and Nav Links (E1.5), by their entities' IDs: each one's world transform
template<typename T>
static std::vector<std::pair<EnvMapVulkanEntityID, glm::mat4>> GetSortedEntities(const EnvMapVulkanScene& scene)
{
	std::vector<std::pair<EnvMapVulkanEntityID, glm::mat4>> entities;
	scene.Each<T>([&](EnvMapVulkanEntityID entity, const T&)
	{
		entities.push_back({ entity, scene.GetWorldTransform(entity) });
	});
	std::sort(entities.begin(), entities.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
	return entities;
}

// A Nav Area's box corners in the world (corners: bit 0 x, bit 1 y, bit 2 z)
static void GetNavAreaCorners(const glm::mat4& world, const NavAreaComponent& area, glm::vec3* corners)
{
	const glm::vec3& h = area.HalfExtents;
	for (int i = 0; i < 8; i++)
	{
		corners[i] = glm::vec3(world * glm::vec4(i & 1 ? h.x : -h.x, i & 2 ? h.y : -h.y, i & 4 ? h.z : -h.z, 1.0f));
	}
}

static uint8_t GetLinkArea(NavLinkComponent::LinkType type)
{
	switch (type)
	{
		case NavLinkComponent::LinkType::Ladder: return NavAreaIds::LadderLink;
		case NavLinkComponent::LinkType::Jump:   return NavAreaIds::JumpLink;
		default:                                 return NavAreaIds::WalkLink;
	}
}

// The Nav Areas' boxes and the Nav Links' ends for the bake, and their entities in the order of their area ids and links
static void GatherNavAreasAndLinks(const EnvMapVulkanScene& scene, NavMeshBuilder::Input& input, std::vector<uint64_t>& areaEntities,
	std::vector<uint64_t>& linkEntities)
{
	uint32_t skipped = 0;
	for (const auto& [entity, world] : GetSortedEntities<NavAreaComponent>(scene))
	{
		const NavAreaComponent& component = scene.Get<NavAreaComponent>(entity);
		NavMeshBuilder::Input::Area area;
		GetNavAreaCorners(world, component, area.Corners);
		area.Blocked = component.Type == NavAreaComponent::AreaType::Blocked;
		if (!area.Blocked)
		{
			if ((int)areaEntities.size() >= NavAreaIds::MaxAreas)
			{
				skipped++;
				continue;
			}
			area.Id = (uint8_t)(NavAreaIds::FirstArea + areaEntities.size());
			areaEntities.push_back(entity);
		}
		input.Areas.push_back(area);
	}
	if (skipped > 0)
	{
		Log::GetLogger()->warn("Navigation: {0} Door and Costly Nav Area(s) past the first {1} are left out of the navmesh", skipped, NavAreaIds::MaxAreas);
	}
	for (const auto& [entity, world] : GetSortedEntities<NavLinkComponent>(scene))
	{
		const NavLinkComponent& component = scene.Get<NavLinkComponent>(entity);
		NavMeshBuilder::Input::Link link;
		link.Start = glm::vec3(world[3]);
		link.End = glm::vec3(world * glm::vec4(component.End, 1.0f));
		link.Bidirectional = component.Bidirectional;
		link.Radius = std::max(component.Radius, 0.05f);
		link.Area = GetLinkArea(component.Type);
		link.UserId = (uint32_t)linkEntities.size();
		linkEntities.push_back(entity);
		input.Links.push_back(link);
	}
}

// What a navmesh is baked from: the static colliders and terrains, the Nav Areas' boxes and the Nav Links' ends, and the
// settings. Not the areas' Open and Cost, nor the links' Enabled: those change while the game runs
static uint64_t GetNavMeshSourceHash(const EnvMapVulkanScene& scene, const NavMeshSettings& settings)
{
	uint64_t geometry = ScenePhysics::GetStaticGeometryHash(scene);
	auto add = [&geometry](const void* data, size_t size)
	{
		const uint8_t* bytes = static_cast<const uint8_t*>(data);
		for (size_t i = 0; i < size; i++)
		{
			geometry = (geometry ^ bytes[i]) * 1099511628211ull;
		}
	};
	for (const auto& [entity, world] : GetSortedEntities<NavAreaComponent>(scene))
	{
		const NavAreaComponent& area = scene.Get<NavAreaComponent>(entity);
		add(&entity, sizeof(entity));
		add(&world, sizeof(world));
		add(&area.Type, sizeof(area.Type));
		add(&area.HalfExtents, sizeof(area.HalfExtents));
	}
	for (const auto& [entity, world] : GetSortedEntities<NavLinkComponent>(scene))
	{
		const NavLinkComponent& link = scene.Get<NavLinkComponent>(entity);
		add(&entity, sizeof(entity));
		add(&world, sizeof(world));
		add(&link.Type, sizeof(link.Type));
		add(&link.End, sizeof(link.End));
		add(&link.Bidirectional, sizeof(link.Bidirectional));
		add(&link.Radius, sizeof(link.Radius));
	}
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

void AddNavArea()
{
	const EnvMapVulkanEntityID entity = s_Scene.CreateEntity("Nav Area");
	NavAreaComponent& area = s_Scene.Add<NavAreaComponent>(entity);
	s_Scene.Get<TransformComponent>(entity).Translation = GetLightSpawnTarget() + glm::vec3(0.0f, area.HalfExtents.y - 0.25f, 0.0f);
	s_EditorSelection.SelectOnly(entity);
}

void AddNavLink()
{
	const EnvMapVulkanEntityID entity = s_Scene.CreateEntity("Nav Link");
	s_Scene.Add<NavLinkComponent>(entity);
	s_Scene.Get<TransformComponent>(entity).Translation = GetLightSpawnTarget();
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
	ScenePhysics::GetStaticGeometry(s_Scene, input->Vertices, input->Indices, &s_Bake.Sources, &input->NotWalkable, &input->Solids);
	s_Bake.AreaEntities.clear();
	s_Bake.LinkEntities.clear();
	GatherNavAreasAndLinks(s_Scene, *input, s_Bake.AreaEntities, s_Bake.LinkEntities);
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
	Log::GetLogger()->info("Navigation: baking '{0}' from {1} static colliders and terrains ({2} triangles, gathered in {3:.0f} ms), "
		"{4} Nav Area(s), {5} Nav Link(s)", s_Scene.Get<NameComponent>(entity).Name, s_Bake.Sources, s_Bake.Triangles, s_Bake.GatherMilliseconds,
		input->Areas.size(), input->Links.size());
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
	result.Data.AreaEntities = s_Bake.AreaEntities;
	result.Data.LinkEntities = s_Bake.LinkEntities;
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
	SceneNavigation::SyncStates(s_Scene);

	// The links whose ends didn't find the navmesh: they can't be walked
	for (const NavMesh::LinkInfo& link : mesh->GetLinks())
	{
		if ((!link.StartJoined || !link.EndJoined) && link.Link < data->LinkEntities.size() && s_Scene.Exists(data->LinkEntities[link.Link]))
		{
			Log::GetLogger()->warn("Navigation: the Nav Link '{0}' has its {1} away from the navmesh: it can't be walked",
				s_Scene.Get<NameComponent>(data->LinkEntities[link.Link]).Name, !link.StartJoined ? "start" : "end");
		}
	}

	const NavMesh::Islands& islands = mesh->GetIslands();
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
	// The doors', costly areas' and links' states as the Inspector has them (the running game puts its own: SceneNavigation)
	if (!IsPlaying())
	{
		SceneNavigation::SyncStates(s_Scene);
	}

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
		const NavMesh::Islands& islands = component->Mesh->GetIslands();
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
	Tooltip("The navmesh in the scene: its polygons, colored by island, their outline bright (a wall or a ledge beyond)\n"
		"and the edges between them faint; hidden by the walls and floors in front of it");
	ImGui::BeginDisabled(!s_ShowNavMesh);
	ImGui::Checkbox("While Playing", &s_ShowNavMeshInPlay);
	Tooltip("Also while the game runs (Play, Simulate), with the agents and their paths. Off: the game is seen as it is");
	ImGui::SliderFloat("Draw Distance", &s_NavMeshDrawDistance, 10.0f, 1000.0f, "%.0f m", ImGuiSliderFlags_Logarithmic);
	Tooltip("Farther from the camera the debug view draws nothing (it fades out over the last tenth)");
	ImGui::Checkbox("Show Hidden", &s_NavMeshShowHidden);
	Tooltip("What walls and floors hide is drawn faintly (the floors above and below, a path behind a wall); off: not at all");

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

// The Nav Link a baked navmesh has for an entity: whether its ends found the navmesh
static bool FindBakedLink(EnvMapVulkanEntityID entity, NavMesh::LinkInfo& info, bool& baked)
{
	baked = false;
	const EnvMapVulkanEntityID navMesh = s_Scene.FindFirst<NavMeshComponent>();
	const NavMeshComponent* component = navMesh != NoEntity ? &s_Scene.Get<NavMeshComponent>(navMesh) : nullptr;
	if (!component || !component->Data || !component->Mesh)
	{
		return false;
	}
	const std::vector<uint64_t>& links = component->Data->LinkEntities;
	const auto it = std::find(links.begin(), links.end(), entity);
	if (it == links.end())
	{
		return false;
	}
	baked = true;
	for (const NavMesh::LinkInfo& link : component->Mesh->GetLinks())
	{
		if (link.Link == (uint32_t)(it - links.begin()))
		{
			info = link;
			return true;
		}
	}
	return false; // baked, but neither end on a tile: Detour left it out
}

static void BakeAgainNote()
{
	ImGui::TextDisabled("Moving it or changing its shape needs a new bake");
}

void NavAreaControls(EnvMapVulkanEntityID entity)
{
	NavAreaComponent* area = s_Scene.TryGet<NavAreaComponent>(entity);
	if (!area)
	{
		return;
	}
	ImGui::PushID("NavArea");
	ImGui::SeparatorText("Nav Area");
	int type = (int)area->Type;
	if (ImGui::Combo("Type", &type, "Door\0Costly\0Blocked\0"))
	{
		area->Type = (NavAreaComponent::AreaType)type;
		if (area->Type == NavAreaComponent::AreaType::Costly && area->Cost <= 1.0f)
		{
			area->Cost = 4.0f;
		}
	}
	Tooltip("Door: its polygons can be closed while the game runs (Open; C#: NavArea.IsOpen), no new bake.\n"
		"Costly: walked through only when going around costs more (mud, water, a lit yard).\n"
		"Blocked: not on the navmesh at all, kept away from by the agent's radius (a pit, a minefield)");
	ImGui::DragFloat3("Half Extents", &area->HalfExtents.x, 0.01f, 0.01f, 1000.0f, "%.2f m");
	area->HalfExtents = glm::max(area->HalfExtents, glm::vec3(0.01f));
	Tooltip("The box around the entity (turned and scaled with it). The floor in it must be inside: its polygons get the area");
	if (area->Type != NavAreaComponent::AreaType::Blocked)
	{
		ImGui::DragFloat("Cost", &area->Cost, 0.05f, 0.01f, 100.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
		Tooltip("Walking a meter in it costs this many meters of ground: paths go around it when that is shorter than\n"
			"this times the way through. 1: as the ground. Changes while the game runs (C#: NavArea.Cost)");
	}
	if (area->Type == NavAreaComponent::AreaType::Door)
	{
		ImGui::Checkbox("Open", &area->Open);
		Tooltip("Closed: paths don't go through it (they go around, or stop before it), and agents walking through it find\n"
			"another way. Changes while the game runs (C#: NavArea.IsOpen)");
	}
	BakeAgainNote();
	ImGui::PopID();
}

void NavLinkControls(EnvMapVulkanEntityID entity)
{
	NavLinkComponent* link = s_Scene.TryGet<NavLinkComponent>(entity);
	if (!link)
	{
		return;
	}
	ImGui::PushID("NavLink");
	ImGui::SeparatorText("Nav Link");
	int type = (int)link->Type;
	if (ImGui::Combo("Type", &type, "Ladder\0Jump\0Walk\0"))
	{
		link->Type = (NavLinkComponent::LinkType)type;
	}
	Tooltip("How characters are carried along it: a Ladder straight up and onto the top (or over the edge and down), a Jump\n"
		"in an arc, a Walk in a line (a gap to step over). A script sees it (C#: NavAgent.IsOnLink, LinkType) for animation");
	ImGui::DragFloat3("End", &link->End.x, 0.05f, 0.0f, 0.0f, "%.2f m");
	Tooltip("The far end, from the entity (turned and scaled with it). The start is the entity's position. Both on the\n"
		"navmesh's floor, within Radius");
	ImGui::Checkbox("Both Ways", &link->Bidirectional);
	Tooltip("Off: only from the start to the end (a jump down that can't be climbed back)");
	ImGui::DragFloat("Radius", &link->Radius, 0.01f, 0.05f, 5.0f, "%.2f m", ImGuiSliderFlags_AlwaysClamp);
	Tooltip("How far from the navmesh an end may be (across; up and down a step)");
	ImGui::Checkbox("Enabled", &link->Enabled);
	Tooltip("Off: paths don't take it (a raised ladder, a lift away). Changes while the game runs (C#: NavLink.Enabled)");

	NavMesh::LinkInfo info;
	bool baked = false;
	const bool found = FindBakedLink(entity, info, baked);
	if (!baked)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "Not in the navmesh yet: bake it");
	}
	else if (!found || !info.StartJoined || !info.EndJoined)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1.0f), "Its %s isn't on the navmesh: it can't be walked",
			!found || !info.StartJoined ? (found && !info.EndJoined ? "start and end" : "start") : "end");
		Tooltip("Move the end onto walkable floor (within Radius across, a step up or down) and bake again");
	}
	else
	{
		ImGui::TextColored(ImVec4(0.55f, 0.9f, 0.55f, 1.0f), "Both ends on the navmesh");
	}
	BakeAgainNote();
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

// A palette entry of the debug drawings (alpha 0: the renderer takes the color from the frame's palette): a Nav Area's
// polygons, whose color follows its state while the game runs without the mesh's drawing made again
static ImU32 PaletteColor(uint8_t area)
{
	return (ImU32)(area & 63);
}

// The mesh's drawing, kept on the GPU while the mesh is the same (its geometry key): the polygons, lifted a little so
// the floor under them doesn't hide them, colored by island (a Nav Area's by the palette), and their edges, the outline
// bright and the inner ones faint
static void FillNavMeshDrawing(const NavMesh& mesh, EnvMapVulkanDebugDrawList& list)
{
	const NavMesh::DebugGeometry& geometry = mesh.GetDebugGeometry();
	const glm::vec3 lift(0.0f, 0.02f, 0.0f), edgeLift(0.0f, 0.03f, 0.0f);
	list.Triangles.reserve(geometry.Triangles.size());
	for (const NavMesh::DebugGeometry::Triangle& t : geometry.Triangles)
	{
		list.AddTriangle(t.A + lift, t.B + lift, t.C + lift, t.Area == NavAreaIds::Ground ? GetIslandColor(t.Island, 70) : PaletteColor(t.Area));
	}
	list.Lines.reserve(geometry.Edges.size());
	for (const NavMesh::DebugGeometry::Edge& e : geometry.Edges)
	{
		list.AddLine(e.A + edgeLift, e.B + edgeLift, e.Outline ? IM_COL32(230, 250, 255, 220) : IM_COL32(230, 250, 255, 45), e.Outline ? 1.5f : 1.0f);
	}
}

void SubmitNavMeshDrawings()
{
	EnvMapVulkanDebugDraw::FrameSettings& settings = EnvMapVulkanDebugDraw::Settings();
	settings.DrawDistance = s_NavMeshDrawDistance;
	settings.HiddenAlpha = s_NavMeshShowHidden ? 0.15f : 0.0f;
	EnvMapVulkanDebugDrawList& list = EnvMapVulkanDebugDraw::Frame();

	// The mesh: the scene's own (nothing when it has none, and its drawing goes with it)
	const std::shared_ptr<const NavMesh> mesh = GetSceneNavMesh();
	if (mesh)
	{
		EnvMapVulkanDebugDraw::SetPersistent(mesh->GetGeometryKey(), [&mesh](EnvMapVulkanDebugDrawList& out) { FillNavMeshDrawing(*mesh, out); });
	}

	// The Nav Areas' polygons (E1.5), by the palette: a Door's green while open, red closed; a Costly area's amber, deeper
	// the more it costs
	std::fill(std::begin(settings.Palette), std::end(settings.Palette), IM_COL32(200, 200, 200, 70));
	{
		const EnvMapVulkanEntityID navMesh = s_Scene.FindFirst<NavMeshComponent>();
		const NavMeshComponent* component = navMesh != NoEntity ? &s_Scene.Get<NavMeshComponent>(navMesh) : nullptr;
		if (mesh && component && component->Data)
		{
			const std::vector<uint64_t>& areas = component->Data->AreaEntities;
			for (size_t i = 0; i < areas.size() && i < (size_t)NavAreaIds::MaxAreas; i++)
			{
				const uint8_t id = (uint8_t)(NavAreaIds::FirstArea + i);
				const NavAreaComponent* area = s_Scene.Exists(areas[i]) ? s_Scene.TryGet<NavAreaComponent>(areas[i]) : nullptr;
				if (area && area->Type == NavAreaComponent::AreaType::Costly)
				{
					const int alpha = (int)glm::clamp(60.0f + mesh->GetAreaCost(id) * 12.0f, 60.0f, 170.0f);
					settings.Palette[id & 63] = IM_COL32(240, 170, 40, alpha);
				}
				else
				{
					settings.Palette[id & 63] = mesh->IsAreaWalkable(id) ? IM_COL32(90, 230, 90, 90) : IM_COL32(240, 60, 50, 130);
				}
			}
		}
	}

	// Lines and dots on the mesh, just above the polygons they lie on
	const glm::vec3 lift(0.0f, 0.05f, 0.0f);
	auto line = [&](const glm::vec3& from, const glm::vec3& to, ImU32 color, float width)
	{
		list.AddLine(from + lift, to + lift, color, width);
	};
	auto dot = [&](const glm::vec3& at, ImU32 color, float radius)
	{
		list.AddDot(at + lift, color, radius);
	};
	auto ring = [&](const glm::vec3& center, float radius, ImU32 color, float width)
	{
		const int Segments = 20;
		for (int i = 0; i < Segments; i++)
		{
			const float a0 = glm::two_pi<float>() * (float)i / Segments, a1 = glm::two_pi<float>() * (float)(i + 1) / Segments;
			line(center + glm::vec3(std::cos(a0), 0.0f, std::sin(a0)) * radius, center + glm::vec3(std::cos(a1), 0.0f, std::sin(a1)) * radius,
				color, width);
		}
	};

	// The Nav Areas' boxes and the Nav Links (E1.5), as the scene has them now (moved since the bake: where they will be):
	// a box in its type's color, a link as an arc from its start to its end (an arrow at the end when one way), green when
	// enabled and red when off, brighter when selected
	{
		auto arcPoint = [](const glm::vec3& a, const glm::vec3& b, float u)
		{
			const float hump = 0.3f + 0.15f * glm::distance(a, b);
			return glm::mix(a, b, u) + glm::vec3(0.0f, 4.0f * u * (1.0f - u) * hump, 0.0f);
		};
		static const int BoxEdges[12][2] = { { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 }, { 0, 2 }, { 1, 3 }, { 4, 6 }, { 5, 7 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };
		s_Scene.Each<NavAreaComponent>([&](EnvMapVulkanEntityID entity, const NavAreaComponent& area)
		{
			const glm::mat4 world = s_Scene.GetWorldTransform(entity);
			glm::vec3 corners[8];
			GetNavAreaCorners(world, area, corners);
			const bool selected = s_EditorSelection.IsSelected(entity);
			const int alpha = selected ? 255 : 150;
			const ImU32 color = area.Type == NavAreaComponent::AreaType::Blocked ? IM_COL32(240, 60, 50, alpha) :
				area.Type == NavAreaComponent::AreaType::Costly ? IM_COL32(240, 170, 40, alpha) :
				area.Open ? IM_COL32(90, 230, 90, alpha) : IM_COL32(240, 60, 50, alpha);
			for (const auto& edge : BoxEdges)
			{
				line(corners[edge[0]] - lift, corners[edge[1]] - lift, color, selected ? 2.0f : 1.0f);
			}
		});
		s_Scene.Each<NavLinkComponent>([&](EnvMapVulkanEntityID entity, const NavLinkComponent& link)
		{
			const glm::mat4 world = s_Scene.GetWorldTransform(entity);
			const glm::vec3 start = glm::vec3(world[3]);
			const glm::vec3 end = glm::vec3(world * glm::vec4(link.End, 1.0f));
			const bool selected = s_EditorSelection.IsSelected(entity);
			const int alpha = selected ? 255 : 190;
			const ImU32 color = link.Enabled ? IM_COL32(110, 240, 110, alpha) : IM_COL32(240, 70, 60, alpha);
			const int Segments = 16;
			for (int i = 0; i < Segments; i++)
			{
				line(arcPoint(start, end, (float)i / Segments), arcPoint(start, end, (float)(i + 1) / Segments), color, selected ? 3.0f : 2.0f);
			}
			ring(start, std::max(link.Radius, 0.05f), color, 1.5f);
			ring(end, std::max(link.Radius, 0.05f), color, 1.5f);
			if (!link.Bidirectional)
			{
				// An arrowhead: the arc's last stretch, with two short strokes back from its end
				const glm::vec3 tip = arcPoint(start, end, 1.0f), back = arcPoint(start, end, 0.9f);
				glm::vec3 along = tip - back;
				const glm::vec3 side = glm::normalize(glm::cross(along, glm::vec3(0.0f, 1.0f, 0.0f)) + glm::vec3(1e-4f, 0.0f, 0.0f)) * 0.15f;
				line(tip, back + side, color, 2.5f);
				line(tip, back - side, color, 2.5f);
			}
		});
	}

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
			ring(s.Position, s.Radius, s.Stuck ? IM_COL32(240, 70, 60, 255) : s.OnLink ? IM_COL32(110, 240, 110, 255) : IM_COL32(60, 230, 230, 255), 2.0f);
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
				// A cross where the straight walk leaves the mesh
				const float size = 0.12f;
				line(t.StraightHit + glm::vec3(-size, 0.0f, -size), t.StraightHit + glm::vec3(size, 0.0f, size), red, 2.0f);
				line(t.StraightHit + glm::vec3(-size, 0.0f, size), t.StraightHit + glm::vec3(size, 0.0f, -size), red, 2.0f);
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
}
