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

void ScenePhysics::Start(EnvMapVulkanScene& scene)
{
	Stop();
	m_Scene = &scene;
	m_World = std::make_unique<PhysicsWorld>();

	uint32_t counts[3] = {}; // by JPH::EMotionType: static, kinematic, dynamic
	uint32_t failed = 0;
	scene.Each<ColliderComponent>([&](EnvMapVulkanEntityID entity, ColliderComponent& collider)
	{
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
		if (!scene.Has<ColliderComponent>(entity))
		{
			Log::GetLogger()->warn("Physics: '{0}' has a rigid body but no collider: not simulated", scene.Get<NameComponent>(entity).Name);
		}
	});

	// The bodies added one by one: the broad phase's trees are rebuilt once, for a first step as fast as the next ones
	m_World->GetSystem().OptimizeBroadPhase();
	Log::GetLogger()->info("Physics: {0} bodies ({1} dynamic, {2} kinematic, {3} static){4}", m_Bodies.size(),
		counts[(int)JPH::EMotionType::Dynamic], counts[(int)JPH::EMotionType::Kinematic], counts[(int)JPH::EMotionType::Static],
		failed > 0 ? ", " + std::to_string(failed) + " could not be made" : std::string());
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

void ScenePhysics::WriteTransforms(float alpha)
{
	if (!m_World)
	{
		return;
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
