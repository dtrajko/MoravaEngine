#pragma once

#include "HazelCamera.h"
#include "../Events/Event.h"
#include "../Events/MouseEvent.h"
#include "../Core/Timestep.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Hazel {

	class EditorCamera : public HazelCamera
	{
	public:
		EditorCamera() = default;
		EditorCamera(float fov, float aspectRatio, float nearClip, float farClip);

		virtual void OnUpdate(Timestep ts) override;
		virtual void OnEvent(Event& e) override;
		virtual void SetViewportSize(float width, float height) override;

		inline float GetDistance() const { return m_Distance; }
		inline void SetDistance(float distance) { m_Distance = distance; }

		glm::vec3 GetUpDirection() const;
		glm::vec3 GetRightDirection() const;
		glm::vec3 GetForwardDirection() const;
		glm::quat GetOrientation() const;

	private:
		void UpdateProjection();
		void UpdateView();

		virtual bool OnMouseScroll(MouseScrolledEvent& e) override;

		void MousePan(const glm::vec2& delta);
		void MouseRotate(const glm::vec2& delta);
		void MouseZoom(float delta);

		glm::vec3 CalculatePosition() const;

		std::pair<float, float> PanSpeed() const;
		float RotationSpeed() const;
		float ZoomSpeed() const;

	private:
		glm::vec3 m_FocalPoint = { 0.0f, 0.0f, 0.0f };
		glm::vec2 m_InitialMousePosition = { 0.0f, 0.0f };
		float m_Distance = 10.0f;

	};

}
