#define _CRT_SECURE_NO_WARNINGS

#include "Scene/SceneEnvMapVulkan.h"

#include "H2M/Scene/ComponentsH2M.h"
#include "H2M/Renderer/TextureH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"

#include "../../ImGuizmo/ImGuizmo.h"

#include "Core/Application.h"
#include "Core/Input.h"
#include "Core/Math.h"
#include "Core/Timer.h"
#include "Core/Util.h"
#include "EnvMapVulkan/EnvMapVulkanEditorLayer.h"
#include "Mesh/Block.h"
#include "Shader/MoravaShader.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include <yaml-cpp/yaml.h>


SceneEnvMapVulkan::SceneEnvMapVulkan()
{
    sceneSettings.cameraPosition = glm::vec3(0.0f, 5.0f, 24.0f);
    sceneSettings.cameraStartYaw = -90.0f;
    sceneSettings.cameraStartPitch = 0.0f;
    sceneSettings.cameraMoveSpeed = 2.0f;
    sceneSettings.waterHeight = 0.0f;
    sceneSettings.waterWaveSpeed = 0.05f;
    sceneSettings.enablePointLights = true;
    sceneSettings.enableSpotLights = true;
    sceneSettings.enableSkybox = false;
    sceneSettings.enableShadows = true;
    sceneSettings.enableOmniShadows = true;
    sceneSettings.enableCascadedShadowMaps = true;
    sceneSettings.enableWaterEffects = false;
    sceneSettings.enableParticles = false;

    // directional light
    sceneSettings.directionalLight.base.enabled = true;
    sceneSettings.directionalLight.base.color = glm::vec3(1.0f, 1.0f, 1.0f);
    sceneSettings.directionalLight.direction = glm::vec3(0.6f, -0.5f, -0.6f);
    sceneSettings.directionalLight.base.ambientIntensity = 0.75f;
    sceneSettings.directionalLight.base.diffuseIntensity = 0.4f;
    sceneSettings.lightProjectionMatrix = glm::ortho(-32.0f, 32.0f, -32.0f, 32.0f, -32.0f, 32.0f);

    // point lights
    sceneSettings.pointLights[0].base.enabled = true;
    sceneSettings.pointLights[0].base.color = glm::vec3(1.0f, 1.0f, 0.0f);
    sceneSettings.pointLights[0].position = glm::vec3(-1.0f, 4.0f, 1.0f);
    sceneSettings.pointLights[0].base.ambientIntensity = 1.0f;
    sceneSettings.pointLights[0].base.diffuseIntensity = 1.0f;

    sceneSettings.pointLights[1].base.enabled = false;
    sceneSettings.pointLights[1].base.color = glm::vec3(1.0f, 1.0f, 1.0f);
    sceneSettings.pointLights[1].position = glm::vec3(5.0f, 2.0f, 5.0f);
    sceneSettings.pointLights[1].base.ambientIntensity = 1.0f;
    sceneSettings.pointLights[1].base.diffuseIntensity = 1.0f;

    sceneSettings.pointLights[2].base.enabled = true;
    sceneSettings.pointLights[2].base.color = glm::vec3(0.0f, 1.0f, 1.0f);
    sceneSettings.pointLights[2].position = glm::vec3(-2.0f, 4.0f, -2.0f);
    sceneSettings.pointLights[2].base.ambientIntensity = 1.0f;
    sceneSettings.pointLights[2].base.diffuseIntensity = 1.0f;

    sceneSettings.pointLights[3].base.enabled = false;
    sceneSettings.pointLights[3].base.color = glm::vec3(0.0f, 1.0f, 0.0f);
    sceneSettings.pointLights[3].position = glm::vec3(5.0f, 2.0f, -5.0f);
    sceneSettings.pointLights[3].base.ambientIntensity = 1.0f;
    sceneSettings.pointLights[3].base.diffuseIntensity = 1.0f;

    // spot lights
    sceneSettings.spotLights[0].base.base.enabled = false;
    sceneSettings.spotLights[0].base.base.color = glm::vec3(1.0f, 0.0f, 0.0f);
    sceneSettings.spotLights[0].base.position = glm::vec3(-5.0f, 2.0f, 0.0f);
    sceneSettings.spotLights[0].direction = glm::vec3(1.0f, 0.0f, 0.0f);
    sceneSettings.spotLights[0].base.base.ambientIntensity = 2.0f;
    sceneSettings.spotLights[0].base.base.diffuseIntensity = 1.0f;
    sceneSettings.spotLights[0].edge = 0.5f;

    sceneSettings.spotLights[1].base.base.enabled = false;
    sceneSettings.spotLights[1].base.base.color = glm::vec3(1.0f, 1.0f, 0.0f);
    sceneSettings.spotLights[1].base.position = glm::vec3(5.0f, 2.0f, 0.0f);
    sceneSettings.spotLights[1].direction = glm::vec3(-1.0f, 0.0f, 0.0f);
    sceneSettings.spotLights[1].base.base.ambientIntensity = 2.0f;
    sceneSettings.spotLights[1].base.base.diffuseIntensity = 1.0f;
    sceneSettings.spotLights[1].edge = 0.5f;

    sceneSettings.spotLights[2].base.base.enabled = false;
    sceneSettings.spotLights[2].base.base.color = glm::vec3(0.0f, 1.0f, 0.0f);
    sceneSettings.spotLights[2].base.position = glm::vec3(0.0f, 2.0f, -5.0f);
    sceneSettings.spotLights[2].direction = glm::vec3(0.0f, 0.0f, 1.0f);
    sceneSettings.spotLights[2].base.base.ambientIntensity = 2.0f;
    sceneSettings.spotLights[2].base.base.diffuseIntensity = 1.0f;
    sceneSettings.spotLights[2].edge = 0.5f;

    sceneSettings.spotLights[3].base.base.enabled = false;
    sceneSettings.spotLights[3].base.base.color = glm::vec3(1.0f, 0.0f, 1.0f);
    sceneSettings.spotLights[3].base.position = glm::vec3(0.0f, 2.0f, 5.0f);
    sceneSettings.spotLights[3].direction = glm::vec3(0.0f, 0.0f, -1.0f);
    sceneSettings.spotLights[3].base.base.ambientIntensity = 2.0f;
    sceneSettings.spotLights[3].base.base.diffuseIntensity = 1.0f;
    sceneSettings.spotLights[3].edge = 0.5f;

    sceneSettings.nearPlane = 0.01f;
    sceneSettings.farPlane = 2000.0f;

    SetCamera();
    SetLightManager();
    SetupShaders();
    SetupTextureSlots();
    SetupTextures();
    SetupMaterials();
    SetupFramebuffers();

    H2M::RefH2M<H2M::UserPreferencesH2M> userPreferences = H2M::RefH2M<H2M::UserPreferencesH2M>::Create();
    userPreferences->FilePath = "Textures/HDR/umhlanga_sunrise_4k.hdr";
    m_EnvMapVulkanEditorLayer = new EnvMapVulkanEditorLayer(userPreferences);

    SetupMeshes();
    SetupModels();

    // m_Grid = new Grid(20);
    m_PivotScene = new Pivot(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(50.0f, 50.0f, 50.0f));

    // H2M::RendererAPI_H2M::Init();

    m_CsGame = new CsGame();
}

