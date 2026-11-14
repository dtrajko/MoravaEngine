#include "ScenePhysics.h"

#include "PhysicsLayers.h"
#include "PhysicsMath.h"

#include "EnvMapVulkan/EnvMapVulkanMaterialLibrary.h" // a terrain's layer materials (TerrainComponent)
#include "EnvMapVulkan/EnvMapVulkanScene.h"
#include "EnvMapVulkan/EnvMapVulkanSceneData.h"
#include "EnvMapVulkan/EnvMapVulkanTerrain.h"

#include "Core/Log.h"

#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <limits>
#include <mutex>


/**
 * Jolt's contact callbacks, which run on its worker threads during the step, several at once: they only write down what
 * happened (behind a mutex), and ScenePhysics turns it into events on the main thread after the step (DispatchContacts).
 * A contact is between two sub-shapes (a mesh's triangle, the body's shape for the others): added when they start
 * touching, removed when they stop.
 */
class ScenePhysicsContactListener : public JPH::ContactListener
{
public:
	struct Change
	{
		JPH::BodyID A, B;
		int Delta = 0;                                        // +1 added, -1 removed
		glm::vec3 Point = glm::vec3(0.0f), Normal = glm::vec3(0.0f); // added: where, and from A towards B
	};
	struct Contact
	{
		glm::vec3 Point, Normal;
	};

	std::mutex Mutex;
	std::vector<Change> Changes;   // since the last DispatchContacts
	std::vector<Contact> Contacts; // the last step's (while Collect)
	bool Collect = false;          // set between the steps only

	virtual void OnContactAdded(const JPH::Body& a, const JPH::Body& b, const JPH::ContactManifold& manifold, JPH::ContactSettings&) override
	{
		Change change;
		change.A = a.GetID();
		change.B = b.GetID();
		change.Delta = 1;
		change.Point = ToGlm(manifold.mRelativeContactPointsOn1.empty() ? manifold.mBaseOffset : manifold.GetWorldSpaceContactPointOn1(0));
		change.Normal = ToGlm(manifold.mWorldSpaceNormal);
		std::lock_guard<std::mutex> lock(Mutex);
		Changes.push_back(change);
		AddContacts(manifold);
	}

	virtual void OnContactPersisted(const JPH::Body&, const JPH::Body&, const JPH::ContactManifold& manifold, JPH::ContactSettings&) override
	{
		if (Collect)
		{
			std::lock_guard<std::mutex> lock(Mutex);
			AddContacts(manifold);
		}
	}

	virtual void OnContactRemoved(const JPH::SubShapeIDPair& pair) override
	{
		Change change;
		change.A = pair.GetBody1ID();
		change.B = pair.GetBody2ID();
		change.Delta = -1;
		std::lock_guard<std::mutex> lock(Mutex);
		Changes.push_back(change);
	}

private:
	void AddContacts(const JPH::ContactManifold& manifold) // under the mutex
	{
		if (!Collect)
		{
			return;
		}
		for (JPH::uint i = 0; i < (JPH::uint)manifold.mRelativeContactPointsOn1.size(); i++)
		{
			Contacts.push_back({ ToGlm(manifold.GetWorldSpaceContactPointOn1(i)), ToGlm(manifold.mWorldSpaceNormal) });
		}
	}
};

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

// The entities a vehicle draws as its wheels (VehicleComponent::Wheel::Entity) of the vehicle entity, or of any (NoEntity)
static bool IsVehicleWheel(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, EnvMapVulkanEntityID vehicleEntity = NoEntity)
{
	bool found = false;
	scene.Each<VehicleComponent>([&](EnvMapVulkanEntityID owner, const VehicleComponent& vehicle)
	{
		if (vehicleEntity == NoEntity || owner == vehicleEntity)
		{
			for (const VehicleComponent::Wheel& wheel : vehicle.Wheels)
			{
				found |= wheel.Entity == entity;
			}
		}
	});
	return found;
}

// The box around the models under an entity without a model of its own (an Empty: a vehicle's root, its body a child),
// in the entity's own space; a vehicle's wheels aren't in it (they aren't its body). False: no models under it
static bool GetChildModelBounds(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, glm::vec3& boundsMin, glm::vec3& boundsMax)
{
	const glm::mat4 toEntity = glm::inverse(scene.GetWorldTransform(entity));
	boundsMin = glm::vec3(std::numeric_limits<float>::max());
	boundsMax = glm::vec3(-std::numeric_limits<float>::max());
	bool any = false;
	for (EnvMapVulkanEntityID child : scene.GetSubtree(entity))
	{
		glm::vec3 childMin, childMax;
		if (child == entity || IsVehicleWheel(scene, child, entity) || !GetModelBounds(scene, child, childMin, childMax))
		{
			continue;
		}
		const glm::mat4 transform = toEntity * scene.GetWorldTransform(child);
		for (int c = 0; c < 8; c++)
		{
			const glm::vec3 corner((c & 1) ? childMax.x : childMin.x, (c & 2) ? childMax.y : childMin.y, (c & 4) ? childMax.z : childMin.z);
			const glm::vec3 position = glm::vec3(transform * glm::vec4(corner, 1.0f));
			boundsMin = glm::min(boundsMin, position);
			boundsMax = glm::max(boundsMax, position);
		}
		any = true;
	}
	return any;
}

