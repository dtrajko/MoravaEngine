#define _CRT_SECURE_NO_WARNINGS

#include "Scene/SceneHazelEnvMap.h"

#include "Hazel/Scene/Components.h"
#include "Hazel/Renderer/HazelTexture.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "../../ImGuizmo/ImGuizmo.h"

#include "Core/Application.h"
#include "Core/Input.h"
#include "Core/Math.h"
#include "Core/Timer.h"
#include "Core/Util.h"
#include "EnvMap/EnvMapEditorLayer.h"
#include "Mesh/Block.h"
#include "Mono/CsBind.h"
#include "Shader/MoravaShader.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include <yaml-cpp/yaml.h>


SceneHazelEnvMap::SceneHazelEnvMap()
{
    sceneSettings.cameraPosition = glm::vec3(-24.0f, 5.0f, 0.0f);
    sceneSettings.cameraStartYaw = 0.0f;
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

    m_EnvMapEditorLayer = std::make_unique<EnvMapEditorLayer>("Textures/HDR/umhlanga_sunrise_4k.hdr", this);

    SetupMeshes();
    SetupModels();

    m_Grid = new Grid(20);
    m_PivotScene = new Pivot(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(50.0f, 50.0f, 50.0f));

    // Hazel::RendererAPI::Init();

    InitMono();
}

SceneHazelEnvMap::~SceneHazelEnvMap()
{
    // Release the domain
    if (m_ptrMonoDomain)
    {
        mono_jit_cleanup(m_ptrMonoDomain);

        Log::GetLogger()->info("Mono Domain cleanup is complete!");
    }
}

void SceneHazelEnvMap::InitMono()
{
    /**** BEGIN Mono ****/

    // Current directory as mono dir
    mono_set_dirs("C:/Program Files/Mono/lib", "C:/Program Files/Mono/etc");

    // Create Mono domain
    m_ptrMonoDomain = mono_jit_init("MoravaEngine");
    if (m_ptrMonoDomain)
    {
        Log::GetLogger()->info("Mono Domain successfully initialized!");

        // Load a Mono assembly HangmanScript.dll
        std::string assemblyFilepath = "Projects/HangmanScript/bin/Debug/netstandard2.0/HangmanScript.dll";
        m_ptrGameAssembly = mono_domain_assembly_open(m_ptrMonoDomain, assemblyFilepath.c_str());
        if (m_ptrGameAssembly)
        {
            Log::GetLogger()->info("Mono Domain Assembly '{0}' successfully opened!", assemblyFilepath);

            // Loading Mono image
            m_ptrGameAssemblyImage = mono_assembly_get_image(m_ptrGameAssembly);
            if (m_ptrGameAssemblyImage)
            {
                Log::GetLogger()->info("Mono Assembly Image successfullu loaded!");

                // Add internal calls
                mono_add_internal_call("CGL.RandomWord::getRandomWord()", nullptr/*, &CsBind::CS_RandomWord_getRandomWord */);
                mono_add_internal_call("CGL.GameConsole::clearBuffer()", nullptr/*, &CGL::Console::clearBuffer */);
                mono_add_internal_call("CGL.GameConsole::present()", nullptr/*, &CGL::Console::present */);
                mono_add_internal_call("CGL.GameConsole::putChar(uint, uint, char)", nullptr/*, &CGL::Console::putChar */);
                mono_add_internal_call("CGL.GameConsole::putString(uint, uint, string, bool)", nullptr/*, &CsBind::CS_GameConsole_putString */);

                // Find IGame
                MonoClass* ptrIGameClass = mono_class_from_name(m_ptrGameAssemblyImage, "HangmanScript", "IGame");
                MonoClass* ptrMainClass = mono_class_from_name(m_ptrGameAssemblyImage, "HangmanScript", "HangMain");
                if (ptrIGameClass && ptrMainClass)
                {
                    // Describe method
                    MonoMethodDesc* ptrMainMethodDesc = mono_method_desc_new(".HangMain:main()", false);
                }
            }
        }
    }

    /**** END Mono ****/
}

