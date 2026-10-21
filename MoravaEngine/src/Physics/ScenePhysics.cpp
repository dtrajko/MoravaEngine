#include "ScenePhysics.h"

#include "PhysicsLayers.h"
#include "PhysicsMath.h"

#include "EnvMapVulkan/EnvMapVulkanScene.h"
#include "EnvMapVulkan/EnvMapVulkanSceneData.h"

#include "Core/Log.h"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <limits>


ScenePhysics::ScenePhysics() = default;

ScenePhysics::~ScenePhysics()
{
	Stop();
}

// A world matrix as a body has it: where, turned how, and the scale (negative on X when the matrix mirrors)
static void DecomposeWorld(const glm::mat4& matrix, glm::vec3& position, glm::quat& rotation, glm::vec3& scale)
{
	position = glm::vec3(matrix[3]);
	const glm::vec3 x(matrix[0]), y(matrix[1]), z(matrix[2]);
	scale = glm::vec3(glm::length(x), glm::length(y), glm::length(z));
	if (glm::dot(glm::cross(x, y), z) < 0.0f)
	{
		scale.x = -scale.x;
	}
	const auto divisor = [](float s) { return std::abs(s) > 1e-6f ? s : 1e-6f; };
	rotation = glm::normalize(glm::quat_cast(glm::mat3(x / divisor(scale.x), y / divisor(scale.y), z / divisor(scale.z))));
}

static glm::mat4 ComposeWorld(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale)
{
	return glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
}

// The triangles of a model entity's meshes in the entity's own space, as they are drawn (each mesh at its place in the model)
static std::vector<std::array<glm::vec3, 3>> GetModelTriangles(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity)
{
	std::vector<std::array<glm::vec3, 3>> triangles;
	H2M::RefH2M<H2M::ModelH2M> model = EnvMapVulkanSceneData::GetModel(scene, entity);
	if (!model)
	{
		return triangles;
	}
	const auto& meshes = model->GetMeshes();
	for (uint32_t m = 0; m < (uint32_t)meshes.size(); m++)
	{
		const glm::mat4 transform = EnvMapVulkanSceneData::GetMeshTransform(model, meshes[m], glm::mat4(1.0f));
		for (const H2M::TriangleH2M& triangle : model->GetTriangleCache(m))
		{
			triangles.push_back({ glm::vec3(transform * glm::vec4(triangle.V0.Position, 1.0f)),
				glm::vec3(transform * glm::vec4(triangle.V1.Position, 1.0f)), glm::vec3(transform * glm::vec4(triangle.V2.Position, 1.0f)) });
		}
	}
	return triangles;
}

// The box around a model entity's meshes, in its own space (false: no model, or no meshes)
static bool GetModelBounds(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, glm::vec3& boundsMin, glm::vec3& boundsMax)
{
	H2M::RefH2M<H2M::ModelH2M> model = EnvMapVulkanSceneData::GetModel(scene, entity);
	if (!model || model->GetMeshes().empty())
	{
		return false;
	}
	boundsMin = glm::vec3(std::numeric_limits<float>::max());
	boundsMax = glm::vec3(-std::numeric_limits<float>::max());
	for (const H2M::RefH2M<H2M::MeshH2M>& mesh : model->GetMeshes())
	{
		const glm::mat4 transform = EnvMapVulkanSceneData::GetMeshTransform(model, mesh, glm::mat4(1.0f));
		for (int c = 0; c < 8; c++)
		{
			const glm::vec3 corner((c & 1) ? mesh->BoundingBox.Max.x : mesh->BoundingBox.Min.x, (c & 2) ? mesh->BoundingBox.Max.y : mesh->BoundingBox.Min.y,
				(c & 4) ? mesh->BoundingBox.Max.z : mesh->BoundingBox.Min.z);
			const glm::vec3 position = glm::vec3(transform * glm::vec4(corner, 1.0f));
			boundsMin = glm::min(boundsMin, position);
			boundsMax = glm::max(boundsMax, position);
		}
	}
	return true;
}