bool ScenePhysics::FitCollider(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, ColliderComponent& collider)
{
	glm::vec3 boundsMin, boundsMax;
	const bool ownModel = (bool)EnvMapVulkanSceneData::GetModel(scene, entity);
	if (ownModel ? !GetModelBounds(scene, entity, boundsMin, boundsMax) : !GetChildModelBounds(scene, entity, boundsMin, boundsMax))
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

bool ScenePhysics::CreateTerrainBody(Body& body, TerrainComponent& terrain)
{
	const std::string& name = m_Scene->Get<NameComponent>(body.Entity).Name;
	// Its heights as the renderer has them (the same settings give the same heights), or made now: a game can start before
	// the first frame is drawn
	EnvMapVulkanTerrain::UpdateHeights(*m_Scene, body.Entity, terrain);
	const TerrainHeights& heights = *terrain.Heights;
	if (heights.SampleCount < 2)
	{
		return false;
	}
	// Jolt's height field: offset + scale * (x, height, z) at sample (x, z), its squares split as the chunks' are
	const float half = heights.Size * 0.5f;
	JPH::HeightFieldShapeSettings settings(heights.Heights.data(), JPH::Vec3(-half, 0.0f, -half), JPH::Vec3(heights.Spacing, 1.0f, heights.Spacing),
		heights.SampleCount);
	JPH::ShapeSettings::ShapeResult result = settings.Create();
	if (result.HasError())
	{
		Log::GetLogger()->warn("Physics: the terrain '{0}' could not be made ({1}): no body", name, result.GetError().c_str());
		return false;
	}

	// Where the entity is: a terrain isn't turned or scaled
	const glm::vec3 position = glm::vec3(m_Scene->GetWorldTransform(body.Entity)[3]);
	body.Motion = JPH::EMotionType::Static;
	body.Scale = glm::vec3(1.0f);
	JPH::BodyCreationSettings bodySettings(result.Get(), ToJolt(position), JPH::Quat::sIdentity(), JPH::EMotionType::Static, ObjectLayers::Static);
	bodySettings.mFriction = terrain.Settings.Friction;
	body.ID = m_World->AddBody(bodySettings);
	if (body.ID.IsInvalid())
	{
		return false;
	}
	body.PreviousPosition = body.Position = position;
	body.Written = m_Scene->Get<TransformComponent>(body.Entity);
	return true;
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
	// A vehicle's mass sits lower than its shape's middle (VehicleComponent::CenterOfMassOffset): it doesn't roll over in turns
	if (const VehicleComponent* vehicle = m_Scene->TryGet<VehicleComponent>(body.Entity);
		vehicle && body.Motion == JPH::EMotionType::Dynamic && !collider.IsTrigger && vehicle->CenterOfMassOffset != glm::vec3(0.0f))
	{
		JPH::ShapeSettings::ShapeResult offset = JPH::OffsetCenterOfMassShapeSettings(ToJolt(vehicle->CenterOfMassOffset), shape).Create();
		if (!offset.HasError())
		{
			shape = offset.Get();
		}
	}

	// A trigger that doesn't move by itself is kinematic and never sleeps: a static sensor loses what is in it when that goes
	// to sleep (a character standing still in it would leave), and only a kinematic one senses kinematic bodies (a
	// character's inner body)
	body.Trigger = collider.IsTrigger;
	if (body.Trigger && body.Motion == JPH::EMotionType::Static)
	{
		body.Motion = JPH::EMotionType::Kinematic;
	}
	const JPH::ObjectLayer layer = body.Trigger ? ObjectLayers::Trigger
		: body.Motion == JPH::EMotionType::Static ? ObjectLayers::Static : ObjectLayers::Dynamic;
	JPH::BodyCreationSettings settings(shape, ToJolt(position), ToJolt(rotation), body.Motion, layer);
	settings.mFriction = collider.Friction;
	settings.mRestitution = collider.Restitution;
	settings.mIsSensor = body.Trigger;
	if (body.Trigger)
	{
		settings.mAllowSleeping = false;
		settings.mCollideKinematicVsNonDynamic = true; // kinematic bodies, and static ones when the project's matrix says so
	}
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

// ---- Vehicles

// A vehicle's forward and up in its own space: forward is -Z, as a camera looks (and so its right is +X, its left -X)
static const JPH::Vec3 VehicleForward(0.0f, 0.0f, -1.0f), VehicleUp(0.0f, 1.0f, 0.0f);

// The wheels across from each other among some of a vehicle's wheels (indices): pairs at about the same Z on opposite sides,
// the left (-X) one first; a wheel without a partner is alone (second: -1)
static std::vector<std::pair<int, int>> GetAxles(const std::vector<VehicleComponent::Wheel>& wheels, const std::vector<int>& indices)
{
	std::vector<std::pair<int, int>> axles;
	std::vector<bool> used(wheels.size(), false);
	for (int i : indices)
	{
		if (used[i])
		{
			continue;
		}
		used[i] = true;
		int partner = -1;
		float closest = FLT_MAX;
		for (int j : indices)
		{
			const float dz = std::abs(wheels[j].Position.z - wheels[i].Position.z);
			if (!used[j] && (wheels[j].Position.x < 0.0f) != (wheels[i].Position.x < 0.0f) && dz < closest && dz < std::max(wheels[i].Radius, 0.1f))
			{
				closest = dz;
				partner = j;
			}
		}
		if (partner < 0)
		{
			axles.push_back({ i, -1 });
			continue;
		}
		used[partner] = true;
		axles.push_back(wheels[i].Position.x < wheels[partner].Position.x ? std::make_pair(i, partner) : std::make_pair(partner, i));
	}
	return axles;
}

static glm::mat4 ToGlm(const JPH::Mat44& m)
{
	glm::mat4 result;
	for (int c = 0; c < 4; c++)
	{
		const JPH::Vec4 column = m.GetColumn4(c);
		result[c] = glm::vec4(column.GetX(), column.GetY(), column.GetZ(), column.GetW());
	}
	return result;
}

bool ScenePhysics::CreateVehicle(Vehicle& vehicle, const VehicleComponent& settings)
{
	const std::string& name = m_Scene->Get<NameComponent>(vehicle.Entity).Name;
	if (settings.Wheels.empty())
	{
		Log::GetLogger()->warn("Physics: the vehicle '{0}' has no wheels: a plain body (Fit Wheels in the Inspector)", name);
		return false;
	}

	JPH::VehicleConstraintSettings constraint;
	constraint.mUp = VehicleUp;
	constraint.mForward = VehicleForward;
	constraint.mMaxPitchRollAngle = glm::radians(glm::clamp(settings.MaxTilt, 1.0f, 180.0f));

	// The wheels. Jolt's wheel hangs from where its spring starts (mPosition), the spring's full length above its center
	// when fully out: the place the wheel has in the editor
	const float travel = std::max(settings.SuspensionTravel, 0.01f);
	for (const VehicleComponent::Wheel& wheel : settings.Wheels)
	{
		JPH::Ref<JPH::WheelSettingsWV> joltWheel = new JPH::WheelSettingsWV();
		joltWheel->mPosition = ToJolt(wheel.Position) + VehicleUp * travel;
		joltWheel->mSuspensionDirection = -VehicleUp;
		joltWheel->mSteeringAxis = VehicleUp;
		joltWheel->mWheelUp = VehicleUp;
		joltWheel->mWheelForward = VehicleForward;
		joltWheel->mSuspensionMinLength = 0.0f;
		joltWheel->mSuspensionMaxLength = travel;
		joltWheel->mSuspensionSpring.mFrequency = std::max(settings.SuspensionFrequency, 0.1f);
		joltWheel->mSuspensionSpring.mDamping = std::max(settings.SuspensionDamping, 0.0f);
		joltWheel->mRadius = std::max(wheel.Radius, 0.01f);
		joltWheel->mWidth = std::max(wheel.Width, 0.01f);
		// A 20 kg wheel, as Jolt's defaults are (0.9 kg m^2 at 0.3 m): a solid disc
		joltWheel->mInertia = 0.5f * 20.0f * joltWheel->mRadius * joltWheel->mRadius;
		joltWheel->mMaxSteerAngle = wheel.Steers ? glm::radians(glm::clamp(settings.MaxSteerAngle, 0.0f, 89.0f)) : 0.0f;
		joltWheel->mMaxBrakeTorque = std::max(settings.BrakeTorque, 0.0f);
		joltWheel->mMaxHandBrakeTorque = wheel.Handbrake ? std::max(settings.HandbrakeTorque, 0.0f) : 0.0f;
		constraint.mWheels.push_back(JPH::Ref<JPH::WheelSettings>(joltWheel.GetPtr()));
	}

	// The engine through an automatic gearbox to the driven wheels: a differential per axle, the torque shared evenly
	JPH::Ref<JPH::WheeledVehicleControllerSettings> controller = new JPH::WheeledVehicleControllerSettings();
	controller->mEngine.mMaxTorque = std::max(settings.EngineTorque, 0.0f);
	vehicle.EngineTorque = controller->mEngine.mMaxTorque;
	// Max RPM: the most the throttle takes it to. Over-revving (a downshift) to Over-Rev RPM more: Jolt's limit is the sum,
	// the throttle cut at Max RPM in each step (LimiterRpm), and the torque curve kept on the same rpm (Jolt's is relative)
	const float maxRpm = std::max(settings.MaxRpm, controller->mEngine.mMinRPM + 500.0f);
	const float overRev = std::max(settings.OverRevRpm, 0.0f);
	controller->mEngine.mMaxRPM = maxRpm + overRev;
	if (overRev > 0.0f)
	{
		const float share = maxRpm / (maxRpm + overRev);
		controller->mEngine.mNormalizedTorque.Clear();
		controller->mEngine.mNormalizedTorque.AddPoint(0.0f, 0.8f);
		controller->mEngine.mNormalizedTorque.AddPoint(0.66f * share, 1.0f);
		controller->mEngine.mNormalizedTorque.AddPoint(share, 0.8f);
		controller->mEngine.mNormalizedTorque.AddPoint(1.0f, 0.8f);
		vehicle.LimiterRpm = maxRpm;
	}
	controller->mTransmission.mSwitchTime = glm::clamp(settings.ShiftTime, 0.0f, 2.0f);
	controller->mTransmission.mShiftDownRPM = glm::clamp(settings.ShiftDownRpm, controller->mEngine.mMinRPM, maxRpm - 100.0f);
	controller->mTransmission.mShiftUpRPM = glm::clamp(settings.ShiftUpRpm, controller->mTransmission.mShiftDownRPM + 100.0f, maxRpm);
	// The vehicle's own gears (each above a tenth: a ratio near 0 would let the engine spin free); none: Jolt's
	if (!settings.GearRatios.empty())
	{
		JPH::Array<float> ratios;
		for (float ratio : settings.GearRatios)
		{
			ratios.push_back(std::max(ratio, 0.1f));
		}
		controller->mTransmission.mGearRatios = ratios;
	}
	std::vector<int> all, driven;
	for (int w = 0; w < (int)settings.Wheels.size(); w++)
	{
		all.push_back(w);
		if (settings.Wheels[w].Driven)
		{
			driven.push_back(w);
		}
	}
	const std::vector<std::pair<int, int>> drivenAxles = GetAxles(settings.Wheels, driven);
	// The final drive for a top speed: the wheels' turning speed at it, and the engine's at Max RPM in the top gear
	float finalDrive = JPH::VehicleDifferentialSettings().mDifferentialRatio;
	if (settings.TopSpeed > 0.0f && !driven.empty() && !controller->mTransmission.mGearRatios.empty())
	{
		float radius = 0.0f;
		for (int w : driven)
		{
			radius += std::max(settings.Wheels[w].Radius, 0.01f);
		}
		radius /= (float)driven.size();
		const float wheelRadiansPerSecond = settings.TopSpeed / radius;
		const float engineRadiansPerSecond = maxRpm * JPH::JPH_PI / 30.0f;
		finalDrive = std::max(engineRadiansPerSecond / (wheelRadiansPerSecond * controller->mTransmission.mGearRatios.back()), 0.1f);
	}
	if (!driven.empty())
	{
		float radius = 0.0f;
		for (int w : driven)
		{
			radius += std::max(settings.Wheels[w].Radius, 0.01f);
		}
		radius /= (float)driven.size();
		vehicle.RpmPerSpeed = finalDrive / radius * 30.0f / JPH::JPH_PI;
		vehicle.EngineBrakeTorque = glm::clamp(settings.EngineBraking, 0.0f, 1.0f) * controller->mEngine.mMaxTorque;
	}
	for (const auto& [left, right] : drivenAxles)
	{
		JPH::VehicleDifferentialSettings differential;
		differential.mLeftWheel = left;
		differential.mRightWheel = right;
		differential.mEngineTorqueRatio = 1.0f / (float)drivenAxles.size();
		differential.mDifferentialRatio = finalDrive;
		controller->mDifferentials.push_back(differential);
	}
	if (drivenAxles.empty())
	{
		Log::GetLogger()->warn("Physics: the vehicle '{0}' has no driven wheels: it only rolls", name);
	}
	constraint.mController = controller;

	// Anti-roll bars: an axle's two wheels pull each other down, so it leans less in a turn
	if (settings.AntiRollBars)
	{
		for (const auto& [left, right] : GetAxles(settings.Wheels, all))
		{
			if (right >= 0)
			{
				JPH::VehicleAntiRollBar bar;
				bar.mLeftWheel = left;
				bar.mRightWheel = right;
				constraint.mAntiRollBars.push_back(bar);
			}
		}
	}

	{
		JPH::BodyLockWrite lock(m_World->GetSystem().GetBodyLockInterface(), vehicle.Body);
		if (!lock.Succeeded())
		{
			return false;
		}
		vehicle.Constraint = new JPH::VehicleConstraint(lock.GetBody(), constraint);
	}
	// The wheels find the ground with a cylinder of their size (a ray would fall into the gaps of a height field), against
	// what the vehicle's body collides with
	vehicle.Tester = new JPH::VehicleCollisionTesterCastCylinder(ObjectLayers::Dynamic);
	vehicle.Constraint->SetVehicleCollisionTester(vehicle.Tester);
	// The tires' sideways grip per wheel (FrontGrip, RearGrip; SetVehicleGrip): front is ahead of the vehicle's middle
	vehicle.FrontGrip = std::max(settings.FrontGrip, 0.0f);
	vehicle.RearGrip = std::max(settings.RearGrip, 0.0f);
	vehicle.WheelGrip = std::make_shared<std::vector<float>>();
	for (const VehicleComponent::Wheel& wheel : settings.Wheels)
	{
		vehicle.FrontWheel.push_back(wheel.Position.z < 0.0f);
		vehicle.WheelGrip->push_back(wheel.Position.z < 0.0f ? vehicle.FrontGrip : vehicle.RearGrip);
	}
	// As in Jolt's samples: its tires were tuned with ten times this grip along the wheel (a fix made later in Jolt)
	std::shared_ptr<std::vector<float>> wheelGrip = vehicle.WheelGrip;
	static_cast<JPH::WheeledVehicleController*>(vehicle.Constraint->GetController())->SetTireMaxImpulseCallback(
		[wheelGrip](JPH::uint wheel, float& longitudinal, float& lateral, float suspensionImpulse, float longitudinalFriction, float lateralFriction, float, float, float)
		{
			longitudinal = 10.0f * longitudinalFriction * suspensionImpulse;
			lateral = lateralFriction * suspensionImpulse * (wheel < wheelGrip->size() ? (*wheelGrip)[wheel] : 1.0f);
		});
	if (settings.TopSpeed > 0.0f)
	{
		Log::GetLogger()->info("Physics: the vehicle '{0}': final drive {1:.2f} for {2:.0f} km/h at {3:.0f} rpm in the top gear", name, finalDrive,
			settings.TopSpeed * 3.6f, maxRpm);
	}
	m_World->GetSystem().AddConstraint(vehicle.Constraint);
	m_World->GetSystem().AddStepListener(vehicle.Constraint);

	// The wheels' models: their axes at rest, from how they are turned in the vehicle now
	glm::vec3 position, scale;
	glm::quat rotation;
	DecomposeWorld(m_Scene->GetWorldTransform(vehicle.Entity), position, rotation, scale);
	for (const VehicleComponent::Wheel& wheel : settings.Wheels)
	{
		Vehicle::WheelModel model;
		if (wheel.Entity != NoEntity && m_Scene->Exists(wheel.Entity) && wheel.Entity != vehicle.Entity)
		{
			glm::vec3 wheelPosition;
			glm::quat wheelRotation;
			DecomposeWorld(m_Scene->GetWorldTransform(wheel.Entity), wheelPosition, wheelRotation, model.Scale);
			const glm::quat toModel = glm::inverse(glm::inverse(rotation) * wheelRotation); // the vehicle's space to the model's
			model.Entity = wheel.Entity;
			model.Right = ToJolt(toModel * glm::vec3(1.0f, 0.0f, 0.0f));
			model.Up = ToJolt(toModel * glm::vec3(0.0f, 1.0f, 0.0f));
		}
		vehicle.Wheels.push_back(model);
	}
	if (std::abs(scale.x - 1.0f) > 0.01f || std::abs(scale.y - 1.0f) > 0.01f || std::abs(scale.z - 1.0f) > 0.01f)
	{
		Log::GetLogger()->warn("Physics: the vehicle '{0}' is scaled: its wheels' models are stretched as they turn (scale a child that "
			"holds the body's model instead)", name);
	}
	return true;
}

void ScenePhysics::RemoveVehicle(Vehicle& vehicle)
{
	if (vehicle.Constraint)
	{
		m_World->GetSystem().RemoveStepListener(vehicle.Constraint);
		m_World->GetSystem().RemoveConstraint(vehicle.Constraint);
	}
}

const ScenePhysics::Vehicle* ScenePhysics::FindVehicle(EnvMapVulkanEntityID entity) const
{
	for (const Vehicle& vehicle : m_Vehicles)
	{
		if (vehicle.Entity == entity)
		{
			return &vehicle;
		}
	}
	return nullptr;
}

bool ScenePhysics::SetVehicleInput(EnvMapVulkanEntityID entity, const VehicleInput& input)
{
	Vehicle* vehicle = const_cast<Vehicle*>(FindVehicle(entity));
	if (!vehicle)
	{
		return false;
	}
	const auto clamp = [](float value, float low) { return std::isfinite(value) ? glm::clamp(value, low, 1.0f) : 0.0f; };
	vehicle->Input.Forward = clamp(input.Forward, -1.0f);
	vehicle->Input.Steer = clamp(input.Steer, -1.0f);
	vehicle->Input.Brake = clamp(input.Brake, 0.0f);
	vehicle->Input.Handbrake = clamp(input.Handbrake, 0.0f);
	return true;
}

bool ScenePhysics::SetVehicleGrip(EnvMapVulkanEntityID entity, float front, float rear)
{
	Vehicle* vehicle = const_cast<Vehicle*>(FindVehicle(entity));
	if (!vehicle || !vehicle->WheelGrip)
	{
		return false;
	}
	const auto factor = [](float value) { return std::isfinite(value) ? glm::clamp(value, 0.0f, 10.0f) : 1.0f; };
	// Between the steps (scripts run before them): the callback reads these in the next one
	for (size_t w = 0; w < vehicle->WheelGrip->size() && w < vehicle->FrontWheel.size(); w++)
	{
		(*vehicle->WheelGrip)[w] = vehicle->FrontWheel[w] ? vehicle->FrontGrip * factor(front) : vehicle->RearGrip * factor(rear);
	}
	return true;
}

bool ScenePhysics::SetVehicleEngineTorque(EnvMapVulkanEntityID entity, float times)
{
	Vehicle* vehicle = const_cast<Vehicle*>(FindVehicle(entity));
	if (!vehicle)
	{
		return false;
	}
	JPH::VehicleEngine& engine = static_cast<JPH::WheeledVehicleController*>(vehicle->Constraint->GetController())->GetEngine();
	engine.mMaxTorque = vehicle->EngineTorque * (std::isfinite(times) ? std::max(times, 0.0f) : 1.0f);
	return true;
}

bool ScenePhysics::SetVehicleManual(EnvMapVulkanEntityID entity, bool manual)
{
	Vehicle* vehicle = const_cast<Vehicle*>(FindVehicle(entity));
	if (!vehicle)
	{
		return false;
	}
	if (vehicle->Manual != manual)
	{
		vehicle->Manual = manual;
		vehicle->ShiftTimeLeft = vehicle->ClutchReleaseLeft = 0.0f;
		JPH::VehicleTransmission& transmission = static_cast<JPH::WheeledVehicleController*>(vehicle->Constraint->GetController())->GetTransmission();
		transmission.mMode = manual ? JPH::ETransmissionMode::Manual : JPH::ETransmissionMode::Auto;
	}
	return true;
}

void ScenePhysics::PutInGear(Vehicle& vehicle, int gear)
{
	JPH::VehicleTransmission& transmission = static_cast<JPH::WheeledVehicleController*>(vehicle.Constraint->GetController())->GetTransmission();
	vehicle.ShiftTimeLeft = transmission.GetCurrentGear() != 0 ? transmission.mSwitchTime : 0.0f;
	vehicle.ClutchReleaseLeft = transmission.mClutchReleaseTime;
	transmission.Set(gear, 0.0f);
}

ScenePhysics::ShiftResult ScenePhysics::ShiftVehicleGear(EnvMapVulkanEntityID entity, int direction)
{
	Vehicle* vehicle = const_cast<Vehicle*>(FindVehicle(entity));
	if (!vehicle)
	{
		return ShiftResult::NotVehicle;
	}
	if (!vehicle->Manual || direction == 0)
	{
		return ShiftResult::Refused;
	}
	const JPH::WheeledVehicleController* controller = static_cast<const JPH::WheeledVehicleController*>(vehicle->Constraint->GetController());
	const JPH::VehicleTransmission& transmission = controller->GetTransmission();
	const int gear = transmission.GetCurrentGear();
	const int gears = (int)transmission.mGearRatios.size();
	int next = 0;
	if (direction > 0)
	{
		next = gear <= 0 ? 1 : gear + 1; // from neutral or reverse: first
		if (next > gears)
		{
			return ShiftResult::Refused;
		}
	}
	else
	{
		if (gear <= 1)
		{
			return ShiftResult::Refused;
		}
		next = gear - 1;
		// The engine in the lower gear, at the wheels' speed or the body's (locked or spinning wheels say little): past Max
		// RPM plus Over-Rev RPM plus a tolerance (the Racing car: 9000), it would be thrown too far over its limit. Within the
		// tolerance the engine stays at its limit (Jolt holds it there) and the clutch brakes the wheels down to it
		const float ratio = transmission.GetCurrentRatio();
		const float lower = transmission.mGearRatios[next - 1];
		const JPH::BodyInterface& bodies = m_World->GetSystem().GetBodyInterface();
		const float speed = std::abs((bodies.GetRotation(vehicle->Body).Conjugated() * bodies.GetLinearVelocity(vehicle->Body)).Dot(VehicleForward));
		const float atWheels = ratio != 0.0f ? std::abs(controller->GetWheelSpeedAtClutch()) / ratio * lower : 0.0f;
		if (std::max(atWheels, speed * vehicle->RpmPerSpeed * lower) > controller->GetEngine().mMaxRPM + DownshiftToleranceRpm)
		{
			return ShiftResult::Refused;
		}
	}
	PutInGear(*vehicle, next);
	return ShiftResult::Shifted;
}

bool ScenePhysics::SetVehicleGear(EnvMapVulkanEntityID entity, int gear)
{
	Vehicle* vehicle = const_cast<Vehicle*>(FindVehicle(entity));
	if (!vehicle)
	{
		return false;
	}
	JPH::VehicleTransmission& transmission = static_cast<JPH::WheeledVehicleController*>(vehicle->Constraint->GetController())->GetTransmission();
	gear = glm::clamp(gear, -(int)transmission.mReverseGearRatios.size(), (int)transmission.mGearRatios.size());
	// No shift time: the clutch is let in at once (the manual's from its release; a car standing still slips it anyway)
	vehicle->ShiftTimeLeft = vehicle->ClutchReleaseLeft = 0.0f;
	transmission.Set(gear, 1.0f);
	return true;
}

void ScenePhysics::UpdateManualGearbox(Vehicle& vehicle, VehicleInput& applied, float fixedStep)
{
	JPH::WheeledVehicleController* controller = static_cast<JPH::WheeledVehicleController*>(vehicle.Constraint->GetController());
	JPH::VehicleTransmission& transmission = controller->GetTransmission();
	// The throttle from neutral, or the other way (once stopped: the brake-first rule ran): first or reverse
	const int gear = transmission.GetCurrentGear();
	if ((applied.Forward > 0.0f && gear <= 0) || (applied.Forward < 0.0f && gear >= 0))
	{
		PutInGear(vehicle, applied.Forward > 0.0f ? 1 : -1);
	}
	float clutch = 1.0f;
	if (vehicle.ShiftTimeLeft > 0.0f)
	{
		vehicle.ShiftTimeLeft = std::max(vehicle.ShiftTimeLeft - fixedStep, 0.0f);
		clutch = 0.0f;
	}
	else if (vehicle.ClutchReleaseLeft > 0.0f)
	{
		vehicle.ClutchReleaseLeft = std::max(vehicle.ClutchReleaseLeft - fixedStep, 0.0f);
		clutch = transmission.mClutchReleaseTime > 0.0f ? 1.0f - vehicle.ClutchReleaseLeft / transmission.mClutchReleaseTime : 1.0f;
	}
	// Slow without throttle: the clutch out (in gear it would creep at idle; the automatic goes to neutral there). At Jolt's
	// default Shift Down RPM, not the component's: an automatic that shifts down late doesn't make the manual's clutch slip
	constexpr float ClutchOutRpm = 2000.0f;
	if (applied.Forward == 0.0f && std::abs(controller->GetWheelSpeedAtClutch()) < ClutchOutRpm)
	{
		clutch = 0.0f;
	}
	transmission.Set(transmission.GetCurrentGear(), clutch);
	// No throttle through a shift, as the automatic's (Jolt's manual mode would let the engine race while the clutch is out)
	applied.Forward *= clutch;

	// Engine braking: off the throttle in a forward gear, the engine's drag (rising with its rpm) through the gears to the
	// wheels on the ground, as a force against the body's motion (Jolt's engine alone barely holds the car back)
	const float ratio = transmission.GetCurrentRatio();
	if (vehicle.EngineBrakeTorque <= 0.0f || applied.Forward != 0.0f || clutch <= 0.0f || ratio <= 0.0f)
	{
		return;
	}
	const JPH::VehicleEngine& engine = controller->GetEngine();
	// 1 at Max RPM, more over it (over-revving after a downshift brakes harder)
	const float limiter = vehicle.LimiterRpm > 0.0f ? vehicle.LimiterRpm : engine.mMaxRPM;
	const float revs = std::max((engine.GetCurrentRPM() - engine.mMinRPM) / std::max(limiter - engine.mMinRPM, 1.0f), 0.0f);
	uint32_t onGround = 0;
	const uint32_t wheels = (uint32_t)vehicle.Constraint->GetWheels().size();
	for (uint32_t w = 0; w < wheels; w++)
	{
		onGround += vehicle.Constraint->GetWheel(w)->HasContact() ? 1u : 0u;
	}
	JPH::BodyInterface& bodies = m_World->GetBodyInterface();
	const JPH::Vec3 forward = bodies.GetRotation(vehicle.Body) * VehicleForward;
	const float speed = bodies.GetLinearVelocity(vehicle.Body).Dot(forward);
	if (onGround == 0 || speed < 0.5f)
	{
		return;
	}
	// The torque at the wheels over their radius: torque x ratio x final drive / radius (RpmPerSpeed holds final drive / radius)
	const float force = vehicle.EngineBrakeTorque * revs * clutch * ratio * vehicle.RpmPerSpeed * JPH::JPH_PI / 30.0f * (float)onGround / (float)wheels;
	bodies.AddForce(vehicle.Body, -forward * force);
}

bool ScenePhysics::GetVehicleState(EnvMapVulkanEntityID entity, VehicleState& state) const
{
	const Vehicle* vehicle = m_World ? FindVehicle(entity) : nullptr;
	if (!vehicle)
	{
		return false;
	}
	const JPH::BodyInterface& bodies = m_World->GetSystem().GetBodyInterface();
	const JPH::Vec3 velocity = bodies.GetRotation(vehicle->Body).Conjugated() * bodies.GetLinearVelocity(vehicle->Body);
	state.Speed = velocity.Dot(VehicleForward);
	const JPH::WheeledVehicleController* controller = static_cast<const JPH::WheeledVehicleController*>(vehicle->Constraint->GetController());
	state.Rpm = controller->GetEngine().GetCurrentRPM();
	state.Gear = controller->GetTransmission().GetCurrentGear();
	state.Manual = vehicle->Manual;
	state.Gears = (int)controller->GetTransmission().mGearRatios.size();
	state.RevLimit = controller->GetEngine().mMaxRPM;
	state.Wheels = (uint32_t)vehicle->Constraint->GetWheels().size();
	state.WheelsOnGround = 0;
	state.LateralSlip = state.LongitudinalSlip = 0.0f;
	for (uint32_t w = 0; w < state.Wheels && w < 32; w++)
	{
		if (vehicle->Constraint->GetWheel(w)->HasContact())
		{
			state.WheelsOnGround |= 1u << w;
			const JPH::WheelWV* wheel = static_cast<const JPH::WheelWV*>(vehicle->Constraint->GetWheel(w));
			state.LateralSlip = std::max(state.LateralSlip, glm::degrees(std::abs(wheel->mLateralSlip)));
			state.LongitudinalSlip = std::max(state.LongitudinalSlip, std::abs(wheel->mLongitudinalSlip));
		}
	}
	state.Input = vehicle->Input;
	state.Applied = vehicle->Applied;
	return true;
}

void ScenePhysics::WriteWheels(const Vehicle& vehicle, const glm::vec3& position, const glm::quat& rotation)
{
	const glm::mat4 body = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation);
	for (uint32_t w = 0; w < (uint32_t)vehicle.Wheels.size(); w++)
	{
		const Vehicle::WheelModel& model = vehicle.Wheels[w];
		if (model.Entity == NoEntity || !m_Scene->Exists(model.Entity))
		{
			continue;
		}
		// The wheel's place in the vehicle (its spring's length, its steering, how far it has rolled), with the model's axes
		const glm::mat4 wheel = ToGlm(vehicle.Constraint->GetWheelLocalTransform(w, model.Right, model.Up));
		m_Scene->SetWorldTransform(model.Entity, body * wheel * glm::scale(glm::mat4(1.0f), model.Scale));
	}
}

bool ScenePhysics::FitWheels(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, VehicleComponent& vehicle)
{
	glm::vec3 position, scale;
	glm::quat rotation;
	DecomposeWorld(scene.GetWorldTransform(entity), position, rotation, scale);
	const glm::quat toVehicle = glm::inverse(rotation);

	std::vector<VehicleComponent::Wheel> wheels;
	for (EnvMapVulkanEntityID child : scene.GetSubtree(entity))
	{
		std::string name = scene.Get<NameComponent>(child).Name;
		std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return (char)std::tolower(c); });
		glm::vec3 boundsMin, boundsMax;
		if (child == entity || name.find("wheel") == std::string::npos || !GetModelBounds(scene, child, boundsMin, boundsMax))
		{
			continue;
		}
		glm::vec3 wheelPosition, wheelScale;
		glm::quat wheelRotation;
		DecomposeWorld(scene.GetWorldTransform(child), wheelPosition, wheelRotation, wheelScale);
		// It turns around its axis closest to the vehicle's X: as wide as the model along it, as round as the larger of the
		// other two
		const glm::vec3 size = (boundsMax - boundsMin) * glm::abs(wheelScale);
		int axle = 0;
		float best = -1.0f;
		for (int a = 0; a < 3; a++)
		{
			glm::vec3 axis(0.0f);
			axis[a] = 1.0f;
			const float alignment = std::abs((toVehicle * wheelRotation * axis).x);
			if (alignment > best)
			{
				best = alignment;
				axle = a;
			}
		}
		VehicleComponent::Wheel wheel;
		wheel.Entity = child;
		wheel.Position = toVehicle * (wheelPosition - position); // the model's origin: its center, the wheel's
		wheel.Width = std::max(size[axle], 0.01f);
		wheel.Radius = std::max(0.5f * std::max(size[(axle + 1) % 3], size[(axle + 2) % 3]), 0.01f);
		wheel.Steers = wheel.Position.z < 0.0f; // the front half (forward is -Z)
		wheel.Handbrake = !wheel.Steers;
		wheel.Driven = true;
		wheels.push_back(wheel);
	}
	if (wheels.empty())
	{
		return false;
	}
	// Front to back, left to right
	std::sort(wheels.begin(), wheels.end(), [](const VehicleComponent::Wheel& a, const VehicleComponent::Wheel& b)
	{
		return std::abs(a.Position.z - b.Position.z) > 0.01f ? a.Position.z < b.Position.z : a.Position.x < b.Position.x;
	});
	vehicle.Wheels = wheels;
	return true;
}

