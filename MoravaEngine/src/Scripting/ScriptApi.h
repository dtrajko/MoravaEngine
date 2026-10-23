#pragma once

#include <glm/glm.hpp>

#include <cstdint>


// The two function tables between C++ and C# (docs/engine/Game_Engine_Runtime_Guide.html, 4.3): plain C functions,
// blittable arguments only, strings as UTF-16 with a length. Same order and layout as NativeApi, ManagedApi and the structs
// in ScriptCore/Interop.cs; a change here is a change there. glm::vec3 is System.Numerics.Vector3 (three floats); a matrix
// is glm's column-major mat4, which is System.Numerics.Matrix4x4 in memory (its row vectors are glm's columns).

struct ScriptQuat
{
	float X = 0.0f, Y = 0.0f, Z = 0.0f, W = 1.0f; // System.Numerics.Quaternion's order (glm's constructor takes w first)
};

// An input action as a script reads it. Pressed and Released are the fixed step's edges in OnFixedUpdate, the frame's in
// OnUpdate (InputSystem: ...SinceFixed or ...ThisFrame)
struct ScriptInputAction
{
	glm::vec2 Value = glm::vec2(0.0f);
	int32_t Held = 0, Pressed = 0, Released = 0;
	int32_t Exists = 0; // the input map has it
};

struct ScriptCharacterState
{
	int32_t Ground = 0; // ScenePhysics::CharacterGround
	glm::vec3 Velocity = glm::vec3(0.0f);
	glm::vec3 GroundNormal = glm::vec3(0.0f, 1.0f, 0.0f);
};

struct ScriptRaycastHit
{
	uint64_t Entity = 0;
	glm::vec3 Point = glm::vec3(0.0f);
	glm::vec3 Normal = glm::vec3(0.0f);
	float Distance = 0.0f;
};

struct ScriptTime
{
	float DeltaTime = 0.0f;      // this update's: the fixed step in OnFixedUpdate, the frame's time in OnUpdate
	float FixedDeltaTime = 0.0f;
	double Time = 0.0;           // game time since the game started, in seconds (the fixed steps')
	uint64_t FrameCount = 0;
	uint64_t FixedStepCount = 0;
};

// The components a script can ask an entity for (Entity.HasComponent<T>, GetComponent<T>)
enum class ScriptComponent : int32_t { Rigidbody = 0, CharacterController = 1, Collider = 2 };

// What C# may call in C++: grows with each phase. The functions about an entity return 0 when it doesn't exist (any more):
// C# turns that into an exception that names it. Only Log works outside a running game; the rest come with the game's
// scene (ScriptGlue)
struct ScriptNativeApi
{
	void (*Log)(int level, const char16_t* text, int length); // 0 info, 1 warning, 2 error

	// Entity
	int32_t (*Entity_Exists)(uint64_t entity);
	int32_t (*Entity_GetName)(uint64_t entity, char16_t* buffer, int capacity); // the length, -1: doesn't fit, -2: no entity
	int32_t (*Entity_SetName)(uint64_t entity, const char16_t* name, int length);
	int32_t (*Entity_GetTranslation)(uint64_t entity, glm::vec3* out);
	int32_t (*Entity_SetTranslation)(uint64_t entity, const glm::vec3* value);
	int32_t (*Entity_GetRotation)(uint64_t entity, ScriptQuat* out);
	int32_t (*Entity_SetRotation)(uint64_t entity, const ScriptQuat* value);
	int32_t (*Entity_GetEulerAngles)(uint64_t entity, glm::vec3* out); // degrees, as the Properties panel shows them
	int32_t (*Entity_SetEulerAngles)(uint64_t entity, const glm::vec3* value);
	int32_t (*Entity_GetScale)(uint64_t entity, glm::vec3* out);
	int32_t (*Entity_SetScale)(uint64_t entity, const glm::vec3* value);
	int32_t (*Entity_GetWorldTransform)(uint64_t entity, glm::mat4* out);
	int32_t (*Entity_HasTag)(uint64_t entity, const char16_t* tag, int length); // 1 yes, 0 no, -1 no entity
	int32_t (*Entity_AddTag)(uint64_t entity, const char16_t* tag, int length);
	int32_t (*Entity_RemoveTag)(uint64_t entity, const char16_t* tag, int length);
	int32_t (*Entity_HasComponent)(uint64_t entity, int32_t component); // 1 yes, 0 no, -1 no entity
	int32_t (*Entity_Destroy)(uint64_t entity); // at the end of the update it was called in

