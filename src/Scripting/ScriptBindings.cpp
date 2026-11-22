#include "ScriptBindings.h"
#include "Assets/AssetFiles.h"

#include "ScriptHost.h"
#include "ScriptRuntime.h"

#include "EnvMapVulkan/EnvMapVulkanPrefab.h"
#include "EnvMapVulkan/EnvMapVulkanRenderer.h"
#include "EnvMapVulkan/EnvMapVulkanScene.h"
#include "EnvMapVulkan/EnvMapVulkanSceneSerializer.h"

#include "Audio/AudioEngine.h"
#include "Animation/SceneAnimation.h"

#include "Audio/SceneAudio.h"

#include "Game/GameSettings.h"

#include "Input/InputSystem.h"

#include "Navigation/NavMesh.h"
#include "Navigation/SceneNavigation.h"

#include "Physics/ScenePhysics.h"

#include "Project/Project.h"

#include "UI/SceneUI.h"

#include "Core/FrameLimiter.h"
#include "Core/Log.h"

#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <system_error>


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
		case ScriptComponentKind::AudioSource:       return scene->Has<AudioSourceComponent>(entity) ? 1 : 0;
		case ScriptComponentKind::AudioListener:     return scene->Has<AudioListenerComponent>(entity) ? 1 : 0;
		case ScriptComponentKind::Camera:            return scene->Has<CameraComponent>(entity) ? 1 : 0;
		case ScriptComponentKind::UIRect:            return scene->Has<UIRectComponent>(entity) ? 1 : 0;
		case ScriptComponentKind::UIText:            return scene->Has<UITextComponent>(entity) ? 1 : 0;
		case ScriptComponentKind::UIImage:           return scene->Has<UIImageComponent>(entity) ? 1 : 0;
		case ScriptComponentKind::UIButton:          return scene->Has<UIButtonComponent>(entity) ? 1 : 0;
		case ScriptComponentKind::UISlider:          return scene->Has<UISliderComponent>(entity) ? 1 : 0;
		case ScriptComponentKind::UIDropdown:        return scene->Has<UIDropdownComponent>(entity) ? 1 : 0;
		case ScriptComponentKind::Animator:          return scene->Has<AnimatorComponent>(entity) ? 1 : 0;
		case ScriptComponentKind::Vehicle:           return scene->Has<VehicleComponent>(entity) ? 1 : 0;
		case ScriptComponentKind::ParticleEmitter:   return scene->Has<ParticleEmitterComponent>(entity) ? 1 : 0;
		case ScriptComponentKind::Trail:             return scene->Has<TrailComponent>(entity) ? 1 : 0;
		case ScriptComponentKind::NavAgent:          return scene->Has<NavAgentComponent>(entity) ? 1 : 0;
		case ScriptComponentKind::NavArea:           return scene->Has<NavAreaComponent>(entity) ? 1 : 0;
		case ScriptComponentKind::NavLink:           return scene->Has<NavLinkComponent>(entity) ? 1 : 0;
		case ScriptComponentKind::Model:             return scene->Has<ModelComponent>(entity) ? 1 : 0;
		case ScriptComponentKind::PointLight:        return scene->Has<PointLightComponent>(entity) ? 1 : 0;
		case ScriptComponentKind::SpotLight:         return scene->Has<SpotLightComponent>(entity) ? 1 : 0;
		case ScriptComponentKind::Decal:             return scene->Has<DecalComponent>(entity) ? 1 : 0;
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

// A file a script names, relative to the project (extension added when it has none): absolute, as the editor keeps paths;
// empty when there is no such file
static std::string FindProjectFile(const std::string& path, const char* extension)
{
	std::filesystem::path stored = std::filesystem::u8path(path);
	if (!stored.has_extension())
	{
		stored += extension;
	}
	const std::string resolved = ProjectPaths::Resolve(stored.generic_u8string());
	return AssetFiles::Exists(resolved) ? ProjectPaths::Normalize(resolved) : std::string();
}

static uint64_t SceneInstantiate(const char16_t* text, int length, const glm::vec3* position, const ScriptQuat* rotation, uint64_t parent)
{
	EnvMapVulkanScene* scene = GetScene();
	if (!scene)
	{
		return NoEntity;
	}
	const std::string path = ToUtf8(text, length);
	const std::string file = FindProjectFile(path, EnvMapVulkanSceneSerializer::PrefabExtension);
	if (file.empty())
	{
		Log::GetLogger()->error("Scene.Instantiate: there is no prefab '{0}' in the project", path);
		return NoEntity;
	}
	if (parent != NoEntity && !scene->Exists(parent))
	{
		Log::GetLogger()->error("Scene.Instantiate: '{0}' was to be placed under an entity that doesn't exist (any more)", path);
		return NoEntity;
	}
	const auto start = std::chrono::steady_clock::now();
	std::string error;
	const EnvMapVulkanEntityID root = EnvMapVulkanPrefab::Instantiate(*scene, file, parent, error);
	if (root == NoEntity)
	{
		Log::GetLogger()->error("Scene.Instantiate: the prefab '{0}' could not be placed: {1}", path, error);
		return NoEntity;
	}
	TransformComponent& transform = scene->Get<TransformComponent>(root);
	transform.Translation = *position;
	transform.Rotation = QuatToEuler(ToGlm(*rotation));

	// What the game runs on them: the bodies, the sounds and the animators, then the scripts (OnCreate may ask for a Rigidbody,
	// play a sound, set an animator's parameter)
	const std::vector<EnvMapVulkanEntityID> entities = scene->GetSubtree(root);
	if (s_Context->Physics)
	{
		s_Context->Physics->AddEntities(entities);
	}
	if (s_Context->Navigation)
	{
		s_Context->Navigation->AddEntities(entities);
	}
	if (s_Context->Audio)
	{
		s_Context->Audio->AddEntities(entities);
	}
	if (s_Context->Animation)
	{
		s_Context->Animation->AddEntities(entities);
	}
	if (s_Context->Scripts)
	{
		s_Context->Scripts->AddEntities(entities);
	}

	// The first of each prefab in the log, with what it took (its models are loaded from their files each time)
	const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
	if (s_Context->InstantiatedPrefabs.insert(file).second)
	{
		Log::GetLogger()->info("Scene.Instantiate: '{0}', {1} entities, {2:.1f} ms (the first one; the next ones in the Engine Details)",
			ProjectPaths::ToStored(file), entities.size(), ms);
	}
	else
	{
		Log::GetLogger()->trace("Scene.Instantiate: '{0}', {1} entities, {2:.1f} ms", ProjectPaths::ToStored(file), entities.size(), ms);
	}
	return root;
}

static int32_t SceneLoad(const char16_t* text, int length)
{
	if (!GetScene())
	{
		return 0;
	}
	std::string file;
	if (text && length > 0)
	{
		const std::string path = ToUtf8(text, length);
		file = FindProjectFile(path, EnvMapVulkanSceneSerializer::FileExtension);
		if (file.empty())
		{
			Log::GetLogger()->error("Scene.Load: there is no scene '{0}' in the project", path);
			return 0;
		}
	}
	s_Context->SceneLoadRequested = true; // the last one asked for in a frame is loaded
	s_Context->SceneLoadPath = file;
	return 1;
}

static int32_t SceneGetName(char16_t* buffer, int capacity)
{
	return s_Context ? CopyString(s_Context->SceneName, buffer, capacity) : 0;
}

static int32_t SceneGetPath(char16_t* buffer, int capacity)
{
	return s_Context ? CopyString(ProjectPaths::ToStored(s_Context->ScenePath), buffer, capacity) : 0;
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
	const InputAction* action = s_Context->Input->GetMap().Find(name);
	if (!action)
	{
		return; // C# says which action is missing
	}
	const InputActionState& state = s_Context->Input->Get(name);
	out->Value = state.Value;
	out->Held = state.Held ? 1 : 0;
	out->Pressed = (s_Context->InFixedStep ? state.PressedSinceFixed : state.PressedThisFrame) ? 1 : 0;
	out->Released = (s_Context->InFixedStep ? state.ReleasedSinceFixed : state.ReleasedThisFrame) ? 1 : 0;
	out->Exists = 1;
	const bool fromBinding = state.Binding >= 0 && state.Binding < (int)action->Bindings.size();
	const InputBindingSource source = fromBinding ? action->Bindings[state.Binding].Source : InputBindingSource::Key;
	out->Gamepad = source == InputBindingSource::GamepadButton || source == InputBindingSource::GamepadAxis ||
		source == InputBindingSource::GamepadStick || source == InputBindingSource::GamepadDPad ? 1 : 0;
}

static void InputSetCursorLocked(int32_t locked)
{
	if (s_Context)
	{
		s_Context->CursorLocked = locked != 0;
	}
}

static int32_t InputGetCursorState()
{
	if (!s_Context)
	{
		return 0;
	}
	return (s_Context->CursorLocked ? 1 : 0) | (s_Context->Input && s_Context->Input->IsMouseCaptured() ? 2 : 0);
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
	out->GroundVelocity = state.GroundVelocity;
	out->GroundAngularVelocity = state.GroundAngularVelocity;
	out->Height = state.Height;
	out->StandingHeight = state.StandingHeight;
	return 1;
}