void ScenePhysics::Start(EnvMapVulkanScene& scene, const PhysicsCollisionMatrix& collisions)
{
	Stop();
	m_Scene = &scene;
	PhysicsWorldSettings settings;
	settings.Collisions = collisions;
	m_World = std::make_unique<PhysicsWorld>(settings);
	m_Listener = std::make_unique<ScenePhysicsContactListener>();
	m_Listener->Collect = m_CollectContacts;
	m_World->GetSystem().SetContactListener(m_Listener.get());

	AddCounts counts;
	scene.Each<TerrainComponent>([&](EnvMapVulkanEntityID entity, TerrainComponent&)
	{
		AddEntity(entity, counts);
	});
	scene.Each<ColliderComponent>([&](EnvMapVulkanEntityID entity, ColliderComponent&)
	{
		if (!scene.Has<TerrainComponent>(entity))
		{
			AddEntity(entity, counts);
		}
	});
	scene.Each<RigidBodyComponent>([&](EnvMapVulkanEntityID entity, RigidBodyComponent&)
	{
		if (!scene.Has<ColliderComponent>(entity))
		{
			AddEntity(entity, counts); // a warning, or a character
		}
	});
	scene.Each<CharacterControllerComponent>([&](EnvMapVulkanEntityID entity, CharacterControllerComponent&)
	{
		if (!scene.Has<ColliderComponent>(entity) && !scene.Has<RigidBodyComponent>(entity))
		{
			AddEntity(entity, counts);
		}
	});
	scene.Each<VehicleComponent>([&](EnvMapVulkanEntityID entity, VehicleComponent&)
	{
		if (!scene.Has<ColliderComponent>(entity))
		{
			Log::GetLogger()->warn("Physics: the vehicle '{0}' has no Collider: no vehicle (its body needs a shape)", scene.Get<NameComponent>(entity).Name);
		}
	});

	// The bodies added one by one: the broad phase's trees are rebuilt once, for a first step as fast as the next ones
	m_World->GetSystem().OptimizeBroadPhase();
	Log::GetLogger()->info("Physics: {0} bodies ({1} dynamic, {2} kinematic, {3} static, {4} trigger{5}), {6} character{7}, {8} vehicle{9}{10}",
		m_Bodies.size(), counts.Motions[(int)JPH::EMotionType::Dynamic], counts.Motions[(int)JPH::EMotionType::Kinematic],
		counts.Motions[(int)JPH::EMotionType::Static], counts.Triggers, counts.Triggers == 1 ? "" : "s", m_Characters.size(),
		m_Characters.size() == 1 ? "" : "s", m_Vehicles.size(), m_Vehicles.size() == 1 ? "" : "s",
		counts.Failed > 0 ? ", " + std::to_string(counts.Failed) + " could not be made" : std::string());
}

