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

    // directional light
    sceneSettings.directionalLight.base.enabled = true;
    sceneSettings.directionalLight.base.color = glm::vec3(1.0f, 1.0f, 1.0f);
    sceneSettings.directionalLight.direction = glm::vec3(0.6f, -0.5f, -0.6f);
    sceneSettings.directionalLight.base.ambientIntensity = 0.75f;
    sceneSettings.directionalLight.base.diffuseIntensity = 0.4f;
    sceneSettings.lightProjectionMatrix = glm::ortho(-40.0f, 40.0f, -40.0f, 40.0f, 0.1f, 40.0f);

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

    m_IsViewportEnabled = true;
    m_ViewportFocused = false;
    m_ViewportHovered = false;

    SetCamera();
    SetLightManager();
    SetupShaders();
    SetupTextureSlots();
    SetupTextures();
    SetupMaterials();
    SetupFramebuffers();
    SetupRenderFramebuffer();

    m_EnvironmentMap = new EnvironmentMap("Textures/HDR/birchwood_4k.hdr", this);
    m_EnvironmentMap->SetSkyboxLOD(0.0f);

    SetupMeshes();
    SetupModels();

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

void SceneHazelEnvMap::SetLightManager()
{
}

void SceneHazelEnvMap::SetWaterManager(int width, int height)
{
}

