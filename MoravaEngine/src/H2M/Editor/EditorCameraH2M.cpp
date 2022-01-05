/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "EditorCameraH2M.h"

#include "H2M/Core/KeyCodesH2M.h"
#include "H2M/Core/MouseCodesH2M.h"
#include "H2M/Core/Events/ApplicationEventH2M.h"

#include "Core/Input.h"

#include <algorithm>


namespace H2M {

	EditorCameraH2M::EditorCameraH2M(const glm::mat4& projectionMatrix)
		: CameraH2M(projectionMatrix)
	{
		// m_Rotation = glm::vec3(90.0f, 0.0f, 0.0f);
		m_FocalPoint = glm::vec3(0.0f);

		glm::vec3 position = { 0, 5, 20 };
		m_Distance = glm::distance(position, m_FocalPoint);

		m_Yaw = 3.0f * (float)M_PI / 4.0f;
		m_Pitch = M_PI / 4.0f;

		UpdateCameraView();
	}

	EditorCameraH2M::EditorCameraH2M(float fov, float aspectRatio, float nearClip, float farClip)
		: CameraH2M(glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip))
	{
		m_PerspectiveFOV = glm::radians(fov);
		m_PerspectiveNear = nearClip;
		m_PerspectiveFar = farClip;
		m_AspectRatio = aspectRatio;

		m_OrthographicNear = nearClip;
		m_OrthographicFar = farClip;

		m_WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
		m_Front = glm::vec3(0.0f, 0.0f, -1.0f);

		UpdateView();
	}

	void EditorCameraH2M::UpdateCameraView()
	{
		m_Position = CalculatePosition();

		glm::quat orientation = GetOrientation();
		// m_Rotation = glm::eulerAngles(orientation) * (180.0f / (float)M_PI);
		m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
		m_ViewMatrix = glm::inverse(m_ViewMatrix);
	}

	void EditorCameraH2M::Focus(const glm::vec3& focusPoint)
	{
		m_FocalPoint = focusPoint;
		if (m_Distance > m_MinFocusDistance)
		{
			float distance = m_Distance - m_MinFocusDistance;
			MouseZoom(distance / ZoomSpeed());
			UpdateCameraView();
		}
	}

	std::pair<float, float> EditorCameraH2M::PanSpeed() const
	{
		float x = std::min(m_ViewportWidth / 1000.0f, 2.4f); // max = 2.4f
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = std::min(m_ViewportHeight / 1000.0f, 2.4f); // max = 2.4f
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	float EditorCameraH2M::RotationSpeed() const
	{
		return 0.8f;
	}

	float EditorCameraH2M::ZoomSpeed() const
	{
		float distance = m_Distance * 0.2f;
		distance = std::max(distance, 0.0f);
		float speed = distance * distance;
		speed = std::min(speed, 100.0f); // max speed = 100
		return speed;
	}

	void EditorCameraH2M::UpdateView()
	{
		// m_Yaw = m_Pitch = 0.0f; // Lock the camera's rotation
		m_Position = CalculatePosition();

		glm::quat orientation = GetOrientation();
		m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
		m_ViewMatrix = glm::inverse(m_ViewMatrix);
	}

	void EditorCameraH2M::UpdateProjection()
	{
		m_AspectRatio = (float)m_ViewportWidth / (float)m_ViewportHeight;
		m_ProjectionMatrix = glm::perspective(m_PerspectiveFOV, m_AspectRatio, m_PerspectiveNear, m_PerspectiveFar);
	}

	void EditorCameraH2M::OnUpdate(TimestepH2M ts)
	{
		if (Input::IsKeyPressed(KeyH2M::LeftAlt))
		{
			const glm::vec2& mouse{ Input::GetMouseX(), Input::GetMouseY() };
			glm::vec2 delta = (mouse - m_InitialMousePosition) * 0.003f;
			m_InitialMousePosition = mouse;

			if (Input::IsMouseButtonPressed(MouseH2M::ButtonMiddle)) {
				MousePan(delta);
			}
			else if (Input::IsMouseButtonPressed(MouseH2M::ButtonLeft)) {
				MouseRotate(delta);
			}
			else if (Input::IsMouseButtonPressed(MouseH2M::ButtonRight)) {
				MouseZoom(delta.y);
			}
		}

		m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
		m_Up = glm::normalize(glm::cross(m_Right, m_Front));

		UpdateView();
	}

	void EditorCameraH2M::OnEvent(EventH2M& e)
	{
		EventDispatcherH2M dispatcher(e);
		dispatcher.Dispatch<MouseScrolledEventH2M>(H2M_BIND_EVENT_FN(EditorCameraH2M::OnMouseScroll));

		if (e.GetEventType() == EventTypeH2M::WindowResize)
		{
			WindowResizeEventH2M& event = (WindowResizeEventH2M&)e;
			if (event.GetWidth() != 0 && event.GetHeight() != 0)
			{
				SetViewportSize((float)event.GetWidth(), (float)event.GetHeight());
			}
		}
	}

	void EditorCameraH2M::SetViewportSize(float width, float height)
	{
		if (width == 0.0f || height == 0.0f) return;

		m_ViewportWidth = (uint32_t)width;
		m_ViewportHeight = (uint32_t)height;

		UpdateView();
		UpdateProjection();
	}

	bool EditorCameraH2M::OnMouseScroll(MouseScrolledEventH2M& e)
	{
		float delta = e.GetYOffset() * 0.1f;
		MouseZoom(delta);
		UpdateView();
		return false;
	}

	void EditorCameraH2M::MousePan(const glm::vec2& delta)
	{
		auto [xSpeed, ySpeed] = PanSpeed();
		m_FocalPoint += -GetRightDirection() * delta.x * xSpeed * m_Distance;
		m_FocalPoint += GetUpDirection() * delta.y * ySpeed * m_Distance;
	}

	void EditorCameraH2M::MouseRotate(const glm::vec2& delta)
	{
		float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
		m_Yaw += yawSign * delta.x * RotationSpeed();
		m_Pitch += delta.y * RotationSpeed();
	}

	void EditorCameraH2M::MouseZoom(float delta)
	{
		m_Distance -= delta * ZoomSpeed();
		if (m_Distance < 1.0f)
		{
			m_FocalPoint += GetForwardDirection();
			m_Distance = 1.0f;
		}
	}

	glm::vec3 EditorCameraH2M::GetUpDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::vec3 EditorCameraH2M::GetRightDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::vec3 EditorCameraH2M::GetForwardDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
	}

	glm::vec3 EditorCameraH2M::CalculatePosition() const
	{
		return m_FocalPoint - GetForwardDirection() * m_Distance;
	}

	glm::quat EditorCameraH2M::GetOrientation() const
	{
		return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
	}

}