void SceneHazelEnvMap::SetupShaders()
{
    m_ShaderBackground = Hazel::Ref<MoravaShader>::Create("Shaders/LearnOpenGL/2.2.2.background.vs", "Shaders/LearnOpenGL/2.2.2.background.fs");
    Log::GetLogger()->info("SceneHazelEnvMap: m_ShaderBackground compiled [programID={0}]", m_ShaderBackground->GetProgramID());

    m_ShaderBasic = Hazel::Ref<MoravaShader>::Create("Shaders/basic.vs", "Shaders/basic.fs");
    Log::GetLogger()->info("SceneHazelEnvMap: m_ShaderBasic compiled [programID={0}]", m_ShaderBasic->GetProgramID());

    ResourceManager::AddShader("LearnOpenGL/2.2.2.background", m_ShaderBackground);
    ResourceManager::AddShader("basic", m_ShaderBasic);
}

void SceneHazelEnvMap::SetLightManager()
{
}

void SceneHazelEnvMap::SetWaterManager(int width, int height)
{
}

void SceneHazelEnvMap::SetupTextures()
{
}

void SceneHazelEnvMap::SetupTextureSlots()
{
}

void SceneHazelEnvMap::SetupMaterials()
{
}

void SceneHazelEnvMap::SetupMeshes()
{
}

void SceneHazelEnvMap::SetupModels()
{
}

void SceneHazelEnvMap::SetupFramebuffers()
{
}

void SceneHazelEnvMap::SetupUniforms()
{
}

void SceneHazelEnvMap::Update(float timestep, Window* mainWindow)
{
    Scene::Update(timestep, mainWindow);

    float deltaTime = Timer::Get()->GetDeltaTime();
    m_EnvMapEditorLayer->OnUpdate(deltaTime);
}

void SceneHazelEnvMap::UpdateImGui(float timestep, Window* mainWindow)
{
    m_EnvMapEditorLayer->OnImGuiRender(mainWindow, this);
}

void SceneHazelEnvMap::ShowExampleAppDockSpace(bool* p_open, Window* mainWindow)
{
    m_EnvMapEditorLayer->ShowExampleAppDockSpace(p_open, mainWindow);
}

bool SceneHazelEnvMap::OnKeyPressed(KeyPressedEvent& e)
{
    // Shortcuts
    if (e.GetRepeatCount() > 0) {
        return false;
    }

    bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
    bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);

    switch (e.GetKeyCode())
    {
        case (int)Key::N:
        {
            if (control)
            {
                m_EnvMapEditorLayer->NewScene();
            }
            break;
        }
        case (int)Key::O:
        {
            if (control)
            {
                m_EnvMapEditorLayer->OpenScene();
            }
            break;
        }
        case (int)Key::S:
        {
            if (control && shift)
            {
                m_EnvMapEditorLayer->SaveSceneAs();
            }
            break;
        }
    }
    return true;
}

void SceneHazelEnvMap::OnEntitySelected(Hazel::Entity entity)
{
    // auto& tc = entity.GetComponent<Hazel::TransformComponent>();
    // m_EnvMapEditorLayer->SetMeshEntity(entity);
}

void SceneHazelEnvMap::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
    std::map<std::string, Hazel::Ref<MoravaShader>> shaders, std::map<std::string, int> uniforms)
{
    if (passType == "shadow" && sceneSettings.enableShadows) {
        m_EnvMapEditorLayer->OnRenderShadow(mainWindow);
    }

    if (passType == "shadow_omni" && sceneSettings.enableOmniShadows) {
        m_EnvMapEditorLayer->OnRenderShadowOmni(mainWindow);
    }

    if (passType == "cascaded_shadow_maps" && sceneSettings.enableCascadedShadowMaps) {
        m_EnvMapEditorLayer->OnRenderCascadedShadowMaps(mainWindow);
    }

    if (passType == "water_reflection" && sceneSettings.enableWaterEffects) {
        m_EnvMapEditorLayer->OnRenderWaterReflection(mainWindow);
    }

    if (passType == "water_refraction" && sceneSettings.enableWaterEffects) {
        m_EnvMapEditorLayer->OnRenderWaterRefraction(mainWindow);
    }

    if (passType == "main") {
        m_EnvMapEditorLayer->OnRender(mainWindow);
    }

    if (passType == "post_processing") {
        m_EnvMapEditorLayer->PostProcessing(mainWindow);
    }

    // m_EnvMapEditorLayer->RenderFullscreen(mainWindow);
}
