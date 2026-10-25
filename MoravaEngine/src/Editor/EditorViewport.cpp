// The Viewport panel: the scene's image, picking, drops, the light icons, the gizmo and the frame rate


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
#include <glm/gtc/matrix_transform.hpp>
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

// Screen rectangle of the scene image in the Viewport window (for the gizmo and mouse picking)
static ImVec2 s_ViewportImageMin = ImVec2(0.0f, 0.0f);
static ImVec2 s_ViewportImageSize = ImVec2(0.0f, 0.0f);

// Mouse position in normalized device coordinates of the scene image in the Viewport window (-1..1, +y up).
// The image is shown vertically flipped (see ImGui::Image in OnImGuiRender), so its top edge is NDC y = +1.
static glm::vec2 GetViewportMouseNdc()
{
	ImVec2 mouse = ImGui::GetMousePos();
	return glm::vec2((mouse.x - s_ViewportImageMin.x) / s_ViewportImageSize.x * 2.0f - 1.0f,
		1.0f - (mouse.y - s_ViewportImageMin.y) / s_ViewportImageSize.y * 2.0f);
}

// World space ray from the camera through a point of the viewport (NDC)
void GetCameraRay(float ndcX, float ndcY, glm::vec3& origin, glm::vec3& direction)
{
	H2M::CameraH2M& camera = *s_Camera;
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 inverseViewProjection = glm::inverse(camera.GetProjectionMatrix() * view);

	origin = glm::vec3(glm::inverse(view)[3]); // camera position
	glm::vec4 farPoint = inverseViewProjection * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
	direction = glm::normalize(glm::vec3(farPoint) / farPoint.w - origin);
}

// Where a model dropped on the viewport goes: the point under the cursor on the ground plane (y = 0, the editor grid), or a few
// units in front of the camera when the cursor ray doesn't meet the ground nearby (looking at the sky or along the horizon)
static glm::vec3 GetDropGroundPosition(float ndcX, float ndcY)
{
	glm::vec3 origin, direction;
	GetCameraRay(ndcX, ndcY, origin, direction);

	const float maxDistance = 100.0f;
	if (direction.y < -1e-4f)
	{
		float t = -origin.y / direction.y;
		if (t > 0.0f && t < maxDistance)
		{
			return origin + direction * t;
		}
	}
	return origin + direction * 5.0f;
}

// Finds the model and mesh under the mouse: a ray through the cursor is tested against every mesh's bounding box,
// then its triangles (as in SceneHazelEnvMap); the nearest hit wins. hitModel is NoEntity and hitMesh -1 when nothing is hit.
// ndcX, ndcY: cursor position in normalized device coordinates of the viewport (-1..1, +y up)
// hitPosition (optional): the world position of the hit, when something is hit
void RaycastMesh(float ndcX, float ndcY, EnvMapVulkanEntityID& hitModel, int& hitMesh, glm::vec3* hitPosition)
{
	glm::vec3 origin, direction;
	GetCameraRay(ndcX, ndcY, origin, direction);

	float nearestT = std::numeric_limits<float>::max();
	hitModel = NoEntity;
	hitMesh = -1;

	for (EnvMapVulkanEntityID m : GetModels(s_Scene))
	{
		H2M::RefH2M<H2M::ModelH2M> model = GetModel(s_Scene, m);
		glm::mat4 modelTransform = s_Scene.GetWorldTransform(m);
		auto& meshes = model->GetMeshes();

		for (int s = 0; s < (int)meshes.size(); s++)
		{
			// Ray in the mesh's local space; the direction is not renormalized, so t is the same distance
			// along the world ray for every mesh and the hits can be compared
			glm::mat4 toLocal = glm::inverse(modelTransform * meshes[s]->Transform);
			H2M::RayH2M ray = { glm::vec3(toLocal * glm::vec4(origin, 1.0f)), glm::mat3(toLocal) * direction };

			float t;
			if (!ray.IntersectsAABB(meshes[s]->BoundingBox, t) || t < 0.0f || t >= nearestT)
			{
				continue;
			}

			const auto triangles = model->GetTriangleCache((uint32_t)s);
			if (triangles.empty())
			{
				nearestT = t; hitModel = m; hitMesh = s; // no triangle data: the bounding box has to do
				continue;
			}
			for (const auto& triangle : triangles)
			{
				if (ray.IntersectsTriangle(triangle.V0.Position, triangle.V1.Position, triangle.V2.Position, t) && t >= 0.0f && t < nearestT)
				{
					nearestT = t; hitModel = m; hitMesh = s;
				}
			}
		}
	}

	if (hitPosition && hitModel != NoEntity)
	{
		*hitPosition = origin + direction * nearestT;
	}
}

// Where the sun's icon is drawn: in the sky, far away in the sun's direction from the camera, so it moves like the skybox
// (after "Align to Environment" it sits on the sun of the HDR map); once moved with the gizmo, at its icon position
// (editor only: a directional light has no position)
static glm::vec3 GetSunIconPosition()
{
	if (s_Lights.Sun.IconMoved)
	{
		return s_Lights.Sun.IconPosition;
	}
	glm::vec3 cameraPosition = glm::vec3(glm::inverse((*s_Camera).GetViewMatrix())[3]);
	return cameraPosition + s_Lights.Sun.GetDirection() * 1000.0f;
}

// Where the sun's gizmo, arrow and aim line are: at the icon once it was moved; before that on the same line from the
// camera as the sky icon (so on top of it on screen), 10 units away, where dragging moves it at a usable scale
static glm::vec3 GetSunGizmoPosition()
{
	if (s_Lights.Sun.IconMoved)
	{
		return s_Lights.Sun.IconPosition;
	}
	glm::vec3 cameraPosition = glm::vec3(glm::inverse((*s_Camera).GetViewMatrix())[3]);
	return cameraPosition + s_Lights.Sun.GetDirection() * 10.0f;
}

// World position -> viewport pixel (same NDC convention as GetViewportMouseNdc). False behind the camera.
static bool ProjectToViewport(const glm::mat4& viewProjection, const glm::vec3& position, ImVec2& pixel)
{
	glm::vec4 clip = viewProjection * glm::vec4(position, 1.0f);
	if (clip.w <= 1e-4f)
	{
		return false;
	}
	glm::vec2 ndc = glm::vec2(clip) / clip.w;
	pixel = ImVec2(s_ViewportImageMin.x + (ndc.x + 1.0f) * 0.5f * s_ViewportImageSize.x,
		s_ViewportImageMin.y + (1.0f - ndc.y) * 0.5f * s_ViewportImageSize.y);
	return true;
}

// A world space line; a line with an end behind the camera is skipped (the shapes are made of short segments)
static void DrawWorldLine(ImDrawList* drawList, const glm::mat4& viewProjection, const glm::vec3& a, const glm::vec3& b, ImU32 color, float thickness = 1.5f)
{
	ImVec2 pa, pb;
	if (ProjectToViewport(viewProjection, a, pa) && ProjectToViewport(viewProjection, b, pb))
	{
		drawList->AddLine(pa, pb, color, thickness);
	}
}

// A world space circle in the plane spanned by the unit vectors u and v
static void DrawWorldCircle(ImDrawList* drawList, const glm::mat4& viewProjection, const glm::vec3& center, const glm::vec3& u, const glm::vec3& v,
	float radius, ImU32 color, int segments = 48)
{
	const float step = glm::two_pi<float>() / segments;
	for (int i = 0; i < segments; i++)
	{
		glm::vec3 a = center + radius * (std::cos(i * step) * u + std::sin(i * step) * v);
		glm::vec3 b = center + radius * (std::cos((i + 1) * step) * u + std::sin((i + 1) * step) * v);
		DrawWorldLine(drawList, viewProjection, a, b, color);
	}
}

// Two unit vectors perpendicular to the unit vector w and to each other
static void GetPerpendicularAxes(const glm::vec3& w, glm::vec3& u, glm::vec3& v)
{
	glm::vec3 up = std::abs(w.y) > 0.99f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
	u = glm::normalize(glm::cross(up, w));
	v = glm::cross(w, u);
}

// The light's color at full brightness (a dark red light still gets a clearly red icon); gray when disabled
static ImU32 GetLightGizmoColor(const glm::vec3& color, bool enabled, float alpha = 1.0f)
{
	if (!enabled)
	{
		return ImGui::ColorConvertFloat4ToU32(ImVec4(0.5f, 0.5f, 0.5f, alpha));
	}
	float maxChannel = std::max(color.r, std::max(color.g, color.b));
	glm::vec3 c = maxChannel > 1e-3f ? color / maxChannel : glm::vec3(1.0f);
	return ImGui::ColorConvertFloat4ToU32(ImVec4(c.r, c.g, c.b, alpha));
}

