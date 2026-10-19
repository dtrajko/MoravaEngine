#include "EnvMapVulkanScene.h"

#include "EnvMapVulkanMaterialLibrary.h" // the components' references, copied by CopyFrom
#include "H2M/Renderer/ModelH2M.h"

#include "Core/Log.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <random>


// TransformComponent

// The matrix from the components and back, computed as ImGuizmo computes them (RecomposeMatrixFromComponents,
// DecomposeMatrixToComponents; https://github.com/CedricGuillemet/ImGuizmo, MIT license, Copyright (c) 2016-2026 Cedric
// Guillemet and contributors), operation by operation: the gizmo the editor draws with ImGuizmo and the scene agree to the
// last bit, and the core needs no ImGui. A matrix is 16 floats in glm's layout, m[column * 4 + row] (ImGuizmo's rows).
namespace GizmoMath
{
	constexpr float Pi = 3.14159265358979323846f;
	constexpr float RadiansToDegrees = 180.f / Pi;
	constexpr float DegreesToRadians = Pi / 180.f;

	static void Multiply(const float* a, const float* b, float* r)
	{
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				r[i * 4 + j] = a[i * 4 + 0] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j] + a[i * 4 + 3] * b[12 + j];
			}
		}
	}

	static float Length(const float* v)
	{
		return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	}

	static void Normalize(float* v)
	{
		const float scale = 1.f / (Length(v) > FLT_EPSILON ? Length(v) : FLT_EPSILON);
		for (int i = 0; i < 4; i++)
		{
			v[i] *= scale;
		}
	}

	static void RotationAxis(const float* axis, float angle, float* m)
	{
		const float length2 = axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2];
		if (length2 < FLT_EPSILON)
		{
			for (int i = 0; i < 16; i++)
			{
				m[i] = i % 5 == 0 ? 1.f : 0.f;
			}
			return;
		}

		const float inverseLength = 1.f / sqrtf(length2);
		const float nx = axis[0] * inverseLength, ny = axis[1] * inverseLength, nz = axis[2] * inverseLength;
		const float s = sinf(angle);
		const float c = cosf(angle);
		const float k = 1.f - c;

		const float xx = nx * nx * k + c;
		const float yy = ny * ny * k + c;
		const float zz = nz * nz * k + c;
		const float xy = nx * ny * k;
		const float yz = ny * nz * k;
		const float zx = nz * nx * k;
		const float xs = nx * s;
		const float ys = ny * s;
		const float zs = nz * s;

		const float result[16] = {
			xx,      xy + zs, zx - ys, 0.f,
			xy - zs, yy,      yz + xs, 0.f,
			zx + ys, yz - xs, zz,      0.f,
			0.f,     0.f,     0.f,     1.f };
		for (int i = 0; i < 16; i++)
		{
			m[i] = result[i];
		}
	}

	static void Recompose(const float* translation, const float* rotation, const float* scale, float* m)
	{
		static const float axes[3][4] = { { 1.f, 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f, 0.f }, { 0.f, 0.f, 1.f, 0.f } };
		float rot[3][16];
		for (int i = 0; i < 3; i++)
		{
			RotationAxis(axes[i], rotation[i] * DegreesToRadians, rot[i]);
		}
		float rot01[16];
		Multiply(rot[0], rot[1], rot01);
		Multiply(rot01, rot[2], m);

		for (int i = 0; i < 3; i++)
		{
			const float validScale = fabsf(scale[i]) < FLT_EPSILON ? 0.001f : scale[i];
			for (int j = 0; j < 4; j++)
			{
				m[i * 4 + j] *= validScale;
			}
		}
		m[12] = translation[0];
		m[13] = translation[1];
		m[14] = translation[2];
		m[15] = 1.f;
	}

	static void Decompose(const float* matrix, float* translation, float* rotation, float* scale)
	{
		float m[16];
		for (int i = 0; i < 16; i++)
		{
			m[i] = matrix[i];
		}

		scale[0] = Length(&m[0]);
		scale[1] = Length(&m[4]);
		scale[2] = Length(&m[8]);

		Normalize(&m[0]);
		Normalize(&m[4]);
		Normalize(&m[8]);

		rotation[0] = RadiansToDegrees * atan2f(m[1 * 4 + 2], m[2 * 4 + 2]);
		rotation[1] = RadiansToDegrees * atan2f(-m[0 * 4 + 2], sqrtf(m[1 * 4 + 2] * m[1 * 4 + 2] + m[2 * 4 + 2] * m[2 * 4 + 2]));
		rotation[2] = RadiansToDegrees * atan2f(m[0 * 4 + 1], m[0 * 4 + 0]);

		translation[0] = m[12];
		translation[1] = m[13];
		translation[2] = m[14];
	}
}