static int32_t CharacterSetHeight(uint64_t entity, float height)
{
	ScenePhysics* physics = GetPhysics();
	ScenePhysics::CharacterState state;
	if (!physics || !physics->GetCharacterState(entity, state))
	{
		return 0;
	}
	return physics->SetCharacterHeight(entity, height) ? 1 : -1;
}

// ---- Model

static int32_t ModelGetHiddenInMainView(uint64_t entity, int32_t* hidden)
{
	EnvMapVulkanScene* scene = GetScene();
	const ModelComponent* model = scene ? scene->TryGet<ModelComponent>(entity) : nullptr;
	if (!model || !hidden)
	{
		return 0;
	}
	*hidden = model->HiddenInMainView ? 1 : 0;
	return 1;
}

static int32_t ModelSetHiddenInMainView(uint64_t entity, int32_t hidden)
{
	EnvMapVulkanScene* scene = GetScene();
	ModelComponent* model = scene ? scene->TryGet<ModelComponent>(entity) : nullptr;
	if (!model)
	{
		return 0;
	}
	model->HiddenInMainView = hidden != 0;
	return 1;
}

static int32_t ModelGetVisible(uint64_t entity, int32_t* visible)
{
	EnvMapVulkanScene* scene = GetScene();
	const ModelComponent* model = scene ? scene->TryGet<ModelComponent>(entity) : nullptr;
	if (!model || !visible)
	{
		return 0;
	}
	*visible = model->Visible ? 1 : 0;
	return 1;
}

static int32_t ModelSetVisible(uint64_t entity, int32_t visible)
{
	EnvMapVulkanScene* scene = GetScene();
	ModelComponent* model = scene ? scene->TryGet<ModelComponent>(entity) : nullptr;
	if (!model)
	{
		return 0;
	}
	model->Visible = visible != 0;
	return 1;
}

// ---- Vehicle

static int32_t VehicleSetInput(uint64_t entity, const ScriptVehicleInput* input)
{
	ScenePhysics* physics = GetPhysics();
	return physics && physics->SetVehicleInput(entity, { input->Forward, input->Steer, input->Brake, input->Handbrake }) ? 1 : 0;
}

static int32_t VehicleGetState(uint64_t entity, ScriptVehicleState* out)
{
	ScenePhysics* physics = GetPhysics();
	ScenePhysics::VehicleState state;
	if (!physics || !physics->GetVehicleState(entity, state))
	{
		return 0;
	}
	out->Speed = state.Speed;
	out->Rpm = state.Rpm;
	out->Gear = state.Gear;
	out->Wheels = state.Wheels;
	out->WheelsOnGround = state.WheelsOnGround;
	out->Applied = { state.Applied.Forward, state.Applied.Steer, state.Applied.Brake, state.Applied.Handbrake };
	out->LateralSlip = state.LateralSlip;
	out->LongitudinalSlip = state.LongitudinalSlip;
	const EnvMapVulkanScene* scene = GetScene();
	const VehicleComponent* vehicle = scene ? scene->TryGet<VehicleComponent>(entity) : nullptr;
	out->MaxRpm = vehicle ? vehicle->MaxRpm : 0.0f;
	out->ShiftUpRpm = vehicle ? vehicle->ShiftUpRpm : 0.0f;
	out->Manual = state.Manual ? 1 : 0;
	out->Gears = state.Gears;
	out->RevLimit = state.RevLimit;
	return 1;
}

static int32_t VehicleGetWheel(uint64_t entity, int32_t wheel, ScriptWheelState* out)
{
	ScenePhysics* physics = GetPhysics();
	ScenePhysics::VehicleWheelState state;
	if (!physics || !out || wheel < 0 || !physics->GetVehicleWheel(entity, (uint32_t)wheel, state))
	{
		return 0;
	}
	out->Ground = state.Ground == NoEntity ? 0 : (uint64_t)state.Ground;
	out->OnGround = state.OnGround ? 1 : 0;
	out->LateralSlip = state.LateralSlip;
	out->LongitudinalSlip = state.LongitudinalSlip;
	out->SideSlide = state.SideSlide;
	out->SpinSlide = state.SpinSlide;
	out->SlideSpeed = state.SlideSpeed;
	out->Radius = state.Radius;
	out->Point = state.Point;
	out->Normal = state.Normal;
	out->LocalPoint = state.LocalPoint;
	out->LocalNormal = state.LocalNormal;
	return 1;
}

static int32_t VehicleSetManual(uint64_t entity, int32_t manual)
{
	ScenePhysics* physics = GetPhysics();
	return physics && physics->SetVehicleManual(entity, manual != 0) ? 1 : 0;
}

static int32_t VehicleSetGear(uint64_t entity, int32_t gear)
{
	ScenePhysics* physics = GetPhysics();
	return physics && physics->SetVehicleGear(entity, gear) ? 1 : 0;
}

static int32_t VehicleShift(uint64_t entity, int32_t direction)
{
	ScenePhysics* physics = GetPhysics();
	if (!physics)
	{
		return 0;
	}
	switch (physics->ShiftVehicleGear(entity, direction))
	{
	case ScenePhysics::ShiftResult::Shifted: return 1;
	case ScenePhysics::ShiftResult::Refused: return -1;
	default:                                 return 0;
	}
}

static int32_t VehicleSetEngineTorque(uint64_t entity, float times)
{
	ScenePhysics* physics = GetPhysics();
	return physics && physics->SetVehicleEngineTorque(entity, times) ? 1 : 0;
}

static int32_t VehicleSetGrip(uint64_t entity, float front, float rear)
{
	ScenePhysics* physics = GetPhysics();
	return physics && physics->SetVehicleGrip(entity, front, rear) ? 1 : 0;
}

// A project's text file: a path as the project's files write them (Assets/Tracks/X.json), found in the project's folder or
// the game's pack (AssetFiles). Its text is read again on each call: once for its length when the buffer is small
static int32_t AssetReadText(const char16_t* path, int length, char16_t* buffer, int capacity)
{
	std::string text;
	if (!AssetFiles::ReadText(ProjectPaths::Resolve(ToUtf8(path, length)), text))
	{
		return -2;
	}
	return CopyString(text, buffer, capacity);
}

static int32_t VehicleGetControls(uint64_t entity, int32_t* out)
{
	EnvMapVulkanScene* scene = GetScene();
	const VehicleComponent* vehicle = scene ? scene->TryGet<VehicleComponent>(entity) : nullptr;
	if (!vehicle)
	{
		return 0;
	}
	*out = (int32_t)vehicle->Controls;
	return 1;
}

