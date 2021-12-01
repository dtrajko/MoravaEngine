#pragma once

#include "H2M/Renderer/HazelCamera.h"
#include "H2M/Renderer/SceneEnvironment.h"
#include "H2M/Scene/SceneCamera.h"

#include "H2M/Renderer/MeshH2M.h"

#include "EnvMap/EnvMapMaterial.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>


namespace H2M
{
	class MeshH2M;
	class SceneCamera;

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
		Ref<MeshH2M> Mesh;

		bool CastShadows = true;    // MeshRenderer property in Unity
		bool ReceiveShadows = true; // MeshRenderer property in Unity

		MeshComponentH2M() = default;
		MeshComponentH2M(const MeshComponentH2M& other) = default;
		MeshComponentH2M(const Ref<MeshH2M>& mesh)
			: Mesh(mesh) {}

		operator Ref<MeshH2M>() { return Mesh; }
	};

	struct MaterialComponentH2M
	{
		H2M::RefH2M<EnvMapMaterial> Material = H2M::RefH2M<EnvMapMaterial>();

		MaterialComponentH2M() = default;
		MaterialComponentH2M(const MaterialComponentH2M& other) = default;
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
		Environment SceneEnvironment;
		float Intensity = 1.0f;
		float Angle = 0.0f;
	};

	struct CameraComponentH2M
	{
		HazelCamera Camera;
		bool Primary = true; // TODO: think about moving to Scene
		bool FixedAspectRatio = false;

		CameraComponentH2M() = default;
		CameraComponentH2M(const CameraComponentH2M& other) = default;

		CameraComponentH2M(H2M::CameraH2M camera)
			: Camera(camera) {};

		operator HazelCamera& () { return Camera; }
		operator const HazelCamera& () const { return Camera; }

		operator SceneCamera& () { return (SceneCamera&)Camera; }
		operator const SceneCamera& () const { return (SceneCamera&)Camera; }
	};

}
