#include "PhysicsWorld.h"

#include "PhysicsLayers.h"
#include "PhysicsMath.h"

#include "Core/Log.h"

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/IssueReporting.h>
#include <Jolt/RegisterTypes.h>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <thread>


// Jolt's messages, in the engine's log
static void JoltTrace(const char* format, ...)
{
	char message[1024];
	va_list arguments;
	va_start(arguments, format);
	vsnprintf(message, sizeof(message), format, arguments);
	va_end(arguments);
	Log::GetLogger()->info("Jolt: {0}", message);
}

#if defined(JPH_ENABLE_ASSERTS)
static bool JoltAssertFailed(const char* expression, const char* message, const char* file, JPH::uint line)
{
	Log::GetLogger()->error("Jolt: assert failed: {0} {1} ({2}:{3})", expression, message ? message : "", file, line);
	return true; // break into the debugger
}
#endif

static bool s_Initialized = false;

void PhysicsWorld::Initialize()
{
	if (s_Initialized)
	{
		return;
	}
	JPH::RegisterDefaultAllocator();
	JPH::Trace = JoltTrace;
	JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = JoltAssertFailed;)
	JPH::Factory::sInstance = new JPH::Factory();
	JPH::RegisterTypes();
	s_Initialized = true;
}

void PhysicsWorld::Shutdown()
{
	if (!s_Initialized)
	{
		return;
	}
	JPH::UnregisterTypes();
	delete JPH::Factory::sInstance;
	JPH::Factory::sInstance = nullptr;
	s_Initialized = false;
}

// Jobs and barriers the worker threads can have queued at once (Jolt's samples use these)
static constexpr JPH::uint MaxPhysicsJobs = 2048;
static constexpr JPH::uint MaxPhysicsBarriers = 8;

PhysicsWorld::PhysicsWorld(const PhysicsWorldSettings& settings)
	: m_BroadPhaseLayers(ObjectLayers::Count, BroadPhaseLayers::Count),
	m_ObjectLayerPairs(ObjectLayers::Count)
{
	JPH_ASSERT(s_Initialized);

	// Who collides with whom. Static bodies don't collide with each other (neither moves), a trigger collides with
	// nothing that doesn't move (it only notices what enters it)
	m_ObjectLayerPairs.EnableCollision(ObjectLayers::Static, ObjectLayers::Dynamic);
	m_ObjectLayerPairs.EnableCollision(ObjectLayers::Static, ObjectLayers::Character);
	m_ObjectLayerPairs.EnableCollision(ObjectLayers::Dynamic, ObjectLayers::Dynamic);
	m_ObjectLayerPairs.EnableCollision(ObjectLayers::Dynamic, ObjectLayers::Character);
	m_ObjectLayerPairs.EnableCollision(ObjectLayers::Dynamic, ObjectLayers::Trigger);
	m_ObjectLayerPairs.EnableCollision(ObjectLayers::Character, ObjectLayers::Character);
	m_ObjectLayerPairs.EnableCollision(ObjectLayers::Character, ObjectLayers::Trigger);

	m_BroadPhaseLayers.MapObjectToBroadPhaseLayer(ObjectLayers::Static, BroadPhaseLayers::NonMoving);
	m_BroadPhaseLayers.MapObjectToBroadPhaseLayer(ObjectLayers::Dynamic, BroadPhaseLayers::Moving);
	m_BroadPhaseLayers.MapObjectToBroadPhaseLayer(ObjectLayers::Character, BroadPhaseLayers::Moving);
	m_BroadPhaseLayers.MapObjectToBroadPhaseLayer(ObjectLayers::Trigger, BroadPhaseLayers::Moving);

	m_ObjectVsBroadPhaseLayers = std::make_unique<JPH::ObjectVsBroadPhaseLayerFilterTable>(
		m_BroadPhaseLayers, BroadPhaseLayers::Count, m_ObjectLayerPairs, ObjectLayers::Count);

	// A step's memory (contacts, islands), taken at once instead of from the heap during the step
	m_TempAllocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024);
	// The step's jobs run on the main thread (which waits for them) and on these: one per hardware thread but that one
	const int threads = std::max(1, (int)std::thread::hardware_concurrency() - 1);
	m_JobSystem = std::make_unique<JPH::JobSystemThreadPool>(MaxPhysicsJobs, MaxPhysicsBarriers, threads);

	const JPH::uint bodyMutexes = 0; // Jolt's default
	m_System.Init(settings.MaxBodies, bodyMutexes, settings.MaxBodyPairs, settings.MaxContactConstraints,
		m_BroadPhaseLayers, *m_ObjectVsBroadPhaseLayers, m_ObjectLayerPairs);
}

