#pragma once

#include "EnvMapVulkanProbes.h"
#include "EnvMapVulkanWater.h"

#include "H2M/Core/RefH2M.h"

#include "Audio/AudioBus.h"

#include "Scripting/ScriptField.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>


namespace H2M
{
	class ModelH2M;
}
class EnvMapVulkanMaterial;

/**
 * The components of the SceneEnvMapVulkan scene (EnvMapVulkanScene): plain data, no logic beyond small helpers. Every
 * entity has an ID, a name, a place in the hierarchy and a transform; the other components say what it is.
 *
 * Conventions:
 * - Entities refer to each other by ID (EnvMapVulkanEntityID), never by registry handle or list index, so references
 *   survive removal, reordering and saving.
 * - Rotations are Euler angles in degrees, in ImGuizmo's convention (RecomposeMatrixFromComponents), so the values in
 *   the panels and the gizmo round-trip exactly.
 * - Lights shine along their transform's local -Y axis: with no rotation a spot light points straight down and the sun
 *   is overhead. A light's position is its transform's translation (for the sun, where its icon is drawn).
 */

// An entity's ID: unique, random, stable for the entity's life and across saving and loading. 0 is "no entity".
using EnvMapVulkanEntityID = uint64_t;
constexpr EnvMapVulkanEntityID NoEntity = 0;

struct IDComponent
{
	EnvMapVulkanEntityID ID = NoEntity;
};

struct NameComponent
{
	std::string Name;
};

// Words the game sorts entities by ("Pickup", "Enemy"): scripts ask HasTag and find entities by tag. Any number, each once
struct TagComponent
{
	std::vector<std::string> Tags;

	bool Has(const std::string& tag) const;
	void Add(const std::string& tag);
	void Remove(const std::string& tag);
	// "Pickup, Coin" (the Inspector's field) and back: trimmed, without empty or repeated ones
	std::string ToText() const;
	void FromText(const std::string& text);
};

// The entity's place in the tree. Children are kept in order (the order the hierarchy panel shows them in).
struct HierarchyComponent
{
	EnvMapVulkanEntityID Parent = NoEntity;
	std::vector<EnvMapVulkanEntityID> Children;
};

// Relative to the parent (to the world for an entity without a parent)
struct TransformComponent
{
	glm::vec3 Translation = glm::vec3(0.0f);
	glm::vec3 Rotation = glm::vec3(0.0f); // degrees (ImGuizmo's Euler convention)
	glm::vec3 Scale = glm::vec3(1.0f);

	glm::mat4 GetMatrix() const;
	void SetMatrix(const glm::mat4& matrix); // decomposed into translation, rotation and scale

	// The direction a light with this transform shines (its local -Y axis, normalized), and the rotation that turns it
	// toward a direction (no roll around it)
	glm::vec3 GetLightDirection() const;
	void SetLightDirection(const glm::vec3& direction);
};

// A model loaded from a file. Its parts (the meshes of the file) are child entities with a MeshPartComponent.
struct ModelComponent
{
	std::string FilePath;
	H2M::RefH2M<H2M::ModelH2M> Model; // the loaded model (each model entity has its own, so its parts can be edited)
};

// A part of a model: one of its meshes. Its TransformComponent is the part's place in the model (relative to the model
// entity, its parent).
struct MeshPartComponent
{
	uint32_t MeshIndex = 0;                            // in the model's meshes (shifts when other parts are removed)
	uint32_t SourceMeshIndex = 0;                      // in the model file's meshes (stable: saved scenes match parts by it)
	H2M::RefH2M<EnvMapVulkanMaterial> Material;        // from the Material Library (by ID in saved scenes, see Phase 4)
	glm::mat4 OriginalTransform = glm::mat4(1.0f);     // as loaded from the file (Reset Mesh)
	// Runtime: the transform last written into the model's mesh (the part's transform is written again only when it
	// differs, so an imported transform Euler angles can't reproduce exactly, e.g. a mirrored part, stays until edited)
	TransformComponent AppliedTransform;
};