void SceneHazelEnvMap::SetupRenderFramebuffer()
{
    if (!m_IsViewportEnabled) return;

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

void SceneHazelEnvMap::SetupTextures()
{
}

void SceneHazelEnvMap::SetupTextureSlots()
{
}

void SceneHazelEnvMap::SetupMaterials()
{
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

void SceneHazelEnvMap::SetupMeshes()
{
    //  for (auto& drawCommand : m_EnvironmentMap->GetSceneRenderer()->s_Data.DrawList)
    //  {
    //      m_Entities.insert(std::make_pair(drawCommand.Name, Entity()));
    //  
    //      m_Entities[drawCommand.Name].Enabled = true;
    //      m_Entities[drawCommand.Name].Transform.Translation = glm::vec3(0.0f, 0.0f, 0.0f);
    //      m_Entities[drawCommand.Name].Transform.Rotation = glm::quat(glm::vec3(0.0f));
    //      m_Entities[drawCommand.Name].Init.Transform.Scale = glm::vec3(1.0f);
    //      m_Entities[drawCommand.Name].Init.AABB.Transform.Scale = glm::vec3(1.0f);
    //      m_Entities[drawCommand.Name].OriginOffset = glm::vec3(0.0f);
    //  
    //      Log::GetLogger()->info("-- BEGIN SceneHazelEnvMap setup M1911");
    //      {
    //          m_Entities[drawCommand.Name].Transform.Scale = m_Entities[drawCommand.Name].Init.Transform.Scale;
    //          m_Entities[drawCommand.Name].Transform.Transform = glm::mat4(1.0f);
    //          m_Entities[drawCommand.Name].Transform.Transform = glm::translate(m_Entities[drawCommand.Name].Transform.Transform, m_Entities[drawCommand.Name].Transform.Translation);
    //          m_Entities[drawCommand.Name].Transform.Transform = glm::scale(m_Entities[drawCommand.Name].Transform.Transform, m_Entities[drawCommand.Name].Transform.Scale);
    //  
    //          m_Entities[drawCommand.Name].AABB = AABB(m_Entities[drawCommand.Name].Transform.Translation + m_Entities[drawCommand.Name].OriginOffset,
    //              m_Entities[drawCommand.Name].Transform.Rotation, m_Entities[drawCommand.Name].Init.AABB.Transform.Scale);
    //      }
    //      Log::GetLogger()->info("-- END SceneHazelEnvMap setup M1911");
    //  }
}

void SceneHazelEnvMap::SetupModels()
{
}

void SceneHazelEnvMap::SetupFramebuffers()
{
}

void SceneHazelEnvMap::Update(float timestep, Window* mainWindow)
{
    m_CurrentTimestamp = timestep;

    for (auto& entity : m_Entities)
    {
        auto [translation, rotation, scale] = Math::GetTransformDecomposition(entity.second.Transform.Transform);
        entity.second.Transform.Translation = translation;
        entity.second.Transform.Rotation = rotation;
        entity.second.Transform.Scale = scale;

        entity.second.AABB.Update(entity.second.Transform.Translation, entity.second.Transform.Rotation, entity.second.Transform.Scale);
    }

    m_EnvironmentMap->GetShaderPBR_Anim()->Bind();
    m_EnvironmentMap->GetShaderPBR_Anim()->setMat4("u_ViewProjectionMatrix", RendererBasic::GetProjectionMatrix() * m_CameraController->CalculateViewMatrix());
    m_EnvironmentMap->GetShaderPBR_Anim()->setVec3("u_CameraPosition", m_Camera->GetPosition());
    // m_EnvironmentMap->GetShaderPBR_Anim()->setFloat("u_TilingFactor", 1.0f);

    float deltaTime = Timer::Get()->GetDeltaTime();

    m_EnvironmentMap->Update(this, deltaTime);

    //  if (m_HDRI_Edit != m_HDRI_Edit_Prev)
    //  {
    //      if (m_HDRI_Edit == HDRI_GREENWICH_PARK) {
    //          // m_MaterialWorkflowPBR->Init("Textures/HDR/greenwich_park_02_1k.hdr");
    //      }
    //      else if (m_HDRI_Edit == HDRI_SAN_GIUSEPPE_BRIDGE) {
    //          // m_MaterialWorkflowPBR->Init("Textures/HDR/san_giuseppe_bridge_1k.hdr");
    //      }
    //      else if (m_HDRI_Edit == HDRI_TROPICAL_BEACH) {
    //          //  m_MaterialWorkflowPBR->Init("Textures/HDR/Tropical_Beach_3k.hdr");
    //      }
    //      else if (m_HDRI_Edit == HDRI_VIGNAIOLI_NIGHT) {
    //          // m_MaterialWorkflowPBR->Init("Textures/HDR/vignaioli_night_1k.hdr");
    //      }
    //      else if (m_HDRI_Edit == HDRI_EARLY_EVE_WARM_SKY) {
    //          // m_MaterialWorkflowPBR->Init("Textures/HDR/006_hdrmaps_com_free.hdr");
    //      }
    //      else if (m_HDRI_Edit == HDRI_BIRCHWOOD) {
    //          // m_MaterialWorkflowPBR->Init("Textures/HDR/birchwood_4k.hdr");
    //      }
    //      else if (m_HDRI_Edit == HDRI_PINK_SUNRISE) {
    //          // m_MaterialWorkflowPBR->Init("Textures/HDR/pink_sunrise_4k.hdr");
    //      }
    //      else if (m_HDRI_Edit == HDRI_ROOITOU_PARK) {
    //          // m_MaterialWorkflowPBR->Init("Textures/HDR/rooitou_park_4k.hdr");
    //      }
    //      else if (m_HDRI_Edit == HDRI_VENICE_DAWN) {
    //          // m_MaterialWorkflowPBR->Init("Textures/HDR/venice_dawn_1_4k.hdr");
    //      }
    //      else if (m_HDRI_Edit == HDRI_PEPPERMINT_POWERPLANT) {
    //          // m_MaterialWorkflowPBR->Init("Textures/HDR/peppermint_powerplant_1k.hdr");
    //      }
    //  
    //      m_HDRI_Edit_Prev = m_HDRI_Edit;
    //  }
}

void SceneHazelEnvMap::UpdateImGui(float timestep, Window* mainWindow)
{
    bool p_open = true;
    ShowExampleAppDockSpace(&p_open, mainWindow);

    m_ImGuiViewportMainX = (int)ImGui::GetMainViewport()->GetWorkPos().x;
    m_ImGuiViewportMainY = (int)ImGui::GetMainViewport()->GetWorkPos().y;

    // MousePicker* mp = MousePicker::Get();

    // ImGui Colors
    //  ImVec4* colors = ImGui::GetStyle().Colors;
    //  colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    //  colors[ImGuiCol_TextDisabled] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    //  colors[ImGuiCol_WindowBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f); // Window background
    //  colors[ImGuiCol_ChildBg] = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
    //  colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    //  colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.5f);
    //  colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    //  colors[ImGuiCol_FrameBg] = ImVec4(0.3f, 0.3f, 0.3f, 0.5f); // Widget backgrounds
    //  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.4f, 0.4f, 0.4f, 0.4f);
    //  colors[ImGuiCol_FrameBgActive] = ImVec4(0.4f, 0.4f, 0.4f, 0.6f);
    //  colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.0f);
    //  colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.29f, 0.29f, 1.0f);
    //  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.51f);
    //  colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.0f);
    //  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    //  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
    //  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.0f);
    //  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);
    //  colors[ImGuiCol_CheckMark] = ImVec4(0.94f, 0.94f, 0.94f, 1.0f);
    //  colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 0.7f);
    //  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.66f, 0.66f, 0.66f, 1.0f);
    //  colors[ImGuiCol_Button] = ImVec4(0.44f, 0.44f, 0.44f, 0.4f);
    //  colors[ImGuiCol_ButtonHovered] = ImVec4(0.46f, 0.47f, 0.48f, 1.0f);
    //  colors[ImGuiCol_ButtonActive] = ImVec4(0.42f, 0.42f, 0.42f, 1.0f);
    //  colors[ImGuiCol_Header] = ImVec4(0.7f, 0.7f, 0.7f, 0.31f);
    //  colors[ImGuiCol_HeaderHovered] = ImVec4(0.7f, 0.7f, 0.7f, 0.8f);
    //  colors[ImGuiCol_HeaderActive] = ImVec4(0.48f, 0.5f, 0.52f, 1.0f);
    //  colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.5f, 0.5f);
    //  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.72f, 0.72f, 0.72f, 0.78f);
    //  colors[ImGuiCol_SeparatorActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);
    //  colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
    //  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
    //  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);
    //  colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.0f);
    //  colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.43f, 0.35f, 1.0f);
    //  colors[ImGuiCol_PlotHistogram] = ImVec4(0.73f, 0.6f, 0.15f, 1.0f);
    //  colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6f, 0.0f, 1.0f);
    //  colors[ImGuiCol_TextSelectedBg] = ImVec4(0.87f, 0.87f, 0.87f, 0.35f);
    //  colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.8f, 0.8f, 0.8f, 0.35f);
    //  colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.9f);
    //  colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.6f, 0.6f, 1.0f);
    //  colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);

    //  ImGui::Begin("Camera");
    //  {
    //      if (ImGui::CollapsingHeader("Display Info"))
    //      {
    //          char buffer[100];
    //          sprintf(buffer, "Pitch      %.2f", m_Camera->GetPitch());
    //          ImGui::Text(buffer);
    //          sprintf(buffer, "Yaw        %.2f", m_Camera->GetYaw());
    //          ImGui::Text(buffer);
    //          sprintf(buffer, "Position   X %.2f Y %.2f Z %.2f", m_Camera->GetPosition().x, m_Camera->GetPosition().y, m_Camera->GetPosition().z);
    //          ImGui::Text(buffer);
    //          sprintf(buffer, "Direction  X %.2f Y %.2f Z %.2f", m_Camera->GetDirection().x, m_Camera->GetDirection().y, m_Camera->GetDirection().z);
    //          ImGui::Text(buffer);
    //          sprintf(buffer, "Front      X %.2f Y %.2f Z %.2f", m_Camera->GetFront().x, m_Camera->GetFront().y, m_Camera->GetFront().z);
    //          ImGui::Text(buffer);
    //          sprintf(buffer, "Up         X %.2f Y %.2f Z %.2f", m_Camera->GetUp().x, m_Camera->GetUp().y, m_Camera->GetUp().z);
    //          ImGui::Text(buffer);
    //          sprintf(buffer, "Right      X %.2f Y %.2f Z %.2f", m_Camera->GetRight().x, m_Camera->GetRight().y, m_Camera->GetRight().z);
    //          ImGui::Text(buffer);
    //      }
    //  }
    //  ImGui::End();

    ImGui::Begin("Framebuffers");
    {
        ImVec2 imageSize(128.0f, 128.0f);

        if (m_IsViewportEnabled)
        {
            ImGui::Text("Viewport");
            ImGui::Image((void*)(intptr_t)m_RenderFramebuffer->GetTextureAttachmentColor()->GetID(), imageSize);

            ImGui::Text("Equirectangular");
            ImGui::Image((void*)(intptr_t)m_EnvironmentMap->GetSceneRenderer()->GetEnvEquirect()->GetID(), imageSize);

            //  ImGui::Text("Radiance Map");
            //  ImGui::Image((void*)(intptr_t)m_EnvironmentMap->GetSceneRenderer()->s_Data.SceneData.SceneEnvironment.RadianceMap->GetID(), imageSize);

            //  ImGui::Text("Irradiance Map");
            //  ImGui::Image((void*)(intptr_t)m_EnvironmentMap->GetSceneRenderer()->s_Data.SceneData.SceneEnvironment.IrradianceMap->GetID(), imageSize);

            //  ImGui::Text("Geo Pass");
            //  ImGui::Image((void*)(intptr_t)m_EnvironmentMap->GetSceneRenderer()->GetFinalColorBufferID(), imageSize);
            //  
            //  ImGui::Text("Composite Pass");
            //  ImGui::Image((void*)(intptr_t)m_EnvironmentMap->GetSceneRenderer()->s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetID(), imageSize);

            //  Log::GetLogger()->debug("Geo Pass Framebuffer Color Attachment ID {0}",
            //      m_EnvironmentMap->GetSceneRenderer()->s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetID());
            //  
            //  Log::GetLogger()->debug("Compo Pass Framebuffer Color Attachment ID {0}",
            //      m_EnvironmentMap->GetSceneRenderer()->s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetID());
        }
    }
    ImGui::End();

    if (m_IsViewportEnabled)
    {
        ImGui::Begin("Viewport Info");
        {
            glm::ivec2 colorAttachmentSize = glm::ivec2(
                m_RenderFramebuffer->GetTextureAttachmentColor()->GetWidth(),
                m_RenderFramebuffer->GetTextureAttachmentColor()->GetHeight());
            glm::ivec2 depthAttachmentSize = glm::ivec2(
                m_RenderFramebuffer->GetAttachmentDepth()->GetWidth(),
                m_RenderFramebuffer->GetAttachmentDepth()->GetHeight());

            ImGui::SliderInt2("Color Attachment Size", glm::value_ptr(colorAttachmentSize), 0, 2048);
            ImGui::SliderInt2("Depth Attachment Size", glm::value_ptr(depthAttachmentSize), 0, 2048);
        }
        ImGui::End();

        ImGui::Begin("Viewport Environment Map Info");
        {
            glm::ivec2 colorAttachmentSize = glm::ivec2(
                m_EnvironmentMap->GetSceneRenderer()->s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetWidth(),
                m_EnvironmentMap->GetSceneRenderer()->s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetHeight());
            glm::ivec2 depthAttachmentSize = glm::ivec2(
                m_EnvironmentMap->GetSceneRenderer()->s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetAttachmentDepth()->GetWidth(),
                m_EnvironmentMap->GetSceneRenderer()->s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetAttachmentDepth()->GetHeight());

            ImGui::SliderInt2("Color Attachment Size", glm::value_ptr(colorAttachmentSize), 0, 2048);
            ImGui::SliderInt2("Depth Attachment Size", glm::value_ptr(depthAttachmentSize), 0, 2048);
        }
        ImGui::End();
    }

    //  ImGui::Begin("Select HDRI");
    //  {
    //      if (ImGui::CollapsingHeader("Display Info"))
    //      {
    //          ImGui::RadioButton("Greenwich Park", &m_HDRI_Edit, HDRI_GREENWICH_PARK);
    //          ImGui::RadioButton("San Giuseppe Bridge", &m_HDRI_Edit, HDRI_SAN_GIUSEPPE_BRIDGE);
    //          ImGui::RadioButton("Tropical Beach", &m_HDRI_Edit, HDRI_TROPICAL_BEACH);
    //          ImGui::RadioButton("Vignaioli Night", &m_HDRI_Edit, HDRI_VIGNAIOLI_NIGHT);
    //          ImGui::RadioButton("Early Eve & Warm Sky", &m_HDRI_Edit, HDRI_EARLY_EVE_WARM_SKY);
    //          ImGui::RadioButton("Birchwood", &m_HDRI_Edit, HDRI_BIRCHWOOD);
    //          ImGui::RadioButton("Pink Sunrise", &m_HDRI_Edit, HDRI_PINK_SUNRISE);
    //          ImGui::RadioButton("Rooitou Park", &m_HDRI_Edit, HDRI_ROOITOU_PARK);
    //          ImGui::RadioButton("Venice Dawn", &m_HDRI_Edit, HDRI_VENICE_DAWN);
    //          ImGui::RadioButton("Peppermint Powerplant", &m_HDRI_Edit, HDRI_PEPPERMINT_POWERPLANT);
    //      }
    //  }
    //  ImGui::End();

    //  ImGui::Begin("Settings");
    //  {
    //      if (ImGui::CollapsingHeader("Display Info"))
    //      {
    //          ImGui::Checkbox("Display Bounding Boxes", m_EnvironmentMap->GetDisplayBoundingBoxes());
    //          ImGui::Checkbox("Display Hazel Grid", m_EnvironmentMap->GetDisplayHazelGrid());
    //          ImGui::Checkbox("Display Line Elements", &m_DisplayLineElements);
    //  
    //          ImGui::Separator();
    //          for (auto& entity : m_Entities)
    //          {
    //              std::string isEnabled = std::string("Is Enabled ") + entity.first;
    //              ImGui::Checkbox(isEnabled.c_str(), &entity.second.Enabled);
    //          }
    //          ImGui::Separator();
    //          for (auto& entity : m_Entities)
    //          {
    //              std::string isIntersecting = std::string("Is Intersecting ") + entity.first;
    //              ImGui::Checkbox(isIntersecting.c_str(), &entity.second.Intersecting);
    //          }
    //          ImGui::Separator();
    //          bool eventLoggingEnabled = Application::Get()->GetWindow()->GetEventLogging();
    //          if (ImGui::Checkbox("Enable Event Logging", &eventLoggingEnabled)) {
    //              Application::Get()->GetWindow()->SetEventLogging(eventLoggingEnabled);
    //          }
    //      }
    //  }
    //  ImGui::End();

    //  ImGui::Begin("Mouse Picker");
    //  {
    //      if (ImGui::CollapsingHeader("Display Info"))
    //      {
    //          char buffer[100];
    //  
    //          sprintf(buffer, "Main Window [ X %i Y %i ]", m_ImGuiViewportMainX, m_ImGuiViewportMainY);
    //          ImGui::Text(buffer);
    //          ImGui::Separator();
    //  
    //          sprintf(buffer, "Viewport [ X %i Y %i W %i H %i ]", mp->m_Viewport.X, mp->m_Viewport.Y, mp->m_Viewport.Width, mp->m_Viewport.Height);
    //          ImGui::Text(buffer);
    //          ImGui::Separator();
    //  
    //          sprintf(buffer, "Screen Mouse [ %i %i ]", mp->m_ScreenMouseX, mp->m_ScreenMouseY);
    //          ImGui::Text(buffer);
    //          ImGui::Separator();
    //  
    //          sprintf(buffer, "Viewport Mouse [ %i %i ]", mp->m_Viewport.MouseX, mp->m_Viewport.MouseY);
    //          ImGui::Text(buffer);
    //          ImGui::Separator();
    //  
    //          sprintf(buffer, "Normalized Coords [ %.2ff %.2ff ]", mp->m_NormalizedCoords.x, mp->m_NormalizedCoords.y);
    //          ImGui::Text(buffer);
    //          ImGui::Separator();
    //  
    //          sprintf(buffer, "Clip Coords [ %.2ff %.2ff ]", mp->m_ClipCoords.x, mp->m_ClipCoords.y);
    //          ImGui::Text(buffer);
    //          ImGui::Separator();
    //  
    //          sprintf(buffer, "Eye Coords [ %.2ff %.2ff %.2ff %.2ff ]", mp->m_EyeCoords.x, mp->m_EyeCoords.y, mp->m_EyeCoords.z, mp->m_EyeCoords.w);
    //          ImGui::Text(buffer);
    //          ImGui::Separator();
    //  
    //          sprintf(buffer, "World Ray [ %.2ff %.2ff %.2ff ]", mp->m_WorldRay.x, mp->m_WorldRay.y, mp->m_WorldRay.z);
    //          ImGui::Text(buffer);
    //      }
    //  }
    //  ImGui::End();

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

                //  meshEntity = m_EnvironmentMap->GetMeshEntity();
                //  if (meshEntity->HasComponent<Hazel::MeshComponent>()) {
                //      auto meshComponent = meshEntity->GetComponent<Hazel::MeshComponent>();
                //      meshFullPath = meshComponent.Mesh->GetFilePath();
                //  }

                std::string fileName = Util::GetFileNameFromFullPath(meshFullPath);
                ImGui::Text(fileName.c_str()); ImGui::SameLine();
                if (ImGui::Button("...##Mesh"))
                {
                    std::string fullPath = Application::Get()->OpenFile("");
                    if (fullPath != "")
                    {
                        Hazel::Entity entity = m_EnvironmentMap->LoadEntity(fullPath);
                        // meshComponent.Mesh = Ref<Hazel::HazelMesh>((Hazel::HazelMesh*)entity.GetMesh());
                    }
                }

                //  auto meshEntities = GetAllEntitiesWith<Hazel::MeshComponent>();
                //  if (meshEntities.size())
                //  {
                //      meshEntity = m_EnvironmentMap->GetMeshEntity();
                //      Ref<Hazel::HazelMesh> meshAnimPBR = meshEntity->GetComponent<Hazel::MeshComponent>().Mesh;
                //      ImGui::Checkbox("Is Animated", &meshAnimPBR->IsAnimated());
                //  }
            }

            ImGui::Separator();
        }
    }
    ImGui::End();
    /**** END Environment Map Scene Settings ****/

    if (!m_IsViewportEnabled)
    {
        ImGui::Begin("ImGuizmo");
        {
            m_EnvironmentMap->UpdateImGuizmo(mainWindow);
        }
        ImGui::End();
    }

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
        
            m_EnvironmentMap->UpdateImGuizmo(mainWindow);

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

            // Currently resize can only work with a single (main) viewport
            // ResizeViewport(viewportPanelSizeEnvMap, m_EnvironmentMap->GetSceneRenderer()->s_Data.CompositePass->GetSpecification().TargetFramebuffer); 
            uint64_t textureID = m_EnvironmentMap->GetSceneRenderer()->s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetID();
            ImGui::Image((void*)(intptr_t)textureID, ImVec2{ m_ViewportEnvMapSize.x, m_ViewportEnvMapSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
        }
        ImGui::End();

        ImGui::PopStyleVar();
    }

    // ImGui::ShowMetricsWindow();

    // Mesh Hierarchy / Mesh Debug
    m_EnvironmentMap->OnImGuiRender();
}

