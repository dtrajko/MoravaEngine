#include "Scene/SceneDeferredOGL.h"


SceneDeferredOGL::SceneDeferredOGL()
{
    sceneSettings.cameraPosition = glm::vec3(0.0f, 0.0f, 8.0f);
    sceneSettings.cameraStartYaw = -90.0f;
    sceneSettings.cameraStartPitch = 0.0f;
    sceneSettings.cameraMoveSpeed = 1.0f;
    sceneSettings.enablePointLights  = false;
    sceneSettings.enableSpotLights   = false;
    sceneSettings.enableOmniShadows  = false;
    sceneSettings.enableSkybox       = false;
    sceneSettings.enableShadows      = false;
    sceneSettings.enableWaterEffects = false;
    sceneSettings.enableParticles    = false;

    ResourceManager::Init();

    SetCamera();
}

SceneDeferredOGL::~SceneDeferredOGL()
{
}

void SceneDeferredOGL::Update(float timestep, Window* mainWindow)
{
    m_Camera->OnUpdate(timestep);

}

void SceneDeferredOGL::UpdateImGui(float timestep, Window* mainWindow)
{
    bool p_open = true;
    ShowExampleAppDockSpace(&p_open, mainWindow);

}

void SceneDeferredOGL::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
}