// The sun: a directional light. It shines along its transform's -Y axis; the translation only places its icon.
struct SunComponent
{
	bool Enabled = true;
	glm::vec3 Color = glm::vec3(1.0f);
	float Intensity = 1.0f;
	bool FollowEnvironmentRotation = true; // turns with the environment map (a sun aligned to the HDR stays on its sun)
	bool CastShadows = true;
	bool IconPlaced = false;               // false: the icon is drawn in the sky, along the light; true: at the translation
};

struct PointLightComponent
{
	bool Enabled = true;
	glm::vec3 Color = glm::vec3(1.0f);
	float Intensity = 3.0f;
	float Range = 10.0f; // the light reaches exactly zero here
	bool CastShadows = false;
};

// Shines along its transform's -Y axis
struct SpotLightComponent
{
	bool Enabled = true;
	glm::vec3 Color = glm::vec3(1.0f);
	float Intensity = 10.0f;
	float Range = 10.0f;
	float InnerAngle = 20.0f; // half-angles of the cone (degrees): full intensity inside, fading to zero at OuterAngle
	float OuterAngle = 30.0f;
	bool CastShadows = false;
};

// The water plane (at most one per scene). Its placement is the entity's transform: translation = center and height,
// rotation around Y only (the other two are kept at 0), scale = size in meters (Y kept at 1). The settings' own Center,
// Height, Size and Rotation are filled from the transform before rendering.
struct WaterComponent
{
	EnvMapVulkanWaterSettings Settings;
};

// The probe volume (at most one per scene): a box with a grid of probes that hold the scene's indirect light (see
// EnvMapVulkanProbes). Its placement is the entity's transform: translation = the box's center, scale = its size in
// meters; it isn't turned (the rotation is kept at 0). The settings' own Center and Size are filled from the transform
// before rendering.
struct ProbeVolumeComponent
{
	EnvMapVulkanProbeVolumeSettings Settings;
};

// A game camera (docs/engine/Game_Engine_Runtime_Guide.html, 6.3): the game is seen through the primary one (the first
// with Primary). Its placement is the entity's world transform: translation = where it is, rotation = where it looks: x
// down (positive) and up (pitch), y around the vertical axis (0 looks along -Z, positive turns to the left); it isn't
// rolled. The editor has a view of its own (the scene file's EditorCamera), which Edit and Simulate show; Play shows this.
struct CameraComponent
{
	bool Primary = true;

	// The angle the view spans from its bottom edge to its top edge, in degrees. Small: a telephoto lens (flat, distant
	// things stay large); large: a wide-angle lens (strong perspective, stretched towards the edges).
	float FieldOfView = 45.0f;

	// Meters: nothing nearer or farther is drawn. A far plane far away from a near plane very near makes distant surfaces
	// flicker into each other (the depth buffer's precision)
	float NearClip = 0.01f;
	float FarClip = 1000.0f;

	// A render texture (7.5, a .mrtex file: absolute here, relative to the project in the scene file) the camera draws into
	// instead of the screen: a mirror, a monitor, a minimap. Empty: the screen. A camera with a target never shows the
	// game (Primary doesn't apply to it)
	std::string Target;
	// It draws every this many frames (1: every frame): a monitor needs fewer pictures than the game
	uint32_t DrawEvery = 1;
	// A camera with a target draws only while enabled (7.5.4): off, its texture keeps the last picture and the camera costs
	// nothing (a monitor nobody looks at, a closed menu's portrait). Enabled again, it draws at once. The screen's cameras
	// don't use it (Primary chooses the game's view)
	bool Enabled = true;

	static constexpr float MinFieldOfView = 10.0f;
	static constexpr float MaxFieldOfView = 120.0f;
	static constexpr float MinNearClip = 0.001f;
};

// The environment: the HDR map the scene is lit by and its skybox (one per scene)
struct EnvironmentComponent
{
	std::string FilePath;            // the .hdr map
	float Rotation = 0.0f;           // degrees around Y
	float Exposure = 1.0f;
	bool AutoExposure = true;
	float HuePreservation = 0.5f;    // tonemapping: 0 ACES per channel, 1 hue-preserving
	bool ExtractSun = true;          // a real sun in the map is taken out of the lighting and given to the sun entity
	float SkyboxLod = 0.0f;
};