bool ScenePhysics::FitCollider(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, ColliderComponent& collider)
{
	glm::vec3 boundsMin, boundsMax;
	if (!GetModelBounds(scene, entity, boundsMin, boundsMax))
	{
		return false;
	}
	const glm::vec3 half = glm::max((boundsMax - boundsMin) * 0.5f, glm::vec3(0.001f));
	collider.Offset = (boundsMin + boundsMax) * 0.5f;
	collider.HalfExtents = half;
	switch (collider.Type)
	{
		case ColliderComponent::Shape::Sphere:
			collider.Radius = std::max({ half.x, half.y, half.z });
			break;
		case ColliderComponent::Shape::Capsule:
			// Standing along Y: as wide as the wider of X and Z, the round ends inside the height
			collider.Radius = std::max(half.x, half.z);
			collider.HalfHeight = std::max(half.y - collider.Radius, 0.0f);
			break;
		default:
			break;
	}
	return true;
}

bool ScenePhysics::FitCharacter(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, CharacterControllerComponent& character)
{
	glm::vec3 boundsMin, boundsMax;
	if (!GetModelBounds(scene, entity, boundsMin, boundsMax))
	{
		return false;
	}
	const glm::mat4 world = scene.GetWorldTransform(entity);
	const glm::vec3 scale(glm::length(glm::vec3(world[0])), glm::length(glm::vec3(world[1])), glm::length(glm::vec3(world[2])));
	const glm::vec3 size = (boundsMax - boundsMin) * scale;
	character.Radius = std::max(0.5f * std::max(size.x, size.z), 0.01f);
	character.Height = std::max(size.y, 2.0f * character.Radius);
	character.Offset = (boundsMin + boundsMax) * 0.5f * scale;
	return true;
}

// The collider's shape at a scale (the entity's world scale), with its offset; nullptr when it can't be made (logged)
static JPH::RefConst<JPH::Shape> CreateShape(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, const ColliderComponent& collider,
	ColliderComponent::Shape type, const glm::vec3& scale)
{
	const std::string& name = scene.Get<NameComponent>(entity).Name;
	const glm::vec3 size = glm::abs(scale);
	JPH::ShapeSettings::ShapeResult result;
	bool offset = true; // the mesh shapes are the meshes where they are: no offset
	switch (type)
	{
		case ColliderComponent::Shape::Box:
		{
			const glm::vec3 half = glm::max(collider.HalfExtents * size, glm::vec3(0.001f));
			// The rounding of its edges (faster, more stable contacts) must fit into its smallest side
			const float convexRadius = std::min(JPH::cDefaultConvexRadius, 0.5f * std::min({ half.x, half.y, half.z }));
			result = JPH::BoxShapeSettings(ToJolt(half), convexRadius).Create();
			break;
		}
		case ColliderComponent::Shape::Sphere:
			result = JPH::SphereShapeSettings(std::max(collider.Radius * std::max({ size.x, size.y, size.z }), 0.001f)).Create();
			break;
		case ColliderComponent::Shape::Capsule:
		{
			const float radius = std::max(collider.Radius * std::max(size.x, size.z), 0.001f);
			const float halfHeight = collider.HalfHeight * size.y;
			result = halfHeight > 0.0f ? JPH::CapsuleShapeSettings(halfHeight, radius).Create() : JPH::SphereShapeSettings(radius).Create();
			break;
		}
		case ColliderComponent::Shape::ConvexMesh:
		case ColliderComponent::Shape::TriangleMesh:
		{
			offset = false;
			const std::vector<std::array<glm::vec3, 3>> triangles = GetModelTriangles(scene, entity);
			if (triangles.empty())
			{
				Log::GetLogger()->warn("Physics: '{0}' has no meshes for its {1} collider: no body", name, ColliderComponent::GetShapeName(type));
				return nullptr;
			}
			// A mirroring scale turns the triangles inside out: their corners swap to keep them facing out
			const bool mirrored = scale.x * scale.y * scale.z < 0.0f;
			if (type == ColliderComponent::Shape::ConvexMesh)
			{
				// Every corner once is enough for a hull; a large mesh gives every n-th, which keeps its outline
				const size_t maxPoints = 10000;
				const size_t stride = std::max<size_t>(1, triangles.size() * 3 / maxPoints);
				JPH::Array<JPH::Vec3> points;
				for (size_t i = 0; i < triangles.size() * 3; i += stride)
				{
					points.push_back(ToJolt(triangles[i / 3][i % 3] * scale));
				}
				result = JPH::ConvexHullShapeSettings(points).Create();
			}
			else
			{
				JPH::TriangleList list;
				list.reserve(triangles.size());
				for (const std::array<glm::vec3, 3>& t : triangles)
				{
					const glm::vec3 a = t[0] * scale, b = t[mirrored ? 2 : 1] * scale, c = t[mirrored ? 1 : 2] * scale;
					list.push_back(JPH::Triangle(JPH::Float3(a.x, a.y, a.z), JPH::Float3(b.x, b.y, b.z), JPH::Float3(c.x, c.y, c.z)));
				}
				result = JPH::MeshShapeSettings(list).Create();
			}
			break;
		}
	}
	if (result.HasError())
	{
		Log::GetLogger()->warn("Physics: the {0} collider of '{1}' could not be made ({2}): no body", ColliderComponent::GetShapeName(type), name,
			result.GetError().c_str());
		return nullptr;
	}
	JPH::RefConst<JPH::Shape> shape = result.Get();
	if (offset && collider.Offset != glm::vec3(0.0f))
	{
		JPH::ShapeSettings::ShapeResult moved = JPH::RotatedTranslatedShapeSettings(ToJolt(collider.Offset * scale), JPH::Quat::sIdentity(), shape).Create();
		if (!moved.HasError())
		{
			shape = moved.Get();
		}
	}
	return shape;
}