// Demonstrate using DockSpace() to create an explicit docking node within an existing window.
// Note that you already dock windows into each others _without_ a DockSpace() by just moving windows 
// from their title bar (or by holding SHIFT if io.ConfigDockingWithShift is set).
// DockSpace() is only useful to construct to a central location for your application.
void SceneHazelEnvMap::ShowExampleAppDockSpace(bool* p_open, Window* mainWindow)
{
    if (!m_IsViewportEnabled) {
        Scene::ShowExampleAppDockSpace(p_open, mainWindow);
        return;
    }

    static bool opt_fullscreen_persistant = true;
    bool opt_fullscreen = opt_fullscreen_persistant;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // ImGuiDockNodeFlags_None |
    // ImGuiDockNodeFlags_PassthruCentralNode |
    // ImGuiDockNodeFlags_NoDockingInCentralNode;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
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

    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
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
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New", "Ctrl+N")) {
                NewScene();
            }

            if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                OpenScene();
            }

            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                SaveSceneAs();
            }

            if (ImGui::MenuItem("Exit")) {
                mainWindow->SetShouldClose(true);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit"))
        {
            ImGui::MenuItem("Undo");
            ImGui::MenuItem("Redo");
            ImGui::MenuItem("Cut");
            ImGui::MenuItem("Copy");
            ImGui::MenuItem("Paste");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Docking"))
        {
            // Disabling fullscreen would allow the window to be moved to the front of other windows, 
            // which we can't undo at the moment without finer window depth/z control.
            //ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);

            if (ImGui::MenuItem("Flag: NoSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0))
                dockspace_flags ^= ImGuiDockNodeFlags_NoSplit;
            if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))
                dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
            if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))
                dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
            if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0))
                dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode;
            if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))
                dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
            ImGui::EndMenu();
        }

        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("When docking is enabled, you can ALWAYS dock MOST window into another! Try it now!" "\n\n"
                " > if io.ConfigDockingWithShift==false (default):" "\n"
                "   drag windows from title bar to dock" "\n"
                " > if io.ConfigDockingWithShift==true:" "\n"
                "   drag windows from anywhere and hold Shift to dock" "\n\n"
                "This demo app has nothing to do with it!" "\n\n"
                "This demo app only demonstrate the use of ImGui::DockSpace() which allows you to manually create a docking node _within_ another window. This is useful so you can decorate your main application window (e.g. with a menu bar)." "\n\n"
                "ImGui::DockSpace() comes with one hard constraint: it needs to be submitted _before_ any window which may be docked into it. Therefore, if you use a dock spot as the central point of your application, you'll probably want it to be part of the very first window you are submitting to imgui every frame." "\n\n"
                "(NB: because of this constraint, the implicit \"Debug\" window can not be docked into an explicit DockSpace() node, because that window is submitted as part of the NewFrame() call. An easy workaround is that you can create your own implicit \"Debug##2\" window after calling DockSpace() and leave it in the window stack for anyone to use.)");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        ImGui::EndMenuBar();
    }
    ImGui::End();
}

