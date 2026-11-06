// The Inspector's Terrain section (TerrainComponent, World/Terrain.h): the shape made from a seed (Randomize: another one),
// the look (each layer's library material, and where the layers are) and physics. The renderer makes the terrain again
// when the shape's settings change (EnvMapVulkanTerrain::Update): the shape's fields here change the terrain when their
// edit ends (a drag let go, a number typed and Enter), so it isn't made again at every step of a drag. The look's fields
// change it at once (they are only the shader's values).


#include "EnvMapVulkan/EnvMapVulkanMaterialLibrary.h"

#include "imgui.h"

#include <glm/gtc/type_ptr.hpp>

#include <string>


#include "EditorState.h"


// The shape's settings being edited, for the selected terrain (written to it when an edit ends)
static TerrainSettings s_EditedShape;
static EnvMapVulkanEntityID s_EditedEntity = NoEntity;

void AddTerrain()
{
	EnvMapVulkanEntityID entity = s_Scene.CreateEntity("Terrain");
	TerrainComponent& terrain = s_Scene.Add<TerrainComponent>(entity);
	terrain.Settings.Seed = Terrain::RandomSeed();
	// Its heights now, to put its flat area at height 0 (the renderer makes the chunks from them)
	terrain.Heights = Terrain::GenerateHeights(terrain.Settings);
	const glm::vec2 center = terrain.Settings.FlatCenter;
	s_Scene.Get<TransformComponent>(entity).Translation = glm::vec3(-center.x, -terrain.Heights->GetHeight(center.x, center.y), -center.y);
	s_EditorSelection.SelectOnly(entity);
}

static void Tooltip(const char* text)
{
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("%s", text);
	}
}

// A layer's material: one of the library's, or none (a plain color). A material dropped from the Material Library too
static void LayerMaterialControl(TerrainComponent& terrain, uint32_t layer)
{
	TerrainComponent::LayerMaterial& slot = terrain.Layers[layer];
	const auto& materials = EnvMapVulkanMaterialLibrary::GetMaterials();
	auto assign = [&](const H2M::RefH2M<EnvMapVulkanMaterial>& material)
	{
		slot.Material = material;
		slot.ID = material ? material->GetID() : 0;
		slot.File = material ? material->GetFilePath() : std::string();
	};

	ImGui::PushID((int)layer);
	const std::string shown = slot.Material ? slot.Material->GetName() : std::string("(none: a plain color)");
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
	if (ImGui::BeginCombo(TerrainComponent::GetLayerName(layer), shown.c_str()))
	{
		if (ImGui::Selectable("(none: a plain color)", !slot.Material))
		{
			assign(H2M::RefH2M<EnvMapVulkanMaterial>());
		}
		for (uint32_t m = 0; m < (uint32_t)materials.size(); m++)
		{
			ImGui::PushID((int)m);
			if (ImGui::Selectable(materials[m]->GetName().c_str(), slot.Material == materials[m]))
			{
				assign(materials[m]);
			}
			ImGui::PopID();
		}
		ImGui::EndCombo();
	}
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(s_MaterialPayload))
		{
			const uint32_t index = *(const uint32_t*)payload->Data;
			if (index < materials.size())
			{
				assign(materials[index]);
			}
		}
		ImGui::EndDragDropTarget();
	}
	Tooltip(layer == TerrainComponent::Sand ? "The material low down: beaches, river beds (below Sand Height)" :
		layer == TerrainComponent::Grass ? "The material everywhere the other two aren't. Rays and probes see the whole terrain as this one" :
		"The material of steep slopes (Rock Slope) and high up (Rock Height)");
	if (slot.Material)
	{
		ImGui::SameLine();
		if (ImGui::SmallButton("Edit"))
		{
			s_SelectedMaterial = slot.Material; // in the Material Editor
		}
		Tooltip("Shows this material in the Material Editor: its maps, its color, its roughness, its tiling (repeats per meter: 0.25 is a 4 m tile)");
	}
	ImGui::PopID();
}

