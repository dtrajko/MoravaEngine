// The Inspector's Trail section (docs/engine/Game_Engine_Runtime_Guide.html, 12.2): the entity's Trail and its settings
// (World/Trails.h). A selected trail leaves marks in the viewport while it is moved (the renderer's preview:
// EnvMapVulkanTrails), so each change shows at once.


#include "imgui.h"

#include <algorithm>
#include <string>


#include "EditorState.h"


static void Tooltip(const char* text)
{
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("%s", text);
	}
}

void TrailControls(EnvMapVulkanEntityID entity)
{
	ImGui::Separator();
	ImGui::PushID("Trail");

	bool hasTrail = s_Scene.Has<TrailComponent>(entity);
	if (ImGui::Checkbox("Trail", &hasTrail))
	{
		if (hasTrail)
		{
			s_Scene.Add<TrailComponent>(entity);
		}
		else
		{
			s_Scene.Remove<TrailComponent>(entity);
		}
	}
	Tooltip("Marks left behind as the entity moves: skid marks. A ribbon on the surface under it, across its up (a script\n"
		"puts the entity where a wheel touches the road). While the game runs every trail leaves marks; in Edit the selected\n"
		"ones do as you move them. Scripts reach it with GetComponent<Trail>() (Emitting, Width, Color, Clear).");
	TrailComponent* trail = s_Scene.TryGet<TrailComponent>(entity);
	if (!trail)
	{
		ImGui::PopID();
		return;
	}
	TrailSettings& p = trail->Settings;

	ImGui::Checkbox("Emitting", &p.Emitting);
	Tooltip("Leaves marks as it moves (off: the ribbon ends; on again starts a new one)");
	ImGui::SameLine();
	if (ImGui::Button("Clear"))
	{
		trail->ClearRequested = true;
	}
	Tooltip("Its marks go (a script's Clear)");
	ImGui::SameLine();
	ImGui::TextDisabled("%u points", trail->Points.GetCount());

	ImGui::DragFloat("Width", &p.Width, 0.005f, 0.001f, 100.0f, "%.3f m");
	Tooltip("Meters across the ribbon");
	ImGui::DragFloat("Min Distance", &p.MinDistance, 0.005f, 0.01f, 100.0f, "%.3f m");
	Tooltip("Meters between two points: smaller follows curves closer, with more points");
	int maxPoints = (int)p.MaxPoints;
	if (ImGui::DragInt("Max Points", &maxPoints, 5.0f, 2, (int)TrailSettings::MaxPointsLimit))
	{
		p.MaxPoints = (uint32_t)std::clamp(maxPoints, 2, (int)TrailSettings::MaxPointsLimit);
	}
	Tooltip("Points kept at once: the oldest marks make room for new ones (fading out over the last tenth).\n"
		"Max Points x Min Distance is how far back the marks reach. A change clears the trail.");
	ImGui::DragFloat("Lifetime", &p.Lifetime, 0.1f, 0.0f, 3600.0f, p.Lifetime > 0.0f ? "%.1f s" : "none");
	Tooltip("Seconds a mark stays, fading over the second half (0: until Max Points makes it go)");
	ImGui::ColorEdit4("Color", &p.Color.x, ImGuiColorEditFlags_Float);
	Tooltip("The mark's color, and how much it covers (alpha). It darkens the surface under it, which keeps its light:\n"
		"the sun, its shadows, the lamps. Black at alpha 1: a black mark; white or alpha 0: none");
	TextureFileControls(p.Texture, "(none: a plain band with soft edges)", "The mark's look along the ribbon (tread, streaks): its color times the mark's,\n"
		"its alpha the shape across. Drop an image from the Content Browser here, or choose one with ...");
	ImGui::BeginDisabled(p.Texture.empty());
	ImGui::DragFloat("Tile Length", &p.TileLength, 0.01f, 0.01f, 1000.0f, "%.2f m");
	Tooltip("Meters along the ribbon the texture covers once");
	ImGui::EndDisabled();

	ImGui::PopID();
}