// A test of the game's input (docs/engine/Game_Engine_Runtime_Guide.html, 2.3), until scripts do this (Phase 4): while the
// game runs, the entity moves with the Move action (as seen from the camera: forward is away from it), jumps with Jump and
// lands back at its height when the game started, and turns with Look. Without a CharacterControllerComponent it goes
// through walls; with one it asks the character for that velocity (the jump and the gravity its own), and physics stops it.
struct InputTestComponent
{
	float Speed = 4.0f;        // meters per second
	float JumpSpeed = 6.0f;    // meters per second, up, when it jumps
	float Gravity = 20.0f;     // meters per second squared
	float TurnSpeed = 1.0f;    // Look turns it: degrees per unit of Look (the mouse: 0.1 degrees per pixel by default)
	bool CaptureMouse = false; // the game captures the mouse (Escape gives it back in the editor)

	// Runtime: not saved
	bool Started = false;
	float GroundHeight = 0.0f;
	float VerticalSpeed = 0.0f;
};

// The shape the entity collides with (docs/engine/Game_Engine_Runtime_Guide.html, 3.4). With a RigidBodyComponent the
// entity is simulated as its Type says; without one it is static (a floor, a wall). In the entity's own space: the
// entity's world scale is applied when the game starts (Sphere and Capsule take the largest scale: they stay round).
struct ColliderComponent
{
	enum class Shape { Box, Sphere, Capsule, ConvexMesh, TriangleMesh };
	Shape Type = Shape::Box;
	glm::vec3 Offset = glm::vec3(0.0f);        // the shape's center (Box, Sphere, Capsule)
	glm::vec3 HalfExtents = glm::vec3(0.5f);   // Box
	float Radius = 0.5f;                       // Sphere, Capsule
	float HalfHeight = 0.5f;                   // Capsule: of its middle part (it stands along Y: 2 * (HalfHeight + Radius) tall)
	float Friction = 0.5f;
	float Restitution = 0.0f;                  // bounciness: 0 stays, 1 bounces back as high
	bool IsTrigger = false;                    // a sensor: things pass through it, and it reports what enters and leaves it

	// ConvexMesh: the model's meshes wrapped tightly (no holes or hollows), for moving bodies. TriangleMesh: the
	// meshes' triangles as they are, for static bodies only (a moving one gets the convex hull)
	static const char* GetShapeName(Shape shape);
};

// The entity moves with physics: Dynamic (falls, is pushed, bounces), Kinematic (moved by the game, pushes the dynamic
// ones and isn't pushed back) or Static. Needs a ColliderComponent on the same entity.
struct RigidBodyComponent
{
	enum class BodyType { Static, Dynamic, Kinematic };
	BodyType Type = BodyType::Dynamic;
	float Mass = 1.0f;                         // kilograms
	float LinearDamping = 0.05f;               // how fast moving and turning slow down by themselves (air)
	float AngularDamping = 0.05f;
	float GravityFactor = 1.0f;                // 0: floats
	bool LockRotationX = false, LockRotationY = false, LockRotationZ = false;
	bool ContinuousCollision = false;          // fast and small: never passes through a thin wall between two steps

	static const char* GetTypeName(BodyType type);
};

// The entity is a character (docs/engine/Game_Engine_Runtime_Guide.html, 3.5): a standing capsule the game moves by
// asking for a velocity. It stops at walls and slides along them, walks up slopes up to MaxSlopeDegrees and slides down
// steeper ones, climbs steps up to StepHeight, stands on what is below it (also when that moves) and pushes dynamic bodies.
// It doesn't turn: the entity's rotation stays the game's. Sizes in meters, in the world (the entity's scale doesn't
// change them). A Collider or Rigid Body on the same entity isn't used: the capsule is its shape.
struct CharacterControllerComponent
{
	float Height = 1.8f;                       // the capsule, top to bottom
	float Radius = 0.3f;
	glm::vec3 Offset = glm::vec3(0.0f);        // the capsule's center, from the entity's position
	float MaxSlopeDegrees = 45.0f;
	float StepHeight = 0.35f;
	float Mass = 70.0f;                        // kilograms: its weight presses on what it stands on
	float PushStrength = 100.0f;               // newtons: the most it pushes a dynamic body with
};

