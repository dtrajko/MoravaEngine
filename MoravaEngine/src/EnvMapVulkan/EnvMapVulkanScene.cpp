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

static std::string TrimTag(const std::string& tag)
{
	const size_t first = tag.find_first_not_of(" \t");
	return first == std::string::npos ? std::string() : tag.substr(first, tag.find_last_not_of(" \t") - first + 1);
}

bool TagComponent::Has(const std::string& tag) const
{
	return std::find(Tags.begin(), Tags.end(), tag) != Tags.end();
}

void TagComponent::Add(const std::string& tag)
{
	const std::string trimmed = TrimTag(tag);
	if (!trimmed.empty() && !Has(trimmed))
	{
		Tags.push_back(trimmed);
	}
}

void TagComponent::Remove(const std::string& tag)
{
	Tags.erase(std::remove(Tags.begin(), Tags.end(), tag), Tags.end());
}

std::string TagComponent::ToText() const
{
	std::string text;
	for (const std::string& tag : Tags)
	{
		text += (text.empty() ? "" : ", ") + tag;
	}
	return text;
}

void TagComponent::FromText(const std::string& text)
{
	Tags.clear();
	size_t start = 0;
	while (start <= text.size())
	{
		const size_t comma = std::min(text.find(',', start), text.size());
		Add(text.substr(start, comma - start));
		start = comma + 1;
	}
}

const ScriptFieldValue* ScriptComponent::FindField(const std::string& name) const
{
	for (const Field& field : Fields)
	{
		if (field.Name == name)
		{
			return &field.Value;
		}
	}
	return nullptr;
}

void ScriptComponent::SetField(const std::string& name, const ScriptFieldValue& value)
{
	for (Field& field : Fields)
	{
		if (field.Name == name)
		{
			field.Value = value;
			return;
		}
	}
	Fields.push_back({ name, value });
}

