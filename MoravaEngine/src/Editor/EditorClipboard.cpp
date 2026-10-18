// Copy and paste of models, mesh parts and lights


#include "EnvMapVulkan/EnvMapVulkanMaterialLibrary.h"
#include "EnvMapVulkan/EnvMapVulkanProfiler.h"
#include "EnvMapVulkan/EnvMapVulkanRenderer.h"
#include "EnvMapVulkan/EnvMapVulkanSceneData.h"
#include "EnvMapVulkan/EnvMapVulkanSceneLoader.h"
#include "EnvMapVulkan/EnvMapVulkanSceneSerializer.h"

#include "Core/Application.h"
#include "Core/FileDialogs.h"
#include "Core/Input.h"
#include "Core/ResourceManager.h"
#include "Core/Util.h"
#include "H2M/Core/Math/RayH2M.h"
#include "ImGui/ImGuiWrapper.h"
#include "Scene/Scene.h"
#include "H2M/Editor/ContentBrowserPanelH2M.h"

#include "imgui.h"
#include "imgui_internal.h" // BeginDragDropTargetCustom (the Models and Meshes panel as one drop area)
#include "ImGuizmo.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/quaternion.hpp>

#include <algorithm>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <unordered_map>


#include "EditorState.h"


using namespace EnvMapVulkanSceneData;

// Copy and paste (Ctrl+C, Ctrl+V in the viewport and the Scene Hierarchy, or its context menu): Copy takes a snapshot of
// the selected models, parts and point and spot lights, with everything under them, so a paste works after the originals
// were changed or removed. Paste makes new entities from it, in place (where the originals were when copied), next to
// the originals (under the same parent, right after them; at the top level when that parent is gone), and selects them.
// A model is loaded again from its file (each model entity has its own, see ModelComponent), with the parts it had then:
// their materials, their transforms (the meshes' exact matrices). A part copied without its model becomes a model of its
// own, with only that mesh. The sun, the water and the environment are one per scene: not copied.
struct ClipboardEntity
{
	struct Part
	{
		std::string Name;
		MeshPartComponent Component;
		TransformComponent Transform;
		glm::mat4 MeshMatrix = glm::mat4(1.0f); // the mesh's matrix, as drawn
		std::vector<ClipboardEntity> Children;
	};

	EntityKind Kind = EntityKind::Other;
	std::string Name;
	TransformComponent Local;               // a child's transform, relative to its parent
	glm::mat4 World = glm::mat4(1.0f);      // a copied (top) entity's place: pasted there
	EnvMapVulkanEntityID Source = NoEntity; // a copied (top) entity: pasted after it, under its parent
	EnvMapVulkanEntityID SourceParent = NoEntity;
	std::string ModelPath;
	std::vector<Part> Parts;
	PointLightComponent PointLight;
	SpotLightComponent SpotLight;
	std::vector<ClipboardEntity> Children;  // the models and lights under it
};

bool CanBeCopied(EntityKind kind)
{
	return kind == EntityKind::Model || kind == EntityKind::Part || kind == EntityKind::PointLight || kind == EntityKind::SpotLight;
}

static std::vector<ClipboardEntity> s_Clipboard;

bool CanPaste()
{
	return !s_Clipboard.empty();
}

void ClearClipboard()
{
	s_Clipboard.clear();
}

static std::optional<ClipboardEntity> CopyEntity(EnvMapVulkanEntityID entity);

// The models and lights under an entity (not its parts: a model's parts are copied with it)
static void CopyChildren(EnvMapVulkanEntityID entity, std::vector<ClipboardEntity>& out)
{
	for (EnvMapVulkanEntityID child : s_Scene.GetChildren(entity))
	{
		if (!s_Scene.Has<MeshPartComponent>(child))
		{
			if (std::optional<ClipboardEntity> copy = CopyEntity(child))
			{
				out.push_back(std::move(*copy));
			}
		}
	}
}