glm::mat4 TransformComponent::GetMatrix() const
{
	glm::mat4 matrix;
	GizmoMath::Recompose(&Translation.x, &Rotation.x, &Scale.x, glm::value_ptr(matrix));
	return matrix;
}

void TransformComponent::SetMatrix(const glm::mat4& matrix)
{
	GizmoMath::Decompose(glm::value_ptr(matrix), &Translation.x, &Rotation.x, &Scale.x);
}

glm::vec3 TransformComponent::GetLightDirection() const
{
	glm::vec3 down = glm::vec3(GetMatrix() * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f));
	float length = glm::length(down);
	return length > 0.0f ? down / length : glm::vec3(0.0f, -1.0f, 0.0f);
}

void TransformComponent::SetLightDirection(const glm::vec3& direction)
{
	if (glm::length(direction) <= 0.0f)
	{
		return;
	}
	// The shortest turn of -Y onto the direction (no roll around it), kept with the translation and the scale
	glm::quat turn = glm::rotation(glm::vec3(0.0f, -1.0f, 0.0f), glm::normalize(direction));
	glm::mat4 matrix = glm::translate(glm::mat4(1.0f), Translation) * glm::toMat4(turn) * glm::scale(glm::mat4(1.0f), Scale);
	SetMatrix(matrix);
}

// EnvMapVulkanScene

EnvMapVulkanEntityID EnvMapVulkanScene::NewID()
{
	static std::mt19937_64 s_Generator(std::random_device{}());
	EnvMapVulkanEntityID id = NoEntity;
	while (id == NoEntity)
	{
		id = s_Generator();
	}
	return id;
}

entt::entity EnvMapVulkanScene::Handle(EnvMapVulkanEntityID id) const
{
	auto it = m_Entities.find(id);
	return it != m_Entities.end() ? it->second : entt::null;
}

std::vector<EnvMapVulkanEntityID>& EnvMapVulkanScene::SiblingList(EnvMapVulkanEntityID parent)
{
	return parent == NoEntity ? m_Roots : Get<HierarchyComponent>(parent).Children;
}

EnvMapVulkanEntityID EnvMapVulkanScene::CreateEntity(const std::string& name, EnvMapVulkanEntityID parent, EnvMapVulkanEntityID id)
{
	if (id == NoEntity || Exists(id))
	{
		id = NewID();
	}
	if (parent != NoEntity && !Exists(parent))
	{
		parent = NoEntity;
	}
	entt::entity entity = m_Registry.create();
	m_Entities[id] = entity;
	m_Registry.emplace<IDComponent>(entity, id);
	m_Registry.emplace<NameComponent>(entity, name);
	m_Registry.emplace<HierarchyComponent>(entity).Parent = parent;
	m_Registry.emplace<TransformComponent>(entity);
	SiblingList(parent).push_back(id);
	return id;
}

void EnvMapVulkanScene::DestroyEntity(EnvMapVulkanEntityID id)
{
	if (!Exists(id))
	{
		return;
	}
	// The children first (a copy: destroying a child edits this entity's list)
	std::vector<EnvMapVulkanEntityID> children = Get<HierarchyComponent>(id).Children;
	for (EnvMapVulkanEntityID child : children)
	{
		DestroyEntity(child);
	}
	std::vector<EnvMapVulkanEntityID>& siblings = SiblingList(Get<HierarchyComponent>(id).Parent);
	siblings.erase(std::remove(siblings.begin(), siblings.end(), id), siblings.end());
	m_Registry.destroy(Handle(id));
	m_Entities.erase(id);
}

void EnvMapVulkanScene::Clear()
{
	m_Registry.clear();
	m_Entities.clear();
	m_Roots.clear();
}

