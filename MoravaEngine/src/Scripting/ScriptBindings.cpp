#include "ScriptBindings.h"

#include "ScriptHost.h"

#include "EnvMapVulkan/EnvMapVulkanRenderer.h"
#include "EnvMapVulkan/EnvMapVulkanScene.h"

#include "Input/InputSystem.h"

#include "Physics/ScenePhysics.h"

#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cstring>


static ScriptContext* s_Context = nullptr;

static std::string ToUtf8(const char16_t* text, int length)
{
	return ScriptUtf16ToUtf8(text, length);
}

// The entity's components, nullptr when there is no game or no such entity
static EnvMapVulkanScene* GetScene()
{
	return s_Context ? s_Context->Scene : nullptr;
}

static TransformComponent* GetTransform(uint64_t entity)
{
	EnvMapVulkanScene* scene = GetScene();
	return scene ? scene->TryGet<TransformComponent>(entity) : nullptr;
}

static int32_t CopyString(const std::string& text, char16_t* buffer, int capacity)
{
	const std::u16string utf16 = ScriptUtf8ToUtf16(text);
	if ((int)utf16.size() > capacity)
	{
		return -1;
	}
	std::copy(utf16.begin(), utf16.end(), buffer);
	return (int32_t)utf16.size();
}

// Rotations: the engine keeps Euler angles in degrees (ImGuizmo's convention, TransformComponent); scripts get quaternions,
// converted through the same matrices, so a rotation set and read back is the same one
static glm::quat EulerToQuat(const glm::vec3& degrees)
{
	TransformComponent rotation;
	rotation.Rotation = degrees;
	return glm::normalize(glm::quat_cast(glm::mat3(rotation.GetMatrix())));
}

// Of the two sets of angles for one rotation, the one with X and Z within -90..90 degrees: a turn around Y alone reads as
// (0, 90, 0), not as (180, 90, 180). Taken only when it gives the same matrix
static glm::vec3 QuatToEuler(const glm::quat& q)
{
	TransformComponent rotation;
	const glm::mat4 matrix = glm::mat4_cast(glm::normalize(q));
	rotation.SetMatrix(matrix);
	const glm::vec3 angles = rotation.Rotation;
	if (std::abs(angles.x) <= 90.0f || std::abs(angles.z) <= 90.0f)
	{
		return angles;
	}
	auto sign = [](float v) { return v < 0.0f ? -1.0f : 1.0f; };
	TransformComponent other;
	other.Rotation = glm::vec3(angles.x - sign(angles.x) * 180.0f, sign(angles.y) * 180.0f - angles.y, angles.z - sign(angles.z) * 180.0f);
	const glm::mat4 check = other.GetMatrix();
	for (int c = 0; c < 3; c++)
	{
		if (glm::length(glm::vec3(check[c]) - glm::vec3(matrix[c])) > 1e-4f)
		{
			return angles;
		}
	}
	return other.Rotation;
}

static ScriptQuat ToScript(const glm::quat& q) { return { q.x, q.y, q.z, q.w }; }
static glm::quat ToGlm(const ScriptQuat& q) { return glm::quat(q.W, q.X, q.Y, q.Z); }

// ---- Entity

static int32_t EntityExists(uint64_t entity)
{
	EnvMapVulkanScene* scene = GetScene();
	return scene && scene->Exists(entity) ? 1 : 0;
}

static int32_t EntityGetName(uint64_t entity, char16_t* buffer, int capacity)
{
	EnvMapVulkanScene* scene = GetScene();
	const NameComponent* name = scene ? scene->TryGet<NameComponent>(entity) : nullptr;
	return name ? CopyString(name->Name, buffer, capacity) : -2;
}

static int32_t EntitySetName(uint64_t entity, const char16_t* text, int length)
{
	EnvMapVulkanScene* scene = GetScene();
	NameComponent* name = scene ? scene->TryGet<NameComponent>(entity) : nullptr;
	if (!name)
	{
		return 0;
	}
	name->Name = ToUtf8(text, length);
	return 1;
}

