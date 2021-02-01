#include "ScriptWrappers.h"

// #include "../Core/Math/Noise.h"

#include "../Scene/HazelScene.h"
#include "../Scene/Entity.h"
#include "../Scene/Components.h"

#include "../../Input.h"

#include <glm/gtc/type_ptr.hpp>

#include <mono/jit/jit.h>

#include <box2d/box2d.h>


namespace Hazel {
	extern std::unordered_map<MonoType*, std::function<bool(Entity&)>> s_HasComponentFuncs;
	extern std::unordered_map<MonoType*, std::function<void(Entity&)>> s_CreateComponentFuncs;
}

namespace Hazel { namespace Script {

	enum class ComponentID
	{
		None = 0,
		Transform = 1,
		Mesh = 2,
		Script = 3,
		SpriteRenderer = 4
	};



	////////////////////////////////////////////////////////////////
	// Math ////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////

	float Hazel_Noise_PerlinNoise(float x, float y)
	{
		// return Noise::PerlinNoise(x, y);

		return 0.0f;
	}

	////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////
	// Input ///////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////

	bool Hazel_Input_IsKeyPressed(KeyCode key)
	{
		return Input::IsKeyPressed(key);
	}

	////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////
	// Entity //////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////

	void Hazel_Entity_GetTransform(uint64_t entityID, glm::mat4* outTransform)
	{
		Ref<HazelScene> scene = ScriptEngine::GetCurrentSceneContext();
		HZ_CORE_ASSERT(scene, "No active scene!");
		const auto& entityMap = scene->GetEntityMap();
		HZ_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = Entity{ entityMap.at(entityID), scene.Raw() };
		auto& transformComponent = entity.GetComponent<TransformComponent>();
		memcpy(outTransform, glm::value_ptr(transformComponent.GetTransform()), sizeof(glm::mat4));
	}

	void Hazel_Entity_SetTransform(uint64_t entityID, glm::mat4* inTransform)
	{
		Ref<HazelScene> scene = ScriptEngine::GetCurrentSceneContext();
		HZ_CORE_ASSERT(scene, "No active scene!");
		const auto& entityMap = scene->GetEntityMap();
		HZ_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		auto& transformComponent = entity.GetComponent<TransformComponent>();
		memcpy((void*)glm::value_ptr(transformComponent.GetTransform()), inTransform, sizeof(glm::mat4));
	}

	void Hazel_Entity_CreateComponent(uint64_t entityID, void* type)
	{
		Ref<HazelScene> scene = ScriptEngine::GetCurrentSceneContext();
		HZ_CORE_ASSERT(scene, "No active scene!");
		const auto& entityMap = scene->GetEntityMap();
		HZ_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		MonoType* monoType = mono_reflection_type_get_type((MonoReflectionType*)type);
		s_CreateComponentFuncs[monoType](entity);
	}

	bool Hazel_Entity_HasComponent(uint64_t entityID, void* type)
	{
		Ref<HazelScene> scene = ScriptEngine::GetCurrentSceneContext();
		HZ_CORE_ASSERT(scene, "No active scene!");
		const auto& entityMap = scene->GetEntityMap();
		HZ_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		MonoType* monoType = mono_reflection_type_get_type((MonoReflectionType*)type);
		bool result = s_HasComponentFuncs[monoType](entity);
		return result;
	}

	uint64_t Hazel_Entity_FindEntityByTag(MonoString* tag)
	{
		Ref<HazelScene> scene = ScriptEngine::GetCurrentSceneContext();
		HZ_CORE_ASSERT(scene, "No active scene!");

		Entity entity = scene->FindEntityByTag(mono_string_to_utf8(tag));
		if (entity) {
			return entity.GetComponent<IDComponent>().ID;
		}

		return 0;
	}

	void* Hazel_MeshComponent_GetMesh(uint64_t entityID)
	{
		Ref<HazelScene> scene = ScriptEngine::GetCurrentSceneContext();
		HZ_CORE_ASSERT(scene, "No active scene!");
		const auto& entityMap = scene->GetEntityMap();
		HZ_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		auto& meshComponent = entity.GetComponent<MeshComponent>();
		return new Ref<Mesh>(meshComponent.Mesh);

		return (void*)nullptr;
	}