void ScenePhysics::StartStatic(EnvMapVulkanScene& scene)
{
	Stop();
	m_Scene = &scene;
	m_World = std::make_unique<PhysicsWorld>(PhysicsWorldSettings());
	AddCounts counts;
	scene.Each<ColliderComponent>([&](EnvMapVulkanEntityID entity, ColliderComponent&)
	{
		if (!scene.Has<TerrainComponent>(entity) && !scene.Has<RigidBodyComponent>(entity) && !scene.Has<CharacterControllerComponent>(entity) &&
			!scene.Has<VehicleComponent>(entity))
		{
			AddEntity(entity, counts);
		}
	});
	m_World->GetSystem().OptimizeBroadPhase();
}

void ScenePhysics::AddEntities(const std::vector<EnvMapVulkanEntityID>& entities)
{
	if (!m_World)
	{
		return;
	}
	AddCounts counts;
	for (EnvMapVulkanEntityID entity : entities)
	{
		AddEntity(entity, counts);
	}
}

void ScenePhysics::AddEntity(EnvMapVulkanEntityID entity, AddCounts& counts)
{
	EnvMapVulkanScene& scene = *m_Scene;
	// A terrain: its height field (a Collider or Rigid Body on it isn't used)
	if (TerrainComponent* terrain = scene.TryGet<TerrainComponent>(entity))
	{
		Body body;
		body.Entity = entity;
		if (!CreateTerrainBody(body, *terrain))
		{
			counts.Failed++;
			return;
		}
		counts.Motions[(int)JPH::EMotionType::Static]++;
		m_BodyInfo[body.ID.GetIndexAndSequenceNumber()] = { entity, false };
		m_Bodies.push_back(body);
		return;
	}
	const ColliderComponent* collider = scene.TryGet<ColliderComponent>(entity);
	// A vehicle's wheel is only drawn (its wheel is the vehicle's): a body there would be in the wheel's way
	if ((collider || scene.Has<RigidBodyComponent>(entity)) && IsVehicleWheel(scene, entity))
	{
		Log::GetLogger()->warn("Physics: '{0}' is a vehicle's wheel: its Collider and Rigid Body aren't used", scene.Get<NameComponent>(entity).Name);
		return;
	}
	if (const CharacterControllerComponent* settings = scene.TryGet<CharacterControllerComponent>(entity))
	{
		if (collider || scene.Has<RigidBodyComponent>(entity))
		{
			Log::GetLogger()->warn("Physics: '{0}' is a character: its Collider and Rigid Body aren't used", scene.Get<NameComponent>(entity).Name);
		}
		Character character;
		character.Entity = entity;
		CreateCharacter(character, *settings);
		m_BodyInfo[character.Controller->GetInnerBodyID().GetIndexAndSequenceNumber()] = { entity, false };
		m_Characters.push_back(character);
		return;
	}
	if (!collider)
	{
		if (scene.Has<RigidBodyComponent>(entity))
		{
			Log::GetLogger()->warn("Physics: '{0}' has a rigid body but no collider: not simulated", scene.Get<NameComponent>(entity).Name);
		}
		return;
	}
	Body body;
	body.Entity = entity;
	if (!CreateBody(body, *collider, scene.TryGet<RigidBodyComponent>(entity)))
	{
		counts.Failed++;
		return;
	}
	if (body.Trigger)
	{
		counts.Triggers++;
	}
	else
	{
		counts.Motions[(int)body.Motion]++;
	}
	m_BodyInfo[body.ID.GetIndexAndSequenceNumber()] = { entity, body.Trigger };
	m_Bodies.push_back(body);

	if (const VehicleComponent* settings = scene.TryGet<VehicleComponent>(entity))
	{
		if (body.Motion != JPH::EMotionType::Dynamic || body.Trigger)
		{
			Log::GetLogger()->warn("Physics: the vehicle '{0}' needs a dynamic Rigid Body (not a trigger): no vehicle", scene.Get<NameComponent>(entity).Name);
			return;
		}
		Vehicle vehicle;
		vehicle.Entity = entity;
		vehicle.Body = body.ID;
		if (CreateVehicle(vehicle, *settings))
		{
			m_Vehicles.push_back(std::move(vehicle));
		}
	}
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
	m_LastStepMs = 0.0f;
	for (Vehicle& vehicle : m_Vehicles)
	{
		RemoveVehicle(vehicle); // before their bodies
	}
	m_Vehicles.clear();
	m_Characters.clear(); // before the world: they remove their inner bodies from it
	m_Bodies.clear();
	m_World.reset();
	m_Listener.reset();   // after the world, which calls it
	m_BodyInfo.clear();
	m_Touching.clear();
	m_Events.clear();
	m_RecentEvents.clear();
	m_Alpha = 1.0f;
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
	m_FixedStep = fixedStep;

	// What the game did since the last step
	RemoveDestroyedEntities();
	for (Body& body : m_Bodies)
	{
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
		// Not the controller's velocity: against a wall, on the ground, it keeps the one asked for
		character.Moved = (character.Position - character.PreviousPosition) / fixedStep;
	}

	// The vehicles: each with its driver's input
	for (Vehicle& vehicle : m_Vehicles)
	{
		VehicleInput applied = vehicle.Input;
		// Asked for the other way while it still rolls: it brakes until it stops, then goes (as Jolt's samples drive)
		if (applied.Forward != 0.0f && (applied.Forward > 0.0f) != (vehicle.Direction > 0.0f))
		{
			const JPH::Vec3 velocity = bodies.GetRotation(vehicle.Body).Conjugated() * bodies.GetLinearVelocity(vehicle.Body);
			const float speed = velocity.Dot(VehicleForward);
			if ((applied.Forward > 0.0f && speed < -0.1f) || (applied.Forward < 0.0f && speed > 0.1f))
			{
				applied.Brake = std::max(applied.Brake, std::abs(applied.Forward));
				applied.Forward = 0.0f;
			}
			else
			{
				vehicle.Direction = applied.Forward > 0.0f ? 1.0f : -1.0f;
			}
		}
		if (applied.Handbrake > 0.0f)
		{
			applied.Forward = 0.0f; // the handbrake lets go of the throttle
		}
		vehicle.Applied = applied;
		const bool driven = applied.Forward != 0.0f || applied.Steer != 0.0f || applied.Brake != 0.0f || applied.Handbrake != 0.0f;
		if (vehicle.Manual)
		{
			UpdateManualGearbox(vehicle, applied, fixedStep); // Applied keeps the driver's throttle (an engine sound's load)
		}
		// The rev limiter: no throttle at Max RPM and over it (an engine that may over-rev; without, Jolt's limit holds it)
		if (vehicle.LimiterRpm > 0.0f && applied.Forward > 0.0f &&
			static_cast<const JPH::WheeledVehicleController*>(vehicle.Constraint->GetController())->GetEngine().GetCurrentRPM() >= vehicle.LimiterRpm)
		{
			applied.Forward = 0.0f;
		}
		static_cast<JPH::WheeledVehicleController*>(vehicle.Constraint->GetController())->SetDriverInput(applied.Forward, applied.Steer,
			applied.Brake, applied.Handbrake);
		if (driven)
		{
			bodies.ActivateBody(vehicle.Body); // a parked vehicle sleeps: the driver wakes it
		}
	}

	if (m_Listener->Collect)
	{
		m_Listener->Contacts.clear(); // the listener isn't called between the steps: no lock needed
	}
	m_World->Step(fixedStep);
	DispatchContacts();

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
	const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - stepStart).count();
	m_StepSeconds += seconds;
	m_LastStepMs = (float)(seconds * 1000.0);
	m_StepCount++;
}

