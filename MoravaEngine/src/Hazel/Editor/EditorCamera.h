#pragma once

#include "Hazel/Renderer/HazelCamera.h"

#include "Hazel/Core/Timestep.h"
#include "Hazel/Events/Event.h"
#include "Hazel/Events/MouseEvent.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>


namespace Hazel {

	class EditorCamera : public HazelCamera
	{
	public:
		EditorCamera() = default;
		EditorCamera(const glm::mat4& projectionMatrix);
		EditorCamera(float fov, float aspectRatio, float nearClip, float farClip);

		void Focus(const glm::vec3& focusPoint);
		virtual void OnUpdate(Timestep ts) override;
		virtual void OnEvent(Event& e) override;

		inline float GetDistance() const { return m_Distance; }
		inline void SetDistance(float distance) { m_Distance = distance; }

		virtual void SetViewportSize(float width, float height) override;

		glm::vec3 GetUpDirection() const;
		glm::vec3 GetRightDirection() const;
		glm::vec3 GetForwardDirection() const;
		const glm::vec3& GetPosition() const { return m_Position; }
		glm::quat GetOrientation() const;

		float GetPitch() const { return m_Pitch; }
		float GetYaw() const { return m_Yaw; }

	private:
		void UpdateCameraView();

		virtual bool OnMouseScroll(MouseScrolledEvent& e) override;

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
		glm::vec3 m_Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };
		glm::vec2 m_InitialMousePosition = { 0.0f, 0.0f };
		float m_Distance = 10.0f;

		float m_MinFocusDistance = 100.0f;

		uint32_t m_ViewportWidth = 1280;
		uint32_t m_ViewportHeight = 720;

	};

}