// A C# script on the entity (docs/engine/Game_Engine_Runtime_Guide.html, 4.5): a class of the project's game assembly that
// derives from Morava.Entity. While the game runs, an instance of it gets the entity's OnCreate, OnUpdate, OnFixedUpdate and
// collision calls (ScriptRuntime keeps the instances, not this component). Fields: the public fields set in the
// Inspector, given to the instance after it is made and before its OnCreate; the others keep the values the class gives them,
// so a changed default in the code reaches every entity that didn't set its own.
struct ScriptComponent
{
	struct Field
	{
		std::string Name;
		ScriptFieldValue Value;
	};
	std::string ClassName;     // with its namespace: "Sample.PlayerController"
	std::vector<Field> Fields; // in the order they were set

	const ScriptFieldValue* FindField(const std::string& name) const;
	void SetField(const std::string& name, const ScriptFieldValue& value);
	void ResetField(const std::string& name); // back to the class's own value
};

// A sound the entity plays while the game runs (docs/engine/Game_Engine_Runtime_Guide.html, 5.1), from where it is: louder
// near the listener and from its side (Spatial), or the same everywhere (music, a menu's sounds). SceneAudio plays it (this
// component holds the settings, not the playing sound). Clip: WAV, FLAC, MP3 or Ogg Vorbis, absolute here and relative to
// the project in the scene file. Stream: read while it plays, not loaded first (music: long, played once at a time)
struct AudioSourceComponent
{
	std::string Clip;
	AudioBus Bus = AudioBus::Effects;
	float Volume = 1.0f;                       // 0 to 1 (above 1 amplifies)
	float Pitch = 1.0f;                        // 2: an octave higher and twice as fast
	bool Loop = false;
	bool PlayOnStart = false;                  // when the game starts; else when a script plays it
	bool Stream = false;
	bool Spatial = true;
	float MinDistance = 1.0f;                  // meters: full volume up to here, then softer (inverse distance)
	float MaxDistance = 30.0f;                 // meters: no softer beyond
};

// Where the game is heard from: its position and the way it faces (-Z: forward, Y: up). Usually on the camera. Without one
// (or none Active), the primary game camera is the listener
struct AudioListenerComponent
{
	bool Active = true;
};

// The entity's skinned model is posed by an animator controller while the game runs (docs/engine/Game_Engine_Runtime_Guide.html,
// 6.4): its states, transitions and parameters (Animation/AnimatorController.h), played by SceneAnimation; scripts set the
// parameters (Animator.SetFloat...). Outside the game the model shows its own animation (the Inspector's Animation section)
struct AnimatorComponent
{
	std::string Controller; // the .manim file: absolute here, relative to the project in the scene file
};

// The root of a placed prefab (docs/engine/Game_Engine_Runtime_Guide.html, 6.1): the prefab file it came from, and what
// it was when it last matched it, to tell what changed since (EnvMapVulkanPrefab). The root's translation and rotation
// are the instance's own (where it is placed); the rest of the instance is the prefab's
struct PrefabComponent
{
	std::string File;            // absolute here, relative to the project in the scene file (an asset handle from Phase 8)
	uint64_t PrefabHash = 0;     // the prefab file's content when the instance last matched it
	uint64_t InstanceHash = 0;   // the instance's own content then (EnvMapVulkanPrefab::GetInstanceHash)
};

// ---- Game UI (docs/engine/Game_Engine_Runtime_Guide.html, 7.1): screen-space elements laid out by UI/UILayout and drawn
// over the finished image. Units are reference pixels (the canvas's ReferenceResolution), x to the right and y down, with
// (0, 0) at the top-left corner (as on the screen and for the mouse). A UI element is drawn only under a canvas.

