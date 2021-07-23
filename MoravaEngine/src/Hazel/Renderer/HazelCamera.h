#pragma once

#include "Hazel/Events/Event.h"
#include "Hazel/Events/MouseEvent.h"
#include "Hazel/Core/Timestep.h"

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

		const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		void SetProjectionMatrix(const glm::mat4& projectionMatrix) { m_ProjectionMatrix = projectionMatrix; }

		virtual void OnUpdate(Timestep ts) {};
		virtual void OnEvent(Event& e) {};
		virtual inline void SetViewportSize(float width, float height) { m_ViewportWidth = width; m_ViewportHeight = height; };
		virtual inline bool OnMouseScroll(MouseScrolledEvent& e) { return false; };

		virtual inline const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		virtual inline glm::mat4& GetViewMatrix() { return m_ViewMatrix; }
		void SetViewMatrix(const glm::mat4& viewMatrix) { m_ViewMatrix = viewMatrix; }

		ProjectionType GetProjectionType() const { return m_ProjectionType; };
		void SetProjectionType(ProjectionType type) { m_ProjectionType = type; RecalculateProjection(); }

		float GetPerspectiveVerticalFOV() const { return glm::degrees(m_PerspectiveFOV); }
		void SetPerspectiveVerticalFOV(float verticalFOV) { m_PerspectiveFOV = glm::radians(verticalFOV); RecalculateProjection(); }
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

		// Getters
		inline const glm::vec3& GetPosition() const { return m_Position; }
		inline virtual glm::mat4 GetViewProjection() { return m_ProjectionMatrix * m_ViewMatrix; }
		inline float GetPitch() const { return m_Pitch; }
		inline float GetYaw() const { return m_Yaw; }
		inline glm::vec3 GetDirection() const { return glm::normalize(m_Front); }
		inline glm::vec3 GetFront() const { return m_Front; }
		inline glm::vec3 GetUp() const { return m_Up; }
		inline glm::vec3 GetRight() const { return m_Right; }
		inline glm::vec3 GetWorldUp() const { return m_WorldUp; }
		inline float GetExposure() const { return m_Exposure; }
		inline float& GetExposure() { return m_Exposure; }
		inline float& GetAspectRatio() { return m_AspectRatio; }

		// setters
		inline void SetPosition(glm::vec3 position) { m_Position = position; };
		inline virtual void SetPitch(float pitch) { m_Pitch = pitch; };
		inline void SetYaw(float yaw) { m_Yaw = yaw; };
		inline void SetWorldUp(glm::vec3 worldUp) { m_WorldUp = worldUp; }
		inline void SetFront(glm::vec3 front) { m_Front = front; };
		inline void SetRight(glm::vec3 right) { m_Right = right; }
		inline void SetUp(glm::vec3 up) { m_Up = up; }
		inline void SetExposure(float exposure) { m_Exposure = exposure; }
		inline void SetAspectRatio(float aspectRatio) { m_AspectRatio = aspectRatio; }

	private:
		virtual void RecalculateProjection();

	protected:
		ProjectionType m_ProjectionType = ProjectionType::Orthographic;

		glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);
		glm::mat4 m_ViewMatrix;

		float m_PerspectiveFOV = glm::radians(45.0f);
		float m_PerspectiveNear = 0.01f;
		float m_PerspectiveFar = 1000.0f;

		float m_OrthographicSize = 10.0f;
		float m_OrthographicNear = -1000.0f;
		float m_OrthographicFar = 1000.0f;

		float m_AspectRatio = 1.778f;

		float m_ViewportWidth = 1280.0f;
		float m_ViewportHeight = 720.0f;

		glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };

		float m_Pitch = 0.0f;
		float m_Yaw = 0.0f;
		float m_Exposure = 1.0f;

		glm::vec3 m_Front;
		glm::vec3 m_Up;
		glm::vec3 m_Right;
		glm::vec3 m_WorldUp;

	};

}