static int32_t EntityGetTranslation(uint64_t entity, glm::vec3* out)
{
	const TransformComponent* transform = GetTransform(entity);
	if (!transform)
	{
		return 0;
	}
	*out = transform->Translation;
	return 1;
}

static int32_t EntitySetTranslation(uint64_t entity, const glm::vec3* value)
{
	TransformComponent* transform = GetTransform(entity);
	if (!transform)
	{
		return 0;
	}
	transform->Translation = *value; // a body there follows at the next step (ScenePhysics)
	return 1;
}

static int32_t EntityGetRotation(uint64_t entity, ScriptQuat* out)
{
	const TransformComponent* transform = GetTransform(entity);
	if (!transform)
	{
		return 0;
	}
	*out = ToScript(EulerToQuat(transform->Rotation));
	return 1;
}

static int32_t EntitySetRotation(uint64_t entity, const ScriptQuat* value)
{
	TransformComponent* transform = GetTransform(entity);
	if (!transform)
	{
		return 0;
	}
	transform->Rotation = QuatToEuler(ToGlm(*value));
	return 1;
}

static int32_t EntityGetEulerAngles(uint64_t entity, glm::vec3* out)
{
	const TransformComponent* transform = GetTransform(entity);
	if (!transform)
	{
		return 0;
	}
	*out = transform->Rotation;
	return 1;
}

static int32_t EntitySetEulerAngles(uint64_t entity, const glm::vec3* value)
{
	TransformComponent* transform = GetTransform(entity);
	if (!transform)
	{
		return 0;
	}
	transform->Rotation = *value;
	return 1;
}

static int32_t EntityGetScale(uint64_t entity, glm::vec3* out)
{
	const TransformComponent* transform = GetTransform(entity);
	if (!transform)
	{
		return 0;
	}
	*out = transform->Scale;
	return 1;
}

static int32_t EntitySetScale(uint64_t entity, const glm::vec3* value)
{
	TransformComponent* transform = GetTransform(entity);
	if (!transform)
	{
		return 0;
	}
	transform->Scale = *value; // the look only: a body's shape keeps the scale it was made with
	return 1;
}

static int32_t EntityGetWorldTransform(uint64_t entity, glm::mat4* out)
{
	EnvMapVulkanScene* scene = GetScene();
	if (!scene || !scene->Exists(entity))
	{
		return 0;
	}
	*out = scene->GetWorldTransform(entity);
	return 1;
}

static int32_t EntityHasTag(uint64_t entity, const char16_t* tag, int length)
{
	EnvMapVulkanScene* scene = GetScene();
	if (!scene || !scene->Exists(entity))
	{
		return -1;
	}
	const TagComponent* tags = scene->TryGet<TagComponent>(entity);
	return tags && tags->Has(ToUtf8(tag, length)) ? 1 : 0;
}

static int32_t EntityAddTag(uint64_t entity, const char16_t* tag, int length)
{
	EnvMapVulkanScene* scene = GetScene();
	if (!scene || !scene->Exists(entity))
	{
		return 0;
	}
	TagComponent* tags = scene->TryGet<TagComponent>(entity);
	(tags ? *tags : scene->Add<TagComponent>(entity)).Add(ToUtf8(tag, length));
	return 1;
}

static int32_t EntityRemoveTag(uint64_t entity, const char16_t* tag, int length)
{
	EnvMapVulkanScene* scene = GetScene();
	if (!scene || !scene->Exists(entity))
	{
		return 0;
	}
	if (TagComponent* tags = scene->TryGet<TagComponent>(entity))
	{
		tags->Remove(ToUtf8(tag, length));
	}
	return 1;
}

