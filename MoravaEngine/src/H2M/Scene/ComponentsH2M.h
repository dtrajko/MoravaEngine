/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

/**
 * IDComponentH2M
 * TagComponentH2M
 * RelationshipComponentH2M
 * TransformComponentH2M
 * MeshComponentH2M
 * CameraComponentH2M
 * NativeScriptComponentH2M
 * MaterialComponentH2M
 * DirectionalLightComponentH2M
 * PointLightComponentH2M
 * SpotLightComponentH2M
 * SkyLightComponentH2M
 * SpriteRendererComponentH2M
 * CircleRendererComponentH2M
 * Rigidbody2DComponentH2M
 * BoxCollider2DComponentH2M
 * CircleCollider2DComponentH2M
 */

#pragma once

#include "H2M/Renderer/CameraH2M.h"
#include "H2M/Renderer/MeshH2M.h"
#include "H2M/Renderer/SceneEnvironmentH2M.h"
#include "H2M/Scene/SceneCameraH2M.h"

#include "EnvMap/EnvMapMaterial.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>


namespace H2M
{
	class MeshH2M;
	class SceneCameraH2M;

	struct IDComponentH2M
	{
		UUID_H2M ID = 0;
	};

	struct TagComponentH2M
	{
		std::string Tag;

		TagComponentH2M() = default;

		TagComponentH2M(const TagComponentH2M&) = default;
		TagComponentH2M(std::string tag)
			: Tag(tag) {};

		operator std::string& () { return Tag; };
		operator const std::string& () const { return Tag; };
	};

	struct RelationshipComponentH2M
	{
		UUID_H2M ParentHandle = 0;
		std::vector<UUID_H2M> Children;

		RelationshipComponentH2M() = default;
		RelationshipComponentH2M(const RelationshipComponentH2M& other) = default;
		RelationshipComponentH2M(UUID_H2M parent)
			: ParentHandle(parent) {}
	};

	struct TransformComponentH2M
	{
		glm::mat4 Transform;

		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

		glm::vec3 Up = { 0.0f, 1.0f, 0.0f };
		glm::vec3 Right = { 1.0f, 0.0f, 0.0f };
		glm::vec3 Forward = { 0.0f, 0.0f, -1.0f };

		TransformComponentH2M() = default;
		TransformComponentH2M(const TransformComponentH2M&) = default;
		TransformComponentH2M(const glm::vec3& translation)
			: Translation(translation), Transform(glm::mat4(1.0f)) {}
		TransformComponentH2M(const glm::mat4& transform)
			: Transform(transform) {}

		operator glm::mat4& () { CalculateTransform();  return Transform; }
		operator const glm::mat4& () { CalculateTransform(); return Transform; }
		glm::mat4 GetTransform() { CalculateTransform(); return Transform; }

		glm::mat4 CalculateTransform()
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));
			Transform = glm::translate(glm::mat4(1.0f), Translation) *
				rotation *
				glm::scale(glm::mat4(1.0f), Scale);
			return Transform;
		}
	};

	struct MeshComponentH2M
	{
		std::string FilePath = "";

		bool CastShadows = true;    // MeshRenderer property in Unity
		bool ReceiveShadows = true; // MeshRenderer property in Unity

		RefH2M<MeshH2M> Mesh;

		MeshComponentH2M() = default;
		MeshComponentH2M(const MeshComponentH2M& other) = default;
		MeshComponentH2M(const RefH2M<MeshH2M>& mesh)
			: Mesh(mesh) {}

		operator RefH2M<MeshH2M>() { return Mesh; }
	};

	struct CameraComponentH2M
	{
		CameraH2M Camera;
		bool Primary = true; // TODO: think about moving to Scene
		bool FixedAspectRatio = false;

		CameraComponentH2M() = default;
		CameraComponentH2M(const CameraComponentH2M& other) = default;

		CameraComponentH2M(CameraH2M camera)
			: Camera(camera) {};

		operator CameraH2M& () { return Camera; }
		operator const CameraH2M& () const { return Camera; }

		operator SceneCameraH2M& () { return (SceneCameraH2M&)Camera; }
		operator const SceneCameraH2M& () const { return (SceneCameraH2M&)Camera; }
	};

	// Forward declaration
	class ScriptableEntityH2M;

	struct NativeScriptComponentH2M
	{
		ScriptableEntityH2M* Instance = nullptr;

		ScriptableEntityH2M* (*InstantiateScript)();
		void (*DestroyScript)(NativeScriptComponentH2M*);

		template<typename T>
		void Bind()
		{
			InstantiateScript = []() { return static_cast<ScriptableEntityH2M*>(new T()); };
			DestroyScript = [](NativeScriptComponentH2M* nsc) { delete nsc->Instance; nsc->Instance = nullptr; };
		}
	};

	struct MaterialComponentH2M
	{
		RefH2M<EnvMapMaterial> Material = RefH2M<EnvMapMaterial>();

		MaterialComponentH2M() = default;
		MaterialComponentH2M(const MaterialComponentH2M& other) = default;
	};

	struct DirectionalLightComponentH2M
	{
		glm::vec3 Radiance = { 1.0f, 1.0f, 1.0f };
		float Intensity = 1.0f;
		bool CastShadows = true;
		bool SoftShadows = true;
		float LightSize = 0.5f; // For PCSS
	};

	struct PointLightComponentH2M
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

	struct SpotLightComponentH2M
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

	struct SkyLightComponentH2M
	{
		EnvironmentH2M SceneEnvironment;
		float Intensity = 1.0f;
		float Angle = 0.0f;
	};

	struct SpriteRendererComponentH2M
	{
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		RefH2M<Texture2D_H2M> Texture;
		float TilingFactor = 1.0f;

		SpriteRendererComponentH2M() = default;
		SpriteRendererComponentH2M(const SpriteRendererComponentH2M&) = default;
		SpriteRendererComponentH2M(const glm::vec4& color)
			: Color(color) {};
	};

	struct CircleRendererComponentH2M
	{
		// Attributes
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };
		float Thickness = 1.0f;
		float Fade = 0.0f;

		CircleRendererComponentH2M() = default;
		CircleRendererComponentH2M(const CircleRendererComponentH2M&) = default;
	};

	// Physics

	struct Rigidbody2DComponentH2M
	{
		enum class BodyType { Static = 0, Dynamic, Kinematic };
		BodyType Type = BodyType::Static;
		bool FixedRotation = false;

		// Storage for runtime
		void* RuntimeBody = nullptr;

		Rigidbody2DComponentH2M() = default;
		Rigidbody2DComponentH2M(const Rigidbody2DComponentH2M& other) = default;
	};

	struct BoxCollider2DComponentH2M
	{
		glm::vec2 Offset = { 0.0f, 0.0f  };
		glm::vec2 Size = { 0.5f, 0.5f };

		// TODO (Yan): move into physics material in the future maybe
		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;

		// Storage for runtime
		void* RuntimeFixture = nullptr;

		BoxCollider2DComponentH2M() = default;
		BoxCollider2DComponentH2M(const BoxCollider2DComponentH2M& other) = default;
	};

	struct CircleCollider2DComponentH2M
	{
		glm::vec2 Offset = { 0.0f, 0.0f };
		float Radius = 0.5f;

		// TODO (Yan): move into physics material in the future maybe
		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;

		// Storage for runtime
		void* RuntimeFixture = nullptr;

		CircleCollider2DComponentH2M() = default;
		CircleCollider2DComponentH2M(const CircleCollider2DComponentH2M& other) = default;
	};

}