// The root of a screen's UI: it covers the whole view. Its elements are laid out for ReferenceResolution and scaled to
// the real view: by its width (MatchWidthOrHeight 0), its height (1) or between them (0.5: a geometric mean, so a wider
// or a taller window changes the size equally). Canvases are drawn by SortOrder (higher: on top), equal ones by the
// scene's order
struct UICanvasComponent
{
	glm::vec2 ReferenceResolution = glm::vec2(1920.0f, 1080.0f);
	float MatchWidthOrHeight = 0.5f;
	int SortOrder = 0;
};

// Where an element is in its parent's rectangle (the canvas: the whole view). The anchors are points of the parent's
// rectangle, from (0, 0) top-left to (1, 1) bottom-right. Both at one point (AnchorMin == AnchorMax): the element keeps
// its Size and stays at Position from that point (a score in the top-left corner stays there in any window). Apart: the
// element stretches with the parent between them, and Size is added to that span (0: exactly the span; -20: 10 short of
// each edge). The pivot is the element's own point that Position places (0.5, 0.5: its center); it is also the point
// it grows from. Children are drawn over their parent, later siblings over earlier ones.
struct UIRectComponent
{
	glm::vec2 AnchorMin = glm::vec2(0.5f);
	glm::vec2 AnchorMax = glm::vec2(0.5f);
	glm::vec2 Pivot = glm::vec2(0.5f);
	glm::vec2 Position = glm::vec2(0.0f);
	glm::vec2 Size = glm::vec2(100.0f, 30.0f);
	bool Visible = true; // false hides the element and its children (a closed menu)
};

enum class UITextAlign { Left, Center, Right };
enum class UITextVerticalAlign { Top, Middle, Bottom };

// Text in the element's rectangle, in a font made into a signed distance field atlas (UI/UIFont): sharp at any size.
// Lines are broken at "\n", and with Wrap also between words where a line would leave the rectangle. Font: a .ttf or
// .otf file, absolute here and relative to the project in the scene file; empty: the engine's font (Open Sans)
struct UITextComponent
{
	std::string Text = "Text";
	std::string Font;
	float Size = 32.0f;                        // the font's em size, in reference pixels
	glm::vec4 Color = glm::vec4(1.0f);         // as it appears on the screen (not exposed or tonemapped)
	UITextAlign Align = UITextAlign::Center;
	UITextVerticalAlign VerticalAlign = UITextVerticalAlign::Middle;
	bool Wrap = false;
	float LineSpacing = 1.0f;                  // times the font's own line height

	static const char* GetAlignName(UITextAlign align);
	static const char* GetVerticalAlignName(UITextVerticalAlign align);
};

// A picture filling the element's rectangle, tinted by Color; without a texture a plain rectangle of that color (a
// panel, a health bar). Texture: absolute here and relative to the project in the scene file. Its colors are used as
// they are in the file: what the file shows is what the screen shows
struct UIImageComponent
{
	std::string Texture;
	glm::vec4 Color = glm::vec4(1.0f);
};

// A button (docs/engine/Game_Engine_Runtime_Guide.html, 7.3): its rectangle takes the mouse's clicks, and the keyboard and
// gamepad move between buttons (the actions UI_Navigate and UI_Submit). UI/SceneUI runs it while the game does. The
// element's own image is multiplied by the color of its state: Normal, Highlighted (under the mouse, or selected by the
// keyboard or the gamepad), Pressed, Disabled (not Interactable); the change fades over FadeDuration seconds of real time
// (a pause menu at time scale 0 still fades). A click calls OnClickMethod, a method without parameters, of the script of
// OnClickTarget; no target: of the first script that has the method, from the button up to its canvas. C# scripts can
// also listen to it: button.Clicked += ...
struct UIButtonComponent
{
	bool Interactable = true;
	glm::vec4 Normal = glm::vec4(0.85f, 0.85f, 0.85f, 1.0f);
	glm::vec4 Highlighted = glm::vec4(1.0f);
	glm::vec4 Pressed = glm::vec4(0.65f, 0.65f, 0.65f, 1.0f);
	glm::vec4 Disabled = glm::vec4(0.55f, 0.55f, 0.55f, 0.5f);
	float FadeDuration = 0.1f;
	EnvMapVulkanEntityID OnClickTarget = NoEntity;
	std::string OnClickMethod;

