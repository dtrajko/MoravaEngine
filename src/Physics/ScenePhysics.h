#pragma once

#include "PhysicsLayers.h"
#include "PhysicsWorld.h"

#include "EnvMapVulkan/EnvMapVulkanComponents.h"

#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Vehicle/VehicleCollisionTester.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>
#include <unordered_map>
#include <vector>


class EnvMapVulkanScene;
class ScenePhysicsContactListener;

/**
 * The physics of a running game's scene (docs/engine/Game_Engine_Runtime_Guide.html, 3.3 and 3.4): a PhysicsWorld with a
 * body for every entity that has a ColliderComponent, and a static height field for every terrain (TerrainComponent), made
 * when the game starts and gone when it stops. The bodies'
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
 * Vehicles (VehicleComponent, the vehicle scene's part B) are dynamic bodies with a Jolt VehicleConstraint: wheels on springs
 * cast down from the body, driven by an engine through an automatic gearbox (or a manual one the game shifts). The game gives each its driver's input
 * (SetVehicleInput: the player's actions, or a script's); the wheels' models are placed where the wheels are, as the
 * bodies are written (WriteTransforms).
 *
 * Events and queries (3.6): bodies that start and stop touching, and what enters and leaves a trigger, are reported once
 * each after the step they happen in (GetEvents); Raycast, SphereCast and the Overlap functions ask what is where, between
 * the steps. GetDebugLines draws the shapes and the contacts for the editor's debug views.
 *
 * An entity destroyed while the game runs takes its body with it at the next step (Phase 4.4: scripts destroy entities);
 * entities placed while it runs (a prefab, Scene.Instantiate: 6.2) get theirs at once (AddEntities).
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
	// Only the static colliders (an entity with a Collider and no Rigid Body; not the terrains, characters or vehicles), for
	// asking what stands where while no game runs: the queries below, never Step (the foliage's keep-out: World/Foliage.h)
	void StartStatic(EnvMapVulkanScene& scene);
	void Stop();
	bool IsRunning() const { return m_World != nullptr; }
	// Entities new in the running game: their bodies and characters, where the entities are now
	void AddEntities(const std::vector<EnvMapVulkanEntityID>& entities);

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
		glm::vec3 GroundVelocity = glm::vec3(0.0f); // of what it stands on (a moving platform), in the world
		glm::vec3 GroundAngularVelocity = glm::vec3(0.0f); // radians per second, of what it stands on (a turning platform)
	};
	void SetCharacterVelocity(EnvMapVulkanEntityID entity, const glm::vec3& velocity);
	// False: the entity isn't a running character
	bool GetCharacterState(EnvMapVulkanEntityID entity, CharacterState& state) const;
	static const char* GetGroundName(CharacterGround ground);

	// ---- Rigid bodies (Phase 4: the scripts' Rigidbody): a dynamic body's forces and velocities, between the steps. False:
	// the entity has no dynamic body (a kinematic one follows its entity, a static one doesn't move)

	// Force: newtons (torque: newton meters), for the next step; Impulse: at once (newton seconds), with the body's mass.
	// Acceleration and VelocityChange: the same without the mass (an acceleration over the next step, a speed added at once)
	enum class ForceMode { Force, Acceleration, Impulse, VelocityChange };
	bool AddForce(EnvMapVulkanEntityID entity, const glm::vec3& force, ForceMode mode);
	bool AddTorque(EnvMapVulkanEntityID entity, const glm::vec3& torque, ForceMode mode);
	// Meters per second in the world; radians per second around the world's axes
	bool GetVelocity(EnvMapVulkanEntityID entity, glm::vec3& linear, glm::vec3& angular) const;
	bool SetLinearVelocity(EnvMapVulkanEntityID entity, const glm::vec3& velocity);
	bool SetAngularVelocity(EnvMapVulkanEntityID entity, const glm::vec3& velocity);

	// ---- Vehicles: the driver's input, kept until the game sets another. Forward: -1 (full reverse) to 1 (full throttle);
	// asking for the other way while it still rolls brakes first. Steer: -1 (left) to 1 (right). Brake and Handbrake: 0 to 1.
	// False: the entity isn't a running vehicle
	struct VehicleInput
	{
		float Forward = 0.0f, Steer = 0.0f, Brake = 0.0f, Handbrake = 0.0f;
	};
	bool SetVehicleInput(EnvMapVulkanEntityID entity, const VehicleInput& input);
	// The tires' sideways grip, front and rear, times the component's (FrontGrip, RearGrip): a drift lowers the rear's for a
	// while. 1, 1: as the component has them. False: not a running vehicle
	bool SetVehicleGrip(EnvMapVulkanEntityID entity, float front, float rear);
	// The engine's torque times the component's (EngineTorque): the whole torque curve scaled, the engine braking as it is.
	// 1: as the component has it. False: not a running vehicle
	bool SetVehicleEngineTorque(EnvMapVulkanEntityID entity, float times);
	// The gearbox: automatic (Jolt shifts by the rpm: the default), or manual: the game shifts (ShiftVehicleGear), and the
	// clutch works by itself: out while it shifts, then in over Jolt's clutch release time, and out when the engine would
	// turn slower than Jolt's Shift Down RPM without throttle (in gear it would creep at idle, where the automatic goes to
	// neutral). The throttle from neutral or reverse still puts it in first, reversing in reverse, as the automatic does.
	// False: not a running vehicle
	bool SetVehicleManual(EnvMapVulkanEntityID entity, bool manual);
	// One gear up (direction 1) or down (-1) in a manual gearbox. Refused: an automatic one, no gear above (top) or below
	// (first: the brake reverses once stopped), or a downshift that would turn the engine more than DownshiftToleranceRpm
	// past its limit (Max RPM plus Over-Rev RPM; within that, the engine stays at the limit and the clutch drags the wheels
	// down to it)
	static constexpr float DownshiftToleranceRpm = 1000.0f;
	enum class ShiftResult { NotVehicle, Shifted, Refused };
	ShiftResult ShiftVehicleGear(EnvMapVulkanEntityID entity, int direction);
	// Straight into a gear (1, 2... forward, -1 reverse, 0 neutral; clamped to the gearbox's), with no check: a car put at
	// the start or respawned, standing still. The automatic shifts on from there by itself. False: not a running vehicle
	bool SetVehicleGear(EnvMapVulkanEntityID entity, int gear);
	struct VehicleState
	{
		float Speed = 0.0f;         // meters per second along its forward (negative: backward)
		float Rpm = 0.0f;           // the engine's revolutions per minute
		int Gear = 0;               // 1, 2... forward, -1 reverse, 0 neutral
		uint32_t Wheels = 0;
		uint32_t WheelsOnGround = 0; // a bit per wheel: it touches something
		VehicleInput Input;         // as set by the game
		VehicleInput Applied;       // as given to the wheels in the last step (reversing: brakes first)
		float LateralSlip = 0.0f;   // degrees: the most a wheel on the ground slides sideways (the angle between where it
		                            // points and where it goes): a few in a turn, 10 and more in a drift
		float LongitudinalSlip = 0.0f; // the most a wheel on the ground spins or locks: 0 rolling, 1 locked or spinning twice as fast
		float RevLimit = 0.0f;      // the most the engine turns: Max RPM + Over-Rev RPM (a rev counter's end)
		bool Manual = false;        // the gearbox is manual (SetVehicleManual)
		int Gears = 0;              // forward gears
	};
	bool GetVehicleState(EnvMapVulkanEntityID entity, VehicleState& state) const;

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
	// The last step's events (the scripts get them: ScriptRuntime), and the last ones of all steps, for the Physics panel
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
	// Anything in an upright capsule from bottom up to top (the centers of its end spheres), radius wide
	bool OverlapsCapsule(const glm::vec3& bottom, const glm::vec3& top, float radius, uint32_t layers = ObjectLayers::Solid) const;

	// ---- Debug views and statistics (3.6)

	// A line of a debug view, in the world. Kind is what it shows (the editor picks the colors)
	enum class DebugLineKind { Static, Active, Sleeping, Trigger, Character, Contact, Wheel, WheelOnGround };
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
	// Capsule, the characters' capsules and the vehicles' wheels (mesh shapes are made at Start, so they show only while the
	// game runs)
	static void GetColliderLines(const EnvMapVulkanScene& scene, std::vector<DebugLine>& lines);
	// Contacts are kept for GetDebugLines only while asked for (they cost a little in every step)
	void SetCollectContacts(bool collect);

	struct Stats
	{
		uint32_t Static = 0, Kinematic = 0, Dynamic = 0, Triggers = 0, Characters = 0, Vehicles = 0;
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
	// A vehicle's wheels from its children named "Wheel..." (and their children) that have a model: each one's center, radius
	// and width from its meshes (the wheel turns around the model's axis closest to the vehicle's X), steering when in the
	// front half, the handbrake on the back ones, all driven. False: none found (the wheels stay as they were)
	static bool FitWheels(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, VehicleComponent& vehicle);

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
		glm::vec3 Moved = glm::vec3(0.0f);  // how it really moved in the last step, per second (CharacterState::Velocity)
		glm::vec3 Written = glm::vec3(0.0f); // the entity's translation as physics last left it
	};

	// The body or the character of an entity, as its components ask (counted by kind, for the log)
	struct AddCounts
	{
		uint32_t Motions[3] = {}; // by JPH::EMotionType: static, kinematic, dynamic (the triggers apart)
		uint32_t Triggers = 0, Failed = 0;
	};
	void AddEntity(EnvMapVulkanEntityID entity, AddCounts& counts);
	// The body of body.Entity, added to the world (false: its shape could not be made, or the world is full)
	bool CreateBody(Body& body, const ColliderComponent& collider, const RigidBodyComponent* rigidBody);
	// A terrain's static height field (its heights made first, when the renderer hasn't yet)
	bool CreateTerrainBody(Body& body, TerrainComponent& terrain);
	void CreateCharacter(Character& character, const CharacterControllerComponent& settings);

	struct Vehicle
	{
		EnvMapVulkanEntityID Entity = NoEntity;
		JPH::BodyID Body;
		JPH::Ref<JPH::VehicleConstraint> Constraint;
		JPH::Ref<JPH::VehicleCollisionTester> Tester;
		VehicleInput Input, Applied;
		float Direction = 1.0f; // the way it was last asked to go: asking for the other one brakes until it stops
		// The tires' sideways grip: each wheel's multiplier, read by the tire callback in Jolt's step (shared: the callback
		// keeps it while the vehicle moves in m_Vehicles). The component's front and rear grip, times the script's
		std::shared_ptr<std::vector<float>> WheelGrip;
		std::vector<bool> FrontWheel;
		float FrontGrip = 1.0f, RearGrip = 1.0f;
		// A manual gearbox (SetVehicleManual): seconds left with the clutch out for a shift, then letting it in
		bool Manual = false;
		float ShiftTimeLeft = 0.0f, ClutchReleaseLeft = 0.0f;
		// The engine's rpm per meter per second of the body's speed, per unit of gear ratio (the final drive and the driven
		// wheels' radius): a downshift's check also when the wheels lock or spin
		float RpmPerSpeed = 0.0f;
		float EngineBrakeTorque = 0.0f; // newton meters at the engine at Max RPM (VehicleComponent::EngineBraking)
		float EngineTorque = 0.0f;      // the component's Engine Torque, SetVehicleEngineTorque's 1
		// The component's Max RPM: the rev limiter's, where the throttle is cut, when the engine may over-rev (Jolt's limit
		// is then Max RPM + Over-Rev RPM); 0: no limiter (Jolt's limit is Max RPM)
		float LimiterRpm = 0.0f;
		// A wheel's model: its axes (the wheel's turning axis and up, in the model's own space, from its rotation at the start)
		// and its scale, kept as it rolls and steers
		struct WheelModel
		{
			EnvMapVulkanEntityID Entity = NoEntity;
			JPH::Vec3 Right = JPH::Vec3::sAxisX(), Up = JPH::Vec3::sAxisY();
			glm::vec3 Scale = glm::vec3(1.0f);
		};
		std::vector<WheelModel> Wheels;
	};
	// The vehicle of a dynamic body just made (false: its settings can't make one; the body stays a plain one)
	bool CreateVehicle(Vehicle& vehicle, const VehicleComponent& settings);
	void RemoveVehicle(Vehicle& vehicle);
	const Vehicle* FindVehicle(EnvMapVulkanEntityID entity) const;
	// A manual gearbox into a gear: the clutch out for Jolt's switch time (none from neutral), then let in
	static void PutInGear(Vehicle& vehicle, int gear);
	// A manual gearbox's clutch for the next step, the throttle through it, and its engine braking (a force on the body)
	void UpdateManualGearbox(Vehicle& vehicle, VehicleInput& applied, float fixedStep);
	// The wheels' models where the wheels are, on the body at its place for this frame
	void WriteWheels(const Vehicle& vehicle, const glm::vec3& position, const glm::quat& rotation);
	void SetEntityPosition(EnvMapVulkanEntityID entity, const glm::vec3& position); // the translation only
	// The bodies and characters of entities that are gone (destroyed by the game, deleted in the editor): out of the world,
	// and what was near them awake, so what stood on them falls
	void RemoveDestroyedEntities();
	// The dynamic body of an entity, nullptr: none
	const Body* FindDynamicBody(EnvMapVulkanEntityID entity) const;
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
	std::vector<Vehicle> m_Vehicles;

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
	float m_FixedStep = 1.0f / 60.0f; // the last step's (an acceleration is a speed added over it)

	// The steps' cost, logged at Stop
	uint64_t m_StepCount = 0;
	double m_StepSeconds = 0.0;
	float m_LastStepMs = 0.0f;
};