// An icon: a disc in the light's color (a ring when disabled) on a dark backdrop, with rays; a white ring when selected.
// The rays and the ring have dark outlines, so a white icon stays visible on a bright sky.
// castsShadow: a small half-dark disc at the lower right (the light casts shadows this frame)
static void DrawLightIcon(ImDrawList* drawList, const ImVec2& center, ImU32 color, bool enabled, bool selected, int rays, bool castsShadow)
{
	const float radius = 6.0f;
	drawList->AddCircleFilled(center, radius + 3.0f, IM_COL32(0, 0, 0, 150));
	if (enabled)
	{
		drawList->AddCircleFilled(center, radius, color);
	}
	else
	{
		drawList->AddCircle(center, radius, color, 0, 2.0f);
	}
	for (int i = 0; i < rays; i++)
	{
		float a = glm::two_pi<float>() * i / rays;
		ImVec2 d(std::cos(a), std::sin(a));
		ImVec2 from(center.x + d.x * (radius + 4.0f), center.y + d.y * (radius + 4.0f));
		ImVec2 to(center.x + d.x * (radius + 8.0f), center.y + d.y * (radius + 8.0f));
		drawList->AddLine(from, to, IM_COL32(0, 0, 0, 150), 4.0f);
		drawList->AddLine(from, to, color, 2.0f);
	}
	if (selected)
	{
		drawList->AddCircle(center, radius + 11.0f, IM_COL32(0, 0, 0, 150), 0, 4.0f);
		drawList->AddCircle(center, radius + 11.0f, IM_COL32(255, 255, 255, 230), 0, 2.0f);
	}
	if (castsShadow)
	{
		const ImVec2 mark(center.x + radius + 7.0f, center.y + radius + 7.0f);
		drawList->AddCircleFilled(mark, 5.5f, IM_COL32(0, 0, 0, 200));
		drawList->PathArcTo(mark, 4.0f, -glm::half_pi<float>(), glm::half_pi<float>(), 10); // lit half (right)
		drawList->PathFillConvex(IM_COL32(255, 255, 255, 230));
		drawList->AddCircle(mark, 4.0f, IM_COL32(255, 255, 255, 230), 0, 1.0f);
	}
}

// Light gizmos over the viewport image. Not depth tested (as editor light gizmos in Unity and Unreal), so a light is
// never lost behind a model:
// - every light: an icon in its color (8 rays: point light, 4 rays and an aim line: spot light, 12 rays: the sun)
// - the selected point light: its range as three circles
// - the selected spot light: its range and outer cone (solid), inner cone (faint)
// - the selected sun: an arrow ending at the world origin, in the direction its light travels
static void DrawLightGizmos()
{
	if (!s_ShowLightGizmos || s_ViewportImageSize.x <= 0.0f || s_ViewportImageSize.y <= 0.0f)
	{
		return;
	}
	const glm::mat4 viewProjection = (*s_Camera).GetViewProjection();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->PushClipRect(s_ViewportImageMin, ImVec2(s_ViewportImageMin.x + s_ViewportImageSize.x, s_ViewportImageMin.y + s_ViewportImageSize.y), true);

	// Shapes of the selected light first, icons on top
	if (s_SelectedLightKind == LightKind::Sun)
	{
		// An arrow from the icon in the direction the light travels
		ImU32 color = GetLightGizmoColor(s_Lights.Sun.Color, s_Lights.Sun.Enabled, 0.8f);
		glm::vec3 travel = -s_Lights.Sun.GetDirection();
		glm::vec3 from = GetSunGizmoPosition();
		glm::vec3 to = from + travel * 2.0f;
		DrawWorldLine(drawList, viewProjection, from, to, color, 2.0f);
		glm::vec3 u, v;
		GetPerpendicularAxes(travel, u, v);
		for (int i = 0; i < 4; i++) // arrowhead
		{
			float a = glm::half_pi<float>() * i;
			DrawWorldLine(drawList, viewProjection, to, to - travel * 0.4f + (std::cos(a) * u + std::sin(a) * v) * 0.15f, color, 2.0f);
		}
	}
	else if (s_SelectedLightKind == LightKind::Point)
	{
		const EnvMapVulkanPointLight& light = s_Lights.PointLights[s_SelectedLightIndex];
		ImU32 color = GetLightGizmoColor(light.Color, light.Enabled, 0.7f);
		const glm::vec3 x(1.0f, 0.0f, 0.0f), y(0.0f, 1.0f, 0.0f), z(0.0f, 0.0f, 1.0f);
		DrawWorldCircle(drawList, viewProjection, light.Position, x, y, light.Range, color);
		DrawWorldCircle(drawList, viewProjection, light.Position, x, z, light.Range, color);
		DrawWorldCircle(drawList, viewProjection, light.Position, y, z, light.Range, color);
	}
	else if (s_SelectedLightKind == LightKind::Spot)
	{
		const EnvMapVulkanSpotLight& light = s_Lights.SpotLights[s_SelectedLightIndex];
		glm::vec3 direction = light.GetDirection();
		glm::vec3 u, v;
		GetPerpendicularAxes(direction, u, v);
		auto drawCone = [&](float angle, ImU32 color, int sides) {
			// The cone ends where it meets the range sphere
			float halfAngle = glm::radians(glm::clamp(angle, 0.1f, 89.0f));
			glm::vec3 center = light.Position + direction * (light.Range * std::cos(halfAngle));
			float radius = light.Range * std::sin(halfAngle);
			DrawWorldCircle(drawList, viewProjection, center, u, v, radius, color);
			for (int i = 0; i < sides; i++)
			{
				float a = glm::two_pi<float>() * i / sides;
				DrawWorldLine(drawList, viewProjection, light.Position, center + radius * (std::cos(a) * u + std::sin(a) * v), color);
			}
		};
		drawCone(light.OuterAngle, GetLightGizmoColor(light.Color, light.Enabled, 0.8f), 8);
		drawCone(light.InnerAngle, GetLightGizmoColor(light.Color, light.Enabled, 0.3f), 4);
	}

	std::vector<int> pointShadowSlots, spotShadowSlots;
	EnvMapVulkanRenderer::GetShadowSlotsPerLight(pointShadowSlots, spotShadowSlots);
	ImVec2 pixel;
	DrawWorldLine(drawList, viewProjection, GetSunGizmoPosition(), GetSunGizmoPosition() - s_Lights.Sun.GetDirection() * 0.75f,
		GetLightGizmoColor(s_Lights.Sun.Color, s_Lights.Sun.Enabled), 2.0f); // aim line, as a spot light's
	if (ProjectToViewport(viewProjection, GetSunIconPosition(), pixel))
	{
		DrawLightIcon(drawList, pixel, GetLightGizmoColor(s_Lights.Sun.Color, s_Lights.Sun.Enabled), s_Lights.Sun.Enabled,
			s_SelectedLightKind == LightKind::Sun, 12, EnvMapVulkanRenderer::AreShadowsRendered());
	}
	for (int i = 0; i < (int)s_Lights.PointLights.size(); i++)
	{
		const EnvMapVulkanPointLight& light = s_Lights.PointLights[i];
		if (ProjectToViewport(viewProjection, light.Position, pixel))
		{
			DrawLightIcon(drawList, pixel, GetLightGizmoColor(light.Color, light.Enabled), light.Enabled,
				s_SelectedLightKind == LightKind::Point && s_SelectedLightIndex == i, 8, pointShadowSlots[i] >= 0);
		}
	}
	for (int i = 0; i < (int)s_Lights.SpotLights.size(); i++)
	{
		const EnvMapVulkanSpotLight& light = s_Lights.SpotLights[i];
		ImU32 color = GetLightGizmoColor(light.Color, light.Enabled);
		DrawWorldLine(drawList, viewProjection, light.Position, light.Position + light.GetDirection() * 0.75f, color, 2.0f);
		if (ProjectToViewport(viewProjection, light.Position, pixel))
		{
			DrawLightIcon(drawList, pixel, color, light.Enabled, s_SelectedLightKind == LightKind::Spot && s_SelectedLightIndex == i, 4,
				spotShadowSlots[i] >= 0);
		}
	}

	drawList->PopClipRect();
}

