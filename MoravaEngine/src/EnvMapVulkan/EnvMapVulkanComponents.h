#pragma once

#include "EnvMapVulkanProbes.h"
#include "EnvMapVulkanWater.h"

#include "H2M/Core/RefH2M.h"

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
	// "Pickup, Coin" (the Properties panel's field) and back: trimmed, without empty or repeated ones
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

// The camera the scene is seen through in the editor (one per scene). Its placement is the entity's transform:
// translation = where it is, rotation = where it looks: x down (positive) and up (pitch), y around the vertical axis (0 looks along
// -Z, positive turns to the left). It isn't rolled or scaled. Saved with the scene, so an opened scene is seen as it was.
struct CameraComponent
{
	// The angle the view spans from its bottom edge to its top edge, in degrees. Small: a telephoto lens (flat, distant
	// things stay large); large: a wide-angle lens (strong perspective, stretched towards the edges).
	float FieldOfView = 45.0f;

	static constexpr float MinFieldOfView = 10.0f;
	static constexpr float MaxFieldOfView = 120.0f;
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
// collision calls (ScriptRuntime keeps the instances, not this component). Fields: the public fields set in the Properties
// panel, given to the instance after it is made and before its OnCreate; the others keep the values the class gives them,
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
	ColliderComponent, RigidBodyComponent, CharacterControllerComponent, ScriptComponent>;
