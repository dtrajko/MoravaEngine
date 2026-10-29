// Copy and paste of models, mesh parts, lights and UI


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
#include <tuple>
#include <type_traits>
#include <unordered_map>


#include "EditorState.h"


using namespace EnvMapVulkanSceneData;

// Copy and paste (Ctrl+C, Ctrl+V in the viewport and the Scene Hierarchy, or its context menu): Copy takes a snapshot of
// the selected models, parts and point and spot lights, with everything under them, so a paste works after the originals
// were changed or removed. Paste makes new entities from it, in place (where the originals were when copied), next to
// the originals (under the same parent, right after them; at the top level when that parent is gone), and selects them.
// UI canvases and elements are copied with their UI components and the UI under them.
// A model is loaded again from its file (each model entity has its own, see ModelComponent), with the parts it had then:
// their materials, their transforms (the meshes' exact matrices). A part copied without its model becomes a model of its
// own, with only that mesh. The sun, the water and the environment are one per scene: not copied.
// Each copied entity and part also carries how it plays: its tags, physics (collider, rigid body, character), script,
// sounds, animator (ClipboardGame). A part copied alone gets its model's too, the collider and the character fitted to
// that one mesh.

// Components of an entity, those it has, kept apart from the scene: put back onto another entity
template<typename... T>
struct ClipboardComponents
{
	std::tuple<std::optional<T>...> Values;

	void Copy(EnvMapVulkanEntityID entity)
	{
		((std::get<std::optional<T>>(Values) = s_Scene.Has<T>(entity) ? std::optional<T>(s_Scene.Get<T>(entity)) : std::nullopt), ...);
	}

	void Paste(EnvMapVulkanEntityID entity) const
	{
		auto paste = [&](const auto& value)
		{
			using C = typename std::decay_t<decltype(value)>::value_type;
			if (value)
			{
				(s_Scene.Has<C>(entity) ? s_Scene.Get<C>(entity) : s_Scene.Add<C>(entity)) = *value;
			}
		};
		(paste(std::get<std::optional<T>>(Values)), ...);
	}
};

// How an entity plays: copied with any entity, whatever its kind
using ClipboardGame = ClipboardComponents<TagComponent, ColliderComponent, RigidBodyComponent, CharacterControllerComponent,
	ScriptComponent, AudioSourceComponent, AudioListenerComponent, AnimatorComponent, InputTestComponent>;

struct ClipboardEntity
{
	struct Part
	{
		std::string Name;
		MeshPartComponent Component;
		ClipboardGame Game;
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
	std::optional<UICanvasComponent> UICanvas;
	std::optional<UIRectComponent> UIRect;
	std::optional<UITextComponent> UIText;
	std::optional<UIImageComponent> UIImage;
	std::optional<UIButtonComponent> UIButton;
	ClipboardGame Game;
	bool RefitShapes = false;              // a part copied alone: its model's collider and character fitted to it (PasteClipboard)
	std::vector<ClipboardEntity> Children;  // the models, lights and UI under it
};

bool CanBeCopied(EntityKind kind)
{
	return kind == EntityKind::Model || kind == EntityKind::Part || kind == EntityKind::PointLight || kind == EntityKind::SpotLight ||
		kind == EntityKind::UICanvas || kind == EntityKind::UIElement;
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
	part.Game.Copy(partEntity);
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
	copy.Game.Copy(entity);
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
			// The model's physics, script... with its collider (and character) fitted again to this one mesh at paste: as
			// copied they fit the whole model
			copy.Game.Copy(model);
			copy.RefitShapes = true;
			copy.Parts.push_back(CopyPart(entity, GetModel(s_Scene, model)));
			return copy;
		}
		case EntityKind::PointLight:
			copy.PointLight = s_Scene.Get<PointLightComponent>(entity);
			break;
		case EntityKind::SpotLight:
			copy.SpotLight = s_Scene.Get<SpotLightComponent>(entity);
			break;
		case EntityKind::UICanvas:
		case EntityKind::UIElement:
			if (const UICanvasComponent* canvas = s_Scene.TryGet<UICanvasComponent>(entity)) copy.UICanvas = *canvas;
			if (const UIRectComponent* rect = s_Scene.TryGet<UIRectComponent>(entity)) copy.UIRect = *rect;
			if (const UITextComponent* text = s_Scene.TryGet<UITextComponent>(entity)) copy.UIText = *text;
			if (const UIImageComponent* image = s_Scene.TryGet<UIImageComponent>(entity)) copy.UIImage = *image;
			if (const UIButtonComponent* button = s_Scene.TryGet<UIButtonComponent>(entity)) copy.UIButton = *button;
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
		Log::GetLogger()->warn("Copy: nothing to copy (models, meshes, point and spot lights and UI can be copied)");
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
		part.Game.Paste(partEntity);
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
		case EntityKind::UICanvas:
		case EntityKind::UIElement:
			entity = s_Scene.CreateEntity(copy.Name, parent);
			if (copy.UICanvas) s_Scene.Add<UICanvasComponent>(entity) = *copy.UICanvas;
			if (copy.UIRect) s_Scene.Add<UIRectComponent>(entity) = *copy.UIRect;
			if (copy.UIText) s_Scene.Add<UITextComponent>(entity) = *copy.UIText;
			if (copy.UIImage) s_Scene.Add<UIImageComponent>(entity) = *copy.UIImage;
			if (copy.UIButton) s_Scene.Add<UIButtonComponent>(entity) = *copy.UIButton;
			break;
		default:
			return NoEntity;
	}
	if (entity != NoEntity)
	{
		copy.Game.Paste(entity);
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
		// A part copied alone: its model's collider and character fitted to it (FitCharacter takes the world scale)
		if (copy.RefitShapes)
		{
			if (ColliderComponent* collider = s_Scene.TryGet<ColliderComponent>(entity))
			{
				ScenePhysics::FitCollider(s_Scene, entity, *collider);
			}
			if (CharacterControllerComponent* character = s_Scene.TryGet<CharacterControllerComponent>(entity))
			{
				ScenePhysics::FitCharacter(s_Scene, entity, *character);
			}
		}
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