static ClipboardEntity::Part CopyPart(EnvMapVulkanEntityID partEntity, const H2M::RefH2M<H2M::ModelH2M>& model)
{
	ClipboardEntity::Part part;
	part.Name = s_Scene.Get<NameComponent>(partEntity).Name;
	part.Component = s_Scene.Get<MeshPartComponent>(partEntity);
	part.Transform = s_Scene.Get<TransformComponent>(partEntity);
	if (model && part.Component.MeshIndex < model->GetMeshes().size())
	{
		part.MeshMatrix = model->GetMeshes()[part.Component.MeshIndex]->Transform;
	}
	else
	{
		part.MeshMatrix = part.Transform.GetMatrix();
	}
	CopyChildren(partEntity, part.Children);
	return part;
}

static std::optional<ClipboardEntity> CopyEntity(EnvMapVulkanEntityID entity)
{
	ClipboardEntity copy;
	copy.Kind = GetEntityKind(entity);
	copy.Name = s_Scene.Get<NameComponent>(entity).Name;
	copy.Local = s_Scene.Get<TransformComponent>(entity);
	copy.World = s_Scene.GetWorldTransform(entity);
	copy.Source = entity;
	copy.SourceParent = s_Scene.GetParent(entity);
	switch (copy.Kind)
	{
		case EntityKind::Model:
		{
			copy.ModelPath = s_Scene.Get<ModelComponent>(entity).FilePath;
			const H2M::RefH2M<H2M::ModelH2M> model = GetModel(s_Scene, entity);
			for (EnvMapVulkanEntityID child : s_Scene.GetChildren(entity))
			{
				if (s_Scene.Has<MeshPartComponent>(child))
				{
					copy.Parts.push_back(CopyPart(child, model));
				}
			}
			break;
		}
		case EntityKind::Part:
		{
			// A model of its own: the part's model's file and place, with only this part (its children are in its Part)
			const EnvMapVulkanEntityID model = s_Scene.GetParent(entity);
			copy.Kind = EntityKind::Model;
			copy.ModelPath = s_Scene.Get<ModelComponent>(model).FilePath;
			copy.Local = s_Scene.Get<TransformComponent>(model);
			copy.World = s_Scene.GetWorldTransform(model);
			copy.Source = model; // pasted after the model
			copy.SourceParent = s_Scene.GetParent(model);
			copy.Parts.push_back(CopyPart(entity, GetModel(s_Scene, model)));
			return copy;
		}
		case EntityKind::PointLight:
			copy.PointLight = s_Scene.Get<PointLightComponent>(entity);
			break;
		case EntityKind::SpotLight:
			copy.SpotLight = s_Scene.Get<SpotLightComponent>(entity);
			break;
		default:
			return std::nullopt;
	}
	CopyChildren(entity, copy.Children);
	return copy;
}

// The selected entities that can be copied, each once: one under another selected one is copied with it
void CopySelection()
{
	const std::vector<EnvMapVulkanEntityID> selection = s_EditorSelection.GetAll();
	std::vector<ClipboardEntity> clipboard;
	for (EnvMapVulkanEntityID entity : selection)
	{
		if (!CanBeCopied(GetEntityKind(entity)))
		{
			continue;
		}
		bool underSelected = false;
		for (EnvMapVulkanEntityID parent = s_Scene.GetParent(entity); parent != NoEntity; parent = s_Scene.GetParent(parent))
		{
			underSelected |= Contains(selection, parent);
		}
		if (!underSelected)
		{
			if (std::optional<ClipboardEntity> copy = CopyEntity(entity))
			{
				clipboard.push_back(std::move(*copy));
			}
		}
	}
	if (clipboard.empty())
	{
		Log::GetLogger()->warn("Copy: nothing to copy (models, meshes and point and spot lights can be copied)");
		return;
	}
	s_Clipboard = std::move(clipboard);
	Log::GetLogger()->info("Copied {0} entit{1} (Ctrl+V pastes)", s_Clipboard.size(), s_Clipboard.size() == 1 ? "y" : "ies");
}

// Lights over the limits aren't pasted (see EnvMapVulkanLightsGPU), and pasted ones cast shadows only while there are
// shadow slots (as with Add)
struct PasteLightCounts
{
	uint32_t Point = 0;
	uint32_t PointShadowed = 0;
	uint32_t Spot = 0;
	uint32_t SpotShadowed = 0;
};

