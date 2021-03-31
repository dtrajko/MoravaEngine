#pragma once

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "../Core/UUID.h"
#include "../Renderer/HazelMesh.h"
#include "../Renderer/HazelCamera.h"
#include "../Renderer/SceneEnvironment.h"


namespace Hazel
{
	class ScriptableEntity;

	struct IDComponent
	{
		UUID ID = 0;
	};

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;

		TagComponent(const TagComponent&) = default;
		TagComponent(std::string tag)
			: Tag(tag) {};

		operator std::string& () { return Tag; };
		operator const std::string& () const { return Tag; };
	};

	struct TransformComponent
	{
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3 & translation)
			: Translation(translation) {}
		TransformComponent(const glm::mat4& transform) {};

		glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));

			return glm::translate(glm::mat4(1.0f), Translation) *
				rotation *
				glm::scale(glm::mat4(1.0f), Scale);
		}
	};

	struct MeshComponent
	{
		Ref<HazelMesh> Mesh;

		MeshComponent() = default;
		MeshComponent(const MeshComponent& other) = default;
		MeshComponent(const Ref<HazelMesh>& mesh)
			: Mesh(mesh) {}

		operator Ref<HazelMesh>() { return Mesh; }
	};

	struct ScriptComponent
	{
		// C# script
		std::string ModuleName;

		ScriptComponent() = default;
		ScriptComponent(const ScriptComponent & other) = default;
		ScriptComponent(const std::string & moduleName)
			: ModuleName(moduleName) {}
	};

	struct CameraComponent
	{
		HazelCamera Camera;
		bool Primary = true; // TODO: think about moving to Scene
		bool FixedAspectRatio = false;

		CameraComponent() = default;
		CameraComponent(const CameraComponent& other) = default;

		CameraComponent(Hazel::HazelCamera* camera)
			: Camera(*camera) {};

		operator HazelCamera& () { return Camera; }
		operator const HazelCamera& () const { return Camera; }
	};

	struct SpriteRendererComponent
	{
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		Ref<HazelTexture2D> Texture;
		float TilingFactor = 1.0f;

		SpriteRendererComponent() = default;
		SpriteRendererComponent(const SpriteRendererComponent&) = default;
		SpriteRendererComponent(const glm::vec4& color)
			: Color(color) {};
	};

	struct RigidBody2DComponent
	{
		enum class Type { Static, Dynamic, Kinematic };
		Type BodyType;
		bool FixedRotation = false;

		// Storage for runtime
		void* RuntimeBody = nullptr;

		RigidBody2DComponent() = default;
		RigidBody2DComponent(const RigidBody2DComponent& other) = default;
	};

	struct BoxCollider2DComponent
	{
		glm::vec2 Offset = { 0.0f, 0.0f };
		glm::vec2 Size = { 1.0f, 1.0f };

		float Density = 1.0f;
		float Friction = 1.0f;

		// Storage for runtime
		void* RuntimeFixture = nullptr;

		BoxCollider2DComponent() = default;
		BoxCollider2DComponent(const BoxCollider2DComponent& other) = default;
	};

	struct CircleCollider2DComponent
	{
		glm::vec2 Offset = { 0.0f, 0.0f };
		float Radius = 1.0f;

		float Density = 1.0f;
		float Friction = 1.0f;

		// Storage for runtime
		void* RuntimeFixture = nullptr;

		CircleCollider2DComponent() = default;
		CircleCollider2DComponent(const CircleCollider2DComponent& other) = default;
	};

	struct MaterialComponent
	{
		Hazel::Ref<EnvMapMaterial> Material = nullptr;

		MaterialComponent() = default;
		MaterialComponent(const MaterialComponent& other) = default;
	};

	// Obsolete?
	struct NativeScriptComponent
	{
		ScriptableEntity* Instance = nullptr;

		ScriptableEntity* (*InstantiateScript)();
		void (*DestroyScript)(NativeScriptComponent*);

		template<typename T>
		void Bind()
		{
			InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
			DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->Instance; nsc->Instance = nullptr; };
		}
	};

	// Lights

	// TODO: Move to renderer
	enum class LightType
	{
		None = 0, Directional = 1, Point = 2, Spot = 3
	};

	struct DirectionalLightComponent
	{
		glm::vec3 Radiance = { 1.0f, 1.0f, 1.0f };
		float Intensity = 1.0f;
		bool CastShadows = true;
		bool SoftShadows = true;
		float LightSize = 0.5f; // For PCSS
	};

	struct SkyLightComponent
	{
		Environment SceneEnvironment;
		float Intensity = 1.0f;
		float Angle = 0.0f;
	};

	struct PointLightComponent
	{
		bool Enabled = true;
		glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
		// glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		float AmbientIntensity = 2.0f;
		float DiffuseIntensity = 1.0f;
		float Constant = 4.0f;
		float Linear = 2.0f;
		float Exponent = 1.0f;
		float FarPlane = 1000.0f;
	};

	struct SpotLightComponent
	{
		bool Enabled = true;
		glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
		// glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		// glm::vec3 Direction = { 0.0f, -1.0f, 0.0f };
		float AmbientIntensity = 2.0f;
		float DiffuseIntensity = 1.0f;
		float Constant = 4.0f;
		float Linear = 2.0f;
		float Exponent = 1.0f;
		float Edge = 0.0f;
		float EdgeProcessed = 0.0f;
		float FarPlane = 1000.0f;
	};

}