bool ScenePhysics::CreateBody(Body& body, const ColliderComponent& collider, const RigidBodyComponent* rigidBody)
{
	const std::string& name = m_Scene->Get<NameComponent>(body.Entity).Name;
	glm::vec3 position;
	glm::quat rotation;
	DecomposeWorld(m_Scene->GetWorldTransform(body.Entity), position, rotation, body.Scale);

	body.Motion = JPH::EMotionType::Static;
	if (rigidBody && rigidBody->Type == RigidBodyComponent::BodyType::Dynamic)
	{
		body.Motion = JPH::EMotionType::Dynamic;
	}
	else if (rigidBody && rigidBody->Type == RigidBodyComponent::BodyType::Kinematic)
	{
		body.Motion = JPH::EMotionType::Kinematic;
	}

	// A triangle mesh has no inside, so it can't be given a mass or be pushed out of another body: moving bodies get its hull
	ColliderComponent::Shape type = collider.Type;
	if (type == ColliderComponent::Shape::TriangleMesh && body.Motion != JPH::EMotionType::Static)
	{
		Log::GetLogger()->warn("Physics: '{0}' moves, so its TriangleMesh collider is a ConvexMesh (a triangle mesh only for static bodies)", name);
		type = ColliderComponent::Shape::ConvexMesh;
	}
	JPH::RefConst<JPH::Shape> shape = CreateShape(*m_Scene, body.Entity, collider, type, body.Scale);
	if (!shape)
	{
		return false;
	}

	const JPH::ObjectLayer layer = collider.IsTrigger ? ObjectLayers::Trigger
		: body.Motion == JPH::EMotionType::Static ? ObjectLayers::Static : ObjectLayers::Dynamic;
	JPH::BodyCreationSettings settings(shape, ToJolt(position), ToJolt(rotation), body.Motion, layer);
	settings.mFriction = collider.Friction;
	settings.mRestitution = collider.Restitution;
	settings.mIsSensor = collider.IsTrigger;
	if (rigidBody && body.Motion != JPH::EMotionType::Static)
	{
		// The mass as set; how it spreads (what makes a long plank harder to spin than a cube) from the shape
		settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
		settings.mMassPropertiesOverride.mMass = std::max(rigidBody->Mass, 0.001f);
		settings.mLinearDamping = rigidBody->LinearDamping;
		settings.mAngularDamping = rigidBody->AngularDamping;
		settings.mGravityFactor = rigidBody->GravityFactor;
		settings.mMotionQuality = rigidBody->ContinuousCollision ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;
		JPH::EAllowedDOFs dofs = JPH::EAllowedDOFs::All;
		if (rigidBody->LockRotationX)
		{
			dofs = dofs & ~JPH::EAllowedDOFs::RotationX;
		}
		if (rigidBody->LockRotationY)
		{
			dofs = dofs & ~JPH::EAllowedDOFs::RotationY;
		}
		if (rigidBody->LockRotationZ)
		{
			dofs = dofs & ~JPH::EAllowedDOFs::RotationZ;
		}
		settings.mAllowedDOFs = dofs;
	}

	body.ID = m_World->AddBody(settings);
	if (body.ID.IsInvalid())
	{
		return false;
	}
	body.PreviousPosition = body.Position = position;
	body.PreviousRotation = body.Rotation = rotation;
	body.Written = m_Scene->Get<TransformComponent>(body.Entity);
	return true;
}

