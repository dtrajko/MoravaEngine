#define _CRT_SECURE_NO_WARNINGS

#include "SceneHazelEnvMap.h"
#include "ImGuiWrapper.h"
#include "../cross-platform/ImGuizmo/ImGuizmo.h"
#include "RendererBasic.h"
#include "ResourceManager.h"
#include "Block.h"
#include "Application.h"
#include "Shader.h"
#include "Timer.h"
#include "MousePicker.h"
#include "Math.h"
#include "Input.h"
#include "Hazel/Renderer/HazelTexture.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>


SceneHazelEnvMap::SceneHazelEnvMap()
{
    sceneSettings.cameraPosition     = glm::vec3(0.0f, 1.0f, 6.0f);
    sceneSettings.cameraStartYaw     = -90.0f;
    sceneSettings.cameraStartPitch   = 0.0f;
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

    m_EnvironmentMap = new EnvironmentMap("Textures/HDR/birchwood_4k.hdr");
    m_EnvironmentMap->SetSkyboxLOD(0.0f);

    SetupMeshes();
    SetupModels();

    m_LightPosition = glm::vec3(20.0f, 20.0f, 20.0f);
    m_LightColor = glm::vec3(1.0f, 1.0f, 1.0f);

    m_Translation_ImGuizmo = glm::vec3(0.0f);
    m_Transform_ImGuizmo = nullptr;

    m_VisibleAABBs = true;

    m_SceneHierarchyPanel = new Hazel::SceneHierarchyPanel((Scene*)this);

    m_DisplayLineElements = true;
    m_Grid = new Grid(20);
    m_PivotScene = new Pivot(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(50.0f, 50.0f, 50.0f));
}

