#pragma once

#include "../Core/Timestep.h"
#include "../Events/MouseEvent.h"

#include <glm/glm.hpp>

namespace Hazel {

	class HazelCamera
	{
	public:
		HazelCamera() = default;
		HazelCamera(const glm::mat4& projectionMatrix);
		virtual ~HazelCamera() = default;

		void Focus();
		void OnUpdate(Timestep ts);
		void OnEvent(Event& e);

		inline float GetDistance() const { return m_Distance; }
		inline void SetDistance(float distance) { m_Distance = distance; }

		inline void SetProjectionMatrix(const glm::mat4& projectionMatrix) { m_ProjectionMatrix = projectionMatrix; }
		inline void SetViewportSize(uint32_t width, uint32_t height) { m_ViewportWidth = width; m_ViewportHeight = height; }

		const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		const glm::mat4& GetViewProjection() const { return m_ProjectionMatrix * m_ViewMatrix; }

		glm::vec3 GetUpDirection();
		glm::vec3 GetRightDirection();
		glm::vec3 GetForwardDirection();
		const glm::vec3& GetPosition() const { return m_Position; }

		inline float GetExposure() const { return m_Exposure; }
		inline float& GetExposure() { return m_Exposure; }

	private:
		void UpdateCameraView();

		bool OnMouseScroll(MouseScrolledEvent& e);

		void MousePan(const glm::vec2& delta);
		void MouseRotate(const glm::vec2& delta);
		void MouseZoom(float delta);

		glm::vec3 CalculatePosition();
		glm::quat GetOrientation();

		std::pair<float, float> PanSpeed() const;
		float RotationSpeed() const;
		float ZoomSpeed() const;

	protected:
		glm::mat4 m_ProjectionMatrix;
		glm::vec3 m_Position;
		float m_Pitch;
		float m_Yaw;
		float m_Exposure = 0.8f; // from Hazel, EnvironmentMap scene

	private:
		glm::mat4 m_ViewMatrix;
		glm::vec3 m_Rotation;
		glm::vec3 m_FocalPoint;

		bool m_Panning, m_Rotating;
		glm::vec2 m_InitialMousePosition;
		glm::vec3 m_InitialFocalPoint, m_InitialRotation;

		float m_Distance;

		uint32_t m_ViewportWidth = 1280, m_ViewportHeight = 720;

	};

}