static int32_t VehicleSetControls(uint64_t entity, int32_t controls)
{
	EnvMapVulkanScene* scene = GetScene();
	VehicleComponent* vehicle = scene ? scene->TryGet<VehicleComponent>(entity) : nullptr;
	if (!vehicle)
	{
		return 0;
	}
	vehicle->Controls = controls == 1 ? VehicleComponent::ControlMode::Script : VehicleComponent::ControlMode::Player;
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

// ---- Navigation (E1.3)

// The running scene's navmesh: its Nav Mesh's, when baked (nullptr: none)
static const NavMesh* GetNavMesh()
{
	EnvMapVulkanScene* scene = GetScene();
	if (!scene)
	{
		return nullptr;
	}
	const EnvMapVulkanEntityID entity = scene->FindFirst<NavMeshComponent>();
	if (entity == NoEntity)
	{
		return nullptr;
	}
	const NavMesh* navMesh = scene->Get<NavMeshComponent>(entity).Mesh.get();
	return navMesh && navMesh->IsReady() ? navMesh : nullptr;
}

static int32_t NavigationIsReady()
{
	return GetNavMesh() ? 1 : 0;
}

static int32_t NavigationFindPath(const glm::vec3* from, const glm::vec3* to, glm::vec3* out, int capacity, int32_t* complete)
{
	*complete = 0;
	const NavMesh* navMesh = GetNavMesh();
	std::vector<glm::vec3> corners;
	bool reached = false;
	if (!navMesh || !navMesh->FindPath(*from, *to, corners, &reached))
	{
		return 0;
	}
	std::copy_n(corners.begin(), std::min((int)corners.size(), std::max(capacity, 0)), out);
	*complete = reached ? 1 : 0;
	return (int32_t)corners.size();
}

static int32_t NavigationRaycast(const glm::vec3* from, const glm::vec3* to, glm::vec3* hit)
{
	const NavMesh* navMesh = GetNavMesh();
	*hit = *from;
	return navMesh && navMesh->Raycast(*from, *to, hit) ? 1 : 0;
}

static int32_t NavigationNearestPoint(const glm::vec3* position, float maxDistance, glm::vec3* out)
{
	const NavMesh* navMesh = GetNavMesh();
	return navMesh && navMesh->NearestPoint(*position, std::max(maxDistance, 0.0f), *out) ? 1 : 0;
}

static int32_t NavigationRandomPointAround(const glm::vec3* center, float radius, glm::vec3* out)
{
	const NavMesh* navMesh = GetNavMesh();
	return navMesh && navMesh->RandomPointAround(*center, radius, *out) ? 1 : 0;
}

// ---- NavAgent (E1.4): the running game's agents (SceneNavigation)

static SceneNavigation* GetNavigation()
{
	return s_Context && s_Context->Navigation && s_Context->Navigation->IsRunning() ? s_Context->Navigation : nullptr;
}

static int32_t NavAgentSetDestination(uint64_t entity, const glm::vec3* destination)
{
	SceneNavigation* navigation = GetNavigation();
	bool found = false;
	if (!navigation || !navigation->SetDestination(entity, *destination, &found))
	{
		return 0;
	}
	return found ? 1 : -1;
}

static int32_t NavAgentStop(uint64_t entity)
{
	SceneNavigation* navigation = GetNavigation();
	return navigation && navigation->StopAgent(entity) ? 1 : 0;
}

static int32_t NavAgentGetState(uint64_t entity, ScriptNavAgentState* out)
{
	SceneNavigation* navigation = GetNavigation();
	SceneNavigation::AgentState state;
	if (!navigation || !navigation->GetAgentState(entity, state))
	{
		return 0;
	}
	out->OnNavMesh = state.OnNavMesh ? 1 : 0;
	out->HasDestination = state.HasDestination ? 1 : 0;
	out->PathPending = state.PathPending ? 1 : 0;
	out->PathPartial = state.PathPartial ? 1 : 0;
	out->Arrived = state.Arrived ? 1 : 0;
	out->Stuck = state.Stuck ? 1 : 0;
	out->Destination = state.Destination;
	out->Velocity = state.Velocity;
	out->DesiredVelocity = state.DesiredVelocity;
	out->RemainingDistance = state.RemainingDistance;
	out->Speed = state.Settings.Speed;
	out->Acceleration = state.Settings.Acceleration;
	out->StoppingDistance = state.Settings.StoppingDistance;
	out->OnLink = state.OnLink ? 1 : 0;
	out->LinkType = state.LinkType;
	out->LinkEntity = state.LinkEntity;
	return 1;
}

static int32_t NavAgentSetSettings(uint64_t entity, float speed, float acceleration, float stoppingDistance)
{
	SceneNavigation* navigation = GetNavigation();
	return navigation && navigation->SetSettings(entity, speed, acceleration, stoppingDistance) ? 1 : 0;
}

static int32_t NavAgentGetPath(uint64_t entity, glm::vec3* out, int capacity, int32_t* complete)
{
	*complete = 0;
	SceneNavigation* navigation = GetNavigation();
	std::vector<glm::vec3> corners;
	bool reached = false;
	if (!navigation || !navigation->GetAgentPath(entity, corners, &reached))
	{
		return -1;
	}
	std::copy_n(corners.begin(), std::min((int)corners.size(), std::max(capacity, 0)), out);
	*complete = reached ? 1 : 0;
	return (int32_t)corners.size();
}

// ---- NavArea, NavLink (E1.5): the components, and their states into the navmesh at once

static int32_t NavAreaGet(uint64_t entity, int32_t* type, int32_t* open, float* cost)
{
	EnvMapVulkanScene* scene = GetScene();
	const NavAreaComponent* area = scene && scene->Exists(entity) ? scene->TryGet<NavAreaComponent>(entity) : nullptr;
	if (!area)
	{
		return 0;
	}
	*type = (int32_t)area->Type;
	*open = area->Open ? 1 : 0;
	*cost = area->Cost;
	return 1;
}

static int32_t NavAreaSet(uint64_t entity, int32_t open, float cost)
{
	EnvMapVulkanScene* scene = GetScene();
	NavAreaComponent* area = scene && scene->Exists(entity) ? scene->TryGet<NavAreaComponent>(entity) : nullptr;
	if (!area)
	{
		return 0;
	}
	area->Open = open != 0;
	area->Cost = std::max(cost, 0.01f);
	SceneNavigation::SyncStates(*scene);
	return 1;
}

static int32_t NavLinkGet(uint64_t entity, int32_t* type, int32_t* enabled)
{
	EnvMapVulkanScene* scene = GetScene();
	const NavLinkComponent* link = scene && scene->Exists(entity) ? scene->TryGet<NavLinkComponent>(entity) : nullptr;
	if (!link)
	{
		return 0;
	}
	*type = (int32_t)link->Type;
	*enabled = link->Enabled ? 1 : 0;
	return 1;
}

static int32_t NavLinkSetEnabled(uint64_t entity, int32_t enabled)
{
	EnvMapVulkanScene* scene = GetScene();
	NavLinkComponent* link = scene && scene->Exists(entity) ? scene->TryGet<NavLinkComponent>(entity) : nullptr;
	if (!link)
	{
		return 0;
	}
	link->Enabled = enabled != 0;
	SceneNavigation::SyncStates(*scene);
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
		out->UnscaledDeltaTime = s_Context->UnscaledDeltaTime;
		out->TimeScale = s_Context->TimeScale;
		out->UnscaledTime = s_Context->UnscaledTime;
	}
}

static void TimeSetTimeScale(float scale)
{
	if (s_Context)
	{
		s_Context->TimeScale = scale > 0.0f ? scale : 0.0f; // below 0 (and NaN): standing still
	}
}

// ---- Particles (12.1)

static ParticleEmitterComponent* GetParticleEmitter(uint64_t entity)
{
	EnvMapVulkanScene* scene = GetScene();
	return scene ? scene->TryGet<ParticleEmitterComponent>(entity) : nullptr;
}

static int32_t ParticleEmitterGetSettings(uint64_t entity, ScriptParticleSettings* out)
{
	const ParticleEmitterComponent* emitter = GetParticleEmitter(entity);
	if (!emitter || !out)
	{
		return 0;
	}
	const ParticleEmitterSettings& p = emitter->Settings;
	out->Emitting = p.Emitting ? 1 : 0;
	out->Rate = p.Rate;
	out->RatePerMeter = p.RatePerMeter;
	out->SpeedMin = p.SpeedMin;
	out->SpeedMax = p.SpeedMax;
	out->SizeStart = p.SizeStart;
	out->SizeEnd = p.SizeEnd;
	for (int i = 0; i < 4; i++)
	{
		out->ColorStart[i] = p.ColorStart[i];
		out->ColorEnd[i] = p.ColorEnd[i];
	}
	return 1;
}

static int32_t ParticleEmitterSetSettings(uint64_t entity, const ScriptParticleSettings* value)
{
	ParticleEmitterComponent* emitter = GetParticleEmitter(entity);
	if (!emitter || !value)
	{
		return 0;
	}
	ParticleEmitterSettings& p = emitter->Settings;
	p.Emitting = value->Emitting != 0;
	p.Rate = std::max(value->Rate, 0.0f);
	p.RatePerMeter = std::max(value->RatePerMeter, 0.0f);
	p.SpeedMin = std::max(value->SpeedMin, 0.0f);
	p.SpeedMax = std::max(value->SpeedMax, p.SpeedMin);
	p.SizeStart = std::max(value->SizeStart, 0.0f);
	p.SizeEnd = std::max(value->SizeEnd, 0.0f);
	p.ColorStart = glm::vec4(value->ColorStart[0], value->ColorStart[1], value->ColorStart[2], value->ColorStart[3]);
	p.ColorEnd = glm::vec4(value->ColorEnd[0], value->ColorEnd[1], value->ColorEnd[2], value->ColorEnd[3]);
	return 1;
}

static int32_t ParticleEmitterEmit(uint64_t entity, int32_t count)
{
	ParticleEmitterComponent* emitter = GetParticleEmitter(entity);
	if (!emitter)
	{
		return 0;
	}
	emitter->Burst += (uint32_t)std::max(count, 0);
	return 1;
}

static int32_t ParticleEmitterClear(uint64_t entity)
{
	ParticleEmitterComponent* emitter = GetParticleEmitter(entity);
	if (!emitter)
	{
		return 0;
	}
	emitter->Burst = 0;
	emitter->PlacedBursts.clear();
	emitter->ClearRequested = true;
	return 1;
}

// Particles from a place of the script's (Shooter Phase 2.3: a muzzle's, a hit's)
static int32_t ParticleEmitterEmitBurst(uint64_t entity, int32_t count, const ScriptParticleBurst* burst)
{
	ParticleEmitterComponent* emitter = GetParticleEmitter(entity);
	if (!emitter || !burst)
	{
		return 0;
	}
	if (count <= 0)
	{
		return 1;
	}
	ParticleBurst placed;
	placed.Count = (uint32_t)count;
	placed.Position = glm::vec3(burst->Position[0], burst->Position[1], burst->Position[2]);
	const glm::vec3 direction(burst->Direction[0], burst->Direction[1], burst->Direction[2]);
	placed.Direction = glm::length(direction) > 1e-6f ? glm::normalize(direction) : glm::vec3(0.0f, 1.0f, 0.0f);
	placed.Speed = std::max(burst->Speed, 0.0f);
	placed.Lifetime = std::max(burst->Lifetime, 0.0f);
	// A runaway script doesn't grow the list without end: the renderer takes it each frame
	if (emitter->PlacedBursts.size() < 256)
	{
		emitter->PlacedBursts.push_back(placed);
	}
	return 1;
}

// ---- Lights (Shooter E8)