void ScenePhysics::RemoveDestroyedEntities()
{
	JPH::BodyInterface& bodies = m_World->GetBodyInterface();
	std::vector<uint32_t> removed; // body IDs, for the pairs they touched
	auto wakeAround = [&](JPH::BodyID id)
	{
		JPH::AABox bounds = bodies.GetTransformedShape(id).GetWorldSpaceBounds();
		bounds.ExpandBy(JPH::Vec3::sReplicate(0.05f));
		bodies.ActivateBodiesInAABox(bounds, JPH::BroadPhaseLayerFilter(), JPH::ObjectLayerFilter());
		removed.push_back(id.GetIndexAndSequenceNumber());
		m_BodyInfo.erase(id.GetIndexAndSequenceNumber());
	};

	for (auto vehicle = m_Vehicles.begin(); vehicle != m_Vehicles.end();)
	{
		if (m_Scene->Exists(vehicle->Entity))
		{
			++vehicle;
			continue;
		}
		RemoveVehicle(*vehicle); // before its body
		vehicle = m_Vehicles.erase(vehicle);
	}
	for (auto body = m_Bodies.begin(); body != m_Bodies.end();)
	{
		if (m_Scene->Exists(body->Entity))
		{
			++body;
			continue;
		}
		wakeAround(body->ID);
		m_World->RemoveBody(body->ID);
		body = m_Bodies.erase(body);
	}
	for (auto character = m_Characters.begin(); character != m_Characters.end();)
	{
		if (m_Scene->Exists(character->Entity))
		{
			++character;
			continue;
		}
		wakeAround(character->Controller->GetInnerBodyID());
		character = m_Characters.erase(character); // its controller removes its inner body
	}

	// Their contacts are gone too, without events: the entity isn't there to get them (nor is it for the other side)
	if (!removed.empty())
	{
		for (auto pair = m_Touching.begin(); pair != m_Touching.end();)
		{
			const uint32_t a = (uint32_t)(pair->first >> 32), b = (uint32_t)pair->first;
			const bool gone = std::find(removed.begin(), removed.end(), a) != removed.end() || std::find(removed.begin(), removed.end(), b) != removed.end();
			pair = gone ? m_Touching.erase(pair) : std::next(pair);
		}
	}
}

const ScenePhysics::Body* ScenePhysics::FindDynamicBody(EnvMapVulkanEntityID entity) const
{
	for (const Body& body : m_Bodies)
	{
		if (body.Entity == entity && body.Motion == JPH::EMotionType::Dynamic)
		{
			return &body;
		}
	}
	return nullptr;
}

bool ScenePhysics::AddForce(EnvMapVulkanEntityID entity, const glm::vec3& force, ForceMode mode)
{
	const Body* body = m_World ? FindDynamicBody(entity) : nullptr;
	if (!body)
	{
		return false;
	}
	JPH::BodyInterface& bodies = m_World->GetBodyInterface();
	switch (mode)
	{
		case ForceMode::Force:          bodies.AddForce(body->ID, ToJolt(force), JPH::EActivation::Activate); break;
		case ForceMode::Impulse:        bodies.AddImpulse(body->ID, ToJolt(force)); break;
		case ForceMode::Acceleration:   bodies.AddLinearVelocity(body->ID, ToJolt(force * m_FixedStep)); break;
		case ForceMode::VelocityChange: bodies.AddLinearVelocity(body->ID, ToJolt(force)); break;
	}
	return true;
}

bool ScenePhysics::AddTorque(EnvMapVulkanEntityID entity, const glm::vec3& torque, ForceMode mode)
{
	const Body* body = m_World ? FindDynamicBody(entity) : nullptr;
	if (!body)
	{
		return false;
	}
	JPH::BodyInterface& bodies = m_World->GetBodyInterface();
	switch (mode)
	{
		case ForceMode::Force:          bodies.AddTorque(body->ID, ToJolt(torque), JPH::EActivation::Activate); break;
		case ForceMode::Impulse:        bodies.AddAngularImpulse(body->ID, ToJolt(torque)); break;
		case ForceMode::Acceleration:
		case ForceMode::VelocityChange:
		{
			const glm::vec3 change = mode == ForceMode::Acceleration ? torque * m_FixedStep : torque;
			bodies.SetAngularVelocity(body->ID, bodies.GetAngularVelocity(body->ID) + ToJolt(change));
			bodies.ActivateBody(body->ID);
			break;
		}
	}
	return true;
}

bool ScenePhysics::GetVelocity(EnvMapVulkanEntityID entity, glm::vec3& linear, glm::vec3& angular) const
{
	const Body* body = m_World ? FindDynamicBody(entity) : nullptr;
	if (!body)
	{
		return false;
	}
	const JPH::BodyInterface& bodies = m_World->GetSystem().GetBodyInterface();
	linear = ToGlm(bodies.GetLinearVelocity(body->ID));
	angular = ToGlm(bodies.GetAngularVelocity(body->ID));
	return true;
}