	void Hazel_MeshComponent_SetMesh(uint64_t entityID, Ref<HazelMesh>* inMesh)
	{
		Ref<HazelScene> scene = ScriptEngine::GetCurrentSceneContext();
		HZ_CORE_ASSERT(scene, "No active scene!");
		const auto& entityMap = scene->GetEntityMap();
		HZ_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = Entity{ entityMap.at(entityID), scene.Raw() };
		auto& meshComponent = entity.GetComponent<MeshComponent>();
		meshComponent.Mesh = inMesh ? *inMesh : nullptr;
	}

	void Hazel_RigidBody2DComponent_ApplyLinearImpulse(uint64_t entityID, glm::vec2* impulse, glm::vec2* offset, bool wake)
	{
		Ref<HazelScene> scene = ScriptEngine::GetCurrentSceneContext();
		HZ_CORE_ASSERT(scene, "No active scene!");
		const auto& entityMap = scene->GetEntityMap();
		HZ_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		HZ_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
		auto& component = entity.GetComponent<RigidBody2DComponent>();
		b2Body* body = (b2Body*)component.RuntimeBody;
		body->ApplyLinearImpulse(*(const b2Vec2*)impulse, body->GetWorldCenter() + *(const b2Vec2*)offset, wake);
	}

	void Hazel_RigidBody2DComponent_GetLinearVelocity(uint64_t entityID, glm::vec2* outVelocity)
	{
		Ref<HazelScene> scene = ScriptEngine::GetCurrentSceneContext();
		HZ_CORE_ASSERT(scene, "No active scene!");
		const auto& entityMap = scene->GetEntityMap();
		HZ_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		HZ_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
		auto& component = entity.GetComponent<RigidBody2DComponent>();
		b2Body* body = (b2Body*)component.RuntimeBody;
		const auto& velocity = body->GetLinearVelocity();
		HZ_CORE_ASSERT(outVelocity);
		*outVelocity = { velocity.x, velocity.y };
	}

	void Hazel_RigidBody2DComponent_SetLinearVelocity(uint64_t entityID, glm::vec2* velocity)
	{
		Ref<HazelScene> scene = ScriptEngine::GetCurrentSceneContext();
		HZ_CORE_ASSERT(scene, "No active scene!");
		const auto& entityMap = scene->GetEntityMap();
		HZ_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");

		Entity entity = entityMap.at(entityID);
		HZ_CORE_ASSERT(entity.HasComponent<RigidBody2DComponent>());
		auto& component = entity.GetComponent<RigidBody2DComponent>();
		b2Body* body = (b2Body*)component.RuntimeBody;
		HZ_CORE_ASSERT(velocity);
		body->SetLinearVelocity({velocity->x, velocity->y});
	}

	Ref<HazelMesh>* Hazel_Mesh_Constructor(MonoString* filepath)
	{
		// return new Ref<HazelMesh>(new HazelMesh(mono_string_to_utf8(filepath)));

		return &Ref<HazelMesh>::Create("", CreateRef<Shader>(), new Material(), false);
	}

	void Hazel_Mesh_Destructor(Ref<HazelMesh>* _this)
	{
		Ref<HazelMesh>* instance = (Ref<HazelMesh>*)_this;
		delete _this;
	}

	Ref<HazelMaterial>* Hazel_Mesh_GetMaterial(Ref<HazelMesh>* inMesh)
	{
		Ref<HazelMesh>& mesh = *(Ref<HazelMesh>*)inMesh;
		// return new Ref<HazelMaterial>(mesh->GetMaterial());

		return &Ref<HazelMaterial>();
	}

	Ref<HazelMaterialInstance>* Hazel_Mesh_GetMaterialByIndex(Ref<HazelMesh>* inMesh, int index)
	{
		Ref<HazelMesh>& mesh = *(Ref<HazelMesh>*)inMesh;
		const auto& materials = mesh->GetMaterials();
		
		HZ_CORE_ASSERT(index < materials.size());
		// return new Ref<HazelMaterialInstance>(materials[index]);

		return new Ref<HazelMaterialInstance>();
	}

