#pragma once

#include "PhysicsLayers.h"
#include "PhysicsWorld.h"

#include "EnvMapVulkan/EnvMapVulkanComponents.h"

#include <Jolt/Physics/Character/CharacterVirtual.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>
#include <unordered_map>
#include <vector>


class EnvMapVulkanScene;
class ScenePhysicsContactListener;

/**
 * The physics of a running game's scene (docs/engine/Game_Engine_Runtime_Guide.html, 3.3 and 3.4): a PhysicsWorld with a
 * body for every entity that has a ColliderComponent, made when the game starts and gone when it stops. The bodies'
 * shapes are fixed at the start, with the entities' world scale in them.
 *
 * The scene stays the place where entities are. Before each step the kinematic bodies are moved to their entities, and a
 * body whose entity was moved by something else (the game, or the editor while paused) is put there. After the steps the
 * dynamic bodies' places are written into their entities' transforms, between their last two steps (WriteTransforms), so
 * they move smoothly whatever the frame rate.
 *
 * Characters (CharacterControllerComponent, 3.5) are Jolt CharacterVirtuals: moved in each step by the velocity the game
 * asked for (SetCharacterVelocity), stopped and guided by what they touch, and written back like the dynamic bodies (their
 * position only: the entity's rotation stays the game's). Each has an inner body, so falling boxes and other characters
 * collide with it.
 *
 * Events and queries (3.6): bodies that start and stop touching, and what enters and leaves a trigger, are reported once
 * each after the step they happen in (GetEvents); Raycast, SphereCast and the Overlap functions ask what is where, between
 * the steps. GetDebugLines draws the shapes and the contacts for the editor's debug views.
 */
class ScenePhysics
{
public:
	ScenePhysics();
	~ScenePhysics();
	ScenePhysics(const ScenePhysics&) = delete;
	ScenePhysics& operator=(const ScenePhysics&) = delete;

	// The bodies of the scene's entities; collisions: the layers that collide (the project's)
	void Start(EnvMapVulkanScene& scene, const PhysicsCollisionMatrix& collisions = PhysicsCollisionMatrix());
	void Stop();
	bool IsRunning() const { return m_World != nullptr; }

	void Step(float fixedStep);
	// The dynamic bodies' entities where the bodies are, alpha of the way from their previous step to their last one
	void WriteTransforms(float alpha);

	PhysicsWorld* GetWorld() { return m_World.get(); }

	// Characters. The game sets the velocity it wants before the steps (meters per second, in the world: the horizontal
	// from its input, the vertical from its jumps and its gravity: the character applies none of its own); the character
	// keeps it until the game sets another. The state is the one after the last step: Velocity is how it really moved
	// (zero towards a wall it ran into, zero downward when it landed).
	enum class CharacterGround { InAir, OnGround, OnSteepGround, NotSupported };
	struct CharacterState
	{
		CharacterGround Ground = CharacterGround::InAir; // OnSteepGround: too steep to stand on, it slides down
		glm::vec3 Velocity = glm::vec3(0.0f);
		glm::vec3 GroundNormal = glm::vec3(0.0f, 1.0f, 0.0f);
	};
	void SetCharacterVelocity(EnvMapVulkanEntityID entity, const glm::vec3& velocity);
	// False: the entity isn't a running character
	bool GetCharacterState(EnvMapVulkanEntityID entity, CharacterState& state) const;
	static const char* GetGroundName(CharacterGround ground);

	// ---- Events (3.6)

	// Bodies that started or stopped touching (Collision), or a body that entered or left a trigger (Trigger: Entity is the
	// trigger, Other what came in or went out). A character is reported with its inner body: when something dynamic bumps
	// into it, and when it enters a trigger. Point and Normal (from Entity towards Other) are where they first touched, zero
	// for an Exit.
	enum class EventType { CollisionEnter, CollisionExit, TriggerEnter, TriggerExit };
	struct Event
	{
		EventType Type = EventType::CollisionEnter;
		EnvMapVulkanEntityID Entity = NoEntity, Other = NoEntity;
		glm::vec3 Point = glm::vec3(0.0f), Normal = glm::vec3(0.0f);
		uint64_t Step = 0; // the step it happened in (the first is 1)
	};
	// The last step's events (Phase 4: the scripts get them), and the last ones of all steps, for the Physics panel
	const std::vector<Event>& GetEvents() const { return m_Events; }
	const std::vector<Event>& GetRecentEvents() const { return m_RecentEvents; } // the newest last
	static const char* GetEventName(EventType type);

	// ---- Queries (3.6), between the steps: where the bodies are after the last one