bool ScenePhysics::SetLinearVelocity(EnvMapVulkanEntityID entity, const glm::vec3& velocity)
{
	const Body* body = m_World ? FindDynamicBody(entity) : nullptr;
	if (!body)
	{
		return false;
	}
	m_World->GetBodyInterface().SetLinearVelocity(body->ID, ToJolt(velocity)); // wakes it
	return true;
}

bool ScenePhysics::SetAngularVelocity(EnvMapVulkanEntityID entity, const glm::vec3& velocity)
{
	const Body* body = m_World ? FindDynamicBody(entity) : nullptr;
	if (!body)
	{
		return false;
	}
	JPH::BodyInterface& bodies = m_World->GetBodyInterface();
	bodies.SetAngularVelocity(body->ID, ToJolt(velocity));
	bodies.ActivateBody(body->ID);
	return true;
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
			state.Velocity = character.Moved;
			state.GroundNormal = ToGlm(character.Controller->GetGroundNormal());
			state.GroundVelocity = ToGlm(character.Controller->GetGroundVelocity());
			// The ground body's turn (a character's heading isn't the physics': its game turns it, AnimatedCharacter)
			const JPH::BodyID ground = character.Controller->GetGroundBodyID();
			state.GroundAngularVelocity = !ground.IsInvalid() && state.Ground == CharacterGround::OnGround ?
				ToGlm(m_World->GetSystem().GetBodyInterface().GetAngularVelocity(ground)) : glm::vec3(0.0f);
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
	m_Alpha = alpha;
	// A body or character the game put somewhere since the last write (a script, between two steps) stays there: the next
	// step takes it from there (Step: "Put somewhere"), instead of this write putting it back where physics had it
	for (Character& character : m_Characters)
	{
		if (m_Scene->Exists(character.Entity) && m_Scene->Get<TransformComponent>(character.Entity).Translation == character.Written)
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
		const TransformComponent& transform = m_Scene->Get<TransformComponent>(body.Entity);
		if (transform.Translation != body.Written.Translation || transform.Rotation != body.Written.Rotation)
		{
			continue;
		}
		const glm::vec3 position = glm::mix(body.PreviousPosition, body.Position, alpha);
		const glm::quat rotation = glm::slerp(body.PreviousRotation, body.Rotation, alpha);
		// In the world: a body under a parent stays where physics puts it, wherever the parent goes
		m_Scene->SetWorldTransform(body.Entity, ComposeWorld(position, rotation, body.Scale));
		body.Written = m_Scene->Get<TransformComponent>(body.Entity);
	}
	// The wheels' models on their vehicles, where those are drawn now
	for (const Vehicle& vehicle : m_Vehicles)
	{
		if (m_Scene->Exists(vehicle.Entity))
		{
			glm::vec3 position, scale;
			glm::quat rotation;
			DecomposeWorld(m_Scene->GetWorldTransform(vehicle.Entity), position, rotation, scale);
			WriteWheels(vehicle, position, rotation);
		}
	}
}

// ---- Events

EnvMapVulkanEntityID ScenePhysics::GetBodyEntity(JPH::BodyID body) const
{
	const auto found = m_BodyInfo.find(body.GetIndexAndSequenceNumber());
	return found != m_BodyInfo.end() ? found->second.Entity : NoEntity;
}

void ScenePhysics::DispatchContacts()
{
	std::vector<ScenePhysicsContactListener::Change> changes;
	{
		std::lock_guard<std::mutex> lock(m_Listener->Mutex);
		changes.swap(m_Listener->Changes);
	}
	m_Events.clear();
	if (changes.empty())
	{
		return;
	}

	// The pairs of bodies whose contacts changed (the lower body ID first), with how many contacts they had before the step:
	// a pair that went from none to some started touching, from some to none it stopped. The changes come from several
	// threads in no order, so only the count at the end of the step matters
	struct PairChange
	{
		uint64_t Key = 0;
		int Before = 0;
		bool HasPoint = false;
		glm::vec3 Point = glm::vec3(0.0f), Normal = glm::vec3(0.0f); // from the first body towards the second
	};
	std::unordered_map<uint64_t, size_t> pairIndex;
	std::vector<PairChange> pairs;
	for (const ScenePhysicsContactListener::Change& change : changes)
	{
		const uint32_t a = change.A.GetIndexAndSequenceNumber(), b = change.B.GetIndexAndSequenceNumber();
		const uint64_t key = a < b ? ((uint64_t)a << 32) | b : ((uint64_t)b << 32) | a;
		const auto [index, added] = pairIndex.try_emplace(key, pairs.size());
		if (added)
		{
			PairChange pair;
			pair.Key = key;
			const auto touching = m_Touching.find(key);
			pair.Before = touching != m_Touching.end() ? touching->second : 0;
			pairs.push_back(pair);
		}
		PairChange& pair = pairs[index->second];
		if (change.Delta > 0 && !pair.HasPoint)
		{
			pair.HasPoint = true;
			pair.Point = change.Point;
			pair.Normal = a < b ? change.Normal : -change.Normal;
		}
		m_Touching[key] += change.Delta;
	}

	// In the same order whatever the threads did
	std::sort(pairs.begin(), pairs.end(), [](const PairChange& x, const PairChange& y) { return x.Key < y.Key; });
	for (const PairChange& pair : pairs)
	{
		const int after = m_Touching[pair.Key];
		if (after <= 0)
		{
			m_Touching.erase(pair.Key);
		}
		const bool enter = pair.Before <= 0 && after > 0, exit = pair.Before > 0 && after <= 0;
		const auto first = m_BodyInfo.find((uint32_t)(pair.Key >> 32)), second = m_BodyInfo.find((uint32_t)pair.Key);
		if ((!enter && !exit) || first == m_BodyInfo.end() || second == m_BodyInfo.end() || first->second.Entity == second->second.Entity)
		{
			continue;
		}
		Event event;
		event.Step = m_StepCount + 1;
		event.Entity = first->second.Entity;
		event.Other = second->second.Entity;
		event.Point = enter ? pair.Point : glm::vec3(0.0f);
		event.Normal = enter ? pair.Normal : glm::vec3(0.0f);
		if (first->second.Trigger || second->second.Trigger)
		{
			event.Type = enter ? EventType::TriggerEnter : EventType::TriggerExit;
			if (!first->second.Trigger)
			{
				// The trigger first
				std::swap(event.Entity, event.Other);
				event.Normal = -event.Normal;
			}
			if (m_Scene->Exists(event.Entity) && m_Scene->Exists(event.Other))
			{
				Log::GetLogger()->info("Physics: '{0}' {1} the trigger '{2}'", m_Scene->Get<NameComponent>(event.Other).Name,
					enter ? "entered" : "left", m_Scene->Get<NameComponent>(event.Entity).Name);
			}
		}
		else
		{
			event.Type = enter ? EventType::CollisionEnter : EventType::CollisionExit;
		}
		m_Events.push_back(event);
	}

	const size_t maxRecent = 100;
	m_RecentEvents.insert(m_RecentEvents.end(), m_Events.begin(), m_Events.end());
	if (m_RecentEvents.size() > maxRecent)
	{
		m_RecentEvents.erase(m_RecentEvents.begin(), m_RecentEvents.end() - maxRecent);
	}
}

const char* ScenePhysics::GetEventName(EventType type)
{
	switch (type)
	{
		case EventType::CollisionEnter: return "Collision Enter";
		case EventType::CollisionExit:  return "Collision Exit";
		case EventType::TriggerEnter:   return "Trigger Enter";
		case EventType::TriggerExit:    return "Trigger Exit";
		default:                        return "?";
	}
}

// ---- Queries

// The layers a query hits (ObjectLayers' bits)
class LayerMaskFilter : public JPH::ObjectLayerFilter
{
public:
	explicit LayerMaskFilter(uint32_t layers) : m_Layers(layers) {}
	virtual bool ShouldCollide(JPH::ObjectLayer layer) const override { return ((m_Layers >> layer) & 1u) != 0; }

private:
	uint32_t m_Layers;
};

std::vector<JPH::BodyID> ScenePhysics::GetEntityBodies(EnvMapVulkanEntityID entity) const
{
	std::vector<JPH::BodyID> bodies;
	if (entity == NoEntity)
	{
		return bodies;
	}
	for (const Body& body : m_Bodies)
	{
		if (body.Entity == entity)
		{
			bodies.push_back(body.ID);
		}
	}
	for (const Character& character : m_Characters)
	{
		if (character.Entity == entity)
		{
			bodies.push_back(character.Controller->GetInnerBodyID());
		}
	}
	return bodies;
}

bool ScenePhysics::Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance, Hit& hit, uint32_t layers,
	EnvMapVulkanEntityID ignore) const
{
	const float length = glm::length(direction);
	if (!m_World || length < 1e-6f || maxDistance <= 0.0f)
	{
		return false;
	}
	const glm::vec3 unit = direction / length;
	const JPH::RRayCast ray(ToJolt(origin), ToJolt(unit * maxDistance));
	JPH::RayCastResult result;
	const LayerMaskFilter layerFilter(layers);
	JPH::IgnoreMultipleBodiesFilter bodyFilter;
	for (JPH::BodyID body : GetEntityBodies(ignore))
	{
		bodyFilter.IgnoreBody(body);
	}
	const JPH::PhysicsSystem& system = m_World->GetSystem();
	if (!system.GetNarrowPhaseQuery().CastRay(ray, result, JPH::BroadPhaseLayerFilter(), layerFilter, bodyFilter))
	{
		return false;
	}
	hit.Entity = GetBodyEntity(result.mBodyID);
	hit.Point = ToGlm(ray.GetPointOnRay(result.mFraction));
	hit.Distance = result.mFraction * maxDistance;
	hit.Normal = -unit;
	JPH::BodyLockRead lock(system.GetBodyLockInterface(), result.mBodyID);
	if (lock.Succeeded())
	{
		hit.Normal = ToGlm(lock.GetBody().GetWorldSpaceSurfaceNormal(result.mSubShapeID2, ToJolt(hit.Point)));
	}
	return true;
}

