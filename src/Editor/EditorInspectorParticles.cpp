// The Inspector's Particles section (docs/engine/Game_Engine_Runtime_Guide.html, 12.1): the entity's Particle Emitter and its
// settings (World/Particles.h). A selected emitter runs in the viewport (the renderer's preview: EnvMapVulkanParticles), so
// each change shows at once.


#include "Project/Project.h"

#include "Core/FileDialogs.h"
#include "Core/Util.h"

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <string>


#include "EditorState.h"


static bool IsImageFile(const std::filesystem::path& path)
{
	std::string extension = path.extension().u8string();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return extension == ".png" || extension == ".tga" || extension == ".jpg" || extension == ".jpeg";
}

static void Tooltip(const char* text)
{
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("%s", text);
	}
}

void TextureFileControls(std::string& texture, const char* noneText, const char* tooltip)
{
	ImGui::PushID("TextureFile");
	const std::string shown = texture.empty() ? std::string(noneText) : ProjectPaths::ToStored(texture);
	ImGui::SetNextItemWidth(-140.0f);
	ImGui::BeginDisabled(true);
	char buffer[512] = {};
	snprintf(buffer, sizeof(buffer), "%s", shown.c_str());
	ImGui::InputText("##Texture", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);
	ImGui::EndDisabled();
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
		{
			std::error_code error;
			const std::filesystem::path path = std::filesystem::absolute(std::filesystem::u8path(Util::to_str((const wchar_t*)payload->Data)), error);
			if (IsImageFile(path))
			{
				texture = path.lexically_normal().generic_u8string();
			}
			else
			{
				ShowNotification("A texture is a PNG, TGA or JPEG image", true);
			}
		}
		ImGui::EndDragDropTarget();
	}
	Tooltip(tooltip);
	ImGui::SameLine();
	if (ImGui::Button("..."))
	{
		const std::string file = FileDialogs::OpenFile({ { "Image", "png,tga,jpg,jpeg" } }, GetAssetFolderForDialog());
		if (!file.empty())
		{
			texture = std::filesystem::u8path(file).lexically_normal().generic_u8string();
		}
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(texture.empty());
	if (ImGui::Button("Clear"))
	{
		texture.clear();
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::TextDisabled("Texture");
	ImGui::PopID();
}

// A range of two values (min, max): the second never below the first
static void RangeControl(const char* label, float& min, float& max, float speed, float low, float high, const char* format, const char* tooltip)
{
	float values[2] = { min, max };
	if (ImGui::DragFloat2(label, values, speed, low, high, format))
	{
		min = std::clamp(values[0], low, high);
		max = std::clamp(std::max(values[1], min), low, high);
	}
	Tooltip(tooltip);
}

void ParticleControls(EnvMapVulkanEntityID entity)
{
	ImGui::Separator();
	ImGui::PushID("Particles");

	bool hasEmitter = s_Scene.Has<ParticleEmitterComponent>(entity);
	if (ImGui::Checkbox("Particle Emitter", &hasEmitter))
	{
		if (hasEmitter)
		{
			s_Scene.Add<ParticleEmitterComponent>(entity);
		}
		else
		{
			s_Scene.Remove<ParticleEmitterComponent>(entity);
		}
	}
	Tooltip("Particles that start here: smoke, dust, sparks. While the game runs every emitter emits; in Edit the selected\n"
		"ones do (and those under a selected entity), so the changes show at once. Scripts reach it with\n"
		"GetComponent<ParticleEmitter>() (Emitting, Rate, Emit, Clear).");
	ParticleEmitterComponent* emitter = s_Scene.TryGet<ParticleEmitterComponent>(entity);
	if (!emitter)
	{
		ImGui::PopID();
		return;
	}
	ParticleEmitterSettings& p = emitter->Settings;

	ImGui::Checkbox("Emitting", &p.Emitting);
	Tooltip("Starts particles at its rates (off: only a script's Emit starts some; those alive live on)");
	ImGui::SameLine();
	if (ImGui::Button("Burst"))
	{
		emitter->Burst += std::max(1u, p.MaxParticles / 4);
	}
	Tooltip("A quarter of Max Particles at once (a script's Emit)");
	ImGui::SameLine();
	if (ImGui::Button("Clear"))
	{
		emitter->ClearRequested = true;
	}
	Tooltip("The particles alive go (a script's Clear)");

	ImGui::DragFloat("Rate", &p.Rate, 0.5f, 0.0f, 10000.0f, "%.1f / s");
	Tooltip("Particles started a second");
	ImGui::DragFloat("Rate per Meter", &p.RatePerMeter, 0.1f, 0.0f, 1000.0f, "%.2f / m");
	Tooltip("Particles started for each meter the emitter moves (added to Rate): an even trail behind a moving one");
	int maxParticles = (int)p.MaxParticles;
	if (ImGui::DragInt("Max Particles", &maxParticles, 5.0f, 1, (int)ParticleEmitterSettings::MaxParticlesLimit))
	{
		p.MaxParticles = (uint32_t)std::clamp(maxParticles, 1, (int)ParticleEmitterSettings::MaxParticlesLimit);
	}
	Tooltip("Alive at once: a new particle takes the oldest one's place. About Rate x Lifetime is enough.\n"
		"A change starts every emitter's particles again (the pool is laid out anew).");

	RangeControl("Lifetime", p.LifetimeMin, p.LifetimeMax, 0.05f, 0.01f, 120.0f, "%.2f s", "Seconds a particle lives: random between these");
	RangeControl("Speed", p.SpeedMin, p.SpeedMax, 0.05f, 0.0f, 1000.0f, "%.2f m/s", "Its speed at the start: random between these");
	ImGui::DragFloat("Cone Angle", &p.ConeAngle, 0.5f, 0.0f, 180.0f, "%.1f deg");
	Tooltip("Degrees around the entity's up (+Y) it may start in: 0 straight along it, 180 any direction");
	ImGui::DragFloat("Start Radius", &p.StartRadius, 0.01f, 0.0f, 100.0f, "%.2f m");
	Tooltip("It starts anywhere within this distance of the emitter (0: at it)");
	ImGui::DragFloat("Inherit Velocity", &p.InheritVelocity, 0.01f, -2.0f, 2.0f, "%.2f");
	Tooltip("The share of the emitter's own velocity a particle starts with (0: none, 1: all of it)");
	ImGui::DragFloat3("Acceleration", &p.Acceleration.x, 0.05f, -100.0f, 100.0f, "%.2f");
	Tooltip("m/s² in the world: (0, -9.81, 0) is gravity; smoke rises with a small upward one; a sideways one is wind");
	ImGui::DragFloat("Drag", &p.Drag, 0.01f, 0.0f, 20.0f, "%.2f");
	Tooltip("The share of its speed it loses each second: smoke slows down soon after it starts");

	// Start and end, not a range: a particle may shrink
	float sizes[2] = { p.SizeStart, p.SizeEnd };
	if (ImGui::DragFloat2("Size", sizes, 0.01f, 0.0f, 100.0f, "%.2f m"))
	{
		p.SizeStart = std::clamp(sizes[0], 0.0f, 100.0f);
		p.SizeEnd = std::clamp(sizes[1], 0.0f, 100.0f);
	}
	Tooltip("Meters across at the start and at the end of its life");
	ImGui::ColorEdit4("Color Start", &p.ColorStart.x, ImGuiColorEditFlags_Float);
	Tooltip("Its color and alpha at the start (the alpha: how much it covers)");
	ImGui::ColorEdit4("Color End", &p.ColorEnd.x, ImGuiColorEditFlags_Float);
	Tooltip("At the end: alpha 0 fades it out");
	ImGui::DragFloat("Spin Speed", &p.SpinSpeed, 1.0f, 0.0f, 1440.0f, "%.0f deg/s");
	Tooltip("It turns at a random speed up to this, either way");

	TextureFileControls(p.Texture, "(none: a soft round puff)", "The particle's look: its alpha is the shape, its color tints the particle's.\n"
		"Drop an image from the Content Browser here, or choose one with ...");
	int blend = (int)p.Blend;
	const char* blendNames[] = { "Alpha", "Additive" };
	if (ImGui::Combo("Blend", &blend, blendNames, 2))
	{
		p.Blend = (ParticleEmitterSettings::BlendMode)blend;
	}
	Tooltip("Alpha: over the scene by its alpha (smoke, dust). Additive: added to it, brightening (fire, sparks)");
	ImGui::Checkbox("Lit", &p.Lit);
	Tooltip("Shaded by the sun (through its shadow) and the ambient light; off: its color as it is (glowing)");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(120.0f);
	ImGui::DragFloat("Soft Distance", &p.SoftDistance, 0.01f, 0.0f, 10.0f, "%.2f m");
	Tooltip("Meters over which it fades where it meets a surface: no hard edge where smoke crosses the road (0: hard)");
	ImGui::DragFloat("Stretch", &p.Stretch, 0.001f, 0.0f, 1.0f, p.Stretch > 0.0f ? "%.3f s" : "none");
	Tooltip("Seconds of its motion a particle is drawn along: a streak from where it was that long ago to where it is,\n"
		"its size across (tracers, sparks). 0: a square facing the camera");
	ImGui::Checkbox("View Model", &p.ViewModel);
	Tooltip("Drawn as the first person's arms and gun are: with the view models' field of view, in front of every wall\n"
		"(a muzzle flash at the first-person gun). Without the game's camera, as any emitter");

	ImGui::PopID();
}
