#pragma once

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "../Core/UUID.h"
#include "../Renderer/HazelMesh.h"
#include "../Renderer/HazelCamera.h"


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

	struct SpriteRendererComponent
	{
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };

		SpriteRendererComponent() = default;
		SpriteRendererComponent(const SpriteRendererComponent&) = default;
		SpriteRendererComponent(const glm::vec4& color)
			: Color(color) {};
	};

	struct CameraComponent
	{
		HazelCamera Camera;
		bool Primary = true; // TODO: think about moving to Scene
		bool FixedAspectRatio = false;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
		CameraComponent(Hazel::HazelCamera* camera)
			: Camera(*camera) {};

		operator HazelCamera& () { return Camera; }
		operator const HazelCamera& () const { return Camera; }
	};

	struct NativeScriptComponent
	{
		ScriptableEntity* Instance = nullptr;

		ScriptableEntity*(*InstantiateScript)();
		void (*DestroyScript)(NativeScriptComponent*);

		template<typename T>
		void Bind()
		{
			InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
			DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->Instance; nsc->Instance = nullptr; };
		}
	};

	struct ScriptComponent
	{
		// TODO: C# script
		std::string ModuleName;
	};

}