void EnvMapVulkanScene::CopyFrom(const EnvMapVulkanScene& other)
{
	if (&other == this)
	{
		return;
	}
	Clear();

	// The entities, in the hierarchy's order, with their IDs and nothing else yet
	auto createEntity = [&](auto& createEntity, EnvMapVulkanEntityID id) -> void
	{
		const entt::entity entity = m_Registry.create();
		m_Entities[id] = entity;
		m_Registry.emplace<IDComponent>(entity, id);
		for (EnvMapVulkanEntityID child : other.GetChildren(id))
		{
			createEntity(createEntity, child);
		}
	};
	for (EnvMapVulkanEntityID root : other.m_Roots)
	{
		createEntity(createEntity, root);
	}

	// Their components, type by type
	SceneComponents::ForEach([&](auto type)
	{
		using T = typename decltype(type)::Type;
		auto view = other.m_Registry.view<const IDComponent, const T>();
		for (entt::entity entity : view)
		{
			m_Registry.emplace<T>(Handle(view.template get<const IDComponent>(entity).ID), view.template get<const T>(entity));
		}
	});
	m_Roots = other.m_Roots;
}

const std::vector<EnvMapVulkanEntityID>& EnvMapVulkanScene::GetChildren(EnvMapVulkanEntityID id) const
{
	static const std::vector<EnvMapVulkanEntityID> s_None;
	return Exists(id) ? Get<HierarchyComponent>(id).Children : s_None;
}

EnvMapVulkanEntityID EnvMapVulkanScene::GetParent(EnvMapVulkanEntityID id) const
{
	return Exists(id) ? Get<HierarchyComponent>(id).Parent : NoEntity;
}

bool EnvMapVulkanScene::IsDescendantOf(EnvMapVulkanEntityID id, EnvMapVulkanEntityID ancestor) const
{
	for (EnvMapVulkanEntityID parent = GetParent(id); parent != NoEntity; parent = GetParent(parent))
	{
		if (parent == ancestor)
		{
			return true;
		}
	}
	return false;
}

bool EnvMapVulkanScene::SetParent(EnvMapVulkanEntityID id, EnvMapVulkanEntityID parent, bool keepWorldTransform, int index)
{
	if (!Exists(id) || (parent != NoEntity && (!Exists(parent) || parent == id || IsDescendantOf(parent, id))))
	{
		return false;
	}
	const glm::mat4 world = GetWorldTransform(id);

	HierarchyComponent& hierarchy = Get<HierarchyComponent>(id);
	std::vector<EnvMapVulkanEntityID>& oldSiblings = SiblingList(hierarchy.Parent);
	oldSiblings.erase(std::remove(oldSiblings.begin(), oldSiblings.end(), id), oldSiblings.end());

	hierarchy.Parent = parent;
	std::vector<EnvMapVulkanEntityID>& newSiblings = SiblingList(parent);
	if (index < 0 || index >= (int)newSiblings.size())
	{
		newSiblings.push_back(id);
	}
	else
	{
		newSiblings.insert(newSiblings.begin() + index, id);
	}

	if (keepWorldTransform)
	{
		SetWorldTransform(id, world);
	}
	return true;
}

glm::mat4 EnvMapVulkanScene::GetWorldTransform(EnvMapVulkanEntityID id) const
{
	glm::mat4 world(1.0f);
	for (EnvMapVulkanEntityID current = id; current != NoEntity && Exists(current); current = GetParent(current))
	{
		world = Get<TransformComponent>(current).GetMatrix() * world;
	}
	return world;
}

void EnvMapVulkanScene::SetWorldTransform(EnvMapVulkanEntityID id, const glm::mat4& world)
{
	if (!Exists(id))
	{
		return;
	}
	EnvMapVulkanEntityID parent = GetParent(id);
	glm::mat4 local = parent != NoEntity ? glm::inverse(GetWorldTransform(parent)) * world : world;
	Get<TransformComponent>(id).SetMatrix(local);
}

// Self-test

static bool Near(const glm::mat4& a, const glm::mat4& b, float epsilon = 1e-3f)
{
	for (int c = 0; c < 4; c++)
	{
		for (int r = 0; r < 4; r++)
		{
			if (std::abs(a[c][r] - b[c][r]) > epsilon)
			{
				return false;
			}
		}
	}
	return true;
}