SceneEnvMapVulkan::~SceneEnvMapVulkan()
{
    delete m_CsGame;
    delete m_EnvMapVulkanEditorLayer;
}

void SceneEnvMapVulkan::SetupShaders()
{
}

void SceneEnvMapVulkan::SetLightManager()
{
}

void SceneEnvMapVulkan::SetWaterManager(int width, int height)
{
}

void SceneEnvMapVulkan::SetupTextures()
{
}

void SceneEnvMapVulkan::SetupTextureSlots()
{
}

void SceneEnvMapVulkan::SetupMaterials()
{
}

void SceneEnvMapVulkan::SetupMeshes()
{
}

void SceneEnvMapVulkan::SetupModels()
{
}

void SceneEnvMapVulkan::SetupFramebuffers()
{
}

void SceneEnvMapVulkan::SetupUniforms()
{
}

void SceneEnvMapVulkan::Update(float timestep, Window* mainWindow)
{
    Scene::Update(timestep, mainWindow);

    float deltaTime = Timer::Get()->GetDeltaTime();
    m_EnvMapVulkanEditorLayer->OnUpdate(deltaTime);

    m_CsGame->tick();
}

void SceneEnvMapVulkan::UpdateImGui(float timestep, Window* mainWindow)
{
    m_EnvMapVulkanEditorLayer->OnImGuiRender();
}