static int32_t LightGetSettings(uint64_t entity, int32_t kind, ScriptLightSettings* out)
{
	EnvMapVulkanScene* scene = GetScene();
	if (!scene || !out)
	{
		return 0;
	}
	auto color = [out](const glm::vec3& c) { out->Color[0] = c.r; out->Color[1] = c.g; out->Color[2] = c.b; };
	if (kind == 0)
	{
		const PointLightComponent* light = scene->TryGet<PointLightComponent>(entity);
		if (!light)
		{
			return 0;
		}
		out->Enabled = light->Enabled ? 1 : 0;
		color(light->Color);
		out->Intensity = light->Intensity;
		out->Range = light->Range;
		out->InnerAngle = out->OuterAngle = 0.0f;
		out->CastShadows = light->CastShadows ? 1 : 0;
		return 1;
	}
	const SpotLightComponent* light = scene->TryGet<SpotLightComponent>(entity);
	if (kind != 1 || !light)
	{
		return 0;
	}
	out->Enabled = light->Enabled ? 1 : 0;
	color(light->Color);
	out->Intensity = light->Intensity;
	out->Range = light->Range;
	out->InnerAngle = light->InnerAngle;
	out->OuterAngle = light->OuterAngle;
	out->CastShadows = light->CastShadows ? 1 : 0;
	return 1;
}

static int32_t LightSetSettings(uint64_t entity, int32_t kind, const ScriptLightSettings* value)
{
	EnvMapVulkanScene* scene = GetScene();
	if (!scene || !value)
	{
		return 0;
	}
	const glm::vec3 color = glm::max(glm::vec3(value->Color[0], value->Color[1], value->Color[2]), glm::vec3(0.0f));
	if (kind == 0)
	{
		PointLightComponent* light = scene->TryGet<PointLightComponent>(entity);
		if (!light)
		{
			return 0;
		}
		light->Enabled = value->Enabled != 0;
		light->Color = color;
		light->Intensity = std::max(value->Intensity, 0.0f);
		light->Range = std::max(value->Range, 0.01f);
		light->CastShadows = value->CastShadows != 0;
		return 1;
	}
	SpotLightComponent* light = scene->TryGet<SpotLightComponent>(entity);
	if (kind != 1 || !light)
	{
		return 0;
	}
	light->Enabled = value->Enabled != 0;
	light->Color = color;
	light->Intensity = std::max(value->Intensity, 0.0f);
	light->Range = std::max(value->Range, 0.01f);
	light->OuterAngle = std::clamp(value->OuterAngle, 0.1f, 89.0f);
	light->InnerAngle = std::clamp(value->InnerAngle, 0.0f, light->OuterAngle);
	light->CastShadows = value->CastShadows != 0;
	return 1;
}

static int32_t LightsFlash(const glm::vec3* position, const glm::vec3* color, float intensity, float range, float seconds)
{
	EnvMapVulkanScene* scene = GetScene();
	if (!scene || !position || !color)
	{
		return 0;
	}
	FlashLight flash;
	flash.Position = *position;
	flash.Color = glm::max(*color, glm::vec3(0.0f));
	flash.Intensity = std::max(intensity, 0.0f);
	flash.Range = std::max(range, 0.01f);
	flash.Seconds = std::max(seconds, 0.001f);
	scene->GetEffects().AddFlash(flash);
	return 1;
}

// ---- Decals (Shooter E6)

static int32_t DecalGetColor(uint64_t entity, glm::vec4* out)
{
	EnvMapVulkanScene* scene = GetScene();
	const DecalComponent* decal = scene ? scene->TryGet<DecalComponent>(entity) : nullptr;
	if (!decal || !out)
	{
		return 0;
	}
	*out = decal->Settings.Color;
	return 1;
}

static int32_t DecalSetColor(uint64_t entity, const glm::vec4* value)
{
	EnvMapVulkanScene* scene = GetScene();
	DecalComponent* decal = scene ? scene->TryGet<DecalComponent>(entity) : nullptr;
	if (!decal || !value)
	{
		return 0;
	}
	decal->Settings.Color = glm::max(*value, glm::vec4(0.0f));
	return 1;
}

static int32_t DecalsSpawn(const char16_t* texture, int length, const glm::mat4* transform, const glm::vec4* color, float lifetime)
{
	EnvMapVulkanScene* scene = GetScene();
	if (!scene || !transform || !color)
	{
		return 0;
	}
	const std::string name = texture && length > 0 ? ToUtf8(texture, length) : std::string();
	SpawnedDecal decal;
	if (!name.empty())
	{
		decal.Settings.Texture = ProjectPaths::Resolve(name);
	}
	decal.Transform = *transform;
	decal.Settings.Color = glm::max(*color, glm::vec4(0.0f));
	decal.Lifetime = std::max(lifetime, 0.0f);
	scene->GetEffects().SpawnDecal(decal);
	return 1;
}

static void DecalsClear()
{
	if (EnvMapVulkanScene* scene = GetScene())
	{
		scene->GetEffects().Decals.clear();
	}
}

// ---- Trails (12.2)

static TrailComponent* GetTrail(uint64_t entity)
{
	EnvMapVulkanScene* scene = GetScene();
	return scene ? scene->TryGet<TrailComponent>(entity) : nullptr;
}

static int32_t TrailGetSettings(uint64_t entity, ScriptTrailSettings* out)
{
	const TrailComponent* trail = GetTrail(entity);
	if (!trail || !out)
	{
		return 0;
	}
	const TrailSettings& p = trail->Settings;
	out->Emitting = p.Emitting ? 1 : 0;
	out->Width = p.Width;
	out->MinDistance = p.MinDistance;
	out->Lifetime = p.Lifetime;
	for (int i = 0; i < 4; i++)
	{
		out->Color[i] = p.Color[i];
	}
	return 1;
}

static int32_t TrailSetSettings(uint64_t entity, const ScriptTrailSettings* value)
{
	TrailComponent* trail = GetTrail(entity);
	if (!trail || !value)
	{
		return 0;
	}
	TrailSettings& p = trail->Settings;
	p.Emitting = value->Emitting != 0;
	p.Width = value->Width;
	p.MinDistance = value->MinDistance;
	p.Lifetime = value->Lifetime;
	p.Color = glm::vec4(value->Color[0], value->Color[1], value->Color[2], value->Color[3]);
	p.Clamp();
	return 1;
}

static int32_t TrailClear(uint64_t entity)
{
	TrailComponent* trail = GetTrail(entity);
	if (!trail)
	{
		return 0;
	}
	trail->ClearRequested = true;
	return 1;
}

// ---- Audio (5.2)

static AudioSourceComponent* GetAudioSource(uint64_t entity)
{
	EnvMapVulkanScene* scene = GetScene();
	return scene ? scene->TryGet<AudioSourceComponent>(entity) : nullptr;
}

static SceneAudio* GetAudio()
{
	return s_Context ? s_Context->Audio : nullptr;
}

static int32_t AudioSourcePlay(uint64_t entity)
{
	SceneAudio* audio = GetAudio();
	return GetAudioSource(entity) && audio && audio->Play(entity) ? 1 : 0;
}

static int32_t AudioSourcePause(uint64_t entity)
{
	SceneAudio* audio = GetAudio();
	return GetAudioSource(entity) && audio && audio->Pause(entity) ? 1 : 0;
}

static int32_t AudioSourceStop(uint64_t entity)
{
	SceneAudio* audio = GetAudio();
	return GetAudioSource(entity) && audio && audio->StopSound(entity) ? 1 : 0;
}

static int32_t AudioSourceIsPlaying(uint64_t entity)
{
	if (!GetAudioSource(entity))
	{
		return -1;
	}
	SceneAudio* audio = GetAudio();
	return audio && audio->IsPlaying(entity) ? 1 : 0;
}

static int32_t AudioSourceGetSettings(uint64_t entity, ScriptAudioSettings* out)
{
	const AudioSourceComponent* source = GetAudioSource(entity);
	if (!source)
	{
		return 0;
	}
	out->Volume = source->Volume;
	out->Pitch = source->Pitch;
	out->MinDistance = source->MinDistance;
	out->MaxDistance = source->MaxDistance;
	out->Bus = (int32_t)source->Bus;
	out->Loop = source->Loop ? 1 : 0;
	out->Spatial = source->Spatial ? 1 : 0;
	return 1;
}

static int32_t AudioSourceSetSettings(uint64_t entity, const ScriptAudioSettings* value)
{
	AudioSourceComponent* source = GetAudioSource(entity);
	if (!source)
	{
		return 0;
	}
	source->Volume = std::max(value->Volume, 0.0f);
	source->Pitch = std::max(value->Pitch, 0.01f);
	source->MinDistance = std::max(value->MinDistance, 0.01f);
	source->MaxDistance = std::max(value->MaxDistance, source->MinDistance);
	if (value->Bus >= 0 && value->Bus < (int32_t)AudioBus::Count)
	{
		source->Bus = (AudioBus)value->Bus; // a new bus or Spatial: from the next Play
	}
	source->Loop = value->Loop != 0;
	source->Spatial = value->Spatial != 0;
	if (SceneAudio* audio = GetAudio())
	{
		audio->ApplySettings(entity);
	}
	return 1;
}