bool ScenePhysics::SphereCast(const glm::vec3& origin, float radius, const glm::vec3& direction, float maxDistance, Hit& hit,
	uint32_t layers, EnvMapVulkanEntityID ignore) const
{
	const float length = glm::length(direction);
	if (!m_World || length < 1e-6f || maxDistance <= 0.0f || radius <= 0.0f)
	{
		return false;
	}
	const glm::vec3 unit = direction / length;
	JPH::SphereShape sphere(radius);
	sphere.SetEmbedded(); // on the stack: not reference counted
	const JPH::RShapeCast cast(&sphere, JPH::Vec3::sOne(), JPH::RMat44::sTranslation(ToJolt(origin)), ToJolt(unit * maxDistance));
	JPH::ShapeCastSettings settings;
	JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> collector;
	const LayerMaskFilter layerFilter(layers);
	JPH::IgnoreMultipleBodiesFilter bodyFilter;
	for (JPH::BodyID body : GetEntityBodies(ignore))
	{
		bodyFilter.IgnoreBody(body);
	}
	m_World->GetSystem().GetNarrowPhaseQuery().CastShape(cast, settings, JPH::RVec3::sZero(), collector, JPH::BroadPhaseLayerFilter(),
		layerFilter, bodyFilter);
	if (!collector.HadHit())
	{
		return false;
	}
	const JPH::ShapeCastResult& result = collector.mHit;
	hit.Entity = GetBodyEntity(result.mBodyID2);
	hit.Point = ToGlm(result.mContactPointOn2);
	hit.Distance = result.mFraction * maxDistance;
	// The axis the sphere would move along to go deeper into the other shape: the surface faces the other way
	const glm::vec3 axis = ToGlm(result.mPenetrationAxis);
	hit.Normal = glm::length(axis) > 1e-6f ? -glm::normalize(axis) : -unit;
	return true;
}

std::vector<EnvMapVulkanEntityID> ScenePhysics::Overlap(const JPH::Shape& shape, const JPH::RMat44& transform, uint32_t layers) const
{
	std::vector<EnvMapVulkanEntityID> entities;
	if (!m_World)
	{
		return entities;
	}
	JPH::CollideShapeSettings settings;
	JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;
	const LayerMaskFilter layerFilter(layers);
	m_World->GetSystem().GetNarrowPhaseQuery().CollideShape(&shape, JPH::Vec3::sOne(), transform, settings, JPH::RVec3::sZero(), collector,
		JPH::BroadPhaseLayerFilter(), layerFilter);
	for (const JPH::CollideShapeResult& result : collector.mHits)
	{
		const EnvMapVulkanEntityID entity = GetBodyEntity(result.mBodyID2);
		if (entity != NoEntity && std::find(entities.begin(), entities.end(), entity) == entities.end())
		{
			entities.push_back(entity);
		}
	}
	return entities;
}

std::vector<EnvMapVulkanEntityID> ScenePhysics::OverlapSphere(const glm::vec3& center, float radius, uint32_t layers) const
{
	JPH::SphereShape sphere(std::max(radius, 0.001f));
	sphere.SetEmbedded();
	return Overlap(sphere, JPH::RMat44::sTranslation(ToJolt(center)), layers);
}

std::vector<EnvMapVulkanEntityID> ScenePhysics::OverlapBox(const glm::vec3& center, const glm::vec3& halfExtents, const glm::quat& rotation,
	uint32_t layers) const
{
	JPH::BoxShape box(ToJolt(glm::max(halfExtents, glm::vec3(0.001f))), 0.0f);
	box.SetEmbedded();
	return Overlap(box, JPH::RMat44::sRotationTranslation(ToJolt(rotation), ToJolt(center)), layers);
}

bool ScenePhysics::OverlapsCapsule(const glm::vec3& bottom, const glm::vec3& top, float radius, uint32_t layers) const
{
	if (!m_World)
	{
		return false;
	}
	const float halfHeight = std::max(0.5f * glm::length(top - bottom), 0.001f);
	JPH::CapsuleShape capsule(halfHeight, std::max(radius, 0.001f)); // along its y: the capsule is upright
	capsule.SetEmbedded();
	JPH::CollideShapeSettings settings;
	settings.mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces; // a mesh's triangles from either side (a road seen from below)
	JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;
	const LayerMaskFilter layerFilter(layers);
	m_World->GetSystem().GetNarrowPhaseQuery().CollideShape(&capsule, JPH::Vec3::sOne(), JPH::RMat44::sTranslation(ToJolt(0.5f * (bottom + top))), settings,
		JPH::RVec3::sZero(), collector, JPH::BroadPhaseLayerFilter(), layerFilter);
	return collector.HadHit();
}

// ---- Debug views

using DebugLines = std::vector<ScenePhysics::DebugLine>;

static void AddLine(DebugLines& lines, const glm::mat4& transform, const glm::vec3& a, const glm::vec3& b, ScenePhysics::DebugLineKind kind)
{
	lines.push_back({ glm::vec3(transform * glm::vec4(a, 1.0f)), glm::vec3(transform * glm::vec4(b, 1.0f)), kind });
}

// An arc around center in the plane of the unit vectors u and v, from angle "from" to "to" (radians)
static void AddArc(DebugLines& lines, const glm::mat4& transform, const glm::vec3& center, const glm::vec3& u, const glm::vec3& v, float radius,
	float from, float to, int segments, ScenePhysics::DebugLineKind kind)
{
	const float step = (to - from) / segments;
	for (int i = 0; i < segments; i++)
	{
		const float a = from + i * step, b = a + step;
		AddLine(lines, transform, center + radius * (std::cos(a) * u + std::sin(a) * v), center + radius * (std::cos(b) * u + std::sin(b) * v), kind);
	}
}

static const glm::vec3 AxisX(1.0f, 0.0f, 0.0f), AxisY(0.0f, 1.0f, 0.0f), AxisZ(0.0f, 0.0f, 1.0f);

static void AddBoxLines(DebugLines& lines, const glm::mat4& transform, const glm::vec3& half, ScenePhysics::DebugLineKind kind)
{
	for (int axis = 0; axis < 3; axis++)
	{
		// The four edges along this axis
		const int u = (axis + 1) % 3, v = (axis + 2) % 3;
		for (int corner = 0; corner < 4; corner++)
		{
			glm::vec3 a(0.0f);
			a[u] = (corner & 1) ? half[u] : -half[u];
			a[v] = (corner & 2) ? half[v] : -half[v];
			glm::vec3 b = a;
			a[axis] = -half[axis];
			b[axis] = half[axis];
			AddLine(lines, transform, a, b, kind);
		}
	}
}

static void AddSphereLines(DebugLines& lines, const glm::mat4& transform, float radius, ScenePhysics::DebugLineKind kind)
{
	const float full = glm::two_pi<float>();
	AddArc(lines, transform, glm::vec3(0.0f), AxisX, AxisY, radius, 0.0f, full, 32, kind);
	AddArc(lines, transform, glm::vec3(0.0f), AxisX, AxisZ, radius, 0.0f, full, 32, kind);
	AddArc(lines, transform, glm::vec3(0.0f), AxisY, AxisZ, radius, 0.0f, full, 32, kind);
}

// Standing along Y: halfHeight is its middle part's
static void AddCapsuleLines(DebugLines& lines, const glm::mat4& transform, float halfHeight, float radius, ScenePhysics::DebugLineKind kind)
{
	if (halfHeight <= 0.0f)
	{
		AddSphereLines(lines, transform, radius, kind);
		return;
	}
	const float pi = glm::pi<float>();
	const glm::vec3 top(0.0f, halfHeight, 0.0f), bottom(0.0f, -halfHeight, 0.0f);
	AddArc(lines, transform, top, AxisX, AxisZ, radius, 0.0f, 2.0f * pi, 32, kind);
	AddArc(lines, transform, bottom, AxisX, AxisZ, radius, 0.0f, 2.0f * pi, 32, kind);
	AddArc(lines, transform, top, AxisX, AxisY, radius, 0.0f, pi, 16, kind);
	AddArc(lines, transform, top, AxisZ, AxisY, radius, 0.0f, pi, 16, kind);
	AddArc(lines, transform, bottom, AxisX, AxisY, radius, pi, 2.0f * pi, 16, kind);
	AddArc(lines, transform, bottom, AxisZ, AxisY, radius, pi, 2.0f * pi, 16, kind);
	for (const glm::vec3& side : { AxisX, -AxisX, AxisZ, -AxisZ })
	{
		AddLine(lines, transform, bottom + side * radius, top + side * radius, kind);
	}
}

// A Jolt shape, in the space of its center of mass (transform: that space in the world, as Jolt draws its shapes). The
// simple shapes as their outlines, a hull as its faces, anything else (a mesh) as its triangles, up to triangleBudget
static void AddShapeLines(DebugLines& lines, const JPH::Shape& shape, const glm::mat4& transform, ScenePhysics::DebugLineKind kind,
	uint32_t& triangleBudget)
{
	switch (shape.GetSubType())
	{
		case JPH::EShapeSubType::RotatedTranslated:
		{
			// Its inner shape's center of mass is at its own, turned
			const JPH::RotatedTranslatedShape& moved = static_cast<const JPH::RotatedTranslatedShape&>(shape);
			AddShapeLines(lines, *moved.GetInnerShape(), transform * glm::mat4_cast(ToGlm(moved.GetRotation())), kind, triangleBudget);
			return;
		}
		case JPH::EShapeSubType::OffsetCenterOfMass:
		{
			// Its inner shape's center of mass is the offset away from its own
			const JPH::OffsetCenterOfMassShape& offset = static_cast<const JPH::OffsetCenterOfMassShape&>(shape);
			AddShapeLines(lines, *offset.GetInnerShape(), transform * glm::translate(glm::mat4(1.0f), -ToGlm(offset.GetOffset())), kind, triangleBudget);
			return;
		}
		case JPH::EShapeSubType::Box:
			AddBoxLines(lines, transform, ToGlm(static_cast<const JPH::BoxShape&>(shape).GetHalfExtent()), kind);
			return;
		case JPH::EShapeSubType::Sphere:
			AddSphereLines(lines, transform, static_cast<const JPH::SphereShape&>(shape).GetRadius(), kind);
			return;
		case JPH::EShapeSubType::Capsule:
		{
			const JPH::CapsuleShape& capsule = static_cast<const JPH::CapsuleShape&>(shape);
			AddCapsuleLines(lines, transform, capsule.GetHalfHeightOfCylinder(), capsule.GetRadius(), kind);
			return;
		}
		case JPH::EShapeSubType::ConvexHull:
		{
			const JPH::ConvexHullShape& hull = static_cast<const JPH::ConvexHullShape&>(shape);
			std::vector<JPH::uint> face(hull.GetNumPoints());
			for (JPH::uint f = 0; f < hull.GetNumFaces(); f++)
			{
				const JPH::uint count = hull.GetFaceVertices(f, (JPH::uint)face.size(), face.data());
				for (JPH::uint i = 0; i < count; i++)
				{
					AddLine(lines, transform, ToGlm(hull.GetPoint(face[i])), ToGlm(hull.GetPoint(face[(i + 1) % count])), kind);
				}
			}
			return;
		}
		default:
		{
			JPH::Shape::GetTrianglesContext context;
			shape.GetTrianglesStart(context, JPH::AABox::sBiggest(), JPH::Vec3::sZero(), JPH::Quat::sIdentity(), JPH::Vec3::sOne());
			JPH::Float3 vertices[JPH::Shape::cGetTrianglesMinTrianglesRequested * 3];
			while (triangleBudget > 0)
			{
				const int count = shape.GetTrianglesNext(context, JPH::Shape::cGetTrianglesMinTrianglesRequested, vertices);
				if (count <= 0)
				{
					break;
				}
				for (int t = 0; t < count && triangleBudget > 0; t++, triangleBudget--)
				{
					const glm::vec3 a(vertices[t * 3].x, vertices[t * 3].y, vertices[t * 3].z);
					const glm::vec3 b(vertices[t * 3 + 1].x, vertices[t * 3 + 1].y, vertices[t * 3 + 1].z);
					const glm::vec3 c(vertices[t * 3 + 2].x, vertices[t * 3 + 2].y, vertices[t * 3 + 2].z);
					AddLine(lines, transform, a, b, kind);
					AddLine(lines, transform, b, c, kind);
					AddLine(lines, transform, c, a, kind);
				}
			}
			return;
		}
	}
}