PhysicsWorld::~PhysicsWorld()
{
	RemoveAllBodies();
}

void PhysicsWorld::Step(float fixedStep)
{
	// One collision step per fixed step: at 60 steps a second Jolt needs no more
	const JPH::EPhysicsUpdateError error = m_System.Update(fixedStep, 1, m_TempAllocator.get(), m_JobSystem.get());
	if (error != JPH::EPhysicsUpdateError::None && !m_StepErrorReported)
	{
		// Too many touching pairs for PhysicsWorldSettings: some contacts were left out (bodies may pass through each other)
		Log::GetLogger()->warn("Physics: a step ran out of room for its contacts (error {0}): raise PhysicsWorldSettings",
			(uint32_t)error);
		m_StepErrorReported = true;
	}
}

JPH::BodyID PhysicsWorld::AddBody(const JPH::BodyCreationSettings& settings)
{
	const JPH::EActivation activation = settings.mMotionType == JPH::EMotionType::Static
		? JPH::EActivation::DontActivate : JPH::EActivation::Activate;
	const JPH::BodyID body = GetBodyInterface().CreateAndAddBody(settings, activation);
	if (body.IsInvalid())
	{
		Log::GetLogger()->warn("Physics: no body added, the world has its maximum of {0}", m_System.GetMaxBodies());
		return body;
	}
	m_Bodies.push_back(body);
	return body;
}

void PhysicsWorld::RemoveBody(JPH::BodyID body)
{
	const auto found = std::find(m_Bodies.begin(), m_Bodies.end(), body);
	if (found == m_Bodies.end())
	{
		return;
	}
	m_Bodies.erase(found);
	GetBodyInterface().RemoveBody(body);
	GetBodyInterface().DestroyBody(body);
}

void PhysicsWorld::RemoveAllBodies()
{
	if (m_Bodies.empty())
	{
		return;
	}
	JPH::BodyInterface& bodies = GetBodyInterface();
	bodies.RemoveBodies(m_Bodies.data(), (int)m_Bodies.size());
	bodies.DestroyBodies(m_Bodies.data(), (int)m_Bodies.size());
	m_Bodies.clear();
}

glm::mat4 PhysicsWorld::GetBodyTransform(JPH::BodyID body) const
{
	JPH::RVec3 position;
	JPH::Quat rotation;
	m_System.GetBodyInterface().GetPositionAndRotation(body, position, rotation);
	return glm::translate(glm::mat4(1.0f), ToGlm(position)) * glm::mat4_cast(ToGlm(rotation));
}

glm::vec3 PhysicsWorld::GetBodyPosition(JPH::BodyID body) const
{
	return ToGlm(m_System.GetBodyInterface().GetPosition(body));
}

void PhysicsWorld::SetGravity(const glm::vec3& gravity)
{
	m_System.SetGravity(ToJolt(gravity));
}

glm::vec3 PhysicsWorld::GetGravity() const
{
	return ToGlm(m_System.GetGravity());
}

uint32_t PhysicsWorld::GetBodyCount() const
{
	return m_System.GetNumBodies();
}

uint32_t PhysicsWorld::GetActiveBodyCount() const
{
	return m_System.GetNumActiveBodies(JPH::EBodyType::RigidBody);
}