void ScriptComponent::ResetField(const std::string& name)
{
	Fields.erase(std::remove_if(Fields.begin(), Fields.end(), [&](const Field& field) { return field.Name == name; }), Fields.end());
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

// ColliderComponent, RigidBodyComponent

const char* ColliderComponent::GetShapeName(Shape shape)
{
	switch (shape)
	{
		case Shape::Box:          return "Box";
		case Shape::Sphere:       return "Sphere";
		case Shape::Capsule:      return "Capsule";
		case Shape::ConvexMesh:   return "ConvexMesh";
		case Shape::TriangleMesh: return "TriangleMesh";
	}
	return "Box";
}

const char* RigidBodyComponent::GetTypeName(BodyType type)
{
	switch (type)
	{
		case BodyType::Static:    return "Static";
		case BodyType::Dynamic:   return "Dynamic";
		case BodyType::Kinematic: return "Kinematic";
	}
	return "Dynamic";
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

std::vector<EnvMapVulkanEntityID> EnvMapVulkanScene::GetSubtree(EnvMapVulkanEntityID root) const
{
	std::vector<EnvMapVulkanEntityID> subtree;
	auto add = [&](auto& add, EnvMapVulkanEntityID id) -> void
	{
		subtree.push_back(id);
		for (EnvMapVulkanEntityID child : GetChildren(id))
		{
			add(add, child);
		}
	};
	if (Exists(root))
	{
		add(add, root);
	}
	return subtree;
}

EnvMapVulkanEntityID EnvMapVulkanScene::CopySubtree(const EnvMapVulkanScene& from, EnvMapVulkanEntityID root, EnvMapVulkanEntityID parent,
	const std::function<EnvMapVulkanEntityID(size_t)>& newId, std::unordered_map<EnvMapVulkanEntityID, EnvMapVulkanEntityID>* copies)
{
	// The entities first, parents before their children: a copy's parent is the copy of its source's parent
	const std::vector<EnvMapVulkanEntityID> sources = from.GetSubtree(root);
	std::unordered_map<EnvMapVulkanEntityID, EnvMapVulkanEntityID> copyOf;
	for (size_t i = 0; i < sources.size(); i++)
	{
		const EnvMapVulkanEntityID source = sources[i];
		const EnvMapVulkanEntityID copyParent = i == 0 ? parent : copyOf.at(from.GetParent(source));
		copyOf[source] = CreateEntity(from.Get<NameComponent>(source).Name, copyParent, newId ? newId(i) : NoEntity);
	}

	// Their components (the name and the place in the hierarchy are the copy's own already)
	SceneComponents::ForEach([&](auto type)
	{
		using T = typename decltype(type)::Type;
		if constexpr (!std::is_same_v<T, NameComponent> && !std::is_same_v<T, HierarchyComponent>)
		{
			for (EnvMapVulkanEntityID source : sources)
			{
				if (const T* component = from.TryGet<T>(source))
				{
					m_Registry.emplace_or_replace<T>(Handle(copyOf.at(source)), *component);
				}
			}
		}
	});

	// References inside the subtree go to the copies (a reference outside it stays: the same entity, when this is that scene)
	for (const auto& [source, copy] : copyOf)
	{
		if (ScriptComponent* script = TryGet<ScriptComponent>(copy))
		{
			for (ScriptComponent::Field& field : script->Fields)
			{
				if (field.Value.Type == ScriptFieldType::Entity)
				{
					if (auto it = copyOf.find(field.Value.Entity); it != copyOf.end())
					{
						field.Value.Entity = it->second;
					}
				}
			}
		}
	}
	if (copies)
	{
		*copies = copyOf;
	}
	return sources.empty() ? NoEntity : copyOf.at(root);
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

	// Tags: trimmed, each once, as the Inspector's text and back
	{
		TagComponent tags;
		tags.FromText(" Pickup, ,Coin,Pickup ");
		if (tags.Tags != std::vector<std::string>{ "Pickup", "Coin" } || tags.ToText() != "Pickup, Coin" || !tags.Has("Coin") || tags.Has("Pick"))
		{
			return fail("tags from text");
		}
		tags.Remove("Pickup");
		tags.Add("Enemy");
		if (tags.Tags != std::vector<std::string>{ "Coin", "Enemy" })
		{
			return fail("tags add / remove");
		}
		scene.Add<TagComponent>(child) = tags;
	}

	// A script's fields: set once each (a second set replaces the value), back to the class's value when reset, numbers
	// kept across a change of the field's type
	{
		ScriptComponent script;
		script.ClassName = "Game.Player";
		ScriptFieldValue speed;
		speed.Type = ScriptFieldType::Float;
		speed.Number = 7.6;
		ScriptFieldValue target;
		target.Type = ScriptFieldType::Entity;
		target.Entity = other;
		script.SetField("Speed", speed);
		script.SetField("Target", target);
		speed.Number = 8.4;
		script.SetField("Speed", speed);
		ScriptFieldValue asInt;
		if (script.Fields.size() != 2 || !script.FindField("Speed") || script.FindField("Speed")->Number != 8.4 ||
			!script.FindField("Speed")->ConvertTo(ScriptFieldType::Int, asInt) || asInt.Integer != 8 ||
			script.FindField("Speed")->ConvertTo(ScriptFieldType::Entity, asInt))
		{
			return fail("script fields");
		}
		script.ResetField("Speed");
		if (script.Fields.size() != 1 || script.FindField("Speed") || *script.FindField("Target") != target)
		{
			return fail("script field reset");
		}
		scene.Add<ScriptComponent>(child) = script;
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
			copy.Get<CameraComponent>(other).FieldOfView != 60.0f || copy.Has<CameraComponent>(root) ||
			!copy.Has<TagComponent>(child) || copy.Get<TagComponent>(child).Tags != scene.Get<TagComponent>(child).Tags ||
			!copy.Has<ScriptComponent>(child) || copy.Get<ScriptComponent>(child).ClassName != "Game.Player" ||
			copy.Get<ScriptComponent>(child).Fields.size() != 1)
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

	// A subtree copied (prefabs, 6.1): new IDs as given, the hierarchy's order, components copied, a script field that
	// points inside the subtree pointing at the copy, one that points outside left as it is
	{
		const EnvMapVulkanEntityID a = scene.CreateEntity("A");
		const EnvMapVulkanEntityID b = scene.CreateEntity("B", a);
		const EnvMapVulkanEntityID c = scene.CreateEntity("C", a);
		const EnvMapVulkanEntityID outside = scene.CreateEntity("Outside");
		scene.Get<TransformComponent>(b).Translation = glm::vec3(1.0f, 2.0f, 3.0f);
		ScriptComponent& script = scene.Add<ScriptComponent>(c);
		ScriptFieldValue inside, away;
		inside.Type = away.Type = ScriptFieldType::Entity;
		inside.Entity = b;
		away.Entity = outside;
		script.SetField("Inside", inside);
		script.SetField("Away", away);

		EnvMapVulkanScene copy;
		std::unordered_map<EnvMapVulkanEntityID, EnvMapVulkanEntityID> copies;
		const EnvMapVulkanEntityID root = copy.CopySubtree(scene, a, NoEntity, [](size_t i) { return EnvMapVulkanEntityID(i + 1); }, &copies);
		if (root != 1 || copy.GetEntityCount() != 3 || copy.GetChildren(1) != std::vector<EnvMapVulkanEntityID>{ 2, 3 } ||
			copies.at(c) != 3 || copy.Get<NameComponent>(2).Name != "B")
		{
			return fail("copy subtree: IDs and hierarchy");
		}
		if (copy.Get<TransformComponent>(2).Translation != glm::vec3(1.0f, 2.0f, 3.0f) || !copy.Has<ScriptComponent>(3))
		{
			return fail("copy subtree: components");
		}
		const ScriptComponent& copied = copy.Get<ScriptComponent>(3);
		if (copied.FindField("Inside")->Entity != 2 || copied.FindField("Away")->Entity != outside)
		{
			return fail("copy subtree: entity references");
		}
		// Into the same scene, under another entity, with random IDs
		const EnvMapVulkanEntityID again = scene.CopySubtree(scene, a, outside);
		if (again == a || scene.GetParent(again) != outside || scene.GetSubtree(again).size() != 3 || scene.GetEntityCount() != 7)
		{
			return fail("copy subtree into its own scene");
		}
		scene.Clear();
	}

	Log::GetLogger()->info("EnvMapVulkanScene self-test passed");
	return true;
}