void SceneEnvMapVulkan::ShowExampleAppDockSpace(bool* p_open, Window* mainWindow)
{
    // m_EnvMapVulkanEditorLayer->ShowExampleAppDockSpace(p_open, mainWindow);
}

bool SceneEnvMapVulkan::OnKeyPressed(H2M::KeyPressedEventH2M& e)
{
    // Shortcuts
    if (e.GetRepeatCount() > 0) {
        return false;
    }

    bool control = Input::IsKeyPressed(KeyH2M::LeftControl) || Input::IsKeyPressed(KeyH2M::RightControl);
    bool shift = Input::IsKeyPressed(KeyH2M::LeftShift) || Input::IsKeyPressed(KeyH2M::RightShift);

    switch (e.GetKeyCode())
    {
        case (int)KeyH2M::N:
        {
            if (control)
            {
                m_EnvMapVulkanEditorLayer->NewScene();
            }
            break;
        }
        case (int)KeyH2M::O:
        {
            if (control)
            {
                m_EnvMapVulkanEditorLayer->OpenScene();
            }
            break;
        }
        case (int)KeyH2M::S:
        {
            if (control)
            {
                if (shift)
                {
                    m_EnvMapVulkanEditorLayer->SaveSceneAs();
                }
                else
                {
                    m_EnvMapVulkanEditorLayer->SaveScene();
                }
            }
            break;
        }
    }
    return true;
}

void SceneEnvMapVulkan::OnEntitySelected(H2M::EntityH2M entity)
{
    // auto& tc = entity.GetComponent<H2M::TransformComponent>();
    // m_EnvMapEditorLayer->SetMeshEntity(entity);
}

void SceneEnvMapVulkan::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
    std::map<std::string, H2M::RefH2M<MoravaShader>> shaders, std::map<std::string, int> uniforms)
{
    if (passType == "shadow" && sceneSettings.enableShadows) {
        // m_EnvMapVulkanEditorLayer->OnRenderShadow(mainWindow);
    }

    if (passType == "shadow_omni" && sceneSettings.enableOmniShadows) {
        // m_EnvMapVulkanEditorLayer->OnRenderShadowOmni(mainWindow);
    }

    if (passType == "cascaded_shadow_maps" && sceneSettings.enableCascadedShadowMaps) {
        // m_EnvMapVulkanEditorLayer->OnRenderCascadedShadowMaps(mainWindow);
    }

    if (passType == "water_reflection" && sceneSettings.enableWaterEffects) {
        // m_EnvMapVulkanEditorLayer->OnRenderWaterReflection(mainWindow);
    }

    if (passType == "water_refraction" && sceneSettings.enableWaterEffects) {
        // m_EnvMapVulkanEditorLayer->OnRenderWaterRefraction(mainWindow);
    }

    if (passType == "main") {
        m_EnvMapVulkanEditorLayer->OnRender();
    }

    if (passType == "post_processing") {
        // m_EnvMapVulkanEditorLayer->PostProcessing(mainWindow);
    }

    // m_EnvMapEditorLayer->RenderFullscreen(mainWindow);
}
