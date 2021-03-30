#pragma once

#include "../Script/ScriptEngine.h"
#include "../Core/KeyCodes.h"

#include <glm/glm.hpp>

extern "C" {
	typedef struct _MonoString MonoString;
	typedef struct _MonoArray MonoArray;
}

namespace Hazel { namespace Script {

	// Math
	float Hazel_Noise_PerlinNoise(float x, float y);

	// Input
	bool Hazel_Input_IsKeyPressed(KeyCode key);

	// Entity
	void Hazel_Entity_GetTransform(uint64_t entityID, glm::mat4* outTransform);
	void Hazel_Entity_SetTransform(uint64_t entityID, glm::mat4* inTransform);
	void Hazel_Entity_CreateComponent(uint64_t entityID, void* type);
	bool Hazel_Entity_HasComponent(uint64_t entityID, void* type);
	uint64_t Hazel_Entity_FindEntityByTag(MonoString* tag);

	void* Hazel_MeshComponent_GetMesh(uint64_t entityID);
	void Hazel_MeshComponent_SetMesh(uint64_t entityID, Ref<HazelMesh>* inMesh);

	void Hazel_RigidBody2DComponent_ApplyLinearImpulse(uint64_t entityID, glm::vec2* impulse, glm::vec2* offset, bool wake);
	void Hazel_RigidBody2DComponent_GetLinearVelocity(uint64_t entityID, glm::vec2* outVelocity);
	void Hazel_RigidBody2DComponent_SetLinearVelocity(uint64_t entityID, glm::vec2* velocity);

	// Renderer
	// Texture2D
	void* Hazel_Texture2D_Constructor(uint32_t width, uint32_t height);
	void Hazel_Texture2D_Destructor(Ref<HazelTexture2D>* _this);
	void Hazel_Texture2D_SetData(Ref<HazelTexture2D>* _this, MonoArray* inData, int32_t count);

	// Material
	void Hazel_Material_Destructor(Ref<HazelMaterial>* _this);
	void Hazel_Material_SetFloat(Ref<HazelMaterial>* _this, MonoString* uniform, float value);
	void Hazel_Material_SetTexture(Ref<HazelMaterial>* _this, MonoString* uniform, Ref<HazelTexture2D>* texture);

	void Hazel_MaterialInstance_Destructor(Ref<HazelMaterial>* _this);
	void Hazel_MaterialInstance_SetFloat(Ref<HazelMaterial>* _this, MonoString* uniform, float value);
	void Hazel_MaterialInstance_SetVector3(Ref<HazelMaterial>* _this, MonoString* uniform, glm::vec3* value);
	void Hazel_MaterialInstance_SetVector4(Ref<HazelMaterial>* _this, MonoString* uniform, glm::vec4* value);
	void Hazel_MaterialInstance_SetTexture(Ref<HazelMaterial>* _this, MonoString* uniform, Ref<HazelTexture2D>* texture);

	// Mesh
	Ref<HazelMesh>* Hazel_Mesh_Constructor(MonoString* filepath);
	void Hazel_Mesh_Destructor(Ref<HazelMesh>* _this);
	Ref<HazelMaterial>* Hazel_Mesh_GetMaterial(Ref<HazelMesh>* inMesh);
	Ref<HazelMaterial>* Hazel_Mesh_GetMaterialByIndex(Ref<HazelMesh>* inMesh, int index);
	int Hazel_Mesh_GetMaterialCount(Ref<HazelMesh>* inMesh);

	void* Hazel_MeshFactory_CreatePlane(float width, float height);

} }