static int32_t AudioSourceGetClip(uint64_t entity, char16_t* buffer, int capacity)
{
	const AudioSourceComponent* source = GetAudioSource(entity);
	return source ? CopyString(ProjectPaths::ToStored(source->Clip), buffer, capacity) : -2;
}

static int32_t AudioSourceSetClip(uint64_t entity, const char16_t* clip, int length)
{
	AudioSourceComponent* source = GetAudioSource(entity);
	if (!source)
	{
		return 0;
	}
	source->Clip = ProjectPaths::Resolve(ToUtf8(clip, length)); // from the next Play
	return 1;
}

static int32_t AudioListenerGetActive(uint64_t entity, int32_t* out)
{
	EnvMapVulkanScene* scene = GetScene();
	const AudioListenerComponent* listener = scene ? scene->TryGet<AudioListenerComponent>(entity) : nullptr;
	if (!listener)
	{
		return 0;
	}
	*out = listener->Active ? 1 : 0;
	return 1;
}

static int32_t AudioListenerSetActive(uint64_t entity, int32_t active)
{
	EnvMapVulkanScene* scene = GetScene();
	AudioListenerComponent* listener = scene ? scene->TryGet<AudioListenerComponent>(entity) : nullptr;
	if (!listener)
	{
		return 0;
	}
	listener->Active = active != 0;
	return 1;
}

static int32_t AudioPlayOneShot(const char16_t* clip, int length, const glm::vec3* position, int32_t bus, float volume, float pitch,
	float minDistance)
{
	SceneAudio* audio = GetAudio();
	if (!audio || bus < 0 || bus >= (int32_t)AudioBus::Count)
	{
		return 0;
	}
	return audio->PlayOneShot(ProjectPaths::Resolve(ToUtf8(clip, length)), position, (AudioBus)bus, volume, pitch, minDistance) ? 1 : 0;
}

static void AudioGetVolumes(float* out)
{
	const AudioVolumes& volumes = AudioEngine::GetVolumes();
	out[0] = volumes.Master;
	for (int i = 0; i < (int)AudioBus::Count; i++)
	{
		out[1 + i] = volumes.Buses[i];
	}
}

static void AudioSetVolumes(const float* in)
{
	if (!GetAudio())
	{
		return; // only a running game's
	}
	AudioVolumes volumes;
	volumes.Master = std::max(in[0], 0.0f);
	for (int i = 0; i < (int)AudioBus::Count; i++)
	{
		volumes.Buses[i] = std::max(in[1 + i], 0.0f);
	}
	AudioEngine::SetVolumes(volumes);
	if (s_Context && s_Context->Settings)
	{
		s_Context->Settings->EditPlayer().SetVolumes(volumes); // the player's choice (a Settings menu's slider), kept
	}
}

// ---- Camera (6.3)

static int32_t CameraGetSettings(uint64_t entity, ScriptCameraSettings* out)
{
	EnvMapVulkanScene* scene = GetScene();
	const CameraComponent* camera = scene ? scene->TryGet<CameraComponent>(entity) : nullptr;
	if (!camera)
	{
		return 0;
	}
	out->Primary = camera->Primary ? 1 : 0;
	out->FieldOfView = camera->FieldOfView;
	out->NearClip = camera->NearClip;
	out->FarClip = camera->FarClip;
	out->Enabled = camera->Enabled ? 1 : 0;
	out->DrawEvery = camera->DrawEvery;
	out->ViewModelFieldOfView = camera->ViewModelFieldOfView;
	out->NormalFieldOfView = camera->NormalFieldOfView;
	return 1;
}

static int32_t CameraSetSettings(uint64_t entity, const ScriptCameraSettings* value)
{
	EnvMapVulkanScene* scene = GetScene();
	CameraComponent* camera = scene ? scene->TryGet<CameraComponent>(entity) : nullptr;
	if (!camera)
	{
		return 0;
	}
	if (value->Primary != 0 && !camera->Primary)
	{
		// One camera shows the game, as in the Inspector
		scene->Each<CameraComponent>([&](EnvMapVulkanEntityID, CameraComponent& other) { other.Primary = false; });
	}
	camera->Primary = value->Primary != 0;
	camera->FieldOfView = glm::clamp(value->FieldOfView, CameraComponent::MinFieldOfView, CameraComponent::MaxFieldOfView);
	camera->NearClip = std::max(value->NearClip, CameraComponent::MinNearClip);
	camera->FarClip = std::max(value->FarClip, camera->NearClip * 2.0f);
	camera->Enabled = value->Enabled != 0;
	camera->DrawEvery = std::clamp(value->DrawEvery, 1u, 120u); // as the Inspector allows
	camera->ViewModelFieldOfView = glm::clamp(value->ViewModelFieldOfView, CameraComponent::MinFieldOfView, CameraComponent::MaxFieldOfView);
	camera->NormalFieldOfView = value->NormalFieldOfView > 0.0f ?
		glm::clamp(value->NormalFieldOfView, CameraComponent::MinFieldOfView, CameraComponent::MaxFieldOfView) : 0.0f;
	return 1;
}

static int32_t CameraGetTarget(uint64_t entity, char16_t* buffer, int capacity)
{
	EnvMapVulkanScene* scene = GetScene();
	const CameraComponent* camera = scene ? scene->TryGet<CameraComponent>(entity) : nullptr;
	return camera ? CopyString(camera->Target.empty() ? std::string() : ProjectPaths::ToStored(camera->Target), buffer, capacity) : -2;
}

static int32_t CameraSetTarget(uint64_t entity, const char16_t* target, int length)
{
	EnvMapVulkanScene* scene = GetScene();
	CameraComponent* camera = scene ? scene->TryGet<CameraComponent>(entity) : nullptr;
	if (!camera)
	{
		return 0;
	}
	const std::string path = ToUtf8(target, length);
	camera->Target = path.empty() ? std::string() : ProjectPaths::Resolve(path);
	return 1;
}

// ---- Animator (6.4)

static SceneAnimation* GetAnimation()
{
	return s_Context ? s_Context->Animation : nullptr;
}

static int32_t ToScriptResult(SceneAnimation::Result result)
{
	switch (result)
	{
		case SceneAnimation::Result::Done:       return 1;
		case SceneAnimation::Result::NoAnimator: return 0;
		default:                                 return -1;
	}
}

static int32_t AnimatorSetParameter(uint64_t entity, const char16_t* name, int length, int32_t type, float value)
{
	SceneAnimation* animation = GetAnimation();
	if (!animation || type < 0 || type > (int32_t)AnimatorParameterType::Trigger)
	{
		return 0;
	}
	return ToScriptResult(animation->SetParameter(entity, ToUtf8(name, length), (AnimatorParameterType)type, value));
}

static int32_t AnimatorGetParameter(uint64_t entity, const char16_t* name, int length, int32_t type, float* out)
{
	SceneAnimation* animation = GetAnimation();
	if (!animation || type < 0 || type > (int32_t)AnimatorParameterType::Trigger)
	{
		return 0;
	}
	return ToScriptResult(animation->GetParameter(entity, ToUtf8(name, length), (AnimatorParameterType)type, *out));
}

static int32_t AnimatorPlay(uint64_t entity, const char16_t* state, int length, const char16_t* layer, int layerLength, float fade)
{
	SceneAnimation* animation = GetAnimation();
	return animation ? ToScriptResult(animation->Play(entity, ToUtf8(state, length), std::max(fade, 0.0f), ToUtf8(layer, layerLength))) : 0;
}

static int32_t AnimatorGetInfo(uint64_t entity, ScriptAnimatorInfo* out)
{
	SceneAnimation* animation = GetAnimation();
	SceneAnimation::Info info;
	if (!animation || !animation->GetInfo(entity, info))
	{
		return 0;
	}
	out->StateTime = info.StateTime;
	out->NormalizedTime = info.NormalizedTime;
	out->FadeProgress = info.FadeProgress;
	out->InTransition = info.InTransition ? 1 : 0;
	return 1;
}

static int32_t AnimatorGetState(uint64_t entity, const char16_t* layer, int layerLength, char16_t* buffer, int capacity)
{
	SceneAnimation* animation = GetAnimation();
	SceneAnimation::Info info;
	if (!animation || !animation->GetInfo(entity, info))
	{
		return -2;
	}
	const std::string name = ToUtf8(layer, layerLength);
	if (name.empty())
	{
		return CopyString(info.State, buffer, capacity);
	}
	for (const SceneAnimation::LayerInfo& layerInfo : info.Layers)
	{
		if (layerInfo.Name == name)
		{
			return CopyString(layerInfo.State, buffer, capacity);
		}
	}
	return -3;
}

static int32_t AnimatorGetBoneTransform(uint64_t entity, const char16_t* bone, int length, glm::mat4* out)
{
	SceneAnimation* animation = GetAnimation();
	return animation && animation->GetNodeTransform(entity, ToUtf8(bone, length), *out) ? 1 : 0;
}

// ---- Game UI (7.1)

template<typename T>
static T* GetUIComponent(uint64_t entity)
{
	EnvMapVulkanScene* scene = GetScene();
	return scene ? scene->TryGet<T>(entity) : nullptr;
}