void ScenePhysics::CreateCharacter(Character& character, const CharacterControllerComponent& settings)
{
	// A standing capsule (a ball when it is no taller than it is wide), around the entity's position plus the offset
	const float radius = std::max(settings.Radius, 0.01f);
	const float halfHeight = std::max(settings.Height * 0.5f - radius, 0.0f);
	JPH::RefConst<JPH::Shape> shape = halfHeight > 0.0f ? JPH::RefConst<JPH::Shape>(new JPH::CapsuleShape(halfHeight, radius))
		: JPH::RefConst<JPH::Shape>(new JPH::SphereShape(radius));

	JPH::Ref<JPH::CharacterVirtualSettings> controller = new JPH::CharacterVirtualSettings();
	controller->mShape = shape;
	controller->mShapeOffset = ToJolt(settings.Offset);
	controller->mMaxSlopeAngle = glm::radians(glm::clamp(settings.MaxSlopeDegrees, 0.0f, 89.0f));
	controller->mMass = std::max(settings.Mass, 0.001f);
	controller->mMaxStrength = std::max(settings.PushStrength, 0.0f);
	// It stands on what touches its lower round end (below that end's center); a touch higher up is a wall or a ceiling
	controller->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -(settings.Offset.y - halfHeight));
	// A body of the same shape that moves with it, for the rest of the world to collide with
	controller->mInnerBodyShape = shape;
	controller->mInnerBodyLayer = ObjectLayers::Character;

	const glm::vec3 position = glm::vec3(m_Scene->GetWorldTransform(character.Entity)[3]);
	character.Controller = new JPH::CharacterVirtual(controller, ToJolt(position), JPH::Quat::sIdentity(), &m_World->GetSystem());
	character.StepHeight = std::max(settings.StepHeight, 0.0f);
	character.PreviousPosition = character.Position = position;
	character.Written = m_Scene->Get<TransformComponent>(character.Entity).Translation;
}

void ScenePhysics::Start(EnvMapVulkanScene& scene)
{
	Stop();
	m_Scene = &scene;
	m_World = std::make_unique<PhysicsWorld>();

	uint32_t counts[3] = {}; // by JPH::EMotionType: static, kinematic, dynamic
	uint32_t failed = 0;
	scene.Each<ColliderComponent>([&](EnvMapVulkanEntityID entity, ColliderComponent& collider)
	{
		if (scene.Has<CharacterControllerComponent>(entity))
		{
			Log::GetLogger()->warn("Physics: '{0}' is a character: its Collider and Rigid Body aren't used", scene.Get<NameComponent>(entity).Name);
			return;
		}
		Body body;
		body.Entity = entity;
		if (CreateBody(body, collider, scene.TryGet<RigidBodyComponent>(entity)))
		{
			counts[(int)body.Motion]++;
			m_Bodies.push_back(body);
		}
		else
		{
			failed++;
		}
	});
	scene.Each<RigidBodyComponent>([&](EnvMapVulkanEntityID entity, RigidBodyComponent&)
	{
		if (!scene.Has<ColliderComponent>(entity) && !scene.Has<CharacterControllerComponent>(entity))
		{
			Log::GetLogger()->warn("Physics: '{0}' has a rigid body but no collider: not simulated", scene.Get<NameComponent>(entity).Name);
		}
	});

	scene.Each<CharacterControllerComponent>([&](EnvMapVulkanEntityID entity, CharacterControllerComponent& settings)
	{
		Character character;
		character.Entity = entity;
		CreateCharacter(character, settings);
		m_Characters.push_back(character);
	});

	// The bodies added one by one: the broad phase's trees are rebuilt once, for a first step as fast as the next ones
	m_World->GetSystem().OptimizeBroadPhase();
	Log::GetLogger()->info("Physics: {0} bodies ({1} dynamic, {2} kinematic, {3} static), {4} character{5}{6}", m_Bodies.size(),
		counts[(int)JPH::EMotionType::Dynamic], counts[(int)JPH::EMotionType::Kinematic], counts[(int)JPH::EMotionType::Static],
		m_Characters.size(), m_Characters.size() == 1 ? "" : "s", failed > 0 ? ", " + std::to_string(failed) + " could not be made" : std::string());
}