// A speaker: a box and a cone facing right, with two sound arcs while it plays; a white ring when selected
static void DrawSoundIcon(ImDrawList* drawList, const ImVec2& c, bool playing, bool selected)
{
	const ImU32 color = playing ? IM_COL32(80, 235, 210, 255) : IM_COL32(80, 170, 160, 200);
	drawList->AddCircleFilled(c, 11.0f, IM_COL32(0, 0, 0, 150));
	drawList->AddRectFilled(ImVec2(c.x - 6.0f, c.y - 2.5f), ImVec2(c.x - 2.5f, c.y + 2.5f), color);
	drawList->AddTriangleFilled(ImVec2(c.x - 3.0f, c.y - 2.5f), ImVec2(c.x + 2.0f, c.y - 6.5f), ImVec2(c.x + 2.0f, c.y + 6.5f), color);
	drawList->AddTriangleFilled(ImVec2(c.x - 3.0f, c.y + 2.5f), ImVec2(c.x - 3.0f, c.y - 2.5f), ImVec2(c.x + 2.0f, c.y + 6.5f), color);
	if (playing)
	{
		drawList->PathArcTo(c, 6.0f, -0.7f, 0.7f, 8);
		drawList->PathStroke(color, 0, 1.5f);
		drawList->PathArcTo(c, 9.0f, -0.7f, 0.7f, 8);
		drawList->PathStroke(color, 0, 1.5f);
	}
	if (selected)
	{
		drawList->AddCircle(c, 15.0f, IM_COL32(0, 0, 0, 150), 0, 4.0f);
		drawList->AddCircle(c, 15.0f, IM_COL32(255, 255, 255, 230), 0, 2.0f);
	}
}

// The Audio debug view (5.3): a speaker icon on every sound (bright while it plays), and the selected spatial sound's
// distances: Min (full volume inside) and Max (no softer beyond) as three circles each. Not depth tested, as the lights
static void DrawAudioGizmos()
{
	if (!s_ShowAudioGizmos || s_ViewportImageSize.x <= 0.0f || s_ViewportImageSize.y <= 0.0f)
	{
		return;
	}
	const glm::mat4 viewProjection = (*s_Camera).GetViewProjection();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->PushClipRect(s_ViewportImageMin, ImVec2(s_ViewportImageMin.x + s_ViewportImageSize.x, s_ViewportImageMin.y + s_ViewportImageSize.y), true);
	const EnvMapVulkanEntityID selected = s_EditorSelection.GetPrimary();
	SceneAudio* game = GetGameAudio();
	s_Scene.Each<AudioSourceComponent>([&](EnvMapVulkanEntityID entity, AudioSourceComponent& source)
	{
		const glm::vec3 position = glm::vec3(s_Scene.GetWorldTransform(entity)[3]);
		if (entity == selected && source.Spatial)
		{
			const glm::vec3 x(1.0f, 0.0f, 0.0f), y(0.0f, 1.0f, 0.0f), z(0.0f, 0.0f, 1.0f);
			for (int i = 0; i < 2; i++)
			{
				const float radius = i == 0 ? source.MinDistance : source.MaxDistance;
				const ImU32 color = i == 0 ? IM_COL32(80, 235, 210, 200) : IM_COL32(80, 235, 210, 90);
				DrawWorldCircle(drawList, viewProjection, position, x, y, radius, color);
				DrawWorldCircle(drawList, viewProjection, position, x, z, radius, color);
				DrawWorldCircle(drawList, viewProjection, position, y, z, radius, color);
			}
		}
		ImVec2 pixel;
		if (ProjectToViewport(viewProjection, position, pixel))
		{
			DrawSoundIcon(drawList, pixel, game && game->IsPlaying(entity), s_EditorSelection.IsSelected(entity));
		}
	});
	drawList->PopClipRect();
}

// A camera: a body and a lens; a white ring when selected
static void DrawCameraIcon(ImDrawList* drawList, const ImVec2& c, ImU32 color, bool selected)
{
	drawList->AddCircleFilled(c, 11.0f, IM_COL32(0, 0, 0, 150));
	drawList->AddRectFilled(ImVec2(c.x - 6.5f, c.y - 4.0f), ImVec2(c.x + 2.5f, c.y + 4.0f), color, 1.5f);
	drawList->AddTriangleFilled(ImVec2(c.x + 2.5f, c.y), ImVec2(c.x + 7.0f, c.y - 4.0f), ImVec2(c.x + 7.0f, c.y + 4.0f), color);
	if (selected)
	{
		drawList->AddCircle(c, 15.0f, IM_COL32(0, 0, 0, 150), 0, 4.0f);
		drawList->AddCircle(c, 15.0f, IM_COL32(255, 255, 255, 230), 0, 2.0f);
	}
}

// The game cameras (6.3): an icon on each (bright: the primary one, which Play shows) and the view it sees as a short
// pyramid (the selected one's longer), with the viewport's aspect ratio. Not depth tested, as the lights. The Light
// Icons checkbox shows and hides them with the lights
static void DrawCameraGizmos()
{
	if (!s_ShowLightGizmos || s_ViewportImageSize.x <= 0.0f || s_ViewportImageSize.y <= 0.0f)
	{
		return;
	}
	const glm::mat4 viewProjection = (*s_Camera).GetViewProjection();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->PushClipRect(s_ViewportImageMin, ImVec2(s_ViewportImageMin.x + s_ViewportImageSize.x, s_ViewportImageMin.y + s_ViewportImageSize.y), true);
	const EnvMapVulkanEntityID primary = FindPrimaryCamera(s_Scene);
	const float aspect = s_ViewportImageSize.x / s_ViewportImageSize.y;
	s_Scene.Each<CameraComponent>([&](EnvMapVulkanEntityID entity, CameraComponent& camera)
	{
		const TransformComponent placement = GetGameCameraPlacement(s_Scene, entity);
		const bool selected = s_EditorSelection.IsSelected(entity);
		const ImU32 color = entity == primary ? IM_COL32(240, 120, 170, 255) : IM_COL32(170, 120, 150, 200);
		const glm::vec3 forward = GetCameraForward(placement.Rotation);
		const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
		const glm::vec3 up = glm::cross(right, forward);
		const float depth = selected ? 3.0f : 1.2f;
		const float halfHeight = depth * std::tan(glm::radians(camera.FieldOfView) * 0.5f);
		const float halfWidth = halfHeight * aspect;
		const glm::vec3 center = placement.Translation + forward * depth;
		const glm::vec3 corners[4] = { center + up * halfHeight - right * halfWidth, center + up * halfHeight + right * halfWidth,
			center - up * halfHeight + right * halfWidth, center - up * halfHeight - right * halfWidth };
		const float thickness = selected ? 2.0f : 1.0f;
		for (int i = 0; i < 4; i++)
		{
			DrawWorldLine(drawList, viewProjection, placement.Translation, corners[i], color, thickness);
			DrawWorldLine(drawList, viewProjection, corners[i], corners[(i + 1) % 4], color, thickness);
		}
		// Which way is up in its view: a small triangle on the top edge
		DrawWorldLine(drawList, viewProjection, corners[0] + right * halfWidth * 0.6f, center + up * halfHeight * 1.3f, color, thickness);
		DrawWorldLine(drawList, viewProjection, center + up * halfHeight * 1.3f, corners[1] - right * halfWidth * 0.6f, color, thickness);
		ImVec2 pixel;
		if (ProjectToViewport(viewProjection, placement.Translation, pixel))
		{
			DrawCameraIcon(drawList, pixel, color, selected);
		}
	});
	drawList->PopClipRect();
}

// The camera whose icon is under the mouse (the nearest one within 12 pixels)
static EnvMapVulkanEntityID PickCameraIcon()
{
	if (!s_ShowLightGizmos)
	{
		return NoEntity;
	}
	const glm::mat4 viewProjection = (*s_Camera).GetViewProjection();
	const ImVec2 mouse = ImGui::GetMousePos();
	float nearest = 12.0f * 12.0f;
	EnvMapVulkanEntityID found = NoEntity;
	s_Scene.Each<CameraComponent>([&](EnvMapVulkanEntityID entity, CameraComponent&)
	{
		ImVec2 pixel;
		if (ProjectToViewport(viewProjection, GetGameCameraPlacement(s_Scene, entity).Translation, pixel))
		{
			const float dx = pixel.x - mouse.x, dy = pixel.y - mouse.y;
			if (dx * dx + dy * dy < nearest)
			{
				nearest = dx * dx + dy * dy;
				found = entity;
			}
		}
	});
	return found;
}

