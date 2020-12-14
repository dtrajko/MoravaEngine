#include "RuntimeCamera.h"


RuntimeCamera::RuntimeCamera()
{
    m_Camera = Camera(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f);
    // m_CameraController = CameraController(&m_Camera, 1.778f, 2.0f, 0.1f);
}

RuntimeCamera::RuntimeCamera(float fov, float aspectRatio, float nearClip, float farClip)
{
    m_Camera = Camera(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f);
    // m_CameraController = CameraController(&m_Camera, aspectRatio, 2.0f, 0.1f);
}

void RuntimeCamera::OnUpdate(Hazel::Timestep ts)
{
    m_Camera.Update();
    // m_CameraController.Update();
}

void RuntimeCamera::OnEvent(Event& e)
{
    m_Camera.OnEvent(e);
}

void RuntimeCamera::SetViewportSize(float width, float height)
{
    // m_CameraController.OnResize((uint32_t)width, (uint32_t)height);
}
