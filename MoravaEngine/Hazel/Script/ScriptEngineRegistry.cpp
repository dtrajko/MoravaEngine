#include "ScriptEngineRegistry.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../Scene/Entity.h"
#include "ScriptWrappers.h"
#include <iostream>

namespace Hazel {

	std::unordered_map<MonoType*, std::function<bool(Entity&)>> s_HasComponentFuncs;
	std::unordered_map<MonoType*, std::function<void(Entity&)>> s_CreateComponentFuncs;

	extern MonoImage* s_CoreAssemblyImage;

#define Component_RegisterType(Type) \
	{\
		MonoType* type = mono_reflection_type_from_name("Hazel." #Type, s_CoreAssemblyImage);\
		if (type) {\
			uint32_t id = mono_type_get_type(type);\
			s_HasComponentFuncs[type] = [](Entity& entity) { return entity.HasComponent<Type>(); };\
			s_CreateComponentFuncs[type] = [](Entity& entity) { entity.AddComponent<Type>(); };\
		} else {\
			HZ_CORE_ERROR("No C# component class found for " #Type "!");\
		}\
	}

	static void InitComponentTypes()
	{
		// Component_RegisterType(TagComponent);
		// Component_RegisterType(TransformComponent);
		// Component_RegisterType(MeshComponent);
		// Component_RegisterType(ScriptComponent);
		// Component_RegisterType(CameraComponent);
		// Component_RegisterType(SpriteRendererComponent);
		// Component_RegisterType(RigidBody2DComponent);
		// Component_RegisterType(BoxCollider2DComponent);
	}

	void ScriptEngineRegistry::RegisterAll()
	{
		InitComponentTypes();

		mono_add_internal_call("Hazel.Noise::PerlinNoise_Native", Hazel::Script::Hazel_Noise_PerlinNoise);

		mono_add_internal_call("Hazel.Entity::GetTransform_Native", Hazel::Script::Hazel_Entity_GetTransform);
		mono_add_internal_call("Hazel.Entity::SetTransform_Native", Hazel::Script::Hazel_Entity_SetTransform);
		mono_add_internal_call("Hazel.Entity::CreateComponent_Native", Hazel::Script::Hazel_Entity_CreateComponent);
		mono_add_internal_call("Hazel.Entity::HasComponent_Native", Hazel::Script::Hazel_Entity_HasComponent);
		mono_add_internal_call("Hazel.Entity::FindEntityByTag_Native", Hazel::Script::Hazel_Entity_FindEntityByTag);

		mono_add_internal_call("Hazel.MeshComponent::GetMesh_Native", Hazel::Script::Hazel_MeshComponent_GetMesh);
		mono_add_internal_call("Hazel.MeshComponent::SetMesh_Native", Hazel::Script::Hazel_MeshComponent_SetMesh);

		mono_add_internal_call("Hazel.RigidBody2DComponent::ApplyLinearImpulse_Native", Hazel::Script::Hazel_RigidBody2DComponent_ApplyLinearImpulse);
		mono_add_internal_call("Hazel.RigidBody2DComponent::GetLinearVelocity_Native", Hazel::Script::Hazel_RigidBody2DComponent_GetLinearVelocity);
		mono_add_internal_call("Hazel.RigidBody2DComponent::SetLinearVelocity_Native", Hazel::Script::Hazel_RigidBody2DComponent_SetLinearVelocity);

		mono_add_internal_call("Hazel.Input::IsKeyPressed_Native", Hazel::Script::Hazel_Input_IsKeyPressed);

		mono_add_internal_call("Hazel.Texture2D::Constructor_Native", Hazel::Script::Hazel_Texture2D_Constructor);
		mono_add_internal_call("Hazel.Texture2D::Destructor_Native", Hazel::Script::Hazel_Texture2D_Destructor);
		mono_add_internal_call("Hazel.Texture2D::SetData_Native", Hazel::Script::Hazel_Texture2D_SetData);

		mono_add_internal_call("Hazel.Material::Destructor_Native", Hazel::Script::Hazel_Material_Destructor);
		mono_add_internal_call("Hazel.Material::SetFloat_Native", Hazel::Script::Hazel_Material_SetFloat);
		mono_add_internal_call("Hazel.Material::SetTexture_Native", Hazel::Script::Hazel_Material_SetTexture);

		mono_add_internal_call("Hazel.MaterialInstance::Destructor_Native", Hazel::Script::Hazel_MaterialInstance_Destructor);
		mono_add_internal_call("Hazel.MaterialInstance::SetFloat_Native", Hazel::Script::Hazel_MaterialInstance_SetFloat);
		mono_add_internal_call("Hazel.MaterialInstance::SetVector3_Native", Hazel::Script::Hazel_MaterialInstance_SetVector3);
		mono_add_internal_call("Hazel.MaterialInstance::SetVector4_Native", Hazel::Script::Hazel_MaterialInstance_SetVector4);
		mono_add_internal_call("Hazel.MaterialInstance::SetTexture_Native", Hazel::Script::Hazel_MaterialInstance_SetTexture);

		mono_add_internal_call("Hazel.Mesh::Constructor_Native", Hazel::Script::Hazel_Mesh_Constructor);
		mono_add_internal_call("Hazel.Mesh::Destructor_Native", Hazel::Script::Hazel_Mesh_Destructor);
		mono_add_internal_call("Hazel.Mesh::GetMaterial_Native", Hazel::Script::Hazel_Mesh_GetMaterial);
		mono_add_internal_call("Hazel.Mesh::GetMaterialByIndex_Native", Hazel::Script::Hazel_Mesh_GetMaterialByIndex);
		mono_add_internal_call("Hazel.Mesh::GetMaterialCount_Native", Hazel::Script::Hazel_Mesh_GetMaterialCount);

		mono_add_internal_call("Hazel.MeshFactory::CreatePlane_Native", Hazel::Script::Hazel_MeshFactory_CreatePlane);

		// static bool IsKeyPressed(KeyCode key) { return s_Instance->IsKeyPressedImpl(key); }
		// 
		// static bool IsMouseButtonPressed(MouseCode button) { return s_Instance->IsMouseButtonPressedImpl(button); }
		// static std::pair<float, float> GetMousePosition() { return s_Instance->GetMousePositionImpl(); }
		// static float GetMouseX() { return s_Instance->GetMouseXImpl(); }
		// static float GetMouseY() { return s_Instance->GetMouseYImpl(); }
	}

}
