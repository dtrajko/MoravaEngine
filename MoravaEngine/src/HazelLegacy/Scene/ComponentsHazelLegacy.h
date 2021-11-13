#pragma once

#include "Hazel/Renderer/HazelCamera.h"
#include "Hazel/Renderer/SceneEnvironment.h"

#include "EnvMap/EnvMapMaterial.h"


namespace Hazel
{
	class MeshHazelLegacy;
	class SceneCamera;

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

	struct MaterialComponent
	{
		Hazel::Ref<EnvMapMaterial> Material = Hazel::Ref<EnvMapMaterial>();

		MaterialComponent() = default;
		MaterialComponent(const MaterialComponent& other) = default;
	};

	struct PointLightLegacyComponent
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

	struct SpotLightLegacyComponent
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

	struct CameraComponentLegacy
	{
		HazelCamera Camera;
		bool Primary = true; // TODO: think about moving to Scene
		bool FixedAspectRatio = false;

		CameraComponentLegacy() = default;
		CameraComponentLegacy(const CameraComponentLegacy& other) = default;

		CameraComponentLegacy(Hazel::HazelCamera camera)
			: Camera(camera) {};

		operator HazelCamera& () { return Camera; }
		operator const HazelCamera& () const { return Camera; }

		operator SceneCamera& () { return (SceneCamera&)Camera; }
		operator const SceneCamera& () const { return (SceneCamera&)Camera; }
	};

	struct SkyLightLegacyComponent
	{
		Ref<Environment> SceneEnvironment;
		float Intensity = 1.0f;
		float Angle = 0.0f;
	};
}
