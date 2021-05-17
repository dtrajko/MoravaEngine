#define _CRT_SECURE_NO_WARNINGS

#include "Scene/SceneHazelVulkan.h"

#include "Hazel/Scene/Components.h"
#include "Hazel/Renderer/HazelTexture.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "../../ImGuizmo/ImGuizmo.h"

#include "Core/Application.h"
#include "Core/Input.h"
#include "Core/Math.h"
#include "Core/Timer.h"
#include "Core/Util.h"
#include "Mesh/Block.h"
#include "Shader/Shader.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include <yaml-cpp/yaml.h>


SceneHazelVulkan::SceneHazelVulkan()
{
    sceneSettings.cameraPosition     = glm::vec3(0.0f, 100.0f, 300.0f);
    sceneSettings.cameraStartYaw     = -90.0f;
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

    m_VulkanTestLayer = std::make_unique<VulkanTestLayer>();
    m_VulkanTestLayer->OnAttach();

    SetCamera();
    SetLightManager();
    SetupShaders();
    SetupTextureSlots();
    SetupTextures();
    SetupMaterials();
    SetupFramebuffers();
    SetupMeshes();
    SetupModels();

    // m_Grid = new Grid(20);
    // m_PivotScene = new Pivot(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(50.0f, 50.0f, 50.0f));

    // Hazel::RendererAPI::Init();
}

SceneHazelVulkan::~SceneHazelVulkan()
{
}

void SceneHazelVulkan::SetupShaders()
{
    // m_ShaderBackground = Hazel::Ref<Shader>::Create("Shaders/LearnOpenGL/2.2.2.background.vs", "Shaders/LearnOpenGL/2.2.2.background.fs");
    // Log::GetLogger()->info("SceneHazelVulkan: m_ShaderBackground compiled [programID={0}]", m_ShaderBackground->GetProgramID());
    // 
    // m_ShaderBasic = Hazel::Ref<Shader>::Create("Shaders/basic.vs", "Shaders/basic.fs");
    // Log::GetLogger()->info("SceneHazelVulkan: m_ShaderBasic compiled [programID={0}]", m_ShaderBasic->GetProgramID());
    // 
    // ResourceManager::AddShader("LearnOpenGL/2.2.2.background", m_ShaderBackground);
    // ResourceManager::AddShader("basic", m_ShaderBasic);
}

void SceneHazelVulkan::SetLightManager()
{
}

void SceneHazelVulkan::SetWaterManager(int width, int height)
{
}

void SceneHazelVulkan::SetupTextures()
{
}

void SceneHazelVulkan::SetupTextureSlots()
{
}

void SceneHazelVulkan::SetupMaterials()
{
}

void SceneHazelVulkan::SetupMeshes()
{
}

void SceneHazelVulkan::SetupModels()
{
}

void SceneHazelVulkan::SetupFramebuffers()
{
}

void SceneHazelVulkan::SetupUniforms()
{
}

void SceneHazelVulkan::Update(float timestep, Window* mainWindow)
{
    Scene::Update(timestep, mainWindow);
}

void SceneHazelVulkan::UpdateImGui(float timestep, Window* mainWindow)
{
    m_VulkanTestLayer->OnImGuiRender(mainWindow, this);
}

void SceneHazelVulkan::ShowExampleAppDockSpace(bool* p_open, Window* mainWindow)
{
    m_VulkanTestLayer->ShowExampleAppDockSpace(p_open, mainWindow);
}

bool SceneHazelVulkan::OnKeyPressed(KeyPressedEvent& e)
{
    return false;
}

void SceneHazelVulkan::OnEntitySelected(Hazel::Entity entity)
{
}

void SceneHazelVulkan::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
    std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
    float deltaTime = Timer::Get()->GetDeltaTime();
    m_VulkanTestLayer->OnUpdate(deltaTime, m_Camera->GetViewMatrix());

    VulkanWeekRenderer::WaitAndRender();

    if (passType == "main") {
        m_VulkanTestLayer->OnRender(mainWindow);
    }
}