void ScenePhysics::Stop()
{
	if (m_World && m_StepCount > 0)
	{
		Log::GetLogger()->info("Physics: {0} steps, {1:.2f} ms per step on average ({2} bodies awake at the end)", m_StepCount,
			m_StepSeconds * 1000.0 / m_StepCount, m_World->GetActiveBodyCount());
	}
	m_StepCount = 0;
	m_StepSeconds = 0.0;
	m_Characters.clear(); // before the world: they remove their inner bodies from it
	m_Bodies.clear();
	m_World.reset();
	m_Scene = nullptr;
}

void ScenePhysics::Step(float fixedStep)
{
	if (!m_World)
	{
		return;
	}
	const auto stepStart = std::chrono::steady_clock::now();
	JPH::BodyInterface& bodies = m_World->GetBodyInterface();

	// What the game did since the last step
	for (Body& body : m_Bodies)
	{
		if (!m_Scene->Exists(body.Entity))
		{
			continue; // the game removed it (Phase 6: its body goes with it)
		}
		const TransformComponent& transform = m_Scene->Get<TransformComponent>(body.Entity);
		const bool moved = transform.Translation != body.Written.Translation || transform.Rotation != body.Written.Rotation;
		if (body.Motion == JPH::EMotionType::Kinematic)
		{
			// Moved by the game: the body follows in this step, with the speed that takes (and pushes what is in the way).
			// Standing still (and stopped already) it is left alone, so it can sleep and the bodies on it too
			glm::vec3 position, scale;
			glm::quat rotation;
			DecomposeWorld(m_Scene->GetWorldTransform(body.Entity), position, rotation, scale);
			JPH::RVec3 currentPosition;
			JPH::Quat currentRotation;
			bodies.GetPositionAndRotation(body.ID, currentPosition, currentRotation);
			const bool still = (currentPosition - ToJolt(position)).LengthSq() < 1e-10f && std::abs(currentRotation.Dot(ToJolt(rotation))) > 0.999999f &&
				bodies.GetLinearVelocity(body.ID).IsNearZero() && bodies.GetAngularVelocity(body.ID).IsNearZero();
			if (!still)
			{
				bodies.MoveKinematic(body.ID, ToJolt(position), ToJolt(rotation), fixedStep);
			}
		}
		else if (moved)
		{
			// Put somewhere (a dynamic or static body): it is there now, with the speed it had
			glm::vec3 position, scale;
			glm::quat rotation;
			DecomposeWorld(m_Scene->GetWorldTransform(body.Entity), position, rotation, scale);
			bodies.SetPositionAndRotation(body.ID, ToJolt(position), ToJolt(rotation),
				body.Motion == JPH::EMotionType::Static ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);
			body.PreviousPosition = body.Position = position;
			body.PreviousRotation = body.Rotation = rotation;
			body.Written = transform;
		}
	}

	// The characters, before the bodies move (as Jolt's samples do): each with the velocity the game asked for
	JPH::PhysicsSystem& system = m_World->GetSystem();
	for (Character& character : m_Characters)
	{
		if (!m_Scene->Exists(character.Entity))
		{
			continue;
		}
		JPH::CharacterVirtual& controller = *character.Controller;
		if (m_Scene->Get<TransformComponent>(character.Entity).Translation != character.Written)
		{
			// Put somewhere (the editor's gizmo while paused, the game): it is there now
			const glm::vec3 position = glm::vec3(m_Scene->GetWorldTransform(character.Entity)[3]);
			controller.SetPosition(ToJolt(position));
			character.PreviousPosition = character.Position = position;
			character.Written = m_Scene->Get<TransformComponent>(character.Entity).Translation;
		}

		// Not up a slope it can't stand on; on the ground, it moves with the ground (a platform, a box it stands on)
		controller.UpdateGroundVelocity();
		JPH::Vec3 velocity = controller.CancelVelocityTowardsSteepSlopes(ToJolt(character.WantedVelocity));
		if (controller.GetGroundState() == JPH::CharacterBase::EGroundState::OnGround)
		{
			velocity += controller.GetGroundVelocity();
		}
		controller.SetLinearVelocity(velocity);

		// The move: stopped and guided by what it touches, up steps, kept on the floor going down slopes and steps
		JPH::CharacterVirtual::ExtendedUpdateSettings update;
		update.mWalkStairsStepUp = JPH::Vec3(0.0f, character.StepHeight, 0.0f);
		controller.ExtendedUpdate(fixedStep, system.GetGravity(), update, system.GetDefaultBroadPhaseLayerFilter(ObjectLayers::Character),
			system.GetDefaultLayerFilter(ObjectLayers::Character), JPH::BodyFilter(), JPH::ShapeFilter(), m_World->GetTempAllocator());
		character.PreviousPosition = character.Position;
		character.Position = ToGlm(controller.GetPosition());
	}

	m_World->Step(fixedStep);

	for (Body& body : m_Bodies)
	{
		if (body.Motion != JPH::EMotionType::Dynamic)
		{
			continue;
		}
		JPH::RVec3 position;
		JPH::Quat rotation;
		bodies.GetPositionAndRotation(body.ID, position, rotation);
		body.PreviousPosition = body.Position;
		body.PreviousRotation = body.Rotation;
		body.Position = ToGlm(position);
		body.Rotation = ToGlm(rotation);
	}
	m_StepSeconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - stepStart).count();
	m_StepCount++;
}