SceneHazelEnvMap::~SceneHazelEnvMap()
{
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

void SceneHazelEnvMap::ResizeViewport(glm::vec2 viewportPanelSize)
{
    // Cooldown
    if (m_CurrentTimestamp - m_ResizeViewport.lastTime < m_ResizeViewport.cooldown) return;
    m_ResizeViewport.lastTime = m_CurrentTimestamp;

    if (viewportPanelSize != m_ViewportMainSize && viewportPanelSize.x > 0 && viewportPanelSize.y > 0)
    {
        m_RenderFramebuffer->Resize((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);
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
    m_ShaderMain = new Shader("Shaders/shader.vert", "Shaders/shader.frag");
    Log::GetLogger()->info("SceneHazelEnvMap: m_ShaderMain compiled [programID={0}]", m_ShaderMain->GetProgramID());

    m_ShaderBackground = new Shader("Shaders/LearnOpenGL/2.2.2.background.vs", "Shaders/LearnOpenGL/2.2.2.background.fs");
    Log::GetLogger()->info("SceneHazelEnvMap: m_ShaderBackground compiled [programID={0}]", m_ShaderBackground->GetProgramID());

    m_ShaderBasic = new Shader("Shaders/basic.vs", "Shaders/basic.fs");
    Log::GetLogger()->info("SceneHazelEnvMap: m_ShaderBasic compiled [programID={0}]", m_ShaderBasic->GetProgramID());
}

void SceneHazelEnvMap::SetupMeshes()
{
    for (auto& drawCommand : m_EnvironmentMap->GetContextData()->DrawList)
    {
        m_Entities.insert(std::make_pair(drawCommand.Name, Entity()));

        m_Entities[drawCommand.Name].Enabled = true;
        m_Entities[drawCommand.Name].Transform.Translation = glm::vec3(0.0f);
        m_Entities[drawCommand.Name].Transform.Rotation = glm::quat(glm::vec3(0.0f));
        m_Entities[drawCommand.Name].Init.Transform.Scale = glm::vec3(1.0f);
        m_Entities[drawCommand.Name].Init.AABB.Transform.Scale = glm::vec3(1.0f);
        m_Entities[drawCommand.Name].OriginOffset = glm::vec3(0.0f);

        Log::GetLogger()->info("-- BEGIN SceneHazelEnvMap setup M1911");
        {
            m_Entities[drawCommand.Name].Transform.Scale = m_Entities[drawCommand.Name].Init.Transform.Scale;
            m_Entities[drawCommand.Name].Transform.Transform = glm::mat4(1.0f);
            m_Entities[drawCommand.Name].Transform.Transform = glm::translate(m_Entities[drawCommand.Name].Transform.Transform, m_Entities[drawCommand.Name].Transform.Translation);
            m_Entities[drawCommand.Name].Transform.Transform = glm::scale(m_Entities[drawCommand.Name].Transform.Transform, m_Entities[drawCommand.Name].Transform.Scale);

            m_Entities[drawCommand.Name].AABB = AABB(m_Entities[drawCommand.Name].Transform.Translation + m_Entities[drawCommand.Name].OriginOffset,
                m_Entities[drawCommand.Name].Transform.Rotation, m_Entities[drawCommand.Name].Init.AABB.Transform.Scale);
        }
        Log::GetLogger()->info("-- END SceneHazelEnvMap setup M1911");
    }
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

    CheckIntersection(mainWindow);

    m_EnvironmentMap->GetPBRShader()->Bind();
    m_EnvironmentMap->GetPBRShader()->setMat4("u_ViewProjectionMatrix", RendererBasic::GetProjectionMatrix() * m_CameraController->CalculateViewMatrix());
    m_EnvironmentMap->GetPBRShader()->setVec3("u_CameraPosition", m_Camera->GetPosition());
    // m_EnvironmentMap->GetPBRShader()->setFloat("u_TilingFactor", 1.0f);

    float deltaTime = Timer::Get()->GetDeltaTime();

    m_EnvironmentMap->Update(this, deltaTime);

    if (m_HDRI_Edit != m_HDRI_Edit_Prev)
    {
        if (m_HDRI_Edit == HDRI_GREENWICH_PARK) {
            // m_MaterialWorkflowPBR->Init("Textures/HDR/greenwich_park_02_1k.hdr");
        }
        else if (m_HDRI_Edit == HDRI_SAN_GIUSEPPE_BRIDGE) {
            // m_MaterialWorkflowPBR->Init("Textures/HDR/san_giuseppe_bridge_1k.hdr");
        }
        else if (m_HDRI_Edit == HDRI_TROPICAL_BEACH) {
            //  m_MaterialWorkflowPBR->Init("Textures/HDR/Tropical_Beach_3k.hdr");
        }
        else if (m_HDRI_Edit == HDRI_VIGNAIOLI_NIGHT) {
            // m_MaterialWorkflowPBR->Init("Textures/HDR/vignaioli_night_1k.hdr");
        }
        else if (m_HDRI_Edit == HDRI_EARLY_EVE_WARM_SKY) {
            // m_MaterialWorkflowPBR->Init("Textures/HDR/006_hdrmaps_com_free.hdr");
        }
        else if (m_HDRI_Edit == HDRI_BIRCHWOOD) {
            // m_MaterialWorkflowPBR->Init("Textures/HDR/birchwood_4k.hdr");
        }
        else if (m_HDRI_Edit == HDRI_PINK_SUNRISE) {
            // m_MaterialWorkflowPBR->Init("Textures/HDR/pink_sunrise_4k.hdr");
        }
        else if (m_HDRI_Edit == HDRI_ROOITOU_PARK) {
            // m_MaterialWorkflowPBR->Init("Textures/HDR/rooitou_park_4k.hdr");
        }
        else if (m_HDRI_Edit == HDRI_VENICE_DAWN) {
            // m_MaterialWorkflowPBR->Init("Textures/HDR/venice_dawn_1_4k.hdr");
        }

        m_HDRI_Edit_Prev = m_HDRI_Edit;
    }
}

void SceneHazelEnvMap::CheckIntersection(Window* mainWindow)
{
    MousePicker::Get()->Update(
        (int)mainWindow->GetMouseX(), (int)mainWindow->GetMouseY(),
        m_ImGuiViewport.X, m_ImGuiViewport.Y, m_ImGuiViewport.Width, m_ImGuiViewport.Height,
        RendererBasic::GetProjectionMatrix(), m_CameraController->CalculateViewMatrix());

    MousePicker::Get()->GetPointOnRay(m_Camera->GetPosition(), MousePicker::Get()->GetCurrentRay(), MousePicker::Get()->m_RayRange);

    for (auto& entity : m_Entities)
    {
        entity.second.Intersecting = entity.second.Enabled &&
            AABB::IntersectRayAab(m_Camera->GetPosition(), MousePicker::Get()->GetCurrentRay(),
                entity.second.AABB.GetMin(), entity.second.AABB.GetMax(), glm::vec2(0.0f));
    }

    if (mainWindow->IsMouseButtonClicked((int)Mouse::ButtonLeft))
    {
        for (auto& entity : m_Entities)
        {
            if (entity.second.Intersecting) {
                m_Translation_ImGuizmo = entity.second.Transform.Translation;
                m_Transform_ImGuizmo = &entity.second.Transform.Transform;
                if (m_ImGuizmoType == -1) {
                    m_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE;
                }
            }
        }
    }
}

void SceneHazelEnvMap::UpdateImGui(float timestep, Window* mainWindow)
{
    bool p_open = true;
    ShowExampleAppDockSpace(&p_open, mainWindow);

    m_ImGuiViewportMainX = (int)ImGui::GetMainViewport()->GetWorkPos().x;
    m_ImGuiViewportMainY = (int)ImGui::GetMainViewport()->GetWorkPos().y;

    MousePicker* mp = MousePicker::Get();

    // ImGui Colors
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f); // Window background
    colors[ImGuiCol_ChildBg] = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.5f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.3f, 0.3f, 0.3f, 0.5f); // Widget backgrounds
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.4f, 0.4f, 0.4f, 0.4f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.4f, 0.4f, 0.4f, 0.6f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.29f, 0.29f, 0.29f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.0f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.94f, 0.94f, 0.94f, 1.0f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 0.7f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.66f, 0.66f, 0.66f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.44f, 0.44f, 0.44f, 0.4f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.46f, 0.47f, 0.48f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.42f, 0.42f, 0.42f, 1.0f);
    colors[ImGuiCol_Header] = ImVec4(0.7f, 0.7f, 0.7f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.7f, 0.7f, 0.7f, 0.8f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.48f, 0.5f, 0.52f, 1.0f);
    colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.5f, 0.5f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.72f, 0.72f, 0.72f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.0f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.43f, 0.35f, 1.0f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.73f, 0.6f, 0.15f, 1.0f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6f, 0.0f, 1.0f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.87f, 0.87f, 0.87f, 0.35f);
    colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.8f, 0.8f, 0.8f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.9f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.6f, 0.6f, 1.0f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);

    ImGui::Begin("Transform");
    {
        if (m_Transform_ImGuizmo != nullptr)
        {
            auto [Location, Rotation, Scale] = Math::GetTransformDecomposition(*m_Transform_ImGuizmo);
            glm::vec3 RotationF3 = glm::degrees(glm::eulerAngles(Rotation));

            char buffer[100];

            sprintf(buffer, "Location  X %.2f Y %.2f Z %.2f", Location.x, Location.y, Location.z);
            ImGui::Text(buffer);

            sprintf(buffer, "Rotation  X %.2f Y %.2f Z %.2f", RotationF3.x, RotationF3.y, RotationF3.z);
            ImGui::Text(buffer);

            sprintf(buffer, "Scale     X %.2f Y %.2f Z %.2f", Scale.x, Scale.y, Scale.z);
            ImGui::Text(buffer);
        }
    }
    ImGui::End();

    ImGui::Begin("Camera");
    {
        char buffer[100];

        sprintf(buffer, "Pitch      %.2f", m_Camera->GetPitch());
        ImGui::Text(buffer);

        sprintf(buffer, "Yaw        %.2f", m_Camera->GetYaw());
        ImGui::Text(buffer);

        sprintf(buffer, "Position   X %.2f Y %.2f Z %.2f", m_Camera->GetPosition().x, m_Camera->GetPosition().y, m_Camera->GetPosition().z);
        ImGui::Text(buffer);

        sprintf(buffer, "Direction  X %.2f Y %.2f Z %.2f", m_Camera->GetDirection().x, m_Camera->GetDirection().y, m_Camera->GetDirection().z);
        ImGui::Text(buffer);

        sprintf(buffer, "Front      X %.2f Y %.2f Z %.2f", m_Camera->GetFront().x, m_Camera->GetFront().y, m_Camera->GetFront().z);
        ImGui::Text(buffer);

        sprintf(buffer, "Up         X %.2f Y %.2f Z %.2f", m_Camera->GetUp().x, m_Camera->GetUp().y, m_Camera->GetUp().z);
        ImGui::Text(buffer);

        sprintf(buffer, "Right      X %.2f Y %.2f Z %.2f", m_Camera->GetRight().x, m_Camera->GetRight().y, m_Camera->GetRight().z);
        ImGui::Text(buffer);

    }
    ImGui::End();

    ImGui::Begin("Light");
    {
        ImGui::SliderFloat3("Light Position", glm::value_ptr(m_LightPosition), -100.0f, 100.0f);
        ImGui::ColorEdit3("Light Color", glm::value_ptr(m_LightColor));
    }
    ImGui::End();

    ImGui::Begin("Framebuffers");
    {
        ImVec2 imageSize(128.0f, 128.0f);

        if (m_IsViewportEnabled)
        {
            ImGui::Text("Viewport");
            ImGui::Image((void*)(intptr_t)m_RenderFramebuffer->GetTextureAttachmentColor()->GetID(), imageSize);

            ImGui::Text("Geo Pass");
            ImGui::Image((void*)(intptr_t)m_EnvironmentMap->GetContextData()->GeoPass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetID(), imageSize);

            ImGui::Text("Composite Pass");
            ImGui::Image((void*)(intptr_t)m_EnvironmentMap->GetContextData()->CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetID(), imageSize);
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
                m_EnvironmentMap->GetContextData()->CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetWidth(),
                m_EnvironmentMap->GetContextData()->CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetHeight());
            glm::ivec2 depthAttachmentSize = glm::ivec2(
                m_EnvironmentMap->GetContextData()->CompositePass->GetSpecification().TargetFramebuffer->GetAttachmentDepth()->GetWidth(),
                m_EnvironmentMap->GetContextData()->CompositePass->GetSpecification().TargetFramebuffer->GetAttachmentDepth()->GetHeight());

            ImGui::SliderInt2("Color Attachment Size", glm::value_ptr(colorAttachmentSize), 0, 2048);
            ImGui::SliderInt2("Depth Attachment Size", glm::value_ptr(depthAttachmentSize), 0, 2048);
        }
        ImGui::End();
    }

    ImGui::Begin("Select HDRI");
    {
        ImGui::RadioButton("Greenwich Park", &m_HDRI_Edit, HDRI_GREENWICH_PARK);
        ImGui::RadioButton("San Giuseppe Bridge", &m_HDRI_Edit, HDRI_SAN_GIUSEPPE_BRIDGE);
        ImGui::RadioButton("Tropical Beach", &m_HDRI_Edit, HDRI_TROPICAL_BEACH);
        ImGui::RadioButton("Vignaioli Night", &m_HDRI_Edit, HDRI_VIGNAIOLI_NIGHT);
        ImGui::RadioButton("Early Eve & Warm Sky", &m_HDRI_Edit, HDRI_EARLY_EVE_WARM_SKY);
        ImGui::RadioButton("Birchwood", &m_HDRI_Edit, HDRI_BIRCHWOOD);
        ImGui::RadioButton("Pink Sunrise", &m_HDRI_Edit, HDRI_PINK_SUNRISE);
        ImGui::RadioButton("Rooitou Park", &m_HDRI_Edit, HDRI_ROOITOU_PARK);
        ImGui::RadioButton("Venice Dawn", &m_HDRI_Edit, HDRI_VENICE_DAWN);

        ImGui::Separator();
        float skyboxLOD = m_EnvironmentMap->GetSkyboxLOD();
        if (ImGui::SliderFloat("Skybox LOD", &skyboxLOD, 0.0f, 6.0f))
        {
            m_EnvironmentMap->SetSkyboxLOD(skyboxLOD);
        }
    }
    ImGui::End();

    ImGui::Begin("Settings");
    {
        ImGui::Checkbox("Display Bounding Boxes", &m_VisibleAABBs);
        ImGui::Checkbox("Display Line Elements", &m_DisplayLineElements);

        ImGui::Separator();
        for (auto& entity : m_Entities)
        {
            std::string isEnabled = std::string("Is Enabled ") + entity.first;
            ImGui::Checkbox(isEnabled.c_str(), &entity.second.Enabled);
        }
        ImGui::Separator();
        for (auto& entity : m_Entities)
        {
            std::string isIntersecting = std::string("Is Intersecting ") + entity.first;
            ImGui::Checkbox(isIntersecting.c_str(), &entity.second.Intersecting);
        }
        ImGui::Separator();
        bool eventLoggingEnabled = Application::Get()->GetWindow()->GetEventLogging();
        if (ImGui::Checkbox("Enable Event Logging", &eventLoggingEnabled)) {
            Application::Get()->GetWindow()->SetEventLogging(eventLoggingEnabled);
        }
    }
    ImGui::End();

    ImGui::Begin("Mouse Picker");
    {
        if (ImGui::CollapsingHeader("Display Info"))
        {
            char buffer[100];

            sprintf(buffer, "Main Window [ X %i Y %i ]", m_ImGuiViewportMainX, m_ImGuiViewportMainY);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "Viewport [ X %i Y %i W %i H %i ]", mp->m_Viewport.X, mp->m_Viewport.Y, mp->m_Viewport.Width, mp->m_Viewport.Height);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "Screen Mouse [ %i %i ]", mp->m_ScreenMouseX, mp->m_ScreenMouseY);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "Viewport Mouse [ %i %i ]", mp->m_Viewport.MouseX, mp->m_Viewport.MouseY);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "Normalized Coords [ %.2ff %.2ff ]", mp->m_NormalizedCoords.x, mp->m_NormalizedCoords.y);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "Clip Coords [ %.2ff %.2ff ]", mp->m_ClipCoords.x, mp->m_ClipCoords.y);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "Eye Coords [ %.2ff %.2ff %.2ff %.2ff ]", mp->m_EyeCoords.x, mp->m_EyeCoords.y, mp->m_EyeCoords.z, mp->m_EyeCoords.w);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "World Ray [ %.2ff %.2ff %.2ff ]", mp->m_WorldRay.x, mp->m_WorldRay.y, mp->m_WorldRay.z);
            ImGui::Text(buffer);
        }
    }
    ImGui::End();

    m_SceneHierarchyPanel->OnImGuiRender();

    /**** BEGIN Environment Map Settings ****/
    ImGui::Begin("Environment Map Settings");
    {
        if (ImGui::CollapsingHeader("Display Info"))
        {
            {
                if (ImGui::Button("Load Environment Map"))
                {
                    std::string filename = Application::Get()->OpenFile("*.hdr");
                    if (filename != "")
                        m_EnvironmentMap->SetEnvironment(m_EnvironmentMap->Load(filename));
                }

                float skyboxLOD = m_EnvironmentMap->GetSkyboxLOD();
                if (ImGui::SliderFloat("Skybox LOD", &skyboxLOD, 0.0f, 6.0f))
                {
                    m_EnvironmentMap->SetSkyboxLOD(skyboxLOD);
                }

                ImGui::Columns(2);
                ImGui::AlignTextToFramePadding();

                auto& light = m_EnvironmentMap->GetLight();
                Property("Light Direction", light.Direction);
                Property("Light Radiance", light.Radiance, PropertyFlag::ColorProperty);
                Property("Light Multiplier", light.Multiplier, 0.0f, 5.0f);
                Property("Exposure", m_Camera->GetExposure(), 0.0f, 5.0f);

                Property("Radiance Prefiltering", m_EnvironmentMap->GetRadiancePrefilter());
                Property("Env Map Rotation", m_EnvironmentMap->GetEnvMapRotation(), -360.0f, 360.0f);

                ImGui::Columns(1);
            }

            ImGui::Separator();

            {
                ImGui::Text("Mesh");

                auto mesh = m_EnvironmentMap->GetContextData()->DrawList[0].Mesh;
                std::string fullpath = mesh ? mesh->GetFilePath() : "None";
                size_t found = fullpath.find_last_of("/\\");
                std::string path = found != std::string::npos ? fullpath.substr(found + 1) : fullpath;
                ImGui::Text(path.c_str()); ImGui::SameLine();
                if (ImGui::Button("...##Mesh"))
                {
                    std::string filename = Application::Get()->OpenFile("");
                    if (filename != "")
                    {
                        auto newMesh = new Hazel::MeshAnimPBR(filename, m_EnvironmentMap->GetPBRShader(), m_EnvironmentMap->GetContextData()->DrawList[0].Material);
                        m_EntityMesh->SetMesh(newMesh);
                    }
                }
            }

            ImGui::Separator();

            // PBR Textures
            {
                {
                    // Albedo
                    if (ImGui::CollapsingHeader("Albedo", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                        ImGui::Image(m_EnvironmentMap->GetAlbedoInput().TextureMap ?
                            (void*)(intptr_t)m_EnvironmentMap->GetAlbedoInput().TextureMap->GetID() :
                            (void*)(intptr_t)m_EnvironmentMap->GetCheckerboardTexture()->GetID(), ImVec2(64, 64));
                        ImGui::PopStyleVar();
                        if (ImGui::IsItemHovered())
                        {
                            if (m_EnvironmentMap->GetAlbedoInput().TextureMap)
                            {
                                ImGui::BeginTooltip();
                                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                ImGui::TextUnformatted(m_EnvironmentMap->GetAlbedoInput().TextureMap->GetPath().c_str());
                                ImGui::PopTextWrapPos();
                                ImGui::Image((void*)(intptr_t)m_EnvironmentMap->GetAlbedoInput().TextureMap->GetID(), ImVec2(384, 384));
                                ImGui::EndTooltip();
                            }
                            if (ImGui::IsItemClicked())
                            {
                                std::string filename = Application::Get()->OpenFile("");
                                if (filename != "")
                                    m_EnvironmentMap->GetAlbedoInput().TextureMap = Hazel::HazelTexture2D::Create(filename, m_EnvironmentMap->GetAlbedoInput().SRGB);
                            }
                        }
                        ImGui::SameLine();
                        ImGui::BeginGroup();
                        ImGui::Checkbox("Use##AlbedoMap", &m_EnvironmentMap->GetAlbedoInput().UseTexture);
                        if (ImGui::Checkbox("sRGB##AlbedoMap", &m_EnvironmentMap->GetAlbedoInput().SRGB))
                        {
                            if (m_EnvironmentMap->GetAlbedoInput().TextureMap)
                                m_EnvironmentMap->GetAlbedoInput().TextureMap = Hazel::HazelTexture2D::Create(m_EnvironmentMap->GetAlbedoInput().TextureMap->GetPath(), m_EnvironmentMap->GetAlbedoInput().SRGB);
                        }
                        ImGui::EndGroup();
                        ImGui::SameLine();
                        ImGui::ColorEdit3("Color##Albedo", glm::value_ptr(m_EnvironmentMap->GetAlbedoInput().Color), ImGuiColorEditFlags_NoInputs);
                    }
                }
                {
                    // Normals
                    if (ImGui::CollapsingHeader("Normals", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                        ImGui::Image(m_EnvironmentMap->GetNormalInput().TextureMap ?
                            (void*)(intptr_t)m_EnvironmentMap->GetNormalInput().TextureMap->GetID() :
                            (void*)(intptr_t)m_EnvironmentMap->GetCheckerboardTexture()->GetID(), ImVec2(64, 64));
                        ImGui::PopStyleVar();
                        if (ImGui::IsItemHovered())
                        {
                            if (m_EnvironmentMap->GetNormalInput().TextureMap)
                            {
                                ImGui::BeginTooltip();
                                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                ImGui::TextUnformatted(m_EnvironmentMap->GetNormalInput().TextureMap->GetPath().c_str());
                                ImGui::PopTextWrapPos();
                                ImGui::Image((void*)(intptr_t)m_EnvironmentMap->GetNormalInput().TextureMap->GetID(), ImVec2(384, 384));
                                ImGui::EndTooltip();
                            }
                            if (ImGui::IsItemClicked())
                            {
                                std::string filename = Application::Get()->OpenFile("");
                                if (filename != "")
                                    m_EnvironmentMap->GetNormalInput().TextureMap = Hazel::HazelTexture2D::Create(filename);
                            }
                        }
                        ImGui::SameLine();
                        ImGui::Checkbox("Use##NormalMap", &m_EnvironmentMap->GetNormalInput().UseTexture);
                    }
                }
                {
                    // Metalness
                    if (ImGui::CollapsingHeader("Metalness", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                        ImGui::Image(m_EnvironmentMap->GetMetalnessInput().TextureMap ?
                            (void*)(intptr_t)m_EnvironmentMap->GetMetalnessInput().TextureMap->GetID() :
                            (void*)(intptr_t)m_EnvironmentMap->GetCheckerboardTexture()->GetID(), ImVec2(64, 64));
                        ImGui::PopStyleVar();
                        if (ImGui::IsItemHovered())
                        {
                            if (m_EnvironmentMap->GetMetalnessInput().TextureMap)
                            {
                                ImGui::BeginTooltip();
                                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                ImGui::TextUnformatted(m_EnvironmentMap->GetMetalnessInput().TextureMap->GetPath().c_str());
                                ImGui::PopTextWrapPos();
                                ImGui::Image((void*)(intptr_t)m_EnvironmentMap->GetMetalnessInput().TextureMap->GetID(), ImVec2(384, 384));
                                ImGui::EndTooltip();
                            }
                            if (ImGui::IsItemClicked())
                            {
                                std::string filename = Application::Get()->OpenFile("");
                                if (filename != "")
                                    m_EnvironmentMap->GetMetalnessInput().TextureMap = Hazel::HazelTexture2D::Create(filename);
                            }
                        }
                        ImGui::SameLine();
                        ImGui::Checkbox("Use##MetalnessMap", &m_EnvironmentMap->GetMetalnessInput().UseTexture);
                        ImGui::SameLine();
                        ImGui::SliderFloat("Value##MetalnessInput", &m_EnvironmentMap->GetMetalnessInput().Value, 0.0f, 1.0f);
                    }
                }
                {
                    // Roughness
                    if (ImGui::CollapsingHeader("Roughness", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                        ImGui::Image(m_EnvironmentMap->GetRoughnessInput().TextureMap ?
                            (void*)(intptr_t)m_EnvironmentMap->GetRoughnessInput().TextureMap->GetID() :
                            (void*)(intptr_t)m_EnvironmentMap->GetCheckerboardTexture()->GetID(), ImVec2(64, 64));
                        ImGui::PopStyleVar();
                        if (ImGui::IsItemHovered())
                        {
                            if (m_EnvironmentMap->GetRoughnessInput().TextureMap)
                            {
                                ImGui::BeginTooltip();
                                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                ImGui::TextUnformatted(m_EnvironmentMap->GetRoughnessInput().TextureMap->GetPath().c_str());
                                ImGui::PopTextWrapPos();
                                ImGui::Image((void*)(intptr_t)m_EnvironmentMap->GetRoughnessInput().TextureMap->GetID(), ImVec2(384, 384));
                                ImGui::EndTooltip();
                            }
                            if (ImGui::IsItemClicked())
                            {
                                std::string filename = Application::Get()->OpenFile("");
                                if (filename != "")
                                    m_EnvironmentMap->GetRoughnessInput().TextureMap = Hazel::HazelTexture2D::Create(filename);
                            }
                        }
                        ImGui::SameLine();
                        ImGui::Checkbox("Use##RoughnessMap", &m_EnvironmentMap->GetRoughnessInput().UseTexture);
                        ImGui::SameLine();
                        ImGui::SliderFloat("Value##RoughnessInput", &m_EnvironmentMap->GetRoughnessInput().Value, 0.0f, 1.0f);
                    }
                }
            }
        }
    }
    ImGui::End();
    /**** END Environment Map Scene Settings ****/

    if (!m_IsViewportEnabled)
    {
        ImGui::Begin("ImGuizmo");
        {
            UpdateImGuizmo(mainWindow);
        }
        ImGui::End();
    }

    if (m_IsViewportEnabled)
    {
        // TheCherno ImGui Viewport displaying the framebuffer content
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });

        ImGui::Begin("Viewport");
        {
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

            ResizeViewport(viewportPanelSize);

            uint64_t textureID = m_RenderFramebuffer->GetTextureAttachmentColor()->GetID();
            ImGui::Image((void*)(intptr_t)textureID, ImVec2{ m_ViewportMainSize.x, m_ViewportMainSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

            UpdateImGuizmo(mainWindow);
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

            ImVec2 viewportPanelSizeImGui = ImGui::GetContentRegionAvail();
            glm::vec2 viewportPanelSize = glm::vec2(viewportPanelSizeImGui.x, viewportPanelSizeImGui.y);

            // ResizeViewport(viewportPanelSize); // Currently resize can only work with a single (main) viewport
            uint64_t textureID = m_EnvironmentMap->GetContextData()->CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetID();
            ImGui::Image((void*)(intptr_t)textureID, ImVec2{ m_ViewportEnvMapSize.x, m_ViewportEnvMapSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
        }
        ImGui::End();

        ImGui::PopStyleVar();
    }

    // ImGui::ShowMetricsWindow();

    for (auto& drawCommand : m_EnvironmentMap->GetContextData()->DrawList)
    {
        drawCommand.Mesh->OnImGuiRender();
    }
}

void SceneHazelEnvMap::UpdateImGuizmo(Window* mainWindow)
{
    // BEGIN ImGuizmo

    // ImGizmo switching modes
    if (Input::IsKeyPressed(Key::D1))
        m_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE;

    if (Input::IsKeyPressed(Key::D2))
        m_ImGuizmoType = ImGuizmo::OPERATION::ROTATE;

    if (Input::IsKeyPressed(Key::D3))
        m_ImGuizmoType = ImGuizmo::OPERATION::SCALE;

    if (Input::IsKeyPressed(Key::D4))
        m_ImGuizmoType = -1;

    // ImGuizmo
    if (m_ImGuizmoType != -1)
    {
        float rw = (float)ImGui::GetWindowWidth();
        float rh = (float)ImGui::GetWindowHeight();
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, rw, rh);

        if (m_Transform_ImGuizmo != nullptr) {
            ImGuizmo::Manipulate(
                glm::value_ptr(m_CameraController->CalculateViewMatrix()),
                glm::value_ptr(RendererBasic::GetProjectionMatrix()),
                (ImGuizmo::OPERATION)m_ImGuizmoType, ImGuizmo::LOCAL, glm::value_ptr(*m_Transform_ImGuizmo));
        }
    }
    // END ImGuizmo
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
            if (ImGui::MenuItem("Exit")) mainWindow->SetShouldClose(true);
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
    if (!m_DisplayLineElements) return;

    m_ShaderBasic->Bind();
    m_ShaderBasic->setMat4("projection", projectionMatrix);
    m_ShaderBasic->setMat4("view", m_CameraController->CalculateViewMatrix());

    // Draw AABBs
    glm::mat4 AABB_Transform = Math::CreateTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f));

    for (auto& entity : m_Entities)
    {
        if (entity.second.Enabled)
        {
            m_ShaderBasic->setMat4("model", AABB_Transform);
            m_ShaderBasic->setVec4("tintColor", glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
            if (m_VisibleAABBs) entity.second.AABB.Draw();
        }
    }

    m_Grid->Draw(shaderBasic, projectionMatrix, m_CameraController->CalculateViewMatrix());
    m_PivotScene->Draw(shaderBasic, projectionMatrix, m_CameraController->CalculateViewMatrix());
}

void SceneHazelEnvMap::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
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

    SetupUniforms();

    glm::mat4 model = glm::mat4(1.0f);

    // BEGIN Skybox backgroundShader
    {
        // render skybox (render as last to prevent overdraw)
        m_ShaderBackground->Bind();

        // Skybox shaderBackground
        RendererBasic::DisableCulling();
        // render skybox (render as last to prevent overdraw)

        model = glm::mat4(1.0f);
        float angleRadians = glm::radians((GLfloat)glfwGetTime());
        // model = glm::rotate(model, angleRadians, glm::vec3(0.0f, 1.0f, 0.0f));
        m_ShaderBackground->setMat4("model", model);
        m_ShaderBackground->setMat4("projection", projectionMatrix);
        m_ShaderBackground->setMat4("view", m_CameraController->CalculateViewMatrix());

        m_EnvironmentMap->GetContextData()->SceneData.SceneEnvironment.RadianceMap->Bind(0);
        m_ShaderBackground->setInt("environmentMap", 0);
        m_ShaderBackground->setFloat("u_TextureLOD", m_EnvironmentMap->GetSkyboxLOD());
    }
    // END Skybox backgroundShader

    /**** BEGIN Animated PBR models ****/
    for (auto& drawCommand : m_EnvironmentMap->GetContextData()->DrawList)
    {
        if (m_Entities[drawCommand.Name].Enabled)
        {
            drawCommand.Mesh->Render(m_EnvironmentMap->GetSamplerSlots()->at("albedo"), m_Entities[drawCommand.Name].Transform.Transform);
        }
    }

    RenderLineElements(m_ShaderBasic, projectionMatrix);

    // Render the Environment Map scene to the s_Data.CompositePass framebuffer
    // m_EnvironmentMap->Render();

    if (m_IsViewportEnabled)
    {
        m_RenderFramebuffer->Unbind();
    }
}

void SceneHazelEnvMap::SetupUniforms()
{
    /**** BEGIN m_ShaderMain ****/
    m_ShaderMain->Bind();

    m_ShaderMain->setMat4("model", glm::mat4(1.0f));
    m_ShaderMain->setMat4("view", m_CameraController->CalculateViewMatrix());
    m_ShaderMain->setMat4("projection", RendererBasic::GetProjectionMatrix());
    m_ShaderMain->setVec3("eyePosition", m_Camera->GetPosition());

    // Directional Light
    m_ShaderMain->setInt("directionalLight.base.enabled", LightManager::directionalLight.GetEnabled());
    m_ShaderMain->setVec3("directionalLight.base.color", LightManager::directionalLight.GetColor());
    m_ShaderMain->setFloat("directionalLight.base.ambientIntensity", LightManager::directionalLight.GetAmbientIntensity());
    m_ShaderMain->setFloat("directionalLight.base.diffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
    m_ShaderMain->setVec3("directionalLight.direction", LightManager::directionalLight.GetDirection());

    m_ShaderMain->setMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());

    m_ShaderMain->setInt("albedoMap", textureSlots["diffuse"]);
    m_ShaderMain->setInt("normalMap", textureSlots["normal"]);
    m_ShaderMain->setInt("shadowMap", textureSlots["shadow"]);
    m_ShaderMain->setVec4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, -10000.0f));
    m_ShaderMain->setFloat("tilingFactor", 1.0f);
    m_ShaderMain->setVec4("tintColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    m_ShaderMain->Validate();
    /**** END m_ShaderMain ****/
}

bool SceneHazelEnvMap::Property(const std::string& name, bool& value)
{
    ImGui::Text(name.c_str());
    ImGui::NextColumn();
    ImGui::PushItemWidth(-1);

    std::string id = "##" + name;
    bool result = ImGui::Checkbox(id.c_str(), &value);

    ImGui::PopItemWidth();
    ImGui::NextColumn();

    return result;
}

void SceneHazelEnvMap::Property(const std::string& name, float& value, float min, float max, PropertyFlag flags)
{
    ImGui::Text(name.c_str());
    ImGui::NextColumn();
    ImGui::PushItemWidth(-1);

    std::string id = "##" + name;
    ImGui::SliderFloat(id.c_str(), &value, min, max);

    ImGui::PopItemWidth();
    ImGui::NextColumn();
}

void SceneHazelEnvMap::Property(const std::string& name, glm::vec2& value, PropertyFlag flags)
{
    Property(name, value, -1.0f, 1.0f, flags);
}

void SceneHazelEnvMap::Property(const std::string& name, glm::vec2& value, float min, float max, PropertyFlag flags)
{
    ImGui::Text(name.c_str());
    ImGui::NextColumn();
    ImGui::PushItemWidth(-1);

    std::string id = "##" + name;
    ImGui::SliderFloat2(id.c_str(), glm::value_ptr(value), min, max);

    ImGui::PopItemWidth();
    ImGui::NextColumn();
}

void SceneHazelEnvMap::Property(const std::string& name, glm::vec3& value, PropertyFlag flags)
{
    Property(name, value, -1.0f, 1.0f, flags);
}

void SceneHazelEnvMap::Property(const std::string& name, glm::vec3& value, float min, float max, PropertyFlag flags)
{
    ImGui::Text(name.c_str());
    ImGui::NextColumn();
    ImGui::PushItemWidth(-1);

    std::string id = "##" + name;
    if ((int)flags & (int)PropertyFlag::ColorProperty)
        ImGui::ColorEdit3(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
    else
        ImGui::SliderFloat3(id.c_str(), glm::value_ptr(value), min, max);

    ImGui::PopItemWidth();
    ImGui::NextColumn();
}

void SceneHazelEnvMap::Property(const std::string& name, glm::vec4& value, PropertyFlag flags)
{
    Property(name, value, -1.0f, 1.0f, flags);
}

void SceneHazelEnvMap::Property(const std::string& name, glm::vec4& value, float min, float max, PropertyFlag flags)
{

    ImGui::Text(name.c_str());
    ImGui::NextColumn();
    ImGui::PushItemWidth(-1);

    std::string id = "##" + name;
    if ((int)flags & (int)PropertyFlag::ColorProperty)
        ImGui::ColorEdit4(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
    else
        ImGui::SliderFloat4(id.c_str(), glm::value_ptr(value), min, max);

    ImGui::PopItemWidth();
    ImGui::NextColumn();
}