// The sound whose icon is under the mouse (the nearest one within 12 pixels)
static EnvMapVulkanEntityID PickSoundIcon()
{
	if (!s_ShowAudioGizmos)
	{
		return NoEntity;
	}
	const glm::mat4 viewProjection = (*s_Camera).GetViewProjection();
	const ImVec2 mouse = ImGui::GetMousePos();
	float nearest = 12.0f * 12.0f;
	EnvMapVulkanEntityID found = NoEntity;
	s_Scene.Each<AudioSourceComponent>([&](EnvMapVulkanEntityID entity, AudioSourceComponent&)
	{
		ImVec2 pixel;
		if (ProjectToViewport(viewProjection, glm::vec3(s_Scene.GetWorldTransform(entity)[3]), pixel))
		{
			const float dx = pixel.x - mouse.x, dy = pixel.y - mouse.y;
			if (dx * dx + dy * dy < nearest)
			{
				nearest = dx * dx + dy * dy;
				found = entity;
			}
		}
	});
	return found;
}

// The physics debug views (the Physics panel): colliders, contacts and the query test, as thin lines over the scene (not
// depth tested: a collider is seen through what is in front of it)
static void DrawPhysicsOverlay()
{
	if (s_ViewportImageSize.x <= 0.0f || s_ViewportImageSize.y <= 0.0f)
	{
		return;
	}
	static std::vector<EditorWorldLine> lines; // kept: its memory is reused each frame
	lines.clear();
	GetPhysicsOverlayLines(lines);
	if (lines.empty())
	{
		return;
	}
	const glm::mat4 viewProjection = (*s_Camera).GetViewProjection();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->PushClipRect(s_ViewportImageMin, ImVec2(s_ViewportImageMin.x + s_ViewportImageSize.x, s_ViewportImageMin.y + s_ViewportImageSize.y), true);
	for (const EditorWorldLine& line : lines)
	{
		DrawWorldLine(drawList, viewProjection, line.A, line.B, line.Color, 1.0f);
	}
	drawList->PopClipRect();
}

// The selected water's border: an orange rectangle (made of short segments, so the parts in front of the camera show when
// the rest is behind it)
static void DrawWaterOutline()
{
	if (!s_WaterSelected || s_ViewportImageSize.x <= 0.0f || s_ViewportImageSize.y <= 0.0f)
	{
		return;
	}
	const glm::mat4 viewProjection = (*s_Camera).GetViewProjection();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->PushClipRect(s_ViewportImageMin, ImVec2(s_ViewportImageMin.x + s_ViewportImageSize.x, s_ViewportImageMin.y + s_ViewportImageSize.y), true);
	const glm::mat4 transform = s_WaterSettings.GetTransform();
	const glm::vec3 corners[4] = {
		glm::vec3(transform * glm::vec4(-0.5f, 0.0f, -0.5f, 1.0f)), glm::vec3(transform * glm::vec4(0.5f, 0.0f, -0.5f, 1.0f)),
		glm::vec3(transform * glm::vec4(0.5f, 0.0f, 0.5f, 1.0f)), glm::vec3(transform * glm::vec4(-0.5f, 0.0f, 0.5f, 1.0f)) };
	const int segments = 32;
	for (int side = 0; side < 4; side++)
	{
		const glm::vec3& a = corners[side];
		const glm::vec3& b = corners[(side + 1) % 4];
		for (int i = 0; i < segments; i++)
		{
			DrawWorldLine(drawList, viewProjection, glm::mix(a, b, (float)i / segments), glm::mix(a, b, (float)(i + 1) / segments), IM_COL32(255, 140, 0, 255), 2.0f);
		}
	}
	drawList->PopClipRect();
}

// The light whose icon is under the mouse (the nearest one within 12 pixels)
static bool PickLightIcon(LightKind& kind, int& index)
{
	if (!s_ShowLightGizmos)
	{
		return false;
	}
	const glm::mat4 viewProjection = (*s_Camera).GetViewProjection();
	const ImVec2 mouse = ImGui::GetMousePos();
	float nearest = 12.0f * 12.0f;
	bool found = false;
	auto test = [&](const glm::vec3& position, LightKind k, int i) {
		ImVec2 pixel;
		if (ProjectToViewport(viewProjection, position, pixel))
		{
			float dx = pixel.x - mouse.x, dy = pixel.y - mouse.y;
			if (dx * dx + dy * dy < nearest)
			{
				nearest = dx * dx + dy * dy;
				kind = k;
				index = i;
				found = true;
			}
		}
	};
	test(GetSunIconPosition(), LightKind::Sun, 0);
	for (int i = 0; i < (int)s_Lights.PointLights.size(); i++)
	{
		test(s_Lights.PointLights[i].Position, LightKind::Point, i);
	}
	for (int i = 0; i < (int)s_Lights.SpotLights.size(); i++)
	{
		test(s_Lights.SpotLights[i].Position, LightKind::Spot, i);
	}
	return found;
}

// The gizmo for the selected sun, at its icon: 1 moves the icon (editor only, the sun has no position), 2 turns the sun.
// Its local Z axis points towards the sun. The first change detaches the icon from the sky: from then on it stays where
// it is (rotating the sky icon would otherwise move it, and the gizmo with it, while turning).
static void ManipulateSun(int gizmoType, bool snap)
{
	const ImGuizmo::OPERATION operation = gizmoType == ImGuizmo::OPERATION::TRANSLATE ? ImGuizmo::OPERATION::TRANSLATE : ImGuizmo::OPERATION::ROTATE;
	glm::vec3 z = s_Lights.Sun.GetDirection(), x, y;
	GetPerpendicularAxes(z, x, y);
	glm::mat4 transform(1.0f);
	transform[0] = glm::vec4(x, 0.0f);
	transform[1] = glm::vec4(y, 0.0f);
	transform[2] = glm::vec4(z, 0.0f);
	transform[3] = glm::vec4(GetSunGizmoPosition(), 1.0f);

	float snapValue = operation == ImGuizmo::OPERATION::ROTATE ? 15.0f : 0.5f;
	float snapValues[3] = { snapValue, snapValue, snapValue };
	if (ImGuizmo::Manipulate(
		glm::value_ptr((*s_Camera).GetViewMatrix()),
		glm::value_ptr((*s_Camera).GetProjectionMatrix()),
		operation,
		ImGuizmo::WORLD,
		glm::value_ptr(transform),
		nullptr,
		snap ? snapValues : nullptr))
	{
		const EnvMapVulkanDirectionalLight before = s_Lights.Sun;
		s_Lights.Sun.IconPosition = glm::vec3(transform[3]);
		s_Lights.Sun.IconMoved = true;
		if (operation == ImGuizmo::OPERATION::ROTATE)
		{
			s_Lights.Sun.SetDirection(glm::vec3(transform[2])); // azimuth and elevation follow (azimuth in -180..180)
		}
		CommitSun(before);
	}
}

// The Range of a point or spot light entity (null: not one)
static float* GetLightRange(EnvMapVulkanEntityID entity)
{
	if (PointLightComponent* point = s_Scene.TryGet<PointLightComponent>(entity))
	{
		return &point->Range;
	}
	if (SpotLightComponent* spot = s_Scene.TryGet<SpotLightComponent>(entity))
	{
		return &spot->Range;
	}
	return nullptr;
}

static constexpr float MinLightRange = 0.1f;
static constexpr float MaxLightRange = 1000.0f; // the Range drag's limit

// The point and spot lights under a scaled entity (a model) are scaled with it, by the hierarchy; their Range then too,
// by the uniform scale (see MoveLightWith). Not under a light: its children don't scale with it (its matrix isn't scaled).
void ScaleChildLightRanges(EnvMapVulkanEntityID entity, float scale)
{
	for (EnvMapVulkanEntityID child : s_Scene.GetChildren(entity))
	{
		if (float* range = GetLightRange(child))
		{
			*range = glm::clamp(*range * scale, MinLightRange, MaxLightRange);
			continue;
		}
		ScaleChildLightRanges(child, scale);
	}
}

// How much the gizmo scaled the selected light's range this frame (1: not scaled); a multi-selection is scaled by it
// around the light (see UpdateImGuizmo)
static float s_LightGizmoScale = 1.0f;