bool EnvMapVulkanScene::SelfTest()
{
	auto fail = [](const char* what) {
		Log::GetLogger()->error("EnvMapVulkanScene self-test failed: {0}", what);
		return false;
	};

	EnvMapVulkanScene scene;
	EnvMapVulkanEntityID root = scene.CreateEntity("Root");
	EnvMapVulkanEntityID child = scene.CreateEntity("Child", root);
	EnvMapVulkanEntityID grandchild = scene.CreateEntity("Grandchild", child);
	EnvMapVulkanEntityID other = scene.CreateEntity("Other");
	if (scene.GetEntityCount() != 4 || scene.GetRoots().size() != 2 || scene.GetChildren(root).size() != 1 || scene.GetParent(grandchild) != child)
	{
		return fail("create / hierarchy");
	}
	if (scene.Get<NameComponent>(child).Name != "Child" || scene.Get<IDComponent>(child).ID != child)
	{
		return fail("ID and name components");
	}

	// World transforms: parents' transforms apply to their children
	scene.Get<TransformComponent>(root).Translation = glm::vec3(10.0f, 0.0f, 0.0f);
	scene.Get<TransformComponent>(root).Rotation = glm::vec3(0.0f, 90.0f, 0.0f);
	scene.Get<TransformComponent>(child).Translation = glm::vec3(0.0f, 0.0f, 5.0f);
	scene.Get<TransformComponent>(grandchild).Scale = glm::vec3(2.0f);
	glm::mat4 expected = scene.Get<TransformComponent>(root).GetMatrix() * scene.Get<TransformComponent>(child).GetMatrix() *
		scene.Get<TransformComponent>(grandchild).GetMatrix();
	glm::mat4 grandchildWorld = scene.GetWorldTransform(grandchild);
	if (!Near(grandchildWorld, expected))
	{
		return fail("world transform");
	}

	// A copy has the same entities, IDs, hierarchy and components, and changes to it don't reach the original
	scene.Add<CameraComponent>(other).FieldOfView = 60.0f;
	{
		EnvMapVulkanScene copy;
		copy.CopyFrom(scene);
		if (copy.GetEntityCount() != scene.GetEntityCount() || copy.GetRoots() != scene.GetRoots() ||
			copy.GetChildren(root) != scene.GetChildren(root) || copy.GetParent(grandchild) != child ||
			copy.Get<NameComponent>(child).Name != "Child" || copy.Get<IDComponent>(grandchild).ID != grandchild ||
			!Near(copy.GetWorldTransform(grandchild), grandchildWorld) || !copy.Has<CameraComponent>(other) ||
			copy.Get<CameraComponent>(other).FieldOfView != 60.0f || copy.Has<CameraComponent>(root))
		{
			return fail("copy");
		}
		copy.Get<TransformComponent>(root).Translation.x = 20.0f;
		copy.DestroyEntity(other);
		if (scene.Get<TransformComponent>(root).Translation.x != 10.0f || !scene.Exists(other))
		{
			return fail("copy independent of the original");
		}
	}

	// Reparenting keeps the world transform; cycles are refused
	if (!scene.SetParent(grandchild, other) || scene.GetChildren(child).size() != 0 || scene.GetParent(grandchild) != other ||
		!Near(scene.GetWorldTransform(grandchild), grandchildWorld))
	{
		return fail("reparent (keep world transform)");
	}
	if (scene.SetParent(root, child) || scene.SetParent(root, root))
	{
		return fail("cycles refused");
	}
	if (!scene.SetParent(child, NoEntity, true, 0) || scene.GetRoots().front() != child)
	{
		return fail("reparent to the roots at a position");
	}

	// Light directions round-trip through the transform
	TransformComponent light;
	glm::vec3 direction = glm::normalize(glm::vec3(0.3f, -0.8f, 0.5f));
	light.SetLightDirection(direction);
	if (glm::length(light.GetLightDirection() - direction) > 1e-3f)
	{
		return fail("light direction");
	}

	// Destroying an entity removes its descendants
	scene.DestroyEntity(other);
	if (scene.Exists(other) || scene.Exists(grandchild) || scene.GetEntityCount() != 2)
	{
		return fail("destroy with descendants");
	}
	if (scene.FindFirst<NameComponent>() == NoEntity)
	{
		return fail("find first");
	}
	scene.Clear();
	if (scene.GetEntityCount() != 0 || !scene.GetRoots().empty())
	{
		return fail("clear");
	}

	Log::GetLogger()->info("EnvMapVulkanScene self-test passed");
	return true;
}