void TerrainControls(EnvMapVulkanEntityID entity)
{
	TerrainComponent& terrain = s_Scene.Get<TerrainComponent>(entity);
	if (s_EditedEntity != entity || !ImGui::IsAnyItemActive())
	{
		s_EditedShape = terrain.Settings; // nothing being edited: what the terrain has (also after a change elsewhere)
		s_EditedEntity = entity;
	}
	bool commit = false; // the shape's edit ended: the terrain is made again
	auto shapeEdited = [&] { commit = commit || ImGui::IsItemDeactivatedAfterEdit(); };

	// Where it is: moved only (the gizmo leaves it; it isn't turned or scaled)
	TransformComponent& transform = s_Scene.Get<TransformComponent>(entity);
	ImGui::DragFloat3("Translation", glm::value_ptr(transform.Translation), 0.1f);
	Tooltip("Where the terrain's center is. Its flat area is at Flat Center from there; the lowest point of the terrain is at this height");
	transform.Rotation = glm::vec3(0.0f);
	transform.Scale = glm::vec3(1.0f);

	ImGui::SeparatorText("Shape");
	TerrainSettings& s = s_EditedShape;
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.45f);
	ImGui::InputScalar("Seed", ImGuiDataType_U32, &s.Seed);
	shapeEdited();
	Tooltip("The terrain's number: the same seed and settings give the same terrain on every machine.\nRandomize picks another seed");
	ImGui::SameLine();
	if (ImGui::Button("Randomize"))
	{
		s.Seed = Terrain::RandomSeed();
		commit = true;
	}
	Tooltip("Another terrain: a new seed, the same settings");

	ImGui::DragFloat("Size", &s.Size, 64.0f, TerrainSettings::MinSize, TerrainSettings::MaxSize, "%.0f m", ImGuiSliderFlags_AlwaysClamp);
	shapeEdited();
	Tooltip("The side of the square, in meters (a multiple of 64: the chunks it is drawn in). A height every meter:\n"
		"1024 m is about a million heights and two million triangles");
	ImGui::DragFloat("Height", &s.Height, 1.0f, 0.0f, 2000.0f, "%.0f m", ImGuiSliderFlags_AlwaysClamp);
	shapeEdited();
	Tooltip("From the lowest valley to the highest peak, about");
	ImGui::DragFloat("Feature Size", &s.FeatureSize, 5.0f, 8.0f, 20000.0f, "%.0f m", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_Logarithmic);
	shapeEdited();
	Tooltip("The size of the largest hills: small gives many small hills, large a few wide ones");
	int octaves = (int)s.Octaves;
	if (ImGui::SliderInt("Detail", &octaves, 1, 10))
	{
		s.Octaves = (uint32_t)octaves;
	}
	shapeEdited();
	Tooltip("Layers of ever smaller bumps on the hills (each half the size of the last)");
	ImGui::SliderFloat("Roughness", &s.Roughness, 0.0f, 1.0f, "%.2f");
	shapeEdited();
	Tooltip("How strong the smaller layers are: 0.3 smooth, 0.6 rugged");
	ImGui::SliderFloat("Ridges", &s.Ridges, 0.0f, 1.0f, "%.2f");
	shapeEdited();
	Tooltip("0: round hills; 1: sharp ridges, like mountains");
	ImGui::SliderFloat("Valleys", &s.Valleys, 0.25f, 4.0f, "%.2f");
	shapeEdited();
	Tooltip("Above 1: wide flat lowlands and steep peaks; 1: as the noise is; below 1: high plateaus");
	ImGui::DragFloat("Rim Height", &s.RimHeight, 1.0f, 0.0f, 4000.0f, "%.0f m", ImGuiSliderFlags_AlwaysClamp);
	shapeEdited();
	Tooltip("How far above Height the ground rises along the edges (0: no rim): a line of hills around the terrain, so a camera\n"
		"near the ground sees hills against the sky and never the sky map's ground below its horizon");
	ImGui::DragFloat("Rim Width", &s.RimWidth, 1.0f, 1.0f, s.Size * 0.5f, "%.0f m", ImGuiSliderFlags_AlwaysClamp);
	shapeEdited();
	Tooltip("Meters from the edge over which the ground rises");

	ImGui::SeparatorText("Flat Area");
	ImGui::DragFloat2("Flat Center", glm::value_ptr(s.FlatCenter), 1.0f, -s.Size * 0.5f, s.Size * 0.5f, "%.0f m");
	shapeEdited();
	Tooltip("Where the level ground is (a place to start), from the terrain's center");
	ImGui::DragFloat("Flat Radius", &s.FlatRadius, 1.0f, 0.0f, s.Size * 0.5f, "%.0f m", ImGuiSliderFlags_AlwaysClamp);
	shapeEdited();
	Tooltip("Meters of level ground around it (0: none). Its height is the one the hills have there");
	ImGui::DragFloat("Flat Blend", &s.FlatBlend, 1.0f, 0.0f, s.Size * 0.5f, "%.0f m", ImGuiSliderFlags_AlwaysClamp);
	shapeEdited();
	Tooltip("Meters over which the ground rises from it back into the hills");

	if (commit)
	{
		s.Clamp();
		TerrainSettings& t = terrain.Settings;
		t.Seed = s.Seed;
		t.Size = s.Size;
		t.Height = s.Height;
		t.FeatureSize = s.FeatureSize;
		t.Octaves = s.Octaves;
		t.Roughness = s.Roughness;
		t.Ridges = s.Ridges;
		t.Valleys = s.Valleys;
		t.RimHeight = s.RimHeight;
		t.RimWidth = s.RimWidth;
		t.FlatCenter = s.FlatCenter;
		t.FlatRadius = s.FlatRadius;
		t.FlatBlend = s.FlatBlend;
	}

	// The look and physics: at once
	ImGui::SeparatorText("Layers");
	for (uint32_t layer = 0; layer < TerrainComponent::LayerCount; layer++)
	{
		LayerMaterialControl(terrain, layer);
	}
	TerrainSettings& t = terrain.Settings;
	ImGui::DragFloat("Sand Height", &t.SandHeight, 0.1f, 0.0f, 2000.0f, "%.1f m", ImGuiSliderFlags_AlwaysClamp);
	Tooltip("Sand below this height (from the terrain's lowest point), grass above");
	ImGui::SliderFloat("Rock Slope", &t.RockSlope, 0.0f, 90.0f, "%.0f deg");
	Tooltip("Slopes steeper than this are rock");
	ImGui::DragFloat("Rock Height", &t.RockHeight, 0.5f, 0.0f, 2000.0f, "%.0f m", ImGuiSliderFlags_AlwaysClamp);
	Tooltip("Rock above this height (from the terrain's lowest point): bare peaks");

	ImGui::SeparatorText("Physics");
	ImGui::SliderFloat("Friction", &t.Friction, 0.0f, 2.0f, "%.2f");
	Tooltip("How well what drives or walks on it grips (wheels, feet): 0 ice, 1 dry asphalt");

	if (terrain.Heights)
	{
		const TerrainHeights& heights = *terrain.Heights;
		ImGui::Spacing();
		const size_t meshes = terrain.Model ? terrain.Model->GetMeshes().size() : 0;
		ImGui::TextDisabled("%u x %u heights, %zu chunks, %.1f to %.1f m high, the flat area at %.1f m; made in %.0f ms", heights.SampleCount,
			heights.SampleCount, meshes, heights.MinHeight, heights.MaxHeight, heights.GetHeight(terrain.Settings.FlatCenter.x, terrain.Settings.FlatCenter.y),
			heights.Milliseconds);
	}
}