void SceneHazelEnvMap::RenderLineElements(Shader* shaderBasic, glm::mat4 projectionMatrix)
{
    //  if (!m_DisplayLineElements) return;
    //  
    //  m_ShaderBasic->Bind();
    //  m_ShaderBasic->setMat4("projection", projectionMatrix);
    //  m_ShaderBasic->setMat4("view", m_CameraController->CalculateViewMatrix());
    //  
    //  // Draw AABBs
    //  glm::mat4 AABB_Transform = Math::CreateTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));
    //  
    //  for (auto& entity : m_Entities)
    //  {
    //      if (entity.second.Enabled)
    //      {
    //          m_ShaderBasic->setMat4("model", AABB_Transform);
    //          m_ShaderBasic->setVec4("tintColor", glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
    //          if (m_EnvironmentMap->GetDisplayBoundingBoxes()) {
    //              entity.second.AABB.Draw();
    //          }
    //      }
    //  }
    //  
    //  m_Grid->Draw(shaderBasic, projectionMatrix, m_CameraController->CalculateViewMatrix());
    //  m_PivotScene->Draw(shaderBasic, projectionMatrix, m_CameraController->CalculateViewMatrix());
}

void SceneHazelEnvMap::SetupUniforms()
{
}

bool SceneHazelEnvMap::OnKeyPressed(KeyPressedEvent& e)
{
    //  // Shortcuts
    //  if (e.GetRepeatCount() > 0) {
    //      return false;
    //  }
    //  
    //  bool control = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
    //  bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);
    //  
    //  switch (e.GetKeyCode())
    //  {
    //      case (int)Key::N:
    //      {
    //          if (control)
    //          {
    //              NewScene();
    //          }
    //          break;
    //      }
    //      case (int)Key::O:
    //      {
    //          if (control)
    //          {
    //              OpenScene();
    //          }
    //          break;
    //      }
    //      case (int)Key::S:
    //      {
    //          if (control && shift)
    //          {
    //              SaveSceneAs();
    //          }
    //          break;
    //      }
    //  }
    return true;
}