// The gizmo on the selected point or spot light, which works on the light entity's transform like on any entity:
// translate moves it, rotate turns it (a spot light's beam; turning a point light changes nothing in its light, but turns
// a multi-selection around it), scale changes its Range (one handle: a light has one reach, the same in every direction)
static void ManipulateSelectedLight(int gizmoType, bool snap)
{
	s_LightGizmoScale = 1.0f;
	const EnvMapVulkanEntityID entity = s_LightEntities.Get(s_SelectedLightKind, s_SelectedLightIndex);
	float* range = GetLightRange(entity);
	if (!range)
	{
		return;
	}

	// The light's world matrix without scale: the gizmo's axes, and a scale handle that starts at 1
	glm::mat4 transform = s_Scene.GetWorldTransform(entity);
	for (int axis = 0; axis < 3; axis++)
	{
		transform[axis] = glm::vec4(glm::normalize(glm::vec3(transform[axis])), 0.0f);
	}

	ImGuizmo::OPERATION operation = (ImGuizmo::OPERATION)gizmoType;
	ImGuizmo::MODE mode = ImGuizmo::WORLD;
	float snapValue = 0.5f;
	if (operation == ImGuizmo::OPERATION::ROTATE)
	{
		snapValue = 15.0f;
	}
	else if (operation == ImGuizmo::OPERATION::SCALE)
	{
		operation = ImGuizmo::OPERATION::SCALE_X; // one handle, along the light's own X axis
		mode = ImGuizmo::LOCAL;
		snapValue = 0.1f;
	}
	// ImGuizmo scales relative to the start of the drag: the range then is the range at the start times the handle's scale
	static float s_RangeAtDragStart = 0.0f;
	if (!ImGuizmo::IsUsing())
	{
		s_RangeAtDragStart = *range;
	}

	float snapValues[3] = { snapValue, snapValue, snapValue };
	if (!ImGuizmo::Manipulate(
		glm::value_ptr((*s_Camera).GetViewMatrix()),
		glm::value_ptr((*s_Camera).GetProjectionMatrix()),
		operation,
		mode,
		glm::value_ptr(transform),
		nullptr,
		snap ? snapValues : nullptr))
	{
		return;
	}

	if (operation == ImGuizmo::OPERATION::SCALE_X)
	{
		const float newRange = glm::clamp(s_RangeAtDragStart * glm::length(glm::vec3(transform[0])), MinLightRange, MaxLightRange);
		s_LightGizmoScale = newRange / *range;
		*range = newRange;
	}
	else if (operation == ImGuizmo::OPERATION::TRANSLATE)
	{
		SetEntityPosition(s_Scene, entity, glm::vec3(transform[3])); // only the position: the rotation isn't recomposed from Euler angles
	}
	else
	{
		// Rotated: the light's own scale (normally 1) is kept
		const glm::mat4 world = s_Scene.GetWorldTransform(entity);
		for (int axis = 0; axis < 3; axis++)
		{
			transform[axis] *= glm::length(glm::vec3(world[axis]));
		}
		s_Scene.SetWorldTransform(entity, transform);
	}
	ExtractLights();
}

// The frame rate in the viewport's top-left corner
static void DrawViewportFps()
{
	if (!s_ShowViewportFps || s_ViewportImageSize.x <= 0.0f)
	{
		return;
	}
	char text[64];
	const auto& results = EnvMapVulkanProfiler::GetResults();
	if (results.Valid)
	{
		snprintf(text, sizeof(text), "%.0f FPS  %.2f ms  GPU %.2f ms", EnvMapVulkanProfiler::GetFps(), EnvMapVulkanProfiler::GetSmoothedCpuFrameMs(),
			results.SmoothedGpuMs);
	}
	else
	{
		snprintf(text, sizeof(text), "%.0f FPS  %.2f ms", EnvMapVulkanProfiler::GetFps(), EnvMapVulkanProfiler::GetSmoothedCpuFrameMs());
	}
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImVec2 textSize = ImGui::CalcTextSize(text);
	const ImVec2 position(s_ViewportImageMin.x + 8.0f, s_ViewportImageMin.y + 8.0f);
	drawList->AddRectFilled(ImVec2(position.x - 5.0f, position.y - 3.0f), ImVec2(position.x + textSize.x + 5.0f, position.y + textSize.y + 3.0f),
		IM_COL32(0, 0, 0, 140), 4.0f);
	drawList->AddText(position, IM_COL32(235, 235, 235, 255), text);
}

static void UpdateImGuizmo(Window* mainWindow);

// The Viewport panel: the scene's image, with picking (click: select, Ctrl or Shift + click: add or remove), drops (a material,
// a model file, an .hdr map), the light icons, the gizmo and the frame rate
void OnImGuiRenderViewport()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::Begin("Viewport");

	// Tell ImGuiWrapper the scene is shown in this ImGui window, so camera input (CameraController)
	// works while the viewport is hovered/focused, instead of being blocked as "mouse over an ImGui window"
	// While the game runs, the viewport is the game's: the editor's camera doesn't fly, nothing is picked or moved with
	// the gizmo, and Delete, Ctrl+C and Ctrl+V do nothing here (the panels still edit the running game)
	const bool gameRunning = IsGameRunning();
	UpdateGameViewportFocus(ImGui::IsWindowFocused());
	ImGuiWrapper::SetViewportEnabled(true);
	ImGuiWrapper::SetViewportHovered(ImGui::IsWindowHovered() && !gameRunning);
	ImGuiWrapper::SetViewportFocused(ImGui::IsWindowFocused() && !gameRunning);
	if (ImGui::IsWindowFocused() && !gameRunning)
	{
		HandleCopyPasteKeys(); // Ctrl+C, Ctrl+V: copy and paste the selected models, meshes and lights
		// Delete: the selected entities, as in the Scene Hierarchy (not while typing in a field)
		if (!ImGui::GetIO().WantTextInput && s_RenamingEntity == NoEntity && ImGui::IsKeyPressed(ImGuiKey_Delete, false))
		{
			DeleteSelectedEntities();
		}
	}

	auto viewportOffset = ImGui::GetCursorPos(); // includes tab bar
	auto viewportSize = ImGui::GetContentRegionAvail();
	ImGui::Image(EnvMapVulkanRenderer::GetViewportImage(), viewportSize, { 0, 1 }, { 1, 0 });
	s_ViewportImageMin = ImGui::GetItemRectMin();
	s_ViewportImageSize = ImGui::GetItemRectSize();
	bool viewportImageHovered = ImGui::IsItemHovered();

	// Drop onto the scene:
	// - a material from the Material Library: the mesh under the cursor gets it and becomes the selection
	//   (so the Material Editor shows the material)
	// - a model file from the Content Browser: placed standing on the ground under the cursor (see LoadModel)
	// - an .hdr file from the Content Browser: loaded as the environment map
	if (ImGui::BeginDragDropTarget())
	{
		glm::vec2 ndc = GetViewportMouseNdc();

		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(s_MaterialPayload))
		{
			uint32_t index = *(const uint32_t*)payload->Data;
			const auto& materials = EnvMapVulkanMaterialLibrary::GetMaterials();

			EnvMapVulkanEntityID hitModel;
			int hitMesh;
			RaycastMesh(ndc.x, ndc.y, hitModel, hitMesh);

			EnvMapVulkanEntityID hitPart = hitModel != NoEntity ? FindPart(s_Scene, hitModel, hitMesh) : NoEntity;
			if (index < materials.size() && hitPart != NoEntity)
			{
				AssignMaterialToDropTarget(hitPart, materials[index]);
			}
		}

		ImVec2 imageMax(s_ViewportImageMin.x + s_ViewportImageSize.x, s_ViewportImageMin.y + s_ViewportImageSize.y);
		auto isSceneFile = [](const std::string& path) { return EnvMapVulkanSceneLoader::IsModelFile(path) || IsEnvironmentMapFile(path) || IsPrefabFile(path); };
		std::string droppedPath;
		if (AcceptFileDrop(s_ViewportImageMin, imageMax, isSceneFile, "a model file, a prefab or an .hdr environment map", droppedPath))
		{
			// loaded at the start of the next frame (see EditorPreFrame)
			if (IsEnvironmentMapFile(droppedPath))
			{
				RequestEnvironmentMap(droppedPath);
			}
			else if (IsPrefabFile(droppedPath))
			{
				// Placed on the ground under the cursor (its root's origin there)
				RequestPrefabOperation(PrefabOperation::Place, NoEntity, droppedPath, GetDropGroundPosition(ndc.x, ndc.y));
			}
			else
			{
				s_PendingModelFilename = droppedPath;
				s_PendingModelGroundPosition = GetDropGroundPosition(ndc.x, ndc.y);
			}
		}
		ImGui::EndDragDropTarget();
	}

	// Compare whole pixels: the panel size can be fractional (DPI scaling, docking), and comparing the float size with
	// the stored integer size requested a framebuffer resize every frame. A collapsed/hidden panel has no area: keep the size.
	uint32_t viewportWidth = (uint32_t)glm::max(viewportSize.x, 0.0f);
	uint32_t viewportHeight = (uint32_t)glm::max(viewportSize.y, 0.0f);
	EnvMapVulkanRenderer::SetViewportSize(viewportWidth, viewportHeight);

	SyncLightSelection();
	DrawPhysicsOverlay();
	if (!gameRunning)
	{
		DrawLightGizmos();
		DrawCameraGizmos();
		DrawAudioGizmos();
	}
	DrawViewportFps();
	if (!gameRunning)
	{
		DrawWaterOutline();
	}
	DrawPlayModeOutline(s_ViewportImageMin, s_ViewportImageSize);

	Window* mainWindow = Application::Get()->GetWindow();
	if (!gameRunning)
	{
		UpdateImGuizmo(mainWindow);
	}

	// Mouse picking: left click on the scene (not on the gizmo, not with Alt) selects the light icon, or else the
	// mesh under the cursor (or the water in front of it). Ctrl or Shift + click adds it to the selection or removes
	// it (as in Unity); a double click selects the whole model of the mesh, and Ctrl or Shift + double click adds
	// all the model's meshes (parts) to the selection.
	if (!gameRunning && viewportImageHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing() &&
		!Input::IsKeyPressed(KeyH2M::LeftAlt) && s_ViewportImageSize.x > 0.0f && s_ViewportImageSize.y > 0.0f)
	{
		const bool toggle = Input::IsKeyPressed(KeyH2M::LeftControl) || Input::IsKeyPressed(KeyH2M::RightControl) ||
			Input::IsKeyPressed(KeyH2M::LeftShift) || Input::IsKeyPressed(KeyH2M::RightShift);
		EnvMapVulkanEntityID clicked = NoEntity;
		EnvMapVulkanEntityID clickedModel = NoEntity;
		LightKind lightKind;
		int lightIndex;
		const EnvMapVulkanEntityID clickedSound = PickSoundIcon();
		const EnvMapVulkanEntityID clickedCamera = PickCameraIcon();
		if (PickLightIcon(lightKind, lightIndex))
		{
			clicked = s_LightEntities.Get(lightKind, lightIndex);
		}
		else if (clickedCamera != NoEntity)
		{
			clicked = clickedCamera;
		}
		else if (clickedSound != NoEntity)
		{
			clicked = clickedSound;
		}
		else
		{
			glm::vec2 ndc = GetViewportMouseNdc();
			glm::vec3 meshHit;
			int hitMesh;
			RaycastMesh(ndc.x, ndc.y, clickedModel, hitMesh, &meshHit);
			clicked = clickedModel != NoEntity ? GetModelOrPart(clickedModel, hitMesh) : NoEntity;
			// The water, when it is in front of the model under the cursor (or there is none)
			glm::vec3 rayOrigin, rayDirection;
			GetCameraRay(ndc.x, ndc.y, rayOrigin, rayDirection);
			float waterT;
			if (s_WaterEntity != NoEntity && RaycastWater(s_WaterSettings, rayOrigin, rayDirection, waterT) &&
				(clickedModel == NoEntity || waterT < glm::length(meshHit - rayOrigin)))
			{
				clicked = s_WaterEntity;
				clickedModel = NoEntity;
			}
		}
		if (toggle && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && clickedModel != NoEntity)
		{
			s_EditorSelection.AddModelParts(clickedModel, clicked); // before the toggle: its first click toggled the mesh already
		}
		else if (toggle)
		{
			s_EditorSelection.Toggle(clicked); // nothing under the cursor: the selection stays
		}
		else if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && clickedModel != NoEntity)
		{
			s_EditorSelection.SelectOnly(clickedModel); // the gizmo moves the model, not the part under the mouse
		}
		else
		{
			s_EditorSelection.SelectOnly(clicked); // NoEntity: nothing under the cursor deselects
		}
		ExtractLights(); // the selected light's kind and the water's selection follow the primary entity
		ExtractWater();
	}

	ImGui::End();
	ImGui::PopStyleVar();
}