static int32_t EntityHasComponent(uint64_t entity, int32_t component)
{
	EnvMapVulkanScene* scene = GetScene();
	if (!scene || !scene->Exists(entity))
	{
		return -1;
	}
	switch ((ScriptComponentKind)component)
	{
		case ScriptComponentKind::Rigidbody:
		{
			const RigidBodyComponent* body = scene->TryGet<RigidBodyComponent>(entity);
			return body && scene->Has<ColliderComponent>(entity) && body->Type == RigidBodyComponent::BodyType::Dynamic ? 1 : 0;
		}
		case ScriptComponentKind::CharacterController: return scene->Has<CharacterControllerComponent>(entity) ? 1 : 0;
		case ScriptComponentKind::Collider:          return scene->Has<ColliderComponent>(entity) ? 1 : 0;
	}
	return 0;
}

static int32_t EntityDestroy(uint64_t entity)
{
	EnvMapVulkanScene* scene = GetScene();
	if (!scene || !scene->Exists(entity))
	{
		return 0;
	}
	if (std::find(s_Context->DestroyQueue.begin(), s_Context->DestroyQueue.end(), entity) == s_Context->DestroyQueue.end())
	{
		s_Context->DestroyQueue.push_back(entity);
	}
	return 1;
}

// ---- Scene

static uint64_t SceneFindEntityByName(const char16_t* text, int length)
{
	EnvMapVulkanScene* scene = GetScene();
	if (!scene)
	{
		return NoEntity;
	}
	const std::string name = ToUtf8(text, length);
	EnvMapVulkanEntityID found = NoEntity;
	scene->Each<NameComponent>([&](EnvMapVulkanEntityID entity, NameComponent& component)
	{
		if (found == NoEntity && component.Name == name)
		{
			found = entity;
		}
	});
	return found;
}

static int32_t SceneFindEntitiesWithTag(const char16_t* text, int length, uint64_t* out, int capacity)
{
	EnvMapVulkanScene* scene = GetScene();
	if (!scene)
	{
		return 0;
	}
	const std::string tag = ToUtf8(text, length);
	int32_t count = 0;
	scene->Each<TagComponent>([&](EnvMapVulkanEntityID entity, TagComponent& tags)
	{
		if (tags.Has(tag))
		{
			if (count < capacity)
			{
				out[count] = entity;
			}
			count++;
		}
	});
	return count;
}

// ---- Input

static void InputGetAction(const char16_t* text, int length, ScriptInputAction* out)
{
	*out = ScriptInputAction();
	if (!s_Context || !s_Context->Input)
	{
		return;
	}
	const std::string name = ToUtf8(text, length);
	if (!s_Context->Input->GetMap().Find(name))
	{
		return; // C# says which action is missing
	}
	const InputActionState& state = s_Context->Input->Get(name);
	out->Value = state.Value;
	out->Held = state.Held ? 1 : 0;
	out->Pressed = (s_Context->InFixedStep ? state.PressedSinceFixed : state.PressedThisFrame) ? 1 : 0;
	out->Released = (s_Context->InFixedStep ? state.ReleasedSinceFixed : state.ReleasedThisFrame) ? 1 : 0;
	out->Exists = 1;
}

// ---- Rigidbody and CharacterController

static ScenePhysics* GetPhysics()
{
	return s_Context ? s_Context->Physics : nullptr;
}

static int32_t RigidbodyAddForce(uint64_t entity, const glm::vec3* force, int32_t mode)
{
	ScenePhysics* physics = GetPhysics();
	return physics && physics->AddForce(entity, *force, (ScenePhysics::ForceMode)mode) ? 1 : 0;
}

static int32_t RigidbodyAddTorque(uint64_t entity, const glm::vec3* torque, int32_t mode)
{
	ScenePhysics* physics = GetPhysics();
	return physics && physics->AddTorque(entity, *torque, (ScenePhysics::ForceMode)mode) ? 1 : 0;
}

static int32_t RigidbodyGetLinearVelocity(uint64_t entity, glm::vec3* out)
{
	ScenePhysics* physics = GetPhysics();
	glm::vec3 angular;
	return physics && physics->GetVelocity(entity, *out, angular) ? 1 : 0;
}

