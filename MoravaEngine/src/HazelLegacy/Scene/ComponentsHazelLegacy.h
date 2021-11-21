#pragma once

#include "Hazel/Renderer/HazelCamera.h"
#include "Hazel/Renderer/SceneEnvironment.h"
#include "Hazel/Scene/SceneCamera.h"

#include "HazelLegacy/Renderer/MeshHazelLegacy.h"

#include "EnvMap/EnvMapMaterial.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>


namespace Hazel
{
	class MeshHazelLegacy;
	class SceneCamera;

	struct TransformComponentHazelLegacy
	{
		glm::mat4 Transform;

		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

		glm::vec3 Up = { 0.0f, 1.0f, 0.0f };
		glm::vec3 Right = { 1.0f, 0.0f, 0.0f };
		glm::vec3 Forward = { 0.0f, 0.0f, -1.0f };

		TransformComponentHazelLegacy() = default;
		TransformComponentHazelLegacy(const TransformComponentHazelLegacy&) = default;
		TransformComponentHazelLegacy(const glm::vec3& translation)
			: Translation(translation) {}
		TransformComponentHazelLegacy(const glm::mat4& transform)
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

	struct MeshComponentHazelLegacy
	{
		Ref<MeshHazelLegacy> Mesh;

		bool CastShadows = true;    // MeshRenderer property in Unity
		bool ReceiveShadows = true; // MeshRenderer property in Unity

		MeshComponentHazelLegacy() = default;
		MeshComponentHazelLegacy(const MeshComponentHazelLegacy& other) = default;
		MeshComponentHazelLegacy(const Ref<MeshHazelLegacy>& mesh)
			: Mesh(mesh) {}

		operator Ref<MeshHazelLegacy>() { return Mesh; }
	};

	struct MaterialComponentHazelLegacy
	{
		Hazel::Ref<EnvMapMaterial> Material = Hazel::Ref<EnvMapMaterial>();

		MaterialComponentHazelLegacy() = default;
		MaterialComponentHazelLegacy(const MaterialComponentHazelLegacy& other) = default;
	};

	struct PointLightComponentHazelLegacy
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

	struct SpotLightComponentHazelLegacy
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

	struct SkyLightComponentHazelLegacy
	{
		Environment SceneEnvironment;
		float Intensity = 1.0f;
		float Angle = 0.0f;
	};

	struct CameraComponentHazelLegacy
	{
		HazelCamera Camera;
		bool Primary = true; // TODO: think about moving to Scene
		bool FixedAspectRatio = false;

		CameraComponentHazelLegacy() = default;
		CameraComponentHazelLegacy(const CameraComponentHazelLegacy& other) = default;

		CameraComponentHazelLegacy(Hazel::HazelCamera camera)
			: Camera(camera) {};

		operator HazelCamera& () { return Camera; }
		operator const HazelCamera& () const { return Camera; }

		operator SceneCamera& () { return (SceneCamera&)Camera; }
		operator const SceneCamera& () const { return (SceneCamera&)Camera; }
	};

}
