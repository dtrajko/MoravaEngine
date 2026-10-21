#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

#include "PhysicsCollisionMatrix.h"


// What a body is, for who collides with whom (docs/engine/Game_Engine_Runtime_Guide.html, 3.3). The pairs that collide
// are the project's PhysicsCollisionMatrix, whose layers these are
namespace ObjectLayers
{
	enum : JPH::ObjectLayer
	{
		Static,    // doesn't move: floors, walls
		Dynamic,   // moved by the simulation, or kinematic (moved by code, pushes the dynamic ones)
		Character, // a character controller
		Trigger,   // a sensor: reports what enters and leaves it, collides with nothing
		Count
	};
	static_assert(uint32_t(Count) == uint32_t(PhysicsCollisionMatrix::LayerCount) && uint32_t(Trigger) == uint32_t(PhysicsCollisionMatrix::Trigger),
		"the matrix's layers are these");

	// Layers as bits, for the queries (ScenePhysics::Raycast...): what they hit
	enum : uint32_t
	{
		StaticBit = 1u << Static, DynamicBit = 1u << Dynamic, CharacterBit = 1u << Character, TriggerBit = 1u << Trigger,
		Solid = StaticBit | DynamicBit | CharacterBit, // everything but triggers (the default)
		All = Solid | TriggerBit
	};
}

// The broad phase keeps two trees: the bodies that never move apart from the rest, so a step doesn't revisit them
namespace BroadPhaseLayers
{
	constexpr JPH::BroadPhaseLayer NonMoving(0);
	constexpr JPH::BroadPhaseLayer Moving(1);
	constexpr JPH::uint Count = 2;
}