	// Runtime: not saved. The color the image is multiplied by now (SceneUI fades it; not set: the state's own)
	glm::vec4 Tint = glm::vec4(1.0f);
	bool TintSet = false;

	glm::vec4 GetTint() const { return TintSet ? Tint : (Interactable ? Normal : Disabled); }
};

// A slider (docs/engine/Game_Engine_Runtime_Guide.html, Milestone 2): a value between Min and Max, chosen along the
// element's width. Its own image is the track; the slider draws the fill (FillColor, from the left to the value) and the
// handle (HandleSize reference pixels, HandleColor times the color of its state, as a button's image) over it, the handle
// kept inside the rectangle. The mouse: a press on it moves the value there and drags it while held. Selected (as buttons
// are, by the mouse, the keyboard or the gamepad), UI_Navigate's left and right change the value by Step (held: it
// repeats), up and down move the selection. A change by the player calls OnValueChangedMethod, a method with one float
// parameter, of the script of OnValueChangedTarget (none: the first script that has it, from the slider up to its
// canvas), and C#'s slider.ValueChanged listeners; a value set by a script or the Inspector calls neither.
struct UISliderComponent
{
	float Value = 0.5f;
	float Min = 0.0f;
	float Max = 1.0f;
	bool WholeNumbers = false; // the value is rounded to whole numbers
	float Step = 0.0f;         // UI_Navigate's change per push; 0: a tenth of the range (at least 1 with WholeNumbers)
	bool Interactable = true;
	glm::vec4 FillColor = glm::vec4(0.16f, 0.42f, 0.78f, 1.0f);
	glm::vec4 HandleColor = glm::vec4(1.0f);
	glm::vec2 HandleSize = glm::vec2(28.0f, 44.0f); // reference pixels; centered on the track's height
	glm::vec4 Normal = glm::vec4(0.85f, 0.85f, 0.85f, 1.0f);
	glm::vec4 Highlighted = glm::vec4(1.0f);
	glm::vec4 Pressed = glm::vec4(0.65f, 0.65f, 0.65f, 1.0f);
	glm::vec4 Disabled = glm::vec4(0.55f, 0.55f, 0.55f, 0.5f);
	float FadeDuration = 0.1f;
	EnvMapVulkanEntityID OnValueChangedTarget = NoEntity;
	std::string OnValueChangedMethod;

	// Runtime: not saved. The color the handle is multiplied by now (SceneUI fades it; not set: the state's own)
	glm::vec4 Tint = glm::vec4(1.0f);
	bool TintSet = false;

	glm::vec4 GetTint() const { return TintSet ? Tint : (Interactable ? Normal : Disabled); }
	// The value as the slider keeps it: within Min and Max (either way round), rounded with WholeNumbers
	float Constrain(float value) const;
	// 0 at Min, 1 at Max
	float GetNormalized() const;
	float GetStep() const;
};

// A list of component types, for code that does the same for each (EnvMapVulkanScene::CopyFrom):
// ComponentList<A, B>::ForEach([](auto type) { using T = typename decltype(type)::Type; ... });
template<typename T>
struct ComponentType
{
	using Type = T;
};

template<typename... T>
struct ComponentList
{
	template<typename F>
	static void ForEach(F&& f)
	{
		(f(ComponentType<T>{}), ...);
	}
};

// The components that are scene data: every component above but IDComponent (the scene gives each entity its own). A new
// component is added here too (Phase 3 on: rigid bodies, scripts, sounds...), or a copied scene (Play) goes without it.
using SceneComponents = ComponentList<NameComponent, TagComponent, HierarchyComponent, TransformComponent, ModelComponent, MeshPartComponent,
	SunComponent, PointLightComponent, SpotLightComponent, WaterComponent, ProbeVolumeComponent, CameraComponent, EnvironmentComponent, InputTestComponent,
	ColliderComponent, RigidBodyComponent, CharacterControllerComponent, ScriptComponent, AudioSourceComponent, AudioListenerComponent, AnimatorComponent, PrefabComponent,
	UICanvasComponent, UIRectComponent, UITextComponent, UIImageComponent, UIButtonComponent, UISliderComponent>;