void ScenePhysics::SetEntityPosition(EnvMapVulkanEntityID entity, const glm::vec3& position)
{
	// The translation alone: the rotation (the game's) isn't decomposed again, so it can't drift
	const EnvMapVulkanEntityID parent = m_Scene->GetParent(entity);
	m_Scene->Get<TransformComponent>(entity).Translation = parent != NoEntity
		? glm::vec3(glm::inverse(m_Scene->GetWorldTransform(parent)) * glm::vec4(position, 1.0f)) : position;
}

void ScenePhysics::SetCharacterVelocity(EnvMapVulkanEntityID entity, const glm::vec3& velocity)
{
	for (Character& character : m_Characters)
	{
		if (character.Entity == entity)
		{
			character.WantedVelocity = velocity;
			return;
		}
	}
}

static ScenePhysics::CharacterGround ToCharacterGround(JPH::CharacterBase::EGroundState state)
{
	switch (state)
	{
		case JPH::CharacterBase::EGroundState::OnGround:      return ScenePhysics::CharacterGround::OnGround;
		case JPH::CharacterBase::EGroundState::OnSteepGround: return ScenePhysics::CharacterGround::OnSteepGround;
		case JPH::CharacterBase::EGroundState::NotSupported:  return ScenePhysics::CharacterGround::NotSupported;
		default:                                              return ScenePhysics::CharacterGround::InAir;
	}
}

bool ScenePhysics::GetCharacterState(EnvMapVulkanEntityID entity, CharacterState& state) const
{
	for (const Character& character : m_Characters)
	{
		if (character.Entity == entity)
		{
			state.Ground = ToCharacterGround(character.Controller->GetGroundState());
			state.Velocity = ToGlm(character.Controller->GetLinearVelocity());
			state.GroundNormal = ToGlm(character.Controller->GetGroundNormal());
			return true;
		}
	}
	return false;
}

const char* ScenePhysics::GetGroundName(CharacterGround ground)
{
	switch (ground)
	{
		case CharacterGround::OnGround:      return "On the ground";
		case CharacterGround::OnSteepGround: return "On a slope too steep: sliding";
		case CharacterGround::NotSupported:  return "Touching something it can't stand on";
		default:                             return "In the air";
	}
}

void ScenePhysics::WriteTransforms(float alpha)
{
	if (!m_World)
	{
		return;
	}
	for (Character& character : m_Characters)
	{
		if (m_Scene->Exists(character.Entity))
		{
			SetEntityPosition(character.Entity, glm::mix(character.PreviousPosition, character.Position, alpha));
			character.Written = m_Scene->Get<TransformComponent>(character.Entity).Translation;
		}
	}
	for (Body& body : m_Bodies)
	{
		if (body.Motion != JPH::EMotionType::Dynamic || !m_Scene->Exists(body.Entity))
		{
			continue;
		}
		const glm::vec3 position = glm::mix(body.PreviousPosition, body.Position, alpha);
		const glm::quat rotation = glm::slerp(body.PreviousRotation, body.Rotation, alpha);
		// In the world: a body under a parent stays where physics puts it, wherever the parent goes
		m_Scene->SetWorldTransform(body.Entity, ComposeWorld(position, rotation, body.Scale));
		body.Written = m_Scene->Get<TransformComponent>(body.Entity);
	}
}