	int Hazel_Mesh_GetMaterialCount(Ref<HazelMesh>* inMesh)
	{
		Ref<HazelMesh>& mesh = *(Ref<HazelMesh>*)inMesh;
		const auto& materials = mesh->GetMaterials();
		return (int)materials.size();
	}

	void* Hazel_Texture2D_Constructor(uint32_t width, uint32_t height)
	{
		auto result = HazelTexture2D::Create(HazelTextureFormat::RGBA, width, height);
		return new Ref<HazelTexture2D>(result);
	}

	void Hazel_Texture2D_Destructor(Ref<HazelTexture2D>* _this)
	{
		delete _this;
	}

	void Hazel_Texture2D_SetData(Ref<HazelTexture2D>* _this, MonoArray* inData, int32_t count)
	{
		Ref<HazelTexture2D>& instance = *_this;
		
		uint32_t dataSize = count * sizeof(glm::vec4) / 4;

		instance->Lock();
		Buffer buffer = instance->GetWriteableBuffer();
		HZ_CORE_ASSERT(dataSize <= buffer.Size);
		// Convert RGBA32F color to RGBA8
		uint8_t* pixels = (uint8_t*)buffer.Data;
		uint32_t index = 0;
		for (uint32_t i = 0; i < instance->GetWidth() * instance->GetHeight(); i++)
		{
			glm::vec4& value = mono_array_get(inData, glm::vec4, i);
			*pixels++ = (uint32_t)(value.x * 255.0f);
			*pixels++ = (uint32_t)(value.y * 255.0f);
			*pixels++ = (uint32_t)(value.z * 255.0f);
			*pixels++ = (uint32_t)(value.w * 255.0f);
		}

		instance->Unlock();
	}

	void Hazel_Material_Destructor(Ref<HazelMaterial>* _this)
	{
		delete _this;
	}

	void Hazel_Material_SetFloat(Ref<HazelMaterial>* _this, MonoString* uniform, float value)
	{
		Ref<HazelMaterial>& instance = *(Ref<HazelMaterial>*)_this;
		instance->Set(mono_string_to_utf8(uniform), value);
	}

	void Hazel_Material_SetTexture(Ref<HazelMaterial>* _this, MonoString* uniform, Ref<HazelTexture2D>* texture)
	{
		Ref<HazelMaterial>& instance = *(Ref<HazelMaterial>*)_this;
		instance->Set(mono_string_to_utf8(uniform), *texture);
	}

	void Hazel_MaterialInstance_Destructor(Ref<HazelMaterialInstance>* _this)
	{
		delete _this;
	}

	void Hazel_MaterialInstance_SetFloat(Ref<HazelMaterialInstance>* _this, MonoString* uniform, float value)
	{
		Ref<HazelMaterialInstance>& instance = *(Ref<HazelMaterialInstance>*)_this;
		instance->Set(mono_string_to_utf8(uniform), value);
	}

	void Hazel_MaterialInstance_SetVector3(Ref<HazelMaterialInstance>* _this, MonoString* uniform, glm::vec3* value)
	{
		Ref<HazelMaterialInstance>& instance = *(Ref<HazelMaterialInstance>*)_this;
		instance->Set(mono_string_to_utf8(uniform), *value);
	}

	void Hazel_MaterialInstance_SetVector4(Ref<HazelMaterialInstance>* _this, MonoString* uniform, glm::vec4* value)
	{
		Ref<HazelMaterialInstance>& instance = *(Ref<HazelMaterialInstance>*)_this;
		instance->Set(mono_string_to_utf8(uniform), *value);
	}

	void Hazel_MaterialInstance_SetTexture(Ref<HazelMaterialInstance>* _this, MonoString* uniform, Ref<HazelTexture2D>* texture)
	{
		Ref<HazelMaterialInstance>& instance = *(Ref<HazelMaterialInstance>*)_this;
		instance->Set(mono_string_to_utf8(uniform), *texture);
	}

	void* Hazel_MeshFactory_CreatePlane(float width, float height)
	{
		// TODO: Implement properly with MeshFactory class!
		// return new Ref<HazelMesh>(new HazelMesh("assets/models/Plane1m.obj"));

		return (void*)nullptr;
	}

	////////////////////////////////////////////////////////////////

} }
