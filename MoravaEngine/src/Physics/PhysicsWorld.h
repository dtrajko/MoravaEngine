#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h>
#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h>
#include <Jolt/Physics/Collision/ObjectLayerPairFilterTable.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <vector>


struct PhysicsWorldSettings
{
	uint32_t MaxBodies = 10240;
	uint32_t MaxBodyPairs = 65536;          // pairs of bodies whose bounding boxes overlap, per step
	uint32_t MaxContactConstraints = 10240; // pairs of bodies that touch, per step
};

/**
 * A world of Jolt bodies (docs/engine/Game_Engine_Runtime_Guide.html, 3.3): its PhysicsSystem with the collision layers
 * (PhysicsLayers.h), the memory for a step, and the worker threads that run it. It knows nothing of scenes: a scene
 * creates the bodies of its objects in it and reads their transforms back after each step.
 *
 * Jolt runs on fixed steps: Step is called by the game loop (GameLoop::FixedStep), never with a frame's time.
 */
class PhysicsWorld
{
public:
	// Once per process, before the first world (Jolt's allocator, its factory of shape and body types), and at exit
	static void Initialize();
	static void Shutdown();

	explicit PhysicsWorld(const PhysicsWorldSettings& settings = PhysicsWorldSettings());
	~PhysicsWorld();
	PhysicsWorld(const PhysicsWorld&) = delete;
	PhysicsWorld& operator=(const PhysicsWorld&) = delete;

	void Step(float fixedStep);

	// A body, added to the world (a moving one awake); an invalid ID when the world is full (MaxBodies)
	JPH::BodyID AddBody(const JPH::BodyCreationSettings& settings);
	void RemoveBody(JPH::BodyID body);
	void RemoveAllBodies();

	// Where a body is: its position and rotation (no scale: the shape has it)
	glm::mat4 GetBodyTransform(JPH::BodyID body) const;
	glm::vec3 GetBodyPosition(JPH::BodyID body) const;

	void SetGravity(const glm::vec3& gravity);
	glm::vec3 GetGravity() const;

	uint32_t GetBodyCount() const;
	uint32_t GetActiveBodyCount() const; // awake: the rest sleep until something touches them

	JPH::BodyInterface& GetBodyInterface() { return m_System.GetBodyInterface(); }
	JPH::PhysicsSystem& GetSystem() { return m_System; }

private:
	// Declared before the system, which keeps references to them
	JPH::BroadPhaseLayerInterfaceTable m_BroadPhaseLayers;
	JPH::ObjectLayerPairFilterTable m_ObjectLayerPairs;
	std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilterTable> m_ObjectVsBroadPhaseLayers;

	std::unique_ptr<JPH::TempAllocatorImpl> m_TempAllocator;
	std::unique_ptr<JPH::JobSystemThreadPool> m_JobSystem;
	JPH::PhysicsSystem m_System;

	std::vector<JPH::BodyID> m_Bodies; // the ones AddBody added, for RemoveAllBodies
	bool m_StepErrorReported = false;
};