static int32_t RigidbodySetLinearVelocity(uint64_t entity, const glm::vec3* value)
{
	ScenePhysics* physics = GetPhysics();
	return physics && physics->SetLinearVelocity(entity, *value) ? 1 : 0;
}

static int32_t RigidbodyGetAngularVelocity(uint64_t entity, glm::vec3* out)
{
	ScenePhysics* physics = GetPhysics();
	glm::vec3 linear;
	return physics && physics->GetVelocity(entity, linear, *out) ? 1 : 0;
}

static int32_t RigidbodySetAngularVelocity(uint64_t entity, const glm::vec3* value)
{
	ScenePhysics* physics = GetPhysics();
	return physics && physics->SetAngularVelocity(entity, *value) ? 1 : 0;
}

static int32_t CharacterSetVelocity(uint64_t entity, const glm::vec3* velocity)
{
	ScenePhysics* physics = GetPhysics();
	ScenePhysics::CharacterState state;
	if (!physics || !physics->GetCharacterState(entity, state))
	{
		return 0;
	}
	physics->SetCharacterVelocity(entity, *velocity);
	return 1;
}

static int32_t CharacterGetState(uint64_t entity, ScriptCharacterState* out)
{
	ScenePhysics* physics = GetPhysics();
	ScenePhysics::CharacterState state;
	if (!physics || !physics->GetCharacterState(entity, state))
	{
		return 0;
	}
	out->Ground = (int32_t)state.Ground;
	out->Velocity = state.Velocity;
	out->GroundNormal = state.GroundNormal;
	return 1;
}

// ---- Physics queries

static void ToScript(const ScenePhysics::Hit& hit, ScriptRaycastHit* out)
{
	out->Entity = hit.Entity;
	out->Point = hit.Point;
	out->Normal = hit.Normal;
	out->Distance = hit.Distance;
}

static int32_t PhysicsRaycast(const glm::vec3* origin, const glm::vec3* direction, float maxDistance, uint32_t layers, uint64_t ignore,
	ScriptRaycastHit* out)
{
	ScenePhysics* physics = GetPhysics();
	ScenePhysics::Hit hit;
	if (!physics || !physics->Raycast(*origin, *direction, maxDistance, hit, layers, ignore))
	{
		return 0;
	}
	ToScript(hit, out);
	return 1;
}

static int32_t PhysicsSphereCast(const glm::vec3* origin, float radius, const glm::vec3* direction, float maxDistance, uint32_t layers,
	uint64_t ignore, ScriptRaycastHit* out)
{
	ScenePhysics* physics = GetPhysics();
	ScenePhysics::Hit hit;
	if (!physics || !physics->SphereCast(*origin, radius, *direction, maxDistance, hit, layers, ignore))
	{
		return 0;
	}
	ToScript(hit, out);
	return 1;
}

static int32_t CopyEntities(const std::vector<EnvMapVulkanEntityID>& entities, uint64_t* out, int capacity)
{
	std::copy_n(entities.begin(), std::min((int)entities.size(), capacity), out);
	return (int32_t)entities.size();
}

static int32_t PhysicsOverlapSphere(const glm::vec3* center, float radius, uint32_t layers, uint64_t* out, int capacity)
{
	ScenePhysics* physics = GetPhysics();
	return physics ? CopyEntities(physics->OverlapSphere(*center, radius, layers), out, capacity) : 0;
}

static int32_t PhysicsOverlapBox(const glm::vec3* center, const glm::vec3* halfExtents, const ScriptQuat* rotation, uint32_t layers,
	uint64_t* out, int capacity)
{
	ScenePhysics* physics = GetPhysics();
	return physics ? CopyEntities(physics->OverlapBox(*center, *halfExtents, glm::normalize(ToGlm(*rotation)), layers), out, capacity) : 0;
}

// ---- Time

