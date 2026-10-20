#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>


// What a body is, for who collides with whom (docs/engine/Game_Engine_Runtime_Guide.html, 3.3). The pairs that collide
// are in PhysicsWorld's constructor
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
}

// The broad phase keeps two trees: the bodies that never move apart from the rest, so a step doesn't revisit them
namespace BroadPhaseLayers
{
	constexpr JPH::BroadPhaseLayer NonMoving(0);
	constexpr JPH::BroadPhaseLayer Moving(1);
	constexpr JPH::uint Count = 2;
}