// The rotate gizmo on a game camera: two rings, pitch around the camera's own right axis (red) and yaw around the
// world's vertical axis (green), in a frame that turns with the camera but stays level, so the yaw ring stays flat
// however far it looks down. No roll ring: a camera isn't rolled. Ctrl snaps to 15 degrees
static void ManipulateCameraRotation(EnvMapVulkanEntityID entity, bool snap)
{
	TransformComponent& local = s_Scene.Get<TransformComponent>(entity);
	const TransformComponent placement = GetGameCameraPlacement(s_Scene, entity);
	const float parentYaw = placement.Rotation.y - local.Rotation.y; // a child turns with its parent
	const glm::vec3 forward = GetCameraForward(placement.Rotation);
	const glm::vec3 up(0.0f, 1.0f, 0.0f);
	const glm::vec3 flat = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z)); // the pitch stays within 89 degrees
	const glm::vec3 right = glm::normalize(glm::cross(flat, up));
	glm::mat4 frame(1.0f);
	frame[0] = glm::vec4(right, 0.0f);
	frame[1] = glm::vec4(up, 0.0f);
	frame[2] = glm::vec4(-flat, 0.0f);
	frame[3] = glm::vec4(placement.Translation, 1.0f);
	// The view's direction in that frame: the gizmo turns the frame, and the view with it
	const glm::vec3 localForward = glm::transpose(glm::mat3(frame)) * forward;

	glm::mat4 result = frame;
	float snapValues[3] = { 15.0f, 15.0f, 15.0f };
	if (!ImGuizmo::Manipulate(
		glm::value_ptr((*s_Camera).GetViewMatrix()),
		glm::value_ptr((*s_Camera).GetProjectionMatrix()),
		ImGuizmo::OPERATION::ROTATE_X | ImGuizmo::OPERATION::ROTATE_Y,
		ImGuizmo::LOCAL,
		glm::value_ptr(result),
		nullptr,
		snap ? snapValues : nullptr))
	{
		return;
	}

	// Back to the camera's angles (as GetCameraForward has them): x down, y to the left, 0 along -Z
	const glm::vec3 turned = glm::normalize(glm::mat3(result) * localForward);
	const float pitch = glm::degrees(std::asin(glm::clamp(-turned.y, -1.0f, 1.0f)));
	float yaw = -glm::degrees(std::atan2(turned.z, turned.x)) - 90.0f - parentYaw;
	yaw = std::fmod(yaw, 360.0f);
	yaw += yaw > 180.0f ? -360.0f : (yaw <= -180.0f ? 360.0f : 0.0f);
	local.Rotation = glm::vec3(glm::clamp(pitch, -89.0f, 89.0f), yaw, 0.0f);
}