void SceneHazelEnvMap::NewScene()
{
    //  m_EnvironmentMap->GetSceneRenderer()->s_Data.ActiveScene = new Hazel::HazelScene();
    //  m_EnvironmentMap->GetSceneRenderer()->s_Data.ActiveScene->OnViewportResize((uint32_t)m_ViewportMainSize.x, (uint32_t)m_ViewportMainSize.y);
    //  m_SceneHierarchyPanel->SetContext(m_EnvironmentMap->GetSceneRenderer()->s_Data.ActiveScene);
}

void SceneHazelEnvMap::OpenScene()
{
    //  std::string filepath = Hazel::FileDialogs::OpenFile("Hazel Scene (*.hazel)\0*.hazel\0");
    //  if (!filepath.empty())
    //  {
    //      m_EnvironmentMap->GetSceneRenderer()->s_Data.ActiveScene = new Hazel::HazelScene();
    //      m_EnvironmentMap->GetSceneRenderer()->s_Data.ActiveScene->OnViewportResize((uint32_t)m_ViewportMainSize.x, (uint32_t)m_ViewportMainSize.y);
    //      m_SceneHierarchyPanel->SetContext(m_EnvironmentMap->GetSceneRenderer()->s_Data.ActiveScene);
    //  
    //      Hazel::SceneSerializer serializer(m_EnvironmentMap->GetSceneRenderer()->s_Data.ActiveScene);
    //      serializer.Deserialize(filepath);
    //  }
}

