#define _CRT_SECURE_NO_WARNINGS

#include "SceneHazelEnvMap.h"

#include "Hazel/Renderer/HazelTexture.h"
#include "Hazel/Utils/PlatformUtils.h"
#include "Hazel/Scene/SceneSerializer.h"
#include "Hazel/Scene/Components.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "ImGuiWrapper.h"
#include "ImGuizmo.h"
#include "RendererBasic.h"
#include "ResourceManager.h"
#include "Block.h"
#include "Application.h"
#include "Shader.h"
#include "Timer.h"
#include "MousePicker.h"
#include "Math.h"
#include "Input.h"
#include "Util.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include <yaml-cpp/yaml.h>


SceneHazelEnvMap::SceneHazelEnvMap()
{
    sceneSettings.cameraPosition     = glm::vec3(20.0f, 10.0f, 0.0f);
    sceneSettings.cameraStartYaw     = -180.0f;
    sceneSettings.cameraStartPitch   = 20.0f;
    sceneSettings.cameraMoveSpeed    = 2.0f;
    sceneSettings.waterHeight        = 0.0f;
    sceneSettings.waterWaveSpeed     = 0.05f;
    sceneSettings.enablePointLights  = true;
    sceneSettings.enableSpotLights   = true;
    sceneSettings.enableOmniShadows  = false;
    sceneSettings.enableSkybox       = false;
    sceneSettings.enableShadows      = false;
    sceneSettings.enableWaterEffects = false;
    sceneSettings.enableParticles    = false;

    m_IsViewportEnabled = true;
    m_ViewportFocused = false;
    m_ViewportHovered = false;

    SetCamera();
    SetLightManager();
    SetupShaders();
    SetupRenderFramebuffer();

    m_EnvironmentMap = new EnvironmentMap("Textures/HDR/birchwood_4k.hdr", this);
    m_EnvironmentMap->SetSkyboxLOD(0.0f);

    m_SceneHierarchyPanel = new Hazel::SceneHierarchyPanel((Scene*)this);

    m_DisplayLineElements = false;
    m_Grid = new Grid(20);
    m_PivotScene = new Pivot(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(50.0f, 50.0f, 50.0f));

    Hazel::RendererAPI::Init();
}

SceneHazelEnvMap::~SceneHazelEnvMap()
{
    delete m_EnvironmentMap;
}

void SceneHazelEnvMap::SetupRenderFramebuffer()
{
    uint32_t width = Application::Get()->GetWindow()->GetWidth();
    uint32_t height = Application::Get()->GetWindow()->GetHeight();

    m_RenderFramebuffer = new Framebuffer(width, height);
    m_RenderFramebuffer->AddAttachmentSpecification(width, height, AttachmentType::Texture, AttachmentFormat::Color);
    m_RenderFramebuffer->AddAttachmentSpecification(width, height, AttachmentType::Renderbuffer, AttachmentFormat::Depth);
    m_RenderFramebuffer->Generate(width, height);
}