static EnvMapVulkanEntityID PasteEntity(const ClipboardEntity& copy, EnvMapVulkanEntityID parent, PasteLightCounts& lights);

static void PasteChildren(const std::vector<ClipboardEntity>& children, EnvMapVulkanEntityID parent, PasteLightCounts& lights)
{
	for (const ClipboardEntity& child : children)
	{
		const EnvMapVulkanEntityID pasted = PasteEntity(child, parent, lights);
		if (pasted != NoEntity)
		{
			s_Scene.Get<TransformComponent>(pasted) = child.Local;
		}
	}
}

static EnvMapVulkanEntityID PasteModel(const ClipboardEntity& copy, EnvMapVulkanEntityID parent, PasteLightCounts& lights)
{
	if (!std::filesystem::exists(copy.ModelPath) || !EnvMapVulkanSceneLoader::IsModelFile(copy.ModelPath))
	{
		Log::GetLogger()->error("Paste: model '{0}' was not found", copy.ModelPath);
		return NoEntity;
	}
	H2M::RefH2M<H2M::ModelH2M> model = H2M::RefH2M<H2M::ModelH2M>::Create(copy.ModelPath);
	if (!model || model->GetMeshes().empty())
	{
		Log::GetLogger()->error("Paste: model '{0}' could not be loaded", copy.ModelPath);
		return NoEntity;
	}
	// Only the meshes of the copied parts (as AttachModel does for a loaded scene), from the last
	const uint32_t meshCount = (uint32_t)model->GetMeshes().size();
	std::set<uint32_t> kept;
	for (const ClipboardEntity::Part& part : copy.Parts)
	{
		if (part.Component.SourceMeshIndex < meshCount)
		{
			kept.insert(part.Component.SourceMeshIndex);
		}
	}
	if (kept.empty())
	{
		Log::GetLogger()->error("Paste: model '{0}' has none of the copied meshes any more", copy.ModelPath);
		return NoEntity;
	}
	for (int m = (int)meshCount - 1; m >= 0; m--)
	{
		if (kept.find((uint32_t)m) == kept.end())
		{
			model->RemoveMesh((uint32_t)m);
		}
	}

	const EnvMapVulkanEntityID entity = s_Scene.CreateEntity(copy.Name, parent);
	ModelComponent& modelComponent = s_Scene.Add<ModelComponent>(entity);
	modelComponent.FilePath = copy.ModelPath;
	modelComponent.Model = model;
	for (const ClipboardEntity::Part& part : copy.Parts)
	{
		if (kept.find(part.Component.SourceMeshIndex) == kept.end())
		{
			continue;
		}
		const EnvMapVulkanEntityID partEntity = s_Scene.CreateEntity(part.Name, entity);
		MeshPartComponent& component = s_Scene.Add<MeshPartComponent>(partEntity);
		component = part.Component;
		component.MeshIndex = (uint32_t)std::distance(kept.begin(), kept.find(part.Component.SourceMeshIndex)); // among the kept meshes
		// The mesh as it was: its exact matrix, and the transform it was written from (see ApplyPartTransforms)
		model->GetMeshes()[component.MeshIndex]->Transform = part.MeshMatrix;
		s_Scene.Get<TransformComponent>(partEntity) = part.Transform;
		PasteChildren(part.Children, partEntity, lights);
	}
	return entity;
}