void SceneHazelEnvMap::SaveSceneAs()
{
    //  std::string filepath = Hazel::FileDialogs::SaveFile("Hazel Scene (*.hazel)\0*.hazel\0");
    //  if (!filepath.empty()) {
    //      Hazel::SceneSerializer serializer(m_EnvironmentMap->GetSceneRenderer()->s_Data.ActiveScene);
    //      serializer.Serialize(filepath);
    //  }
}

void SceneHazelEnvMap::OnEntitySelected(Hazel::Entity entity)
{
    // auto& tc = entity.GetComponent<Hazel::TransformComponent>();
    // m_EnvironmentMap->SetMeshEntity(entity);
}

void SceneHazelEnvMap::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
    std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
    /**** BEGIN Render to Main Viewport ****/
    {
        if (m_IsViewportEnabled)
        {
            m_RenderFramebuffer->Bind();
            m_RenderFramebuffer->Clear(); // Clear the window
        }
        else
        {
            // configure the viewport to the original framebuffer's screen dimensions
            glViewport(0, 0, (GLsizei)mainWindow->GetWidth(), (GLsizei)mainWindow->GetHeight());
            RendererBasic::SetDefaultFramebuffer((unsigned int)mainWindow->GetWidth(), (unsigned int)mainWindow->GetHeight());
        }

        // SetupUniforms();

        m_EnvironmentMap->Render(m_RenderFramebuffer);

        // RenderLineElements(m_ShaderBasic, projectionMatrix);

        if (m_IsViewportEnabled)
        {
            m_RenderFramebuffer->Unbind();
        }
    }
}