	// Scene
	uint64_t (*Scene_FindEntityByName)(const char16_t* name, int length); // 0: none
	int32_t (*Scene_FindEntitiesWithTag)(const char16_t* tag, int length, uint64_t* out, int capacity); // how many there are

	// Input
	void (*Input_GetAction)(const char16_t* name, int length, ScriptInputAction* out);

	// Rigidbody: 0 when the entity has no dynamic body. mode: ScenePhysics::ForceMode
	int32_t (*Rigidbody_AddForce)(uint64_t entity, const glm::vec3* force, int32_t mode);
	int32_t (*Rigidbody_AddTorque)(uint64_t entity, const glm::vec3* torque, int32_t mode);
	int32_t (*Rigidbody_GetLinearVelocity)(uint64_t entity, glm::vec3* out);
	int32_t (*Rigidbody_SetLinearVelocity)(uint64_t entity, const glm::vec3* value);
	int32_t (*Rigidbody_GetAngularVelocity)(uint64_t entity, glm::vec3* out);
	int32_t (*Rigidbody_SetAngularVelocity)(uint64_t entity, const glm::vec3* value);

	// CharacterController: 0 when the entity isn't a running character
	int32_t (*Character_SetVelocity)(uint64_t entity, const glm::vec3* velocity);
	int32_t (*Character_GetState)(uint64_t entity, ScriptCharacterState* out);

	// Physics queries. layers: ObjectLayers bits (Solid, All...); ignore: an entity the query goes through (0: none).
	// The overlaps return how many entities there are (more than capacity: call again with more room)
	int32_t (*Physics_Raycast)(const glm::vec3* origin, const glm::vec3* direction, float maxDistance, uint32_t layers, uint64_t ignore,
		ScriptRaycastHit* hit);
	int32_t (*Physics_SphereCast)(const glm::vec3* origin, float radius, const glm::vec3* direction, float maxDistance, uint32_t layers,
		uint64_t ignore, ScriptRaycastHit* hit);
	int32_t (*Physics_OverlapSphere)(const glm::vec3* center, float radius, uint32_t layers, uint64_t* out, int capacity);
	int32_t (*Physics_OverlapBox)(const glm::vec3* center, const glm::vec3* halfExtents, const ScriptQuat* rotation, uint32_t layers,
		uint64_t* out, int capacity);

	// Time
	void (*Time_Get)(ScriptTime* out);
};

// What C++ calls in C#; Interop.Initialize fills it in. A script instance is a GCHandle of the C# object. The Invoke
// functions return 0, or 1 when the script threw (C# has logged it)
struct ScriptManagedApi
{
	int (*GetRuntimeVersion)(char16_t* buffer, int capacity);   // the length written, -1 when it doesn't fit
	int (*GetRuntimeDirectory)(char16_t* buffer, int capacity); // the folder of the base library
	int (*LoadGameAssembly)(const char16_t* path, int length);  // 0 when loaded
	int (*UnloadGameAssembly)();                                // 0 collected, 1 still alive, -1 refused (instances left)
	void* (*CreateInstance)(const char16_t* className, int length, uint64_t entity); // nullptr: no such class, or it threw
	void (*DestroyInstance)(void* instance);
	int (*InvokeCreate)(void* instance);
	int (*InvokeUpdate)(void* instance, float dt);
	int (*InvokeFixedUpdate)(void* instance, float dt);
	// kind: ScriptCollisionKind; point and normal (from the script's entity towards the other) for CollisionEnter
	int (*InvokeCollision)(void* instance, int kind, uint64_t other, const glm::vec3* point, const glm::vec3* normal);
};

enum class ScriptCollisionKind : int32_t { CollisionEnter = 0, CollisionExit = 1, TriggerEnter = 2, TriggerExit = 3 };
