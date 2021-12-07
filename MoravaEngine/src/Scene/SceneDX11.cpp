#define _CRT_SECURE_NO_WARNINGS

#include "Scene/SceneDX11.h"

#include "H2M/Scene/ComponentsH2M.h"
#include "H2M/Renderer/TextureH2M.h"
#include "H2M/Scene/EntityH2M.h"

#include "Core/Application.h"
#include "Core/Input.h"
#include "Core/Math.h"
#include "Core/Timer.h"
#include "Core/Util.h"
#include "Mesh/Block.h"
#include "Shader/MoravaShader.h"

#include "../../ImGuizmo/ImGuizmo.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include <yaml-cpp/yaml.h>


SceneDX11::SceneDX11()
{
    sceneSettings.cameraPosition     = glm::vec3(-125.0f, -16.0f, 50.0f);
    sceneSettings.cameraStartYaw     = 0.0f;
    sceneSettings.cameraStartPitch   = 0.0f;
    sceneSettings.cameraMoveSpeed    = 2.0f;
    sceneSettings.waterHeight        = 0.0f;
    sceneSettings.waterWaveSpeed     = 0.05f;
    sceneSettings.enablePointLights  = true;
    sceneSettings.enableSpotLights   = true;
    sceneSettings.enableOmniShadows  = true;
    sceneSettings.enableSkybox       = false;
    sceneSettings.enableShadows      = true;
    sceneSettings.enableWaterEffects = false;
    sceneSettings.enableParticles    = false;

    sceneSettings.nearPlane = 0.01f;
    sceneSettings.farPlane = 2000.0f;

    SetCamera();
    SetLightManager();
    SetupShaders();
    SetupTextureSlots();
    SetupTextures();
    SetupMaterials();
    SetupFramebuffers();
    SetupMeshes();
    SetupModels();
}

SceneDX11::~SceneDX11()
{
}

void SceneDX11::SetupShaders()
{
}

void SceneDX11::SetLightManager()
{
}

void SceneDX11::SetWaterManager(int width, int height)
{
}

void SceneDX11::SetupTextures()
{
}

void SceneDX11::SetupTextureSlots()
{
}

void SceneDX11::SetupMaterials()
{
}

void SceneDX11::SetupMeshes()
{
}

void SceneDX11::SetupModels()
{
}

void SceneDX11::SetupFramebuffers()
{
}

void SceneDX11::SetupUniforms()
{
}

void SceneDX11::Update(float timestep, Window* mainWindow)
{
    Timer::Get()->Update();
 
    // Log::GetLogger()->debug("SceneDX11::Update(timestep: '{0}', Timer::GetCurrentTimestamp: '{1}'): ", timestep, Timer::Get()->GetCurrentTimestamp());

    m_Camera->OnUpdate(timestep);

    // Scene::Update(timestep, mainWindow);
}

void SceneDX11::UpdateImGui(float timestep, Window* mainWindow)
{
}

void SceneDX11::ShowExampleAppDockSpace(bool* p_open, Window* mainWindow)
{
}

bool SceneDX11::OnKeyPressed(H2M::KeyPressedEventH2M& e)
{
    return false;
}

void SceneDX11::OnEntitySelected(H2M::EntityH2M entity)
{
}

void SceneDX11::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
    std::map<std::string, H2M::RefH2M<MoravaShader>> shaders, std::map<std::string, int> uniforms)
{
}