static int32_t UIRectGet(uint64_t entity, ScriptUIRect* out)
{
	const UIRectComponent* rect = GetUIComponent<UIRectComponent>(entity);
	if (!rect)
	{
		return 0;
	}
	out->AnchorMin = rect->AnchorMin;
	out->AnchorMax = rect->AnchorMax;
	out->Pivot = rect->Pivot;
	out->Position = rect->Position;
	out->Size = rect->Size;
	out->Visible = rect->Visible ? 1 : 0;
	out->Rotation = rect->Rotation;
	return 1;
}

static int32_t UIRectSet(uint64_t entity, const ScriptUIRect* value)
{
	UIRectComponent* rect = GetUIComponent<UIRectComponent>(entity);
	if (!rect)
	{
		return 0;
	}
	rect->AnchorMin = glm::clamp(value->AnchorMin, glm::vec2(0.0f), glm::vec2(1.0f));
	rect->AnchorMax = glm::clamp(value->AnchorMax, glm::vec2(0.0f), glm::vec2(1.0f));
	rect->Pivot = value->Pivot;
	rect->Position = value->Position;
	rect->Size = value->Size;
	rect->Visible = value->Visible != 0;
	rect->Rotation = std::isfinite(value->Rotation) ? value->Rotation : 0.0f;
	return 1;
}

static int32_t UITextGetText(uint64_t entity, char16_t* buffer, int capacity)
{
	const UITextComponent* text = GetUIComponent<UITextComponent>(entity);
	return text ? CopyString(text->Text, buffer, capacity) : -2;
}

static int32_t UITextSetText(uint64_t entity, const char16_t* value, int length)
{
	UITextComponent* text = GetUIComponent<UITextComponent>(entity);
	if (!text)
	{
		return 0;
	}
	text->Text = ToUtf8(value, length);
	return 1;
}

static int32_t UITextGetStyle(uint64_t entity, ScriptUITextStyle* out)
{
	const UITextComponent* text = GetUIComponent<UITextComponent>(entity);
	if (!text)
	{
		return 0;
	}
	out->Color = text->Color;
	out->Size = text->Size;
	out->LineSpacing = text->LineSpacing;
	out->Align = (int32_t)text->Align;
	out->VerticalAlign = (int32_t)text->VerticalAlign;
	out->Wrap = text->Wrap ? 1 : 0;
	return 1;
}

static int32_t UITextSetStyle(uint64_t entity, const ScriptUITextStyle* value)
{
	UITextComponent* text = GetUIComponent<UITextComponent>(entity);
	if (!text)
	{
		return 0;
	}
	text->Color = value->Color;
	text->Size = std::max(value->Size, 0.0f);
	text->LineSpacing = value->LineSpacing;
	text->Align = (UITextAlign)glm::clamp(value->Align, 0, 2);
	text->VerticalAlign = (UITextVerticalAlign)glm::clamp(value->VerticalAlign, 0, 2);
	text->Wrap = value->Wrap != 0;
	return 1;
}

static int32_t UITextGetFont(uint64_t entity, char16_t* buffer, int capacity)
{
	const UITextComponent* text = GetUIComponent<UITextComponent>(entity);
	return text ? CopyString(text->Font.empty() ? std::string() : ProjectPaths::ToStored(text->Font), buffer, capacity) : -2;
}

static int32_t UITextSetFont(uint64_t entity, const char16_t* font, int length)
{
	UITextComponent* text = GetUIComponent<UITextComponent>(entity);
	if (!text)
	{
		return 0;
	}
	const std::string path = ToUtf8(font, length);
	text->Font = path.empty() ? std::string() : ProjectPaths::Resolve(path);
	return 1;
}

static int32_t UIImageGetColor(uint64_t entity, glm::vec4* out)
{
	const UIImageComponent* image = GetUIComponent<UIImageComponent>(entity);
	if (!image)
	{
		return 0;
	}
	*out = image->Color;
	return 1;
}

static int32_t UIImageSetColor(uint64_t entity, const glm::vec4* value)
{
	UIImageComponent* image = GetUIComponent<UIImageComponent>(entity);
	if (!image)
	{
		return 0;
	}
	image->Color = *value;
	return 1;
}

static int32_t UIImageGetTexture(uint64_t entity, char16_t* buffer, int capacity)
{
	const UIImageComponent* image = GetUIComponent<UIImageComponent>(entity);
	return image ? CopyString(image->Texture.empty() ? std::string() : ProjectPaths::ToStored(image->Texture), buffer, capacity) : -2;
}

static int32_t UIImageSetTexture(uint64_t entity, const char16_t* texture, int length)
{
	UIImageComponent* image = GetUIComponent<UIImageComponent>(entity);
	if (!image)
	{
		return 0;
	}
	const std::string path = ToUtf8(texture, length);
	image->Texture = path.empty() ? std::string() : ProjectPaths::Resolve(path);
	return 1;
}

// ---- Buttons and the selection (7.3)

static int32_t UIButtonGetInteractable(uint64_t entity, int32_t* out)
{
	const UIButtonComponent* button = GetUIComponent<UIButtonComponent>(entity);
	if (!button)
	{
		return 0;
	}
	*out = button->Interactable ? 1 : 0;
	return 1;
}

static int32_t UIButtonSetInteractable(uint64_t entity, int32_t interactable)
{
	UIButtonComponent* button = GetUIComponent<UIButtonComponent>(entity);
	if (!button)
	{
		return 0;
	}
	button->Interactable = interactable != 0;
	return 1;
}

static uint64_t UIGetSelected()
{
	return s_Context && s_Context->UI ? s_Context->UI->GetSelected() : NoEntity;
}

static int32_t UISelect(uint64_t entity)
{
	return s_Context && s_Context->UI && s_Context->UI->Select(entity) ? 1 : 0;
}

static int32_t UIIsPointerOver()
{
	return s_Context && s_Context->UI && s_Context->UI->IsPointerOverUI() ? 1 : 0;
}

// ---- UISlider (Milestone 2)

static int32_t UISliderGet(uint64_t entity, ScriptUISlider* out)
{
	const UISliderComponent* slider = GetUIComponent<UISliderComponent>(entity);
	if (!slider)
	{
		return 0;
	}
	out->Value = slider->Value;
	out->Min = slider->Min;
	out->Max = slider->Max;
	out->Step = slider->Step;
	out->WholeNumbers = slider->WholeNumbers ? 1 : 0;
	out->Interactable = slider->Interactable ? 1 : 0;
	return 1;
}

static int32_t UISliderSet(uint64_t entity, const ScriptUISlider* value)
{
	UISliderComponent* slider = GetUIComponent<UISliderComponent>(entity);
	if (!slider)
	{
		return 0;
	}
	slider->Min = std::isfinite(value->Min) ? value->Min : slider->Min;
	slider->Max = std::isfinite(value->Max) ? value->Max : slider->Max;
	slider->Step = std::isfinite(value->Step) ? std::max(value->Step, 0.0f) : slider->Step;
	slider->WholeNumbers = value->WholeNumbers != 0;
	slider->Interactable = value->Interactable != 0;
	slider->Value = slider->Constrain(value->Value);
	return 1;
}

// ---- UIDropdown (11.4)

static int32_t UIDropdownGet(uint64_t entity, ScriptUIDropdown* out)
{
	const UIDropdownComponent* dropdown = GetUIComponent<UIDropdownComponent>(entity);
	if (!dropdown)
	{
		return 0;
	}
	out->Value = dropdown->Constrain(dropdown->Value);
	out->Count = (int32_t)dropdown->Options.size();
	out->Interactable = dropdown->Interactable ? 1 : 0;
	out->Open = s_Context && s_Context->UI && s_Context->UI->GetOpenDropdown() == entity ? 1 : 0;
	return 1;
}

static int32_t UIDropdownSet(uint64_t entity, const ScriptUIDropdown* value)
{
	UIDropdownComponent* dropdown = GetUIComponent<UIDropdownComponent>(entity);
	if (!dropdown)
	{
		return 0;
	}
	dropdown->Value = dropdown->Constrain(value->Value);
	dropdown->Interactable = value->Interactable != 0;
	if (s_Context && s_Context->UI)
	{
		const bool open = s_Context->UI->GetOpenDropdown() == entity;
		if (value->Open && !open)
		{
			s_Context->UI->OpenDropdown(entity); // not when it can't be selected now, or has no options
		}
		else if (!value->Open && open)
		{
			s_Context->UI->CloseDropdown();
		}
	}
	return 1;
}

static int32_t UIDropdownGetOptions(uint64_t entity, char16_t* buffer, int capacity)
{
	const UIDropdownComponent* dropdown = GetUIComponent<UIDropdownComponent>(entity);
	if (!dropdown)
	{
		return -2;
	}
	std::string joined;
	for (size_t i = 0; i < dropdown->Options.size(); i++)
	{
		joined += (i ? "\n" : "") + dropdown->Options[i];
	}
	return CopyString(joined, buffer, capacity);
}