// A contact: a small cross at the point, and its normal
static void AddContactLines(DebugLines& lines, const glm::vec3& point, const glm::vec3& normal)
{
	const float size = 0.04f, normalLength = 0.25f;
	const glm::mat4 identity(1.0f);
	for (const glm::vec3& axis : { AxisX, AxisY, AxisZ })
	{
		AddLine(lines, identity, point - axis * size, point + axis * size, ScenePhysics::DebugLineKind::Contact);
	}
	AddLine(lines, identity, point, point + normal * normalLength, ScenePhysics::DebugLineKind::Contact);
}

void ScenePhysics::SetCollectContacts(bool collect)
{
	m_CollectContacts = collect;
	if (m_Listener)
	{
		m_Listener->Collect = collect; // between the steps: the listener isn't running
		if (!collect)
		{
			m_Listener->Contacts.clear();
		}
	}
}

// A wheel: its two rims (transform: the wheel's center, X along its axle), its axle and a spoke that shows it turn
static void AddWheelLines(DebugLines& lines, const glm::mat4& transform, float radius, float width, ScenePhysics::DebugLineKind kind)
{
	const float full = glm::two_pi<float>();
	for (float side : { -0.5f * width, 0.5f * width })
	{
		AddArc(lines, transform, glm::vec3(side, 0.0f, 0.0f), AxisY, AxisZ, radius, 0.0f, full, 24, kind);
	}
	AddLine(lines, transform, glm::vec3(0.0f), glm::vec3(0.0f, radius, 0.0f), kind);
	AddLine(lines, transform, glm::vec3(-0.5f * width, 0.0f, 0.0f), glm::vec3(0.5f * width, 0.0f, 0.0f), kind);
}

void ScenePhysics::GetDebugLines(bool colliders, bool contacts, std::vector<DebugLine>& lines, uint32_t maxTriangles) const
{
	if (!m_World)
	{
		return;
	}
	const JPH::BodyInterface& bodies = m_World->GetSystem().GetBodyInterface();
	if (colliders)
	{
		uint32_t triangleBudget = maxTriangles;
		for (const Body& body : m_Bodies)
		{
			glm::vec3 position;
			glm::quat rotation;
			if (body.Motion == JPH::EMotionType::Dynamic)
			{
				position = glm::mix(body.PreviousPosition, body.Position, m_Alpha);
				rotation = glm::slerp(body.PreviousRotation, body.Rotation, m_Alpha);
			}
			else
			{
				JPH::RVec3 joltPosition;
				JPH::Quat joltRotation;
				bodies.GetPositionAndRotation(body.ID, joltPosition, joltRotation);
				position = ToGlm(joltPosition);
				rotation = ToGlm(joltRotation);
			}
			const DebugLineKind kind = body.Trigger ? DebugLineKind::Trigger : body.Motion == JPH::EMotionType::Static ? DebugLineKind::Static
				: bodies.IsActive(body.ID) ? DebugLineKind::Active : DebugLineKind::Sleeping;
			const JPH::RefConst<JPH::Shape> shape = bodies.GetShape(body.ID);
			const glm::mat4 centerOfMass = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation) *
				glm::translate(glm::mat4(1.0f), ToGlm(shape->GetCenterOfMass()));
			AddShapeLines(lines, *shape, centerOfMass, kind, triangleBudget);
		}
		for (const Character& character : m_Characters)
		{
			const JPH::CharacterVirtual& controller = *character.Controller;
			const glm::vec3 position = glm::mix(character.PreviousPosition, character.Position, m_Alpha) +
				ToGlm(controller.GetShapeOffset() + controller.GetShape()->GetCenterOfMass());
			AddShapeLines(lines, *controller.GetShape(), glm::translate(glm::mat4(1.0f), position), DebugLineKind::Character, triangleBudget);
		}
		// The vehicles' wheels where they are (touching the ground or not), each with its spring: from where it hangs to its center
		for (const Vehicle& vehicle : m_Vehicles)
		{
			if (!m_Scene->Exists(vehicle.Entity))
			{
				continue;
			}
			glm::vec3 position, scale;
			glm::quat rotation;
			DecomposeWorld(m_Scene->GetWorldTransform(vehicle.Entity), position, rotation, scale);
			const glm::mat4 body = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation);
			for (uint32_t w = 0; w < (uint32_t)vehicle.Constraint->GetWheels().size(); w++)
			{
				const JPH::Wheel& wheel = *vehicle.Constraint->GetWheel(w);
				const JPH::WheelSettings& settings = *wheel.GetSettings();
				const DebugLineKind kind = wheel.HasContact() ? DebugLineKind::WheelOnGround : DebugLineKind::Wheel;
				const glm::mat4 local = ToGlm(vehicle.Constraint->GetWheelLocalTransform(w, JPH::Vec3::sAxisX(), JPH::Vec3::sAxisY()));
				AddWheelLines(lines, body * local, settings.mRadius, settings.mWidth, kind);
				AddLine(lines, body, ToGlm(settings.mPosition), glm::vec3(local[3]), kind);
			}
		}
	}
	if (contacts && m_Listener)
	{
		for (const ScenePhysicsContactListener::Contact& contact : m_Listener->Contacts)
		{
			AddContactLines(lines, contact.Point, contact.Normal);
		}
		// The characters' own (they aren't bodies: Jolt's listener doesn't see them)
		for (const Character& character : m_Characters)
		{
			for (const JPH::CharacterContact& contact : character.Controller->GetActiveContacts())
			{
				if (contact.mHadCollision)
				{
					AddContactLines(lines, ToGlm(contact.mPosition), ToGlm(contact.mContactNormal));
				}
			}
		}
	}
}

void ScenePhysics::GetColliderLines(const EnvMapVulkanScene& scene, std::vector<DebugLine>& lines)
{
	// As CreateShape makes them from the world scale (the mesh shapes aren't made here: they take time)
	scene.Each<ColliderComponent>([&](EnvMapVulkanEntityID entity, const ColliderComponent& collider)
	{
		if (scene.Has<CharacterControllerComponent>(entity))
		{
			return;
		}
		glm::vec3 position, scale;
		glm::quat rotation;
		DecomposeWorld(scene.GetWorldTransform(entity), position, rotation, scale);
		const glm::vec3 size = glm::abs(scale);
		const RigidBodyComponent* rigidBody = scene.TryGet<RigidBodyComponent>(entity);
		const DebugLineKind kind = collider.IsTrigger ? DebugLineKind::Trigger
			: rigidBody && rigidBody->Type != RigidBodyComponent::BodyType::Static ? DebugLineKind::Active : DebugLineKind::Static;
		const glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation) *
			glm::translate(glm::mat4(1.0f), collider.Offset * scale);
		switch (collider.Type)
		{
			case ColliderComponent::Shape::Box:
				AddBoxLines(lines, transform, glm::max(collider.HalfExtents * size, glm::vec3(0.001f)), kind);
				break;
			case ColliderComponent::Shape::Sphere:
				AddSphereLines(lines, transform, std::max(collider.Radius * std::max({ size.x, size.y, size.z }), 0.001f), kind);
				break;
			case ColliderComponent::Shape::Capsule:
				AddCapsuleLines(lines, transform, collider.HalfHeight * size.y, std::max(collider.Radius * std::max(size.x, size.z), 0.001f), kind);
				break;
			default:
				break;
		}
	});
	scene.Each<CharacterControllerComponent>([&](EnvMapVulkanEntityID entity, const CharacterControllerComponent& character)
	{
		const float radius = std::max(character.Radius, 0.01f);
		const glm::vec3 position = glm::vec3(scene.GetWorldTransform(entity)[3]) + character.Offset;
		AddCapsuleLines(lines, glm::translate(glm::mat4(1.0f), position), std::max(character.Height * 0.5f - radius, 0.0f), radius,
			DebugLineKind::Character);
	});
	// The vehicles' wheels as placed (their springs fully out), each with its spring's travel above it
	scene.Each<VehicleComponent>([&](EnvMapVulkanEntityID entity, const VehicleComponent& vehicle)
	{
		glm::vec3 position, scale;
		glm::quat rotation;
		DecomposeWorld(scene.GetWorldTransform(entity), position, rotation, scale);
		const glm::mat4 body = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation);
		for (const VehicleComponent::Wheel& wheel : vehicle.Wheels)
		{
			AddWheelLines(lines, body * glm::translate(glm::mat4(1.0f), wheel.Position), std::max(wheel.Radius, 0.01f), std::max(wheel.Width, 0.01f),
				DebugLineKind::Wheel);
			AddLine(lines, body, wheel.Position, wheel.Position + glm::vec3(0.0f, std::max(vehicle.SuspensionTravel, 0.01f), 0.0f), DebugLineKind::Wheel);
		}
	});
}

ScenePhysics::Stats ScenePhysics::GetStats() const
{
	Stats stats;
	if (!m_World)
	{
		return stats;
	}
	for (const Body& body : m_Bodies)
	{
		if (body.Trigger)
		{
			stats.Triggers++;
		}
		else if (body.Motion == JPH::EMotionType::Dynamic)
		{
			stats.Dynamic++;
		}
		else if (body.Motion == JPH::EMotionType::Kinematic)
		{
			stats.Kinematic++;
		}
		else
		{
			stats.Static++;
		}
	}
	stats.Characters = (uint32_t)m_Characters.size();
	stats.Vehicles = (uint32_t)m_Vehicles.size();
	stats.Awake = m_World->GetActiveBodyCount();
	stats.TouchingPairs = (uint32_t)m_Touching.size();
	stats.Steps = m_StepCount;
	stats.LastStepMs = m_LastStepMs;
	stats.AverageStepMs = m_StepCount > 0 ? (float)(m_StepSeconds * 1000.0 / m_StepCount) : 0.0f;
	return stats;
}
