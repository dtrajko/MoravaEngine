/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Renderer/CameraH2M.h"

#include "H2M/Core/TimestepH2M.h"
#include "H2M/Core/Events/EventH2M.h"
#include "H2M/Core/Events/MouseEventH2M.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>


namespace H2M
{

	enum class CameraMode
	{
		NONE, FLYCAM, ARCBALL
	};

	class EditorCameraH2M : public CameraH2M
	{
	public:
		EditorCameraH2M() = default;
		EditorCameraH2M(const glm::mat4& projectionMatrix);
		EditorCameraH2M(float fov, float aspectRatio, float nearClip, float farClip);

		void Focus(const glm::vec3& focusPoint);
		virtual void OnUpdate(TimestepH2M ts) override;
		virtual void OnEvent(EventH2M& e) override;

		bool IsActive() const { return m_IsActive; }
		void SetActive(bool active) { m_IsActive = active; }

		inline float GetDistance() const { return m_Distance; }
		inline void SetDistance(float distance) { m_Distance = distance; }

		virtual void SetViewportSize(float width, float height) override;

		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		glm::mat4 GetViewProjection() const { return m_ProjectionMatrix * m_ViewMatrix; }

		glm::vec3 GetUpDirection() const;
		glm::vec3 GetRightDirection() const;
		glm::vec3 GetForwardDirection() const;
		const glm::vec3& GetPosition() const { return m_Position; }
		glm::quat GetOrientation() const;

		float GetPitch() const { return m_Pitch; }
		float GetYaw() const { return m_Yaw; }

	private:
		void UpdateCameraView();

		virtual bool OnMouseScroll(MouseScrolledEventH2M& e) override;

		void MousePan(const glm::vec2& delta);
		void MouseRotate(const glm::vec2& delta);
		void MouseZoom(float delta);

		glm::vec3 CalculatePosition() const;

		std::pair<float, float> PanSpeed() const;
		float RotationSpeed() const;
		float ZoomSpeed() const;

		void UpdateProjection();
		void UpdateView();

	private:
		glm::mat4 m_ViewMatrix;
		glm::vec3 m_Position, m_WorldRotation, m_FocalPoint;

		bool m_IsActive = false;
		bool m_Panning, m_Rotating;
		glm::vec2 m_InitialMousePosition = { 0.0f, 0.0f };
		glm::vec3 m_InitialFocalPoint, m_InitialRotation;

		float m_Distance = 10.0f;
		float m_Speed { 0.002f };
		float m_LastSpeed = 0.0f;

		float m_Pitch, m_Yaw;
		float m_PitchDelta{}, m_YawDelta{};
		glm::vec3 m_PositionDelta{};
		glm::vec3 m_RightDirection{};

		CameraMode m_CameraMode{ CameraMode::ARCBALL };

		float m_MinFocusDistance = 100.0f;

		uint32_t m_ViewportWidth = 1280, m_ViewportHeight = 720;

		// glm::vec3 m_Rotation = { 0.0f, 0.0f, 0.0f };

	};

}