static int32_t UIDropdownSetOptions(uint64_t entity, const char16_t* options, int length)
{
	UIDropdownComponent* dropdown = GetUIComponent<UIDropdownComponent>(entity);
	if (!dropdown)
	{
		return 0;
	}
	const std::string joined = ToUtf8(options, length);
	dropdown->Options.clear();
	for (size_t begin = 0; length > 0;)
	{
		const size_t end = joined.find('\n', begin);
		dropdown->Options.push_back(joined.substr(begin, end == std::string::npos ? std::string::npos : end - begin));
		if (end == std::string::npos)
		{
			break;
		}
		begin = end + 1;
	}
	dropdown->Value = dropdown->Constrain(dropdown->Value);
	dropdown->HighlightedOption = dropdown->Constrain(dropdown->HighlightedOption);
	if (dropdown->Options.empty() && s_Context && s_Context->UI && s_Context->UI->GetOpenDropdown() == entity)
	{
		s_Context->UI->CloseDropdown();
	}
	return 1;
}

// ---- Application, Settings (Milestone 2)

static void ApplicationQuit()
{
	if (s_Context)
	{
		if (!s_Context->QuitRequested)
		{
			Log::GetLogger()->info("Application.Quit: the game ends after this frame");
		}
		s_Context->QuitRequested = true;
	}
}

static int32_t ApplicationGetFullscreen()
{
	return s_Context && s_Context->Fullscreen ? 1 : 0;
}

static GameSettings* GetSettings()
{
	return s_Context ? s_Context->Settings : nullptr;
}

static void ApplicationSetFullscreen(int32_t fullscreen)
{
	if (s_Context)
	{
		s_Context->FullscreenRequest = fullscreen != 0 ? 1 : 0;
	}
	if (GameSettings* settings = GetSettings())
	{
		settings->EditPlayer().Fullscreen = fullscreen != 0;
	}
}

// ---- The player's settings (11.4)

static int32_t ApplicationGetResolutions(uint32_t* out, int capacity)
{
	if (!s_Context)
	{
		return 0;
	}
	const std::vector<PlayerResolution>& resolutions = s_Context->Resolutions;
	for (int i = 0; i < capacity && i < (int)resolutions.size(); i++)
	{
		out[i * 2] = resolutions[i].Width;
		out[i * 2 + 1] = resolutions[i].Height;
	}
	return (int32_t)resolutions.size();
}

static void ApplicationGetResolution(uint32_t* out)
{
	const PlayerResolution resolution = s_Context ? s_Context->Resolution : PlayerResolution();
	out[0] = resolution.Width;
	out[1] = resolution.Height;
}

static void ApplicationSetResolution(uint32_t width, uint32_t height)
{
	if (!s_Context || width == 0 || height == 0)
	{
		return;
	}
	s_Context->ResolutionRequested = true;
	s_Context->ResolutionRequest = PlayerResolution{ width, height };
	if (GameSettings* settings = GetSettings())
	{
		settings->EditPlayer().Resolution = PlayerResolution{ width, height };
	}
}

static int32_t ApplicationGetVSync()
{
	return s_Context && s_Context->VSync ? 1 : 0;
}

static void ApplicationSetVSync(int32_t vsync)
{
	if (!s_Context)
	{
		return;
	}
	s_Context->VSyncRequested = true;
	s_Context->VSyncRequest = vsync != 0;
	if (GameSettings* settings = GetSettings())
	{
		settings->EditPlayer().VSync = vsync != 0;
	}
}

// The frame rate limits as the player chose them, else as the project has them
static int32_t ApplicationGetFrameRateLimit()
{
	const GameSettings* settings = GetSettings();
	const Project* project = Project::GetOpen();
	return settings && settings->GetPlayer().FrameRateLimit ? *settings->GetPlayer().FrameRateLimit : project ? project->Window.FrameRateLimit : 0;
}

static void ApplicationSetFrameRateLimit(int32_t framesPerSecond)
{
	if (!s_Context)
	{
		return;
	}
	s_Context->FrameRateLimitRequested = true;
	if (GameSettings* settings = GetSettings())
	{
		settings->EditPlayer().FrameRateLimit = std::max(framesPerSecond, 0);
	}
}

static int32_t ApplicationGetBatteryFrameRateLimit()
{
	const GameSettings* settings = GetSettings();
	const Project* project = Project::GetOpen();
	return settings && settings->GetPlayer().BatteryFrameRateLimit ? *settings->GetPlayer().BatteryFrameRateLimit :
		project ? project->Window.BatteryFrameRateLimit : 0;
}

static void ApplicationSetBatteryFrameRateLimit(int32_t framesPerSecond)
{
	if (!s_Context)
	{
		return;
	}
	s_Context->FrameRateLimitRequested = true;
	if (GameSettings* settings = GetSettings())
	{
		settings->EditPlayer().BatteryFrameRateLimit = std::max(framesPerSecond, 0);
	}
}

static int32_t ApplicationIsOnBattery()
{
	return FrameLimiter::IsOnBattery() ? 1 : 0;
}

static int32_t QualityGetLevel()
{
	return (int32_t)(s_Context ? s_Context->Quality : QualityTier::High);
}

static void QualitySetLevel(int32_t level)
{
	if (!s_Context || level < (int32_t)QualityTier::Auto || level > (int32_t)QualityTier::Ultra)
	{
		return;
	}
	const QualityTier tier = (QualityTier)level;
	s_Context->QualityRequested = true;
	s_Context->QualityRequest = tier;
	if (GameSettings* settings = GetSettings())
	{
		PlayerSettings& player = settings->EditPlayer();
		if (tier == QualityTier::Auto)
		{
			player.Quality.reset();
		}
		else
		{
			player.Quality = tier;
		}
	}
}

static int32_t QualityIsAutomatic()
{
	const GameSettings* settings = GetSettings();
	return !settings || !settings->GetPlayer().Quality ? 1 : 0;
}

static int32_t QualityIsRayTracingSupported()
{
	return s_Context && s_Context->RayTracingSupported ? 1 : 0;
}

static const InputAction* FindInputAction(const char16_t* name, int length)
{
	return s_Context && s_Context->Input ? s_Context->Input->GetMap().Find(ToUtf8(name, length)) : nullptr;
}

static int32_t InputGetBindingCount(const char16_t* name, int length)
{
	const InputAction* action = FindInputAction(name, length);
	return action ? (int32_t)action->Bindings.size() : -1;
}

static int32_t InputGetBinding(const char16_t* name, int length, int32_t binding, int32_t part, int32_t* device, char16_t* buffer, int capacity)
{
	const InputAction* action = FindInputAction(name, length);
	if (!action || binding < 0 || binding >= (int32_t)action->Bindings.size() || part < 0 || part > (int32_t)InputBindingPart::Right)
	{
		return -2;
	}
	const InputBinding& found = action->Bindings[binding];
	if (part != (int32_t)InputBindingPart::Whole && found.Source != InputBindingSource::Composite)
	{
		return -2; // only a composite has parts
	}
	*device = (int32_t)InputMap::GetDevice(found);
	return CopyString(InputMap::DescribeBinding(found, (InputBindingPart)part), buffer, capacity);
}

static int32_t InputRebind(const char16_t* name, int length, int32_t binding, int32_t part)
{
	if (!s_Context || !s_Context->Input || part < 0 || part > (int32_t)InputBindingPart::Right)
	{
		return 0;
	}
	return s_Context->Input->StartRebind(ToUtf8(name, length), binding, (InputBindingPart)part) ? 1 : 0;
}

static int32_t InputGetRebindState()
{
	return (int32_t)(s_Context && s_Context->Input ? s_Context->Input->GetRebindState() : InputRebindState::None);
}

static void InputCancelRebind()
{
	if (s_Context && s_Context->Input)
	{
		s_Context->Input->CancelRebind();
	}
}

static void InputResetBindings()
{
	if (!s_Context || !s_Context->Input || !s_Context->ProjectInputMap)
	{
		return;
	}
	s_Context->Input->SetMap(*s_Context->ProjectInputMap);
	if (GameSettings* settings = GetSettings())
	{
		settings->EditPlayer().Bindings.clear();
	}
	Log::GetLogger()->info("Input.ResetBindings: the project's bindings again");
}

static GameSettings* GetSaveData()
{
	return s_Context ? s_Context->SaveData : nullptr;
}

static int32_t SaveDataGet(const char16_t* key, int keyLength, char16_t* buffer, int capacity)
{
	const GameSettings* data = GetSaveData();
	std::string value;
	if (!data || !data->Get(ToUtf8(key, keyLength), value))
	{
		return -2;
	}
	return CopyString(value, buffer, capacity);
}

static void SaveDataSet(const char16_t* key, int keyLength, const char16_t* value, int valueLength)
{
	if (GameSettings* data = GetSaveData())
	{
		data->Set(ToUtf8(key, keyLength), ToUtf8(value, valueLength));
	}
}

static int32_t SaveDataRemove(const char16_t* key, int keyLength)
{
	GameSettings* data = GetSaveData();
	return data && data->Remove(ToUtf8(key, keyLength)) ? 1 : 0;
}

static void SaveDataRemoveAll()
{
	if (GameSettings* data = GetSaveData())
	{
		data->RemoveAll();
	}
}

static int32_t SaveDataSave()
{
	GameSettings* data = GetSaveData();
	return !data || data->Save() ? 1 : 0;
}