static EnvMapVulkanEntityID PasteEntity(const ClipboardEntity& copy, EnvMapVulkanEntityID parent, PasteLightCounts& lights)
{
	EnvMapVulkanEntityID entity = NoEntity;
	switch (copy.Kind)
	{
		case EntityKind::Model:
			entity = PasteModel(copy, parent, lights);
			break;
		case EntityKind::PointLight:
		{
			if (lights.Point >= EnvMapVulkanLightsGPU::MaxPointLights)
			{
				Log::GetLogger()->warn("Paste: point light '{0}' not pasted: the scene has the most point lights ({1})", copy.Name,
					EnvMapVulkanLightsGPU::MaxPointLights);
				return NoEntity;
			}
			entity = s_Scene.CreateEntity(copy.Name, parent);
			PointLightComponent& light = s_Scene.Add<PointLightComponent>(entity);
			light = copy.PointLight;
			light.CastShadows = light.CastShadows && lights.PointShadowed < MaxShadowedPointLights;
			lights.Point++;
			lights.PointShadowed += light.CastShadows ? 1 : 0;
			break;
		}
		case EntityKind::SpotLight:
		{
			if (lights.Spot >= EnvMapVulkanLightsGPU::MaxSpotLights)
			{
				Log::GetLogger()->warn("Paste: spot light '{0}' not pasted: the scene has the most spot lights ({1})", copy.Name,
					EnvMapVulkanLightsGPU::MaxSpotLights);
				return NoEntity;
			}
			entity = s_Scene.CreateEntity(copy.Name, parent);
			SpotLightComponent& light = s_Scene.Add<SpotLightComponent>(entity);
			light = copy.SpotLight;
			light.CastShadows = light.CastShadows && lights.SpotShadowed < MaxShadowedSpotLights;
			lights.Spot++;
			lights.SpotShadowed += light.CastShadows ? 1 : 0;
			break;
		}
		default:
			return NoEntity;
	}
	if (entity != NoEntity)
	{
		PasteChildren(copy.Children, entity, lights);
	}
	return entity;
}

// Called at the start of a frame (see EditorPreFrame)
void PasteClipboard()
{
	PasteLightCounts lights;
	s_Scene.Each<PointLightComponent>([&](EnvMapVulkanEntityID, PointLightComponent& light) {
		lights.Point++;
		lights.PointShadowed += light.CastShadows ? 1 : 0;
	});
	s_Scene.Each<SpotLightComponent>([&](EnvMapVulkanEntityID, SpotLightComponent& light) {
		lights.Spot++;
		lights.SpotShadowed += light.CastShadows ? 1 : 0;
	});

	std::vector<EnvMapVulkanEntityID> pasted;
	for (const ClipboardEntity& copy : s_Clipboard)
	{
		const EnvMapVulkanEntityID parent = s_Scene.Exists(copy.SourceParent) ? copy.SourceParent : NoEntity;
		const EnvMapVulkanEntityID entity = PasteEntity(copy, parent, lights);
		if (entity == NoEntity)
		{
			continue;
		}
		s_Scene.SetWorldTransform(entity, copy.World);
		// Right after the original, when it is still there under the same parent
		if (s_Scene.Exists(copy.Source) && s_Scene.GetParent(copy.Source) == parent)
		{
			const std::vector<EnvMapVulkanEntityID>& siblings = parent != NoEntity ? s_Scene.GetChildren(parent) : s_Scene.GetRoots();
			const int index = (int)std::distance(siblings.begin(), std::find(siblings.begin(), siblings.end(), copy.Source));
			s_Scene.SetParent(entity, parent, true, index + 1);
		}
		pasted.push_back(entity);
	}
	if (pasted.empty())
	{
		return;
	}
	s_EditorSelection.Set(pasted);
	ExtractLights();
	Log::GetLogger()->info("Pasted {0} entit{1}", pasted.size(), pasted.size() == 1 ? "y" : "ies");
}

void RequestPaste()
{
	if (!s_Clipboard.empty())
	{
		s_PendingPaste = true;
	}
}

// Ctrl+C and Ctrl+V while the viewport or the Scene Hierarchy has the focus (once a frame; not while typing in a field)
void HandleCopyPasteKeys()
{
	static int s_HandledFrame = -1;
	const ImGuiIO& io = ImGui::GetIO();
	if (s_HandledFrame == ImGui::GetFrameCount() || io.WantTextInput || !io.KeyCtrl || s_RenamingEntity != NoEntity)
	{
		return;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_C, false))
	{
		s_HandledFrame = ImGui::GetFrameCount();
		CopySelection();
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_V, false))
	{
		s_HandledFrame = ImGui::GetFrameCount();
		RequestPaste();
	}
}
