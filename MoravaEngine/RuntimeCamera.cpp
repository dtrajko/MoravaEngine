#include "RuntimeCamera.h"

#include "Application.h"
#include "MousePicker.h"


RuntimeCamera::RuntimeCamera()
{
    Camera camera = Camera(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f);
    m_CameraController = CameraController(&camera, 1.778f, 2.0f, 0.1f);
}

RuntimeCamera::RuntimeCamera(float fov, float aspectRatio, float nearClip, float farClip)
{
    Camera camera = Camera(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f);
    m_CameraController = CameraController(&camera, aspectRatio, 2.0f, 0.1f);
}

void RuntimeCamera::OnUpdate(Hazel::Timestep ts)
{
    m_CameraController.Update();

	MousePicker::Get()->Update(
		(int)Application::Get()->GetWindow()->GetMouseX(), (int)Application::Get()->GetWindow()->GetMouseY(),
		0, 0, (int)Application::Get()->GetWindow()->GetWidth(), (int)Application::Get()->GetWindow()->GetHeight(),
		RendererBasic::GetProjectionMatrix(), m_CameraController.CalculateViewMatrix());
}

void RuntimeCamera::OnEvent(Event& e)
{
    m_CameraController.OnEvent(e);
}

void RuntimeCamera::SetViewportSize(float width, float height)
{
    m_CameraController.OnResize((uint32_t)width, (uint32_t)height);
}