static int32_t SaveDataGetFile(char16_t* buffer, int capacity)
{
	const GameSettings* data = GetSaveData();
	return CopyString(data ? data->GetFile() : std::string(), buffer, capacity);
}

static int32_t SettingsGet(const char16_t* key, int keyLength, char16_t* buffer, int capacity)
{
	const GameSettings* settings = GetSettings();
	std::string value;
	if (!settings || !settings->Get(ToUtf8(key, keyLength), value))
	{
		return -2;
	}
	return CopyString(value, buffer, capacity);
}

static void SettingsSet(const char16_t* key, int keyLength, const char16_t* value, int valueLength)
{
	if (GameSettings* settings = GetSettings())
	{
		settings->Set(ToUtf8(key, keyLength), ToUtf8(value, valueLength));
	}
}

static int32_t SettingsRemove(const char16_t* key, int keyLength)
{
	GameSettings* settings = GetSettings();
	return settings && settings->Remove(ToUtf8(key, keyLength)) ? 1 : 0;
}

static int32_t SettingsSave()
{
	GameSettings* settings = GetSettings();
	return !settings || settings->Save() ? 1 : 0;
}

static int32_t SettingsGetFile(char16_t* buffer, int capacity)
{
	const GameSettings* settings = GetSettings();
	return CopyString(settings ? settings->GetFile() : std::string(), buffer, capacity);
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
		api.AudioSource_Play = AudioSourcePlay;
		api.AudioSource_Pause = AudioSourcePause;
		api.AudioSource_Stop = AudioSourceStop;
		api.AudioSource_IsPlaying = AudioSourceIsPlaying;
		api.AudioSource_GetSettings = AudioSourceGetSettings;
		api.AudioSource_SetSettings = AudioSourceSetSettings;
		api.AudioSource_GetClip = AudioSourceGetClip;
		api.AudioSource_SetClip = AudioSourceSetClip;
		api.AudioListener_GetActive = AudioListenerGetActive;
		api.AudioListener_SetActive = AudioListenerSetActive;
		api.Audio_PlayOneShot = AudioPlayOneShot;
		api.Audio_GetVolumes = AudioGetVolumes;
		api.Audio_SetVolumes = AudioSetVolumes;
		api.Camera_GetSettings = CameraGetSettings;
		api.Camera_SetSettings = CameraSetSettings;
		api.Scene_Instantiate = SceneInstantiate;
		api.Scene_Load = SceneLoad;
		api.Scene_GetName = SceneGetName;
		api.Scene_GetPath = SceneGetPath;
		api.Animator_SetParameter = AnimatorSetParameter;
		api.Animator_GetParameter = AnimatorGetParameter;
		api.Animator_Play = AnimatorPlay;
		api.Animator_GetInfo = AnimatorGetInfo;
		api.Animator_GetState = AnimatorGetState;
	api.Animator_GetBoneTransform = AnimatorGetBoneTransform;
		api.UIRect_Get = UIRectGet;
		api.UIRect_Set = UIRectSet;
		api.UIText_GetText = UITextGetText;
		api.UIText_SetText = UITextSetText;
		api.UIText_GetStyle = UITextGetStyle;
		api.UIText_SetStyle = UITextSetStyle;
		api.UIText_GetFont = UITextGetFont;
		api.UIText_SetFont = UITextSetFont;
		api.UIImage_GetColor = UIImageGetColor;
		api.UIImage_SetColor = UIImageSetColor;
		api.UIImage_GetTexture = UIImageGetTexture;
		api.UIImage_SetTexture = UIImageSetTexture;
		api.Input_SetCursorLocked = InputSetCursorLocked;
		api.Input_GetCursorState = InputGetCursorState;
		api.UIButton_GetInteractable = UIButtonGetInteractable;
		api.UIButton_SetInteractable = UIButtonSetInteractable;
		api.UI_GetSelected = UIGetSelected;
		api.UI_Select = UISelect;
		api.UI_IsPointerOver = UIIsPointerOver;
		api.Time_SetTimeScale = TimeSetTimeScale;
		api.Camera_GetTarget = CameraGetTarget;
		api.Camera_SetTarget = CameraSetTarget;
		api.Application_Quit = ApplicationQuit;
		api.Settings_Get = SettingsGet;
		api.Settings_Set = SettingsSet;
		api.Settings_Remove = SettingsRemove;
		api.Settings_Save = SettingsSave;
		api.Settings_GetFile = SettingsGetFile;
		api.UISlider_Get = UISliderGet;
		api.UISlider_Set = UISliderSet;
		api.Application_GetFullscreen = ApplicationGetFullscreen;
		api.Application_SetFullscreen = ApplicationSetFullscreen;
		api.Application_GetResolutions = ApplicationGetResolutions;
		api.Application_GetResolution = ApplicationGetResolution;
		api.Application_SetResolution = ApplicationSetResolution;
		api.Application_GetVSync = ApplicationGetVSync;
		api.Application_SetVSync = ApplicationSetVSync;
		api.Application_GetFrameRateLimit = ApplicationGetFrameRateLimit;
		api.Application_SetFrameRateLimit = ApplicationSetFrameRateLimit;
		api.Application_GetBatteryFrameRateLimit = ApplicationGetBatteryFrameRateLimit;
		api.Application_SetBatteryFrameRateLimit = ApplicationSetBatteryFrameRateLimit;
		api.Application_IsOnBattery = ApplicationIsOnBattery;
		api.Quality_GetLevel = QualityGetLevel;
		api.Quality_SetLevel = QualitySetLevel;
		api.Quality_IsAutomatic = QualityIsAutomatic;
		api.Quality_IsRayTracingSupported = QualityIsRayTracingSupported;
		api.Input_GetBindingCount = InputGetBindingCount;
		api.Input_GetBinding = InputGetBinding;
		api.Input_Rebind = InputRebind;
		api.Input_GetRebindState = InputGetRebindState;
		api.Input_CancelRebind = InputCancelRebind;
		api.Input_ResetBindings = InputResetBindings;
		api.SaveData_Get = SaveDataGet;
		api.SaveData_Set = SaveDataSet;
		api.SaveData_Remove = SaveDataRemove;
		api.SaveData_RemoveAll = SaveDataRemoveAll;
		api.SaveData_Save = SaveDataSave;
		api.SaveData_GetFile = SaveDataGetFile;
		api.UIDropdown_Get = UIDropdownGet;
		api.UIDropdown_Set = UIDropdownSet;
		api.UIDropdown_GetOptions = UIDropdownGetOptions;
		api.UIDropdown_SetOptions = UIDropdownSetOptions;
		api.Vehicle_SetInput = VehicleSetInput;
		api.Vehicle_GetState = VehicleGetState;
		api.Vehicle_GetControls = VehicleGetControls;
		api.Vehicle_SetControls = VehicleSetControls;
		api.Vehicle_SetGrip = VehicleSetGrip;
		api.Asset_ReadText = AssetReadText;
		api.Vehicle_SetManual = VehicleSetManual;
		api.Vehicle_Shift = VehicleShift;
		api.Vehicle_SetGear = VehicleSetGear;
		api.Vehicle_SetEngineTorque = VehicleSetEngineTorque;
		api.ParticleEmitter_GetSettings = ParticleEmitterGetSettings;
		api.ParticleEmitter_SetSettings = ParticleEmitterSetSettings;
		api.ParticleEmitter_Emit = ParticleEmitterEmit;
		api.ParticleEmitter_Clear = ParticleEmitterClear;
		api.Trail_GetSettings = TrailGetSettings;
		api.Trail_SetSettings = TrailSetSettings;
		api.Trail_Clear = TrailClear;
		api.Vehicle_GetWheel = VehicleGetWheel;
		api.Navigation_IsReady = NavigationIsReady;
		api.Navigation_FindPath = NavigationFindPath;
		api.Navigation_Raycast = NavigationRaycast;
		api.Navigation_NearestPoint = NavigationNearestPoint;
		api.Navigation_RandomPointAround = NavigationRandomPointAround;
		api.NavAgent_SetDestination = NavAgentSetDestination;
		api.NavAgent_Stop = NavAgentStop;
		api.NavAgent_GetState = NavAgentGetState;
		api.NavAgent_SetSettings = NavAgentSetSettings;
		api.NavAgent_GetPath = NavAgentGetPath;
		api.NavArea_Get = NavAreaGet;
		api.NavArea_Set = NavAreaSet;
		api.NavLink_Get = NavLinkGet;
		api.NavLink_SetEnabled = NavLinkSetEnabled;
		api.Character_SetHeight = CharacterSetHeight;
		api.Model_GetHiddenInMainView = ModelGetHiddenInMainView;
		api.Model_SetHiddenInMainView = ModelSetHiddenInMainView;
		api.Model_GetVisible = ModelGetVisible;
		api.Model_SetVisible = ModelSetVisible;
		api.ParticleEmitter_EmitBurst = ParticleEmitterEmitBurst;
		api.Light_GetSettings = LightGetSettings;
		api.Light_SetSettings = LightSetSettings;
		api.Lights_Flash = LightsFlash;
		api.Decal_GetColor = DecalGetColor;
		api.Decal_SetColor = DecalSetColor;
		api.Decals_Spawn = DecalsSpawn;
		api.Decals_Clear = DecalsClear;
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