static void TimeGet(ScriptTime* out)
{
	*out = ScriptTime();
	if (s_Context)
	{
		out->DeltaTime = s_Context->DeltaTime;
		out->FixedDeltaTime = s_Context->FixedDeltaTime;
		out->Time = s_Context->Time;
		out->FrameCount = s_Context->FrameCount;
		out->FixedStepCount = s_Context->FixedStepCount;
	}
}

namespace ScriptBindings
{
	void Bind()
	{
		ScriptNativeApi& api = ScriptHost::GetNativeApi();
		api.Entity_Exists = EntityExists;
		api.Entity_GetName = EntityGetName;
		api.Entity_SetName = EntitySetName;
		api.Entity_GetTranslation = EntityGetTranslation;
		api.Entity_SetTranslation = EntitySetTranslation;
		api.Entity_GetRotation = EntityGetRotation;
		api.Entity_SetRotation = EntitySetRotation;
		api.Entity_GetEulerAngles = EntityGetEulerAngles;
		api.Entity_SetEulerAngles = EntitySetEulerAngles;
		api.Entity_GetScale = EntityGetScale;
		api.Entity_SetScale = EntitySetScale;
		api.Entity_GetWorldTransform = EntityGetWorldTransform;
		api.Entity_HasTag = EntityHasTag;
		api.Entity_AddTag = EntityAddTag;
		api.Entity_RemoveTag = EntityRemoveTag;
		api.Entity_HasComponent = EntityHasComponent;
		api.Entity_Destroy = EntityDestroy;
		api.Scene_FindEntityByName = SceneFindEntityByName;
		api.Scene_FindEntitiesWithTag = SceneFindEntitiesWithTag;
		api.Input_GetAction = InputGetAction;
		api.Rigidbody_AddForce = RigidbodyAddForce;
		api.Rigidbody_AddTorque = RigidbodyAddTorque;
		api.Rigidbody_GetLinearVelocity = RigidbodyGetLinearVelocity;
		api.Rigidbody_SetLinearVelocity = RigidbodySetLinearVelocity;
		api.Rigidbody_GetAngularVelocity = RigidbodyGetAngularVelocity;
		api.Rigidbody_SetAngularVelocity = RigidbodySetAngularVelocity;
		api.Character_SetVelocity = CharacterSetVelocity;
		api.Character_GetState = CharacterGetState;
		api.Physics_Raycast = PhysicsRaycast;
		api.Physics_SphereCast = PhysicsSphereCast;
		api.Physics_OverlapSphere = PhysicsOverlapSphere;
		api.Physics_OverlapBox = PhysicsOverlapBox;
		api.Time_Get = TimeGet;
	}

	void SetContext(ScriptContext* context)
	{
		s_Context = context;
	}

	// An entity or one of its children with a model
	static bool HasModel(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity)
	{
		if (scene.Has<ModelComponent>(entity))
		{
			return true;
		}
		for (EnvMapVulkanEntityID child : scene.GetChildren(entity))
		{
			if (HasModel(scene, child))
			{
				return true;
			}
		}
		return false;
	}

	void DestroyQueued(ScriptContext& context)
	{
		if (context.DestroyQueue.empty() || !context.Scene)
		{
			context.DestroyQueue.clear();
			return;
		}
		const std::vector<EnvMapVulkanEntityID> queue = std::move(context.DestroyQueue);
		context.DestroyQueue.clear();

		// A model's buffers may still be in use by the frame on the GPU (the editor waits the same way when it deletes one)
		const bool models = std::any_of(queue.begin(), queue.end(), [&](EnvMapVulkanEntityID entity)
		{
			return context.Scene->Exists(entity) && HasModel(*context.Scene, entity);
		});
		if (models)
		{
			EnvMapVulkanRenderer::WaitIdle();
		}
		for (EnvMapVulkanEntityID entity : queue)
		{
			context.Scene->DestroyEntity(entity); // a child destroyed with its parent first is gone already: nothing happens
		}
	}
}