	// What a ray or a moving sphere hit first: its entity (a character's for its inner body), the point, the surface's normal
	// there (towards the ray) and how far along the ray
	struct Hit
	{
		EnvMapVulkanEntityID Entity = NoEntity;
		glm::vec3 Point = glm::vec3(0.0f), Normal = glm::vec3(0.0f);
		float Distance = 0.0f;
	};
	// layers: the ones it can hit (ObjectLayers::StaticBit | ...; Solid: all but triggers). ignore: an entity it goes through
	// (the one asking, so a ray from inside a character doesn't hit the character). False: nothing within maxDistance
	bool Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, Hit& hit, uint32_t layers = ObjectLayers::Solid,
		EnvMapVulkanEntityID ignore = NoEntity) const;
	// A sphere moved from origin along direction: the first thing it touches (Point: where; Distance: how far its center went)
	bool SphereCast(const glm::vec3& origin, float radius, const glm::vec3& direction, float maxDistance, Hit& hit,
		uint32_t layers = ObjectLayers::Solid, EnvMapVulkanEntityID ignore = NoEntity) const;
	// The entities whose shapes are in a sphere or a box (turned by rotation), each once
	std::vector<EnvMapVulkanEntityID> OverlapSphere(const glm::vec3& center, float radius, uint32_t layers = ObjectLayers::Solid) const;
	std::vector<EnvMapVulkanEntityID> OverlapBox(const glm::vec3& center, const glm::vec3& halfExtents, const glm::quat& rotation,
		uint32_t layers = ObjectLayers::Solid) const;

	// ---- Debug views and statistics (3.6)

	// A line of a debug view, in the world. Kind is what it shows (the editor picks the colors)
	enum class DebugLineKind { Static, Active, Sleeping, Trigger, Character, Contact };
	struct DebugLine
	{
		glm::vec3 A, B;
		DebugLineKind Kind;
	};
	// The running world's shapes (where they are drawn: the dynamic bodies between their last two steps, as WriteTransforms
	// put them) and the last step's contacts (a cross at each point, a line along its normal). Mesh shapes stop after
	// maxTriangles triangles in all (a large model's would be more lines than they show)
	void GetDebugLines(bool colliders, bool contacts, std::vector<DebugLine>& lines, uint32_t maxTriangles = 5000) const;
	// The colliders of a scene that isn't running (the editor's), from its components, as Start would make them: Box, Sphere,
	// Capsule and the characters' capsules (mesh shapes are made at Start, so they show only while the game runs)
	static void GetColliderLines(const EnvMapVulkanScene& scene, std::vector<DebugLine>& lines);
	// Contacts are kept for GetDebugLines only while asked for (they cost a little in every step)
	void SetCollectContacts(bool collect);

	struct Stats
	{
		uint32_t Static = 0, Kinematic = 0, Dynamic = 0, Triggers = 0, Characters = 0;
		uint32_t Awake = 0;          // bodies (characters' inner bodies too) that aren't sleeping
		uint32_t TouchingPairs = 0;  // pairs of bodies that touch (sleeping ones too)
		uint64_t Steps = 0;
		float LastStepMs = 0.0f, AverageStepMs = 0.0f;
	};
	Stats GetStats() const;

	// A collider that fits around a model entity's meshes, in the entity's own space (without its scale), for the shape it
	// has: its size and center (ConvexMesh and TriangleMesh are the meshes themselves: nothing to fit). False: the entity
	// has no model, or a model without meshes.
	static bool FitCollider(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, ColliderComponent& collider);
	// A character's capsule that fits around a model entity's meshes, at its scale: as tall as the model, as wide as its
	// wider side (X or Z), around its center
	static bool FitCharacter(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, CharacterControllerComponent& character);

private:
	struct Body
	{
		EnvMapVulkanEntityID Entity = NoEntity;
		JPH::BodyID ID;
		JPH::EMotionType Motion = JPH::EMotionType::Static;
		glm::vec3 Scale = glm::vec3(1.0f);         // the entity's world scale at the start (in the shape)
		glm::vec3 PreviousPosition = glm::vec3(0.0f), Position = glm::vec3(0.0f);
		glm::quat PreviousRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		TransformComponent Written;                // the entity's transform as physics last left it: anything else moved it
		bool Trigger = false;
	};

	struct Character
	{
		EnvMapVulkanEntityID Entity = NoEntity;
		JPH::Ref<JPH::CharacterVirtual> Controller;
		float StepHeight = 0.0f;
		glm::vec3 WantedVelocity = glm::vec3(0.0f);
		glm::vec3 PreviousPosition = glm::vec3(0.0f), Position = glm::vec3(0.0f);
		glm::vec3 Written = glm::vec3(0.0f); // the entity's translation as physics last left it
	};

	// The body of body.Entity, added to the world (false: its shape could not be made, or the world is full)
	bool CreateBody(Body& body, const ColliderComponent& collider, const RigidBodyComponent* rigidBody);
	void CreateCharacter(Character& character, const CharacterControllerComponent& settings);
	void SetEntityPosition(EnvMapVulkanEntityID entity, const glm::vec3& position); // the translation only
	// The contacts the listener collected in the step, as events
	void DispatchContacts();
	// The entity of a body (a character's for its inner body; NoEntity: none of the scene's)
	EnvMapVulkanEntityID GetBodyEntity(JPH::BodyID body) const;
	// The bodies of an entity (a character's inner body), for a query to go through them
	std::vector<JPH::BodyID> GetEntityBodies(EnvMapVulkanEntityID entity) const;
	// The entities whose bodies a shape overlaps (at transform: its center of mass), each once
	std::vector<EnvMapVulkanEntityID> Overlap(const JPH::Shape& shape, const JPH::RMat44& transform, uint32_t layers) const;

	EnvMapVulkanScene* m_Scene = nullptr;
	std::unique_ptr<PhysicsWorld> m_World;
	std::vector<Body> m_Bodies;
	std::vector<Character> m_Characters;

	// What each body is: its entity, a trigger or not (by the body ID's index and sequence number)
	struct BodyInfo
	{
		EnvMapVulkanEntityID Entity = NoEntity;
		bool Trigger = false;
	};
	std::unordered_map<uint32_t, BodyInfo> m_BodyInfo;

	// Events: the listener collects a step's contacts on Jolt's threads; how many sub-shape contacts each pair of bodies has
	// (a pair touches while it has any: a mesh touches with each of its triangles)
	std::unique_ptr<ScenePhysicsContactListener> m_Listener;
	std::unordered_map<uint64_t, int> m_Touching;
	std::vector<Event> m_Events, m_RecentEvents;
	bool m_CollectContacts = false;
	float m_Alpha = 1.0f; // WriteTransforms' last

	// The steps' cost, logged at Stop
	uint64_t m_StepCount = 0;
	double m_StepSeconds = 0.0;
	float m_LastStepMs = 0.0f;
};
