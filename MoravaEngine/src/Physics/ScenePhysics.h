#pragma once

#include "PhysicsWorld.h"

#include "EnvMapVulkan/EnvMapVulkanComponents.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>
#include <vector>


class EnvMapVulkanScene;

/**
 * The physics of a running game's scene (docs/engine/Game_Engine_Runtime_Guide.html, 3.3 and 3.4): a PhysicsWorld with a
 * body for every entity that has a ColliderComponent, made when the game starts and gone when it stops. The bodies'
 * shapes are fixed at the start, with the entities' world scale in them.
 *
 * The scene stays the place where entities are. Before each step the kinematic bodies are moved to their entities, and a
 * body whose entity was moved by something else (the game, or the editor while paused) is put there. After the steps the
 * dynamic bodies' places are written into their entities' transforms, between their last two steps (WriteTransforms), so
 * they move smoothly whatever the frame rate.
 */
class ScenePhysics
{
public:
	ScenePhysics();
	~ScenePhysics();
	ScenePhysics(const ScenePhysics&) = delete;
	ScenePhysics& operator=(const ScenePhysics&) = delete;

	void Start(EnvMapVulkanScene& scene);
	void Stop();
	bool IsRunning() const { return m_World != nullptr; }

	void Step(float fixedStep);
	// The dynamic bodies' entities where the bodies are, alpha of the way from their previous step to their last one
	void WriteTransforms(float alpha);

	PhysicsWorld* GetWorld() { return m_World.get(); }

	// A collider that fits around a model entity's meshes, in the entity's own space (without its scale), for the shape it
	// has: its size and center (ConvexMesh and TriangleMesh are the meshes themselves: nothing to fit). False: the entity
	// has no model, or a model without meshes.
	static bool FitCollider(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, ColliderComponent& collider);

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
	};

	// The body of body.Entity, added to the world (false: its shape could not be made, or the world is full)
	bool CreateBody(Body& body, const ColliderComponent& collider, const RigidBodyComponent* rigidBody);

	EnvMapVulkanScene* m_Scene = nullptr;
	std::unique_ptr<PhysicsWorld> m_World;
	std::vector<Body> m_Bodies;

	// The steps' cost, logged at Stop
	uint64_t m_StepCount = 0;
	double m_StepSeconds = 0.0;
};