// The gizmo on the primary selected entity (s_EditorSelection.GetPrimary()): a light (the sun turns), the water, a part of a model or a model
static void ManipulatePrimarySelection()
{
	// The probe volume: translate moves its box, scale sizes it along the world's axes; it can't be turned
	if (s_ProbeVolumeEntity != NoEntity && s_EditorSelection.GetPrimary() == s_ProbeVolumeEntity)
	{
		const bool movable = Scene::s_ImGuizmoType == ImGuizmo::OPERATION::TRANSLATE || Scene::s_ImGuizmoType == ImGuizmo::OPERATION::SCALE;
		if (movable && s_ViewportImageSize.x > 0.0f && s_ViewportImageSize.y > 0.0f)
		{
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(s_ViewportImageMin.x, s_ViewportImageMin.y, s_ViewportImageSize.x, s_ViewportImageSize.y);
			float snapValues[3] = { 1.0f, 1.0f, 1.0f };
			glm::mat4 world = GetProbeVolumeTransform(s_ProbeVolumeSettings);
			if (ImGuizmo::Manipulate(
				glm::value_ptr((*s_Camera).GetViewMatrix()),
				glm::value_ptr((*s_Camera).GetProjectionMatrix()),
				(ImGuizmo::OPERATION)Scene::s_ImGuizmoType,
				ImGuizmo::WORLD,
				glm::value_ptr(world),
				nullptr,
				Input::IsKeyPressed(KeyH2M::LeftControl) ? snapValues : nullptr))
			{
				const EnvMapVulkanProbeVolumeSettings before = s_ProbeVolumeSettings;
				s_ProbeVolumeSettings.Center = glm::vec3(world[3]);
				s_ProbeVolumeSettings.Size = glm::vec3(glm::length(glm::vec3(world[0])), glm::length(glm::vec3(world[1])), glm::length(glm::vec3(world[2])));
				CommitProbeVolume(before);
			}
		}
		return;
	}

	// A sound alone or a game camera: translate moves it (a sound has no size, and only its place is heard); rotate aims a
	// camera (pitch and yaw only: it isn't rolled)
	if (const EnvMapVulkanEntityID primary = s_EditorSelection.GetPrimary(); s_Scene.Exists(primary) &&
		(GetEntityKind(primary) == EntityKind::Sound || GetEntityKind(primary) == EntityKind::Camera))
	{
		if (Scene::s_ImGuizmoType == ImGuizmo::OPERATION::ROTATE && GetEntityKind(primary) == EntityKind::Camera &&
			s_ViewportImageSize.x > 0.0f && s_ViewportImageSize.y > 0.0f)
		{
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(s_ViewportImageMin.x, s_ViewportImageMin.y, s_ViewportImageSize.x, s_ViewportImageSize.y);
			ManipulateCameraRotation(primary, Input::IsKeyPressed(KeyH2M::LeftControl));
		}
		else if (Scene::s_ImGuizmoType == ImGuizmo::OPERATION::TRANSLATE && s_ViewportImageSize.x > 0.0f && s_ViewportImageSize.y > 0.0f)
		{
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(s_ViewportImageMin.x, s_ViewportImageMin.y, s_ViewportImageSize.x, s_ViewportImageSize.y);
			float snapValues[3] = { 0.5f, 0.5f, 0.5f };
			glm::mat4 world = glm::translate(glm::mat4(1.0f), glm::vec3(s_Scene.GetWorldTransform(primary)[3]));
			if (ImGuizmo::Manipulate(
				glm::value_ptr((*s_Camera).GetViewMatrix()),
				glm::value_ptr((*s_Camera).GetProjectionMatrix()),
				ImGuizmo::OPERATION::TRANSLATE,
				ImGuizmo::WORLD,
				glm::value_ptr(world),
				nullptr,
				Input::IsKeyPressed(KeyH2M::LeftControl) ? snapValues : nullptr))
			{
				SetEntityPosition(s_Scene, primary, glm::vec3(world[3]));
			}
		}
		return;
	}

	// The gizmo moves the selected point or spot light, or else the model or mesh selected in the Models and Meshes panel (or picked with the mouse)
	if (Scene::s_ImGuizmoType != -1 && s_SelectedLightKind != LightKind::None && s_ViewportImageSize.x > 0.0f && s_ViewportImageSize.y > 0.0f)
	{
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(s_ViewportImageMin.x, s_ViewportImageMin.y, s_ViewportImageSize.x, s_ViewportImageSize.y);
		if (s_SelectedLightKind == LightKind::Sun)
		{
			ManipulateSun(Scene::s_ImGuizmoType, Input::IsKeyPressed(KeyH2M::LeftControl));
		}
		else
		{
			ManipulateSelectedLight(Scene::s_ImGuizmoType, Input::IsKeyPressed(KeyH2M::LeftControl));
		}
		return;
	}

	// The water: translate moves it (its position and height), rotate turns it around Y only (it is always level), scale
	// resizes it along its own X and Z (a flat surface: no height scale)
	if (Scene::s_ImGuizmoType != -1 && s_WaterSelected && s_ViewportImageSize.x > 0.0f && s_ViewportImageSize.y > 0.0f)
	{
		ImGuizmo::OPERATION operation = (ImGuizmo::OPERATION)Scene::s_ImGuizmoType;
		ImGuizmo::MODE mode = ImGuizmo::WORLD;
		float snap = 1.0f;
		if (operation == ImGuizmo::OPERATION::ROTATE)
		{
			operation = ImGuizmo::OPERATION::ROTATE_Y;
			snap = 45.0f;
		}
		else if (operation == ImGuizmo::OPERATION::SCALE)
		{
			operation = ImGuizmo::OPERATION::SCALE_X | ImGuizmo::OPERATION::SCALE_Z;
			mode = ImGuizmo::LOCAL; // along the water's own sides
		}
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(s_ViewportImageMin.x, s_ViewportImageMin.y, s_ViewportImageSize.x, s_ViewportImageSize.y);
		float snapValues[3] = { snap, snap, snap };
		glm::mat4 transform = s_WaterSettings.GetTransform();
		if (ImGuizmo::Manipulate(
			glm::value_ptr((*s_Camera).GetViewMatrix()),
			glm::value_ptr((*s_Camera).GetProjectionMatrix()),
			operation,
			mode,
			glm::value_ptr(transform),
			nullptr,
			Input::IsKeyPressed(KeyH2M::LeftControl) ? snapValues : nullptr))
		{
			// Read back from the matrix's columns (Euler angles would turn a Y rotation past 90 degrees into 180, y, 180):
			// the water's X side is the first column (cos, 0, -sin) * size x, its Z side the third
			const glm::vec3 sideX = glm::vec3(transform[0]);
			const glm::vec3 sideZ = glm::vec3(transform[2]);
			const EnvMapVulkanWaterSettings before = s_WaterSettings;
			s_WaterSettings.Center = glm::vec2(transform[3].x, transform[3].z);
			s_WaterSettings.Height = transform[3].y;
			s_WaterSettings.Rotation = glm::degrees(std::atan2(-sideX.z, sideX.x));
			s_WaterSettings.Size = glm::max(glm::vec2(glm::length(sideX), glm::length(sideZ)), glm::vec2(0.1f));
			CommitWater(before);
		}
		return;
	}

	const EnvMapVulkanEntityID selectedModel = GetSelectedModel();
	if (Scene::s_ImGuizmoType == -1 || selectedModel == NoEntity || s_ViewportImageSize.x <= 0.0f || s_ViewportImageSize.y <= 0.0f)
	{
		return;
	}
	const int selectedMesh = GetSelectedMeshIndex();

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist();
	// The scene image, not the whole window (which includes the tab bar)
	ImGuizmo::SetRect(s_ViewportImageMin.x, s_ViewportImageMin.y, s_ViewportImageSize.x, s_ViewportImageSize.y);

	// Snapping with Ctrl: 1 unit for translation/scale, 45 degrees for rotation
	bool snap = Input::IsKeyPressed(KeyH2M::LeftControl);
	float snapValue = Scene::s_ImGuizmoType == ImGuizmo::OPERATION::ROTATE ? 45.0f : 1.0f;
	float snapValues[3] = { snapValue, snapValue, snapValue };

	// A selected part of a multi-part model: the gizmo moves the part. It sits at the center of the part's bounding box
	// (as in SceneHazelEnvMap): imported parts often have their origin at the model's origin, far from the part, and would
	// turn around it. world = model * part * T(center); after the gizmo: part = model^-1 * world * T(-center)
	if (CanManipulateMesh(selectedModel, selectedMesh))
	{
		H2M::RefH2M<H2M::MeshH2M> part = GetModel(s_Scene, selectedModel)->GetMeshes()[selectedMesh];
		const glm::mat4 modelMatrix = s_Scene.GetWorldTransform(selectedModel);
		const glm::vec3 center = (part->BoundingBox.Min + part->BoundingBox.Max) * 0.5f;
		glm::mat4 world = modelMatrix * part->Transform * glm::translate(glm::mat4(1.0f), center);
		if (ImGuizmo::Manipulate(
			glm::value_ptr((*s_Camera).GetViewMatrix()),
			glm::value_ptr((*s_Camera).GetProjectionMatrix()),
			(ImGuizmo::OPERATION)Scene::s_ImGuizmoType,
			ImGuizmo::WORLD,
			glm::value_ptr(world),
			nullptr,
			snap ? snapValues : nullptr))
		{
			SetPartMatrix(s_EditorSelection.GetPrimary(), glm::inverse(modelMatrix) * world * glm::translate(glm::mat4(1.0f), -center));
		}
		return;
	}

	glm::mat4 transform = s_Scene.GetWorldTransform(selectedModel);
	if (ImGuizmo::Manipulate(
		glm::value_ptr((*s_Camera).GetViewMatrix()),
		glm::value_ptr((*s_Camera).GetProjectionMatrix()),
		(ImGuizmo::OPERATION)Scene::s_ImGuizmoType,
		ImGuizmo::WORLD,
		glm::value_ptr(transform),
		nullptr,
		snap ? snapValues : nullptr))
	{
		// Back into the values shown (and editable) in the Models and Meshes panel
		s_Scene.SetWorldTransform(selectedModel, transform);
	}
}