void SceneHazelEnvMap::ResizeViewport(glm::vec2 viewportPanelSize, Framebuffer* renderFramebuffer)
{
    // Cooldown
    if (m_CurrentTimestamp - m_ResizeViewport.lastTime < m_ResizeViewport.cooldown) return;
    m_ResizeViewport.lastTime = m_CurrentTimestamp;
    
    if (viewportPanelSize != m_ViewportMainSize && viewportPanelSize.x > 0 && viewportPanelSize.y > 0)
    {
        renderFramebuffer->Resize((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);
        m_ViewportMainSize = glm::vec2(viewportPanelSize.x, viewportPanelSize.y);
    
        m_CameraController->OnResize((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);
    }
}

void SceneHazelEnvMap::SetupShaders()
{
    m_ShaderBackground = new Shader("Shaders/LearnOpenGL/2.2.2.background.vs", "Shaders/LearnOpenGL/2.2.2.background.fs");
    Log::GetLogger()->info("SceneHazelEnvMap: m_ShaderBackground compiled [programID={0}]", m_ShaderBackground->GetProgramID());

    m_ShaderBasic = new Shader("Shaders/basic.vs", "Shaders/basic.fs");
    Log::GetLogger()->info("SceneHazelEnvMap: m_ShaderBasic compiled [programID={0}]", m_ShaderBasic->GetProgramID());

    ResourceManager::AddShader("LearnOpenGL/2.2.2.background", m_ShaderBackground);
    ResourceManager::AddShader("basic", m_ShaderBasic);
}

void SceneHazelEnvMap::Update(float timestep, Window* mainWindow)
{
    m_CurrentTimestamp = timestep;

    m_EnvironmentMap->GetShaderPBR_Anim()->Bind();
    m_EnvironmentMap->GetShaderPBR_Anim()->setMat4("u_ViewProjectionMatrix", RendererBasic::GetProjectionMatrix() * m_CameraController->CalculateViewMatrix());
    m_EnvironmentMap->GetShaderPBR_Anim()->setVec3("u_CameraPosition", m_Camera->GetPosition());
    m_EnvironmentMap->GetShaderPBR_Anim()->Unbind();

    float deltaTime = Timer::Get()->GetDeltaTime();

    m_EnvironmentMap->Update(this, deltaTime);
}

void SceneHazelEnvMap::UpdateImGui(float timestep, Window* mainWindow)
{
    bool p_open = true;
    ShowExampleAppDockSpace(&p_open, mainWindow);

    m_ImGuiViewportMainX = (int)ImGui::GetMainViewport()->GetWorkPos().x;
    m_ImGuiViewportMainY = (int)ImGui::GetMainViewport()->GetWorkPos().y;

    ImGui::Begin("Framebuffers");
    {
        ImVec2 imageSize(128.0f, 128.0f);

        if (m_IsViewportEnabled)
        {
            ImGui::Text("Viewport");
            ImGui::Image((void*)(intptr_t)m_RenderFramebuffer->GetTextureAttachmentColor()->GetID(), imageSize);

            ImGui::Text("Equirectangular");
            ImGui::Image((void*)(intptr_t)m_EnvironmentMap->GetSceneRenderer()->GetEnvEquirect()->GetID(), imageSize);
        }
    }
    ImGui::End();

    m_SceneHierarchyPanel->OnImGuiRender();

    Application::Get()->OnImGuiRender();

    /**** BEGIN Environment Map Settings ****/
    ImGui::Begin("Environment Map Settings");
    {
        if (ImGui::CollapsingHeader("Display Info", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
        {
            {
                if (ImGui::Button("Load Environment Map"))
                {
                    std::string filename = Application::Get()->OpenFile("*.hdr");
                    if (filename != "")
                        m_EnvironmentMap->GetSceneRenderer()->SetEnvironment(m_EnvironmentMap->GetSceneRenderer()->Load(filename));
                }

                float skyboxLOD = m_EnvironmentMap->GetSkyboxLOD();
                if (ImGui::SliderFloat("Skybox LOD", &skyboxLOD, 0.0f, 2.0f))
                {
                    m_EnvironmentMap->SetSkyboxLOD(skyboxLOD);
                }

                ImGui::Columns(2);
                ImGui::AlignTextToFramePadding();

                auto light = m_EnvironmentMap->GetSceneRenderer()->GetLight();
                ImGuiWrapper::Property("Light Direction", light.Direction);
                ImGuiWrapper::Property("Light Radiance", light.Radiance, PropertyFlag::ColorProperty);
                ImGuiWrapper::Property("Light Multiplier", light.Multiplier, 0.0f, 5.0f);
                ImGuiWrapper::Property("Exposure", m_Camera->GetExposure(), 0.0f, 40.0f);
                ImGuiWrapper::Property("Skybox Exposure Factor", m_EnvironmentMap->GetSkyboxExposureFactor(), 0.0f, 10.0f);
                ImGuiWrapper::Property("Radiance Prefiltering", m_EnvironmentMap->GetRadiancePrefilter());
                ImGuiWrapper::Property("Env Map Rotation", m_EnvironmentMap->GetEnvMapRotation(), -360.0f, 360.0f);
                m_EnvironmentMap->GetSceneRenderer()->SetLight(light);

                ImGui::Columns(1);
            }

            ImGui::Separator();

            {
                ImGui::Text("Mesh");

                Hazel::Entity* meshEntity = nullptr;
                std::string meshFullPath = "None";

                std::string fileName = Util::GetFileNameFromFullPath(meshFullPath);
                ImGui::Text(fileName.c_str()); ImGui::SameLine();
                if (ImGui::Button("...##Mesh"))
                {
                    std::string fullPath = Application::Get()->OpenFile("");
                    if (fullPath != "")
                    {
                        Hazel::Entity entity = m_EnvironmentMap->LoadEntity(fullPath);
                    }
                }
            }

            ImGui::Separator();
        }
    }
    ImGui::End();
    /**** END Environment Map Scene Settings ****/

    if (m_IsViewportEnabled)
    {
        // TheCherno ImGui Viewport displaying the framebuffer content
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });

        ImGui::Begin("Viewport");
        {
            ImVec2 screen_pos = ImGui::GetCursorScreenPos();

            m_ImGuiViewport.X = (int)(ImGui::GetWindowPos().x - m_ImGuiViewportMainX);
            m_ImGuiViewport.Y = (int)(ImGui::GetWindowPos().y - m_ImGuiViewportMainY);
            m_ImGuiViewport.Width = (int)ImGui::GetWindowWidth();
            m_ImGuiViewport.Height = (int)ImGui::GetWindowHeight();
            m_ImGuiViewport.MouseX = (int)ImGui::GetMousePos().x;
            m_ImGuiViewport.MouseY = (int)ImGui::GetMousePos().y;
        
            m_ViewportFocused = ImGui::IsWindowFocused();
            m_ViewportHovered = ImGui::IsWindowHovered();
        
            ImVec2 viewportPanelSizeImGui = ImGui::GetContentRegionAvail();
            glm::vec2 viewportPanelSize = glm::vec2(viewportPanelSizeImGui.x, viewportPanelSizeImGui.y);
        
            ResizeViewport(viewportPanelSize, m_RenderFramebuffer);
        
            uint64_t textureID = m_RenderFramebuffer->GetTextureAttachmentColor()->GetID();
            ImGui::Image((void*)(intptr_t)textureID, ImVec2{ m_ViewportMainSize.x, m_ViewportMainSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

            // Calculate Viewport bounds (used in EnvironmentMap::CastRay)
            auto viewportOffset = ImGui::GetCursorPos(); // includes tab bar

            auto windowSize = ImGui::GetWindowSize();
            ImVec2 minBound = ImGui::GetWindowPos();

            minBound.x += viewportOffset.x;
            // minBound.y += viewportOffset.y;

            ImVec2 maxBound = { minBound.x + windowSize.x, minBound.y + windowSize.y };
            m_ViewportBounds[0] = { minBound.x, minBound.y };
            m_ViewportBounds[1] = { maxBound.x, maxBound.y };

            m_EnvironmentMap->SetViewportBounds(m_ViewportBounds);
            m_EnvironmentMap->m_AllowViewportCameraEvents = ImGui::IsMouseHoveringRect(minBound, maxBound); // EditorLayer
        }
        ImGui::End();

        ImGui::Begin("Viewport Environment Map");
        {
            m_ImGuiViewportEnvMap.X = (int)(ImGui::GetWindowPos().x - m_ImGuiViewportEnvMapX);
            m_ImGuiViewportEnvMap.Y = (int)(ImGui::GetWindowPos().y - m_ImGuiViewportEnvMapY);
            m_ImGuiViewportEnvMap.Width = (int)ImGui::GetWindowWidth();
            m_ImGuiViewportEnvMap.Height = (int)ImGui::GetWindowHeight();
            m_ImGuiViewportEnvMap.MouseX = (int)ImGui::GetMousePos().x;
            m_ImGuiViewportEnvMap.MouseY = (int)ImGui::GetMousePos().y;

            m_ViewportEnvMapFocused = ImGui::IsWindowFocused();
            m_ViewportEnvMapHovered = ImGui::IsWindowHovered();

            ImVec2 viewportPanelSizeImGuiEnvMap = ImGui::GetContentRegionAvail();
            glm::vec2 viewportPanelSizeEnvMap = glm::vec2(viewportPanelSizeImGuiEnvMap.x, viewportPanelSizeImGuiEnvMap.y);

            uint64_t textureID = m_EnvironmentMap->GetSceneRenderer()->s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetID();
            ImGui::Image((void*)(intptr_t)textureID, ImVec2{ m_ViewportEnvMapSize.x, m_ViewportEnvMapSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
        }
        ImGui::End();

        ImGui::PopStyleVar();
    }
}

void SceneHazelEnvMap::ShowExampleAppDockSpace(bool* p_open, Window* mainWindow)
{
    if (!m_IsViewportEnabled) {
        Scene::ShowExampleAppDockSpace(p_open, mainWindow);
        return;
    }

    static bool opt_fullscreen_persistant = true;
    bool opt_fullscreen = opt_fullscreen_persistant;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", p_open, window_flags);
    ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }
    else
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("ERROR: Docking is not enabled! See Demo > Configuration.");
        ImGui::Text("Set io.ConfigFlags |= ImGuiConfigFlags_DockingEnable in your code, or ");
        ImGui::SameLine(0.0f, 0.0f);
        if (ImGui::SmallButton("click here"))
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    if (ImGui::BeginMenuBar())
    {
        // ...
        ImGui::EndMenuBar();
    }
    ImGui::End();
}

bool SceneHazelEnvMap::OnKeyPressed(KeyPressedEvent& e)
{
    return true;
}

void SceneHazelEnvMap::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
    std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
    m_RenderFramebuffer->Bind();
    m_RenderFramebuffer->Clear(); // Clear the window

    m_EnvironmentMap->Render(m_RenderFramebuffer);

    m_RenderFramebuffer->Unbind();
}
