#pragma once

#include <glm/glm.hpp>

namespace Hazel {

	class HazelCamera
	{
	public:
		enum class ProjectionType { Perspective = 0, Orthographic = 1 };

	public:
		HazelCamera() = default;
		HazelCamera(const glm::mat4& projection)
			: m_ProjectionMatrix(projection) {}

		virtual ~HazelCamera() = default;

		const glm::mat4& GetProjection() const { return m_ProjectionMatrix; }

		virtual inline void SetViewportSize(float width, float height) { m_ViewportWidth = width; m_ViewportHeight = height; };

		ProjectionType GetProjectionType() const { return m_ProjectionType; };
		void SetProjectionType(ProjectionType type) { m_ProjectionType = type; RecalculateProjection(); }

		float GetPerspectiveVerticalFOV() const { return m_PerspectiveFOV; }
		void SetPerspectiveVerticalFOV(float verticalFOV) { m_PerspectiveFOV = verticalFOV; RecalculateProjection(); }
		float GetPerspectiveNearClip() const { return m_PerspectiveNear; };
		void SetPerspectiveNearClip(float nearClip) { m_PerspectiveNear = nearClip; RecalculateProjection(); }
		float GetPerspectiveFarClip() const { return m_PerspectiveFar; };
		void SetPerspectiveFarClip(float farClip) { m_PerspectiveFar = farClip; RecalculateProjection(); }

		float GetOrthographicSize() const { return m_OrthographicSize; }
		void SetOrthographicSize(float size) { m_OrthographicSize = size; RecalculateProjection(); }
		float GetOrthographicNearClip() const { return m_OrthographicNear; };
		void SetOrthographicNearClip(float nearClip) { m_OrthographicNear = nearClip; RecalculateProjection(); }
		float GetOrthographicFarClip() const { return m_OrthographicFar; };
		void SetOrthographicFarClip(float farClip) { m_OrthographicFar = farClip; RecalculateProjection(); }

		inline float& GetExposure() { return m_Exposure; }
		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		const glm::vec3& GetPosition() const { return m_Position; }

		virtual glm::mat4 GetViewProjection() { return m_ProjectionMatrix * m_ViewMatrix; }

	private:
		virtual void RecalculateProjection();

	protected:
		ProjectionType m_ProjectionType = ProjectionType::Orthographic;

		glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);

		float m_PerspectiveFOV = glm::radians(45.0f);
		float m_PerspectiveNear = 0.01f;
		float m_PerspectiveFar = 1000.0f;

		float m_OrthographicSize = 10.0f;
		float m_OrthographicNear = -1000.0f;
		float m_OrthographicFar = 1000.0f;

		float m_AspectRatio = 1.0f;

		float m_ViewportWidth = 1280.0f;
		float m_ViewportHeight = 720.0f;

		float m_Exposure = 1.0f;
		glm::mat4 m_ViewMatrix;
		glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };

	};

}