// Multi-selection: what the gizmo moves for a selected entity, as it does for the primary one (see
// ManipulatePrimarySelection): a model, a point or spot light, the water, a part the gizmo can move on its own, or else
// the part's model (a part of a single-mesh model, a rigged part). NoEntity: it doesn't move (the sun, the environment).
static EnvMapVulkanEntityID GetMovedEntity(EnvMapVulkanEntityID entity)
{
	switch (GetEntityKind(entity))
	{
		case EntityKind::Model:
		case EntityKind::PointLight:
		case EntityKind::SpotLight:
		case EntityKind::Water:
			return entity;
		case EntityKind::Part:
		{
			const EnvMapVulkanEntityID model = s_Scene.GetParent(entity);
			return CanManipulateMesh(model, (int)s_Scene.Get<MeshPartComponent>(entity).MeshIndex) ? entity : model;
		}
		default:
			return NoEntity;
	}
}

// The world matrix of a moved entity (a part's: its mesh matrix, which drawing uses, in its model)

static glm::mat4 GetMovableWorldMatrix(EnvMapVulkanEntityID entity)
{
	if (const MeshPartComponent* part = s_Scene.TryGet<MeshPartComponent>(entity))
	{
		const EnvMapVulkanEntityID model = s_Scene.GetParent(entity);
		return s_Scene.GetWorldTransform(model) * GetModel(s_Scene, model)->GetMeshes()[part->MeshIndex]->Transform;
	}
	return s_Scene.GetWorldTransform(entity);
}

static void SetMovableWorldMatrix(EnvMapVulkanEntityID entity, const glm::mat4& world)
{
	if (s_Scene.Has<MeshPartComponent>(entity))
	{
		SetPartMatrix(entity, glm::inverse(s_Scene.GetWorldTransform(s_Scene.GetParent(entity))) * world);
	}
	else
	{
		s_Scene.SetWorldTransform(entity, world);
	}
}

// A point or spot light moved with a multi-selection: it moves and turns with it, but isn't scaled: its Range is, by the
// change's uniform scale (the cube root of its volume change: a light's reach is the same in every direction)
static void MoveLightWith(EnvMapVulkanEntityID entity, const glm::mat4& delta)
{
	const glm::mat4 old = s_Scene.GetWorldTransform(entity);
	const glm::mat4 moved = delta * old;

	// Its axes without the change's scale (and the shear a non-uniform scale gives a turned light), the Y axis (the
	// light's direction) kept; then its own scale (normally 1)
	const glm::vec3 y = glm::normalize(glm::vec3(moved[1]));
	const glm::vec3 x = glm::normalize(glm::vec3(moved[0]) - glm::dot(glm::vec3(moved[0]), y) * y);
	const glm::vec3 z = glm::cross(x, y);
	glm::mat4 world = moved;
	world[0] = glm::vec4(x * glm::length(glm::vec3(old[0])), 0.0f);
	world[1] = glm::vec4(y * glm::length(glm::vec3(old[1])), 0.0f);
	world[2] = glm::vec4(z * glm::length(glm::vec3(old[2])), 0.0f);
	s_Scene.SetWorldTransform(entity, world);

	const float scale = std::cbrt(std::abs(glm::determinant(glm::mat3(delta))));
	float* range = GetLightRange(entity);
	if (range && std::abs(scale - 1.0f) > 1e-5f)
	{
		*range = glm::clamp(*range * scale, MinLightRange, MaxLightRange);
	}
}

// The water moved with a multi-selection, only as its own gizmo can move it: translated in X, Y and Z, turned only
// around Y (the part of the change that turns its X side around Y; it stays level), sized only along its own X and Z
// (its sides' new lengths; never in height)
static void MoveWaterWith(EnvMapVulkanEntityID entity, const glm::mat4& delta)
{
	const glm::mat4 moved = delta * s_Scene.GetWorldTransform(entity);
	const glm::vec3 sideX = glm::vec3(moved[0]);
	const glm::vec3 sideZ = glm::vec3(moved[2]);
	EnvMapVulkanWaterSettings& settings = s_Scene.Get<WaterComponent>(entity).Settings;
	settings.Center = glm::vec2(moved[3].x, moved[3].z);
	settings.Height = moved[3].y;
	if (glm::length(glm::vec2(sideX.x, sideX.z)) > 1e-4f)
	{
		settings.Rotation = glm::degrees(std::atan2(-sideX.z, sideX.x));
	}
	settings.Size = glm::max(glm::vec2(glm::length(sideX), glm::length(sideZ)), glm::vec2(0.1f));
	s_Scene.SetWorldTransform(entity, settings.GetTransform());
}

// An entity under another moved entity moves with that one (it isn't moved twice)
static bool HasMovedAncestor(EnvMapVulkanEntityID entity, const std::vector<EnvMapVulkanEntityID>& moved)
{
	for (EnvMapVulkanEntityID parent = s_Scene.GetParent(entity); parent != NoEntity; parent = s_Scene.GetParent(parent))
	{
		if (Contains(moved, parent))
		{
			return true;
		}
	}
	return false;
}

static void UpdateImGuizmo(Window* mainWindow)
{
	// BEGIN ImGuizmo

	// Gizmo mode: 1 translate, 2 rotate, 3 scale, 4 hide (not while typing into an ImGui text field)
	if (!ImGui::GetIO().WantTextInput)
	{
		if (Input::IsKeyPressed(KeyH2M::D1))
			Scene::s_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE;

		if (Input::IsKeyPressed(KeyH2M::D2))
			Scene::s_ImGuizmoType = ImGuizmo::OPERATION::ROTATE;

		if (Input::IsKeyPressed(KeyH2M::D3))
			Scene::s_ImGuizmoType = ImGuizmo::OPERATION::SCALE;

		if (Input::IsKeyPressed(KeyH2M::D4))
			Scene::s_ImGuizmoType = -1;
	}

	// The gizmo is on the primary selected entity. The other selected entities that can move go along: the change of the
	// primary entity's world matrix (a translation, a rotation around the gizmo, a scale) is applied to each of them.
	std::vector<EnvMapVulkanEntityID> moved; // what moves for the selected entities, once each
	for (EnvMapVulkanEntityID entity : s_EditorSelection.GetAll())
	{
		const EnvMapVulkanEntityID movedEntity = GetMovedEntity(entity);
		if (movedEntity != NoEntity && !Contains(moved, movedEntity))
		{
			moved.push_back(movedEntity);
		}
	}
	const EnvMapVulkanEntityID primary = GetMovedEntity(s_EditorSelection.GetPrimary());
	const bool moveOthers = moved.size() > 1 && primary != NoEntity;
	const glm::mat4 before = primary != NoEntity ? GetMovableWorldMatrix(primary) : glm::mat4(1.0f);
	s_LightGizmoScale = 1.0f; // set by the light gizmo when it scales a light's Range

	ManipulatePrimarySelection();

	if (primary == NoEntity || !s_Scene.Exists(primary))
	{
		return;
	}
	const glm::mat4 after = GetMovableWorldMatrix(primary);
	if (after == before && s_LightGizmoScale == 1.0f)
	{
		return;
	}
	glm::mat4 delta = after * glm::inverse(before);
	if (s_LightGizmoScale != 1.0f)
	{
		// The primary light's Range was scaled (its matrix doesn't change): the others scale around the light
		const glm::vec3 center = GetEntityPosition(s_Scene, primary);
		delta = glm::translate(glm::mat4(1.0f), center) * glm::scale(glm::mat4(1.0f), glm::vec3(s_LightGizmoScale)) *
			glm::translate(glm::mat4(1.0f), -center);
	}
	const float uniformScale = std::cbrt(std::abs(glm::determinant(glm::mat3(delta))));
	const bool scaled = std::abs(uniformScale - 1.0f) > 1e-5f;

	if (!moveOthers)
	{
		if (scaled && !GetLightRange(primary))
		{
			ScaleChildLightRanges(primary, uniformScale);
			ExtractLights();
		}
		return;
	}

	if (HasMovedAncestor(primary, moved))
	{
		SetMovableWorldMatrix(primary, before); // a moved parent of it moves it (below)
	}
	for (EnvMapVulkanEntityID entity : moved)
	{
		if (!s_Scene.Exists(entity) || HasMovedAncestor(entity, moved))
		{
			continue;
		}
		if (GetLightRange(entity))
		{
			if (entity != primary)
			{
				MoveLightWith(entity, delta);
			}
			continue;
		}
		if (s_Scene.Has<WaterComponent>(entity))
		{
			if (entity != primary)
			{
				MoveWaterWith(entity, delta);
			}
			continue;
		}
		if (entity != primary)
		{
			SetMovableWorldMatrix(entity, delta * GetMovableWorldMatrix(entity));
		}
		if (scaled)
		{
			ScaleChildLightRanges(entity, uniformScale);
		}
	}
	ExtractLights(); // moved lights
	ExtractWater();  // the moved water
}
