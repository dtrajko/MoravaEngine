#include "SceneAnimPBR.h"
#include "ImGuiWrapper.h"
#include "../cross-platform/ImGuizmo/ImGuizmo.h"
#include "RendererBasic.h"
#include "ResourceManager.h"
#include "Block.h"
#include "Application.h"
#include "Shader.h"
#include "Hazel/Renderer/MeshAnimPBR.h"
#include "Timer.h"


SceneAnimPBR::SceneAnimPBR()
{
    sceneSettings.cameraPosition = glm::vec3(0.0f, 8.0f, 20.0f);
    sceneSettings.cameraStartYaw = -90.0f;
    sceneSettings.cameraStartPitch = 0.0f;
    sceneSettings.cameraMoveSpeed = 1.0f;
    sceneSettings.waterHeight = 0.0f;
    sceneSettings.waterWaveSpeed = 0.05f;
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
    SetupMeshes();
    SetupModels();
    SetupRenderFramebuffer();
    
    //  // PBR texture inputs
    m_SamplerSlots.insert(std::make_pair("albedo",     1)); // uniform sampler2D u_AlbedoTexture
    m_SamplerSlots.insert(std::make_pair("normal",     2)); // uniform sampler2D u_NormalTexture
    m_SamplerSlots.insert(std::make_pair("metalness",  3)); // uniform sampler2D u_MetalnessTexture
    m_SamplerSlots.insert(std::make_pair("roughness",  4)); // uniform sampler2D u_RoughnessTexture
    m_SamplerSlots.insert(std::make_pair("ao",         5)); // uniform sampler2D u_AOTexture
    // Environment maps
    m_SamplerSlots.insert(std::make_pair("irradiance", 6)); // uniform samplerCube u_IrradianceMap
    m_SamplerSlots.insert(std::make_pair("prefilter",  7)); // uniform samplerCube u_PrefilterMap
    // BRDF LUT
    m_SamplerSlots.insert(std::make_pair("BRDF_LUT",   8)); // uniform sampler2D u_BRDFLUT

    m_MaterialWorkflowPBR = new MaterialWorkflowPBR();
    m_MaterialWorkflowPBR->m_CaptureSize       = 512; // 512
    m_MaterialWorkflowPBR->m_PrefilterMapSize  = 128; // 128
    m_MaterialWorkflowPBR->m_IrradianceMapSize = 32;  //  32
    m_MaterialWorkflowPBR->Init("Textures/HDR/rooitou_park_4k.hdr");

    m_LightPosition = glm::vec3(20.0f, 20.0f, 20.0f);
    m_LightColor = glm::vec3(1.0f, 1.0f, 1.0f);

    m_SkyboxLOD = 0.0f;

    m_Transform_Gizmo = glm::mat4(1.0f);

    m_VisibleAABBs = true;
}

void SceneAnimPBR::SetLightManager()
{
}

void SceneAnimPBR::SetupRenderFramebuffer()
{
    if (!m_IsViewportEnabled) return;

    uint32_t width = Application::Get()->GetWindow()->GetBufferWidth();
    uint32_t height = Application::Get()->GetWindow()->GetBufferHeight();
    m_RenderFramebuffer = new Framebuffer(width, height);

    m_RenderFramebuffer->AddAttachmentSpecification(width, height, AttachmentType::Texture, AttachmentFormat::Color);
    m_RenderFramebuffer->AddAttachmentSpecification(width, height, AttachmentType::Renderbuffer, AttachmentFormat::Depth);

    m_RenderFramebuffer->Generate(width, height);
}

void SceneAnimPBR::ResizeViewport(glm::vec2 viewportPanelSize)
{
    // Cooldown
    if (m_CurrentTimestamp - m_ResizeViewport.lastTime < m_ResizeViewport.cooldown) return;
    m_ResizeViewport.lastTime = m_CurrentTimestamp;

    if (viewportPanelSize != m_ViewportSize && viewportPanelSize.x > 0 && viewportPanelSize.y > 0)
    {
        m_RenderFramebuffer->Resize((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);
        m_ViewportSize = glm::vec2(viewportPanelSize.x, viewportPanelSize.y);

        m_CameraController->OnResize(viewportPanelSize.x, viewportPanelSize.y);

        //  Log::GetLogger()->info("SceneAnimPBR::ResizeViewport Viewport Width: {0} Viewport Height: {1}, AspectRatio: {2}",
        //      viewportPanelSize.x, viewportPanelSize.y, m_CameraController->GetAspectRatio());
    }
}

void SceneAnimPBR::SetupTextures()
{
    ResourceManager::LoadTexture("crate", "Textures/crate.png");
    ResourceManager::LoadTexture("crateNormal", "Textures/crateNormal.png");
}

void SceneAnimPBR::SetupTextureSlots()
{
}

void SceneAnimPBR::SetupMaterials()
{
}

void SceneAnimPBR::SetupShaders()
{
    m_ShaderMain = new Shader("Shaders/shader.vert", "Shaders/shader.frag");
    Log::GetLogger()->info("SceneAnimPBR: m_ShaderMain compiled [programID={0}]", m_ShaderMain->GetProgramID());

    m_ShaderBackground = new Shader("Shaders/LearnOpenGL/2.2.2.background.vs", "Shaders/LearnOpenGL/2.2.2.background.fs");
    Log::GetLogger()->info("SceneAnimPBR: m_ShaderBackground compiled [programID={0}]", m_ShaderBackground->GetProgramID());

    m_ShaderHybridAnimPBR = new Shader("Shaders/HybridAnimPBR.vs", "Shaders/HybridAnimPBR.fs");
    Log::GetLogger()->info("SceneAnimPBR: m_ShaderHybridAnimPBR compiled [programID={0}]", m_ShaderHybridAnimPBR->GetProgramID());

    m_ShaderEquirectangularConversion = new Shader("Shaders/Hazel/EquirectangularToCubeMap.cs");
    Log::GetLogger()->info("SceneAnimPBR: m_ShaderEquirectangularConversion compiled [programID={0}]", m_ShaderEquirectangularConversion->GetProgramID());

    m_ShaderEnvFiltering = new Shader("Shaders/Hazel/EnvironmentMipFilter.cs");
    Log::GetLogger()->info("SceneAnimPBR: m_ShaderEnvFiltering compiled [programID={0}]", m_ShaderEnvFiltering->GetProgramID());

    m_ShaderEnvIrradiance = new Shader("Shaders/Hazel/EnvironmentIrradiance.cs");
    Log::GetLogger()->info("SceneAnimPBR: m_ShaderEnvIrradiance compiled [programID={0}]", m_ShaderEnvIrradiance->GetProgramID());

    m_ShaderBasic = new Shader("Shaders/basic.vs", "Shaders/basic.fs");
    Log::GetLogger()->info("SceneAnimPBR: m_ShaderBasic compiled [programID={0}]", m_ShaderBasic->GetProgramID());
}

void SceneAnimPBR::SetupMeshes()
{
    float materialSpecular = 0.0f;
    float materialShininess = 0.0f;

    Block* floor = new Block(glm::vec3(30.0f, 5.0f, 30.0f));
    meshes.insert(std::make_pair("floor", floor));


    Log::GetLogger()->info("-- BEGIN loading the Cube mesh --");

    Block* cube = new Block(glm::vec3(1.0f, 1.0f, 1.0f));
    meshes.insert(std::make_pair("cube", cube));

    // Setup transform matrix and AABB for the cube mesh
    m_Transform_Cube = glm::mat4(1.0f);
    m_Position_Cube = glm::vec3(0.0f, 5.0f, -20.0f);
    m_Scale_Cube = glm::vec3(10.0f);
    m_Transform_Cube = glm::translate(m_Transform_Cube, m_Position_Cube);
    m_Transform_Cube = glm::scale(m_Transform_Cube, m_Scale_Cube);

    m_AABB_Position_Cube = m_Position_Cube;
    m_AABB_Scale_Cube = m_Scale_Cube;
    m_AABB_Cube = new AABB(m_AABB_Position_Cube, glm::quat(glm::vec3(0.0f)), m_AABB_Scale_Cube);

    Log::GetLogger()->info("-- BEGIN loading the Cube mesh --");

    Log::GetLogger()->info("-- BEGIN loading the animated PBR model M1911 --");

    // M1911
    TextureInfo textureInfoM1911 = {};
    textureInfoM1911.albedo    = "Models/m1911/m1911_color.png";
    textureInfoM1911.normal    = "Models/m1911/m1911_normal.png";
    textureInfoM1911.metallic  = "Models/m1911/m1911_metalness.png";
    textureInfoM1911.roughness = "Models/m1911/m1911_roughness.png";
    textureInfoM1911.ao        = "Textures/PBR/silver/ao.png";

    m_BaseMaterial_M1911 = new Material(textureInfoM1911, materialSpecular, materialShininess);
    m_MeshAnimPBR_M1911 = new Hazel::MeshAnimPBR("Models/m1911/m1911.fbx", m_ShaderHybridAnimPBR, m_BaseMaterial_M1911);
    m_MeshAnimPBR_M1911->SetTimeMultiplier(1.0f);

    m_Position_M1911 = glm::vec3(0.0f, 5.0f, 5.0f);
    m_Scale_M1911 = glm::vec3(20.0f);
    m_Transform_M1911 = glm::mat4(1.0f);
    m_Transform_M1911 = glm::translate(m_Transform_M1911, m_Position_M1911);
    m_Transform_M1911 = glm::scale(m_Transform_M1911, m_Scale_M1911);

    m_AABB_Position_M1911 = m_Position_M1911;
    m_AABB_Scale_M1911 = m_Scale_M1911 * glm::vec3(0.24f, 0.14f, 0.03f);
    m_AABB_M1911 = new AABB(m_AABB_Position_M1911, glm::quat(glm::vec3(0.0f)), m_AABB_Scale_M1911);

    Log::GetLogger()->info("-- END loading the animated PBR model M1911 --");

    Log::GetLogger()->info("-- BEGIN loading the animated PBR model BobLamp --");

    // BobLamp
    TextureInfo textureInfoBobLamp = {};
    textureInfoBobLamp.albedo    = "Textures/plain.png";
    textureInfoBobLamp.normal    = "Textures/PBR/non_reflective/normal.png";
    textureInfoBobLamp.metallic  = "Textures/PBR/non_reflective/metallic.png";
    textureInfoBobLamp.roughness = "Textures/PBR/non_reflective/roughness.png";
    textureInfoBobLamp.ao        = "Textures/PBR/non_reflective/ao.png";

    m_BaseMaterial_BobLamp = new Material(textureInfoBobLamp, materialSpecular, materialShininess);
    m_MeshAnimPBR_BobLamp = new Hazel::MeshAnimPBR("Models/OGLdev/BobLamp/boblampclean.md5mesh", m_ShaderHybridAnimPBR, m_BaseMaterial_BobLamp);
    m_MeshAnimPBR_BobLamp->SetTimeMultiplier(1.0f);

    m_Position_BobLamp = glm::vec3(5.0f, 0.0f, -5.0f);
    m_Scale_BobLamp = glm::vec3(0.1f);
    m_Transform_BobLamp = glm::mat4(1.0f);
    m_Transform_BobLamp = glm::translate(m_Transform_BobLamp, m_Position_BobLamp);
    m_Transform_BobLamp = glm::scale(m_Transform_BobLamp, m_Scale_BobLamp);

    m_AABB_Position_BobLamp = m_Position_BobLamp + glm::vec3(0.0f, 3.0f, 0.0f);
    m_AABB_Scale_BobLamp = m_Scale_BobLamp * glm::vec3(20.0f, 60.0f, 20.0f);
    m_AABB_BobLamp = new AABB(m_AABB_Position_BobLamp, glm::quat(glm::vec3(0.0f)), m_AABB_Scale_BobLamp);

    Log::GetLogger()->info("-- END loading the animated PBR model BobLamp --");

    Log::GetLogger()->info("-- BEGIN loading the animated PBR model Animated Boy --");

    // Animated Boy
    TextureInfo textureInfoAnimBoy = {};
    textureInfoAnimBoy.albedo    = "Models/ThinMatrix/AnimatedCharacter/AnimatedCharacterDiffuse.png";
    textureInfoAnimBoy.normal    = "Textures/PBR/non_reflective/normal.png";
    textureInfoAnimBoy.metallic  = "Textures/PBR/non_reflective/metallic.png";
    textureInfoAnimBoy.roughness = "Textures/PBR/non_reflective/roughness.png";
    textureInfoAnimBoy.ao        = "Textures/PBR/non_reflective/ao.png";

    m_BaseMaterial_AnimBoy = new Material(textureInfoAnimBoy, materialSpecular, materialShininess);
    m_MeshAnimPBR_AnimBoy = new Hazel::MeshAnimPBR("Models/ThinMatrix/AnimatedCharacter/AnimatedCharacter.dae", m_ShaderHybridAnimPBR, m_BaseMaterial_AnimBoy);
    m_MeshAnimPBR_AnimBoy->SetTimeMultiplier(800.0f);

    m_Position_AnimBoy = glm::vec3(-5.0f, 0.0f, -5.0f);
    m_Scale_AnimBoy = glm::vec3(0.6f);
    m_Transform_AnimBoy = glm::mat4(1.0f);
    m_Transform_AnimBoy = glm::translate(m_Transform_AnimBoy, m_Position_AnimBoy);
    m_Transform_AnimBoy = glm::scale(m_Transform_AnimBoy, m_Scale_AnimBoy);

    m_AABB_Position_AnimBoy = m_Position_AnimBoy + glm::vec3(0.0f, 2.6f, 0.0f);
    m_AABB_Scale_AnimBoy = m_Scale_AnimBoy * glm::vec3(2.4f, 8.8f, 2.0f);
    m_AABB_AnimBoy = new AABB(m_AABB_Position_AnimBoy, glm::quat(glm::vec3(0.0f)), m_AABB_Scale_AnimBoy);

    Log::GetLogger()->info("-- END loading the animated PBR model Animated Boy --");

    m_Transform_Gizmo = glm::translate(m_Transform_Gizmo, m_Position_M1911);
}

void SceneAnimPBR::SetupModels()
{
}

void SceneAnimPBR::SetupFramebuffers()
{
}

void SceneAnimPBR::Update(float timestep, Window& mainWindow)
{
    m_CurrentTimestamp = timestep;

    m_ShaderHybridAnimPBR->Bind();

    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        std::string uniformName = std::string("lightPositions[") + std::to_string(i) + std::string("]");
        m_ShaderHybridAnimPBR->setVec3(uniformName, m_LightPosition);
        uniformName = std::string("lightColors[") + std::to_string(i) + std::string("]");
        m_ShaderHybridAnimPBR->setVec3(uniformName, m_LightColor);
    }

    m_ShaderHybridAnimPBR->setMat4("u_ViewProjectionMatrix", RendererBasic::GetProjectionMatrix() * m_CameraController->CalculateViewMatrix());
    m_ShaderHybridAnimPBR->setVec3("u_CameraPosition", m_Camera->GetPosition());
    m_ShaderHybridAnimPBR->setFloat("u_TilingFactor", 1.0f);

    float deltaTime = Timer::Get()->GetDeltaTime();
    m_MeshAnimPBR_M1911->OnUpdate(deltaTime, false);
    m_MeshAnimPBR_BobLamp->OnUpdate(deltaTime, false);
    m_MeshAnimPBR_AnimBoy->OnUpdate(deltaTime, false);

    if (m_HDRI_Edit != m_HDRI_Edit_Prev)
    {
        if (m_HDRI_Edit == HDRI_GREENWICH_PARK)
            m_MaterialWorkflowPBR->Init("Textures/HDR/greenwich_park_02_1k.hdr");
        else if (m_HDRI_Edit == HDRI_SAN_GIUSEPPE_BRIDGE)
            m_MaterialWorkflowPBR->Init("Textures/HDR/san_giuseppe_bridge_1k.hdr");
        else if (m_HDRI_Edit == HDRI_TROPICAL_BEACH)
            m_MaterialWorkflowPBR->Init("Textures/HDR/Tropical_Beach_3k.hdr");
        else if (m_HDRI_Edit == HDRI_VIGNAIOLI_NIGHT)
            m_MaterialWorkflowPBR->Init("Textures/HDR/vignaioli_night_1k.hdr");
        else if (m_HDRI_Edit == HDRI_EARLY_EVE_WARM_SKY)
            m_MaterialWorkflowPBR->Init("Textures/HDR/006_hdrmaps_com_free.hdr");
        else if (m_HDRI_Edit == HDRI_BIRCHWOOD)
            m_MaterialWorkflowPBR->Init("Textures/HDR/birchwood_4k.hdr");
        else if (m_HDRI_Edit == HDRI_PINK_SUNRISE)
            m_MaterialWorkflowPBR->Init("Textures/HDR/pink_sunrise_4k.hdr");
        else if (m_HDRI_Edit == HDRI_ROOITOU_PARK)
            m_MaterialWorkflowPBR->Init("Textures/HDR/rooitou_park_4k.hdr");
        else if (m_HDRI_Edit == HDRI_VENICE_DAWN)
            m_MaterialWorkflowPBR->Init("Textures/HDR/venice_dawn_1_4k.hdr");

        m_HDRI_Edit_Prev = m_HDRI_Edit;
    }
}

void SceneAnimPBR::UpdateImGui(float timestep, Window& mainWindow)
{
    bool p_open = true;
    ShowExampleAppDockSpace(&p_open, mainWindow);

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
        ImGui::SliderFloat("Skybox LOD", &m_SkyboxLOD, 0.0f, 6.0f);
    }
    ImGui::End();

    ImGui::Begin("Settings");
    {
        ImGui::Checkbox("Display Bounding Boxes", &m_VisibleAABBs);
    }
    ImGui::End();

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
            // HZ_CORE_WARN("Focused: {0}", ImGui::IsWindowFocused());
            // HZ_CORE_WARN("Hovered: {0}", ImGui::IsWindowHovered());

            m_ViewportFocused = ImGui::IsWindowFocused();
            m_ViewportHovered = ImGui::IsWindowHovered();
            // Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportFocused || !m_ViewportHovered);

            ImVec2 viewportPanelSizeImGui = ImGui::GetContentRegionAvail();
            glm::vec2 viewportPanelSize = glm::vec2(viewportPanelSizeImGui.x, viewportPanelSizeImGui.y);

            ResizeViewport(viewportPanelSize);

            uint64_t textureID = m_RenderFramebuffer->GetTextureAttachmentColor()->GetID();
            // uint64_t textureID = m_BlurEffect->GetVerticalOutputTexture()->GetID();
            ImGui::Image((void*)(intptr_t)textureID, ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

            UpdateImGuizmo(mainWindow);
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    m_MeshAnimPBR_M1911->OnImGuiRender();
    m_MeshAnimPBR_BobLamp->OnImGuiRender();
    m_MeshAnimPBR_AnimBoy->OnImGuiRender();
}

void SceneAnimPBR::UpdateImGuizmo(Window& mainWindow)
{
    // BEGIN ImGuizmo

    // ImGizmo switching modes
    if (mainWindow.getKeys()[GLFW_KEY_1])
        m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;

    if (mainWindow.getKeys()[GLFW_KEY_2])
        m_GizmoType = ImGuizmo::OPERATION::ROTATE;

    if (mainWindow.getKeys()[GLFW_KEY_3])
        m_GizmoType = ImGuizmo::OPERATION::SCALE;

    if (mainWindow.getKeys()[GLFW_KEY_4])
        m_GizmoType = -1;

    // Gizmo
    if (m_GizmoType != -1)
    {
        float rw = (float)ImGui::GetWindowWidth();
        float rh = (float)ImGui::GetWindowHeight();
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, rw, rh);

        ImGuizmo::Manipulate(
            glm::value_ptr(m_CameraController->CalculateViewMatrix()),
            glm::value_ptr(RendererBasic::GetProjectionMatrix()), 
            (ImGuizmo::OPERATION)m_GizmoType, ImGuizmo::LOCAL, glm::value_ptr(m_Transform_Gizmo));
    }

    glm::vec3 translation;
    glm::vec3 rotation;
    glm::vec3 scale;
    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(m_Transform_Gizmo), glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scale));
    // Log::GetLogger()->info("EditTransform Translation {0} {1} {2}", translation.x, translation.y, translation.z);
    // Log::GetLogger()->info("EditTransform Rotation {0} {1} {2}", rotation.x, rotation.y, rotation.z);
    // Log::GetLogger()->info("EditTransform Scale {0} {1} {2}", scale.x, scale.y, scale.z);

    // END ImGuizmo
}

// Demonstrate using DockSpace() to create an explicit docking node within an existing window.
// Note that you already dock windows into each others _without_ a DockSpace() by just moving windows 
// from their title bar (or by holding SHIFT if io.ConfigDockingWithShift is set).
// DockSpace() is only useful to construct to a central location for your application.
void SceneAnimPBR::ShowExampleAppDockSpace(bool* p_open, Window& mainWindow)
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
            if (ImGui::MenuItem("Exit")) mainWindow.SetShouldClose(true);
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

            if (ImGui::MenuItem("Flag: NoSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0))                 dockspace_flags ^= ImGuiDockNodeFlags_NoSplit;
            if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))                dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
            if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))  dockspace_flags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
            if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0))     dockspace_flags ^= ImGuiDockNodeFlags_PassthruCentralNode;
            if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))          dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
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

void SceneAnimPBR::Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
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
        glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());
        RendererBasic::SetDefaultFramebuffer((unsigned int)mainWindow.GetBufferWidth(), (unsigned int)mainWindow.GetBufferHeight());
    }

    SetupUniforms();

    m_MaterialWorkflowPBR->SetGlobalRenderState();

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

        m_MaterialWorkflowPBR->BindEnvironmentCubemap(0);
        // m_MaterialWorkflowPBR->BindIrradianceMap(0); // display irradiance map
        // m_MaterialWorkflowPBR->BindPrefilterMap(0); // display prefilter map
        m_ShaderBackground->setInt("environmentMap", 0);
        m_ShaderBackground->setFloat("u_TextureLOD", m_SkyboxLOD);

        m_MaterialWorkflowPBR->GetSkyboxCube()->Render();
    }
    // END Skybox backgroundShader

    /**** BEGIN Animated PBR models ****/
    m_ShaderHybridAnimPBR->Bind();

    m_ShaderHybridAnimPBR->setInt("u_AlbedoTexture",    m_SamplerSlots["albedo"]);
    m_ShaderHybridAnimPBR->setInt("u_NormalTexture",    m_SamplerSlots["normal"]);
    m_ShaderHybridAnimPBR->setInt("u_MetalnessTexture", m_SamplerSlots["metalness"]);
    m_ShaderHybridAnimPBR->setInt("u_RoughnessTexture", m_SamplerSlots["roughness"]);
    m_ShaderHybridAnimPBR->setInt("u_AOTexture",        m_SamplerSlots["ao"]);
    m_ShaderHybridAnimPBR->setInt("u_EnvRadianceTex",   m_SamplerSlots["irradiance"]);
    m_ShaderHybridAnimPBR->setInt("u_PrefilterMap",     m_SamplerSlots["prefilter"]);
    m_ShaderHybridAnimPBR->setInt("u_BRDFLUT",          m_SamplerSlots["BRDF_LUT"]);

    m_MaterialWorkflowPBR->BindTextures(m_SamplerSlots["irradiance"]);

    // BEGIN rendering the animated PBR model M1911
    {
        m_BaseMaterial_M1911->GetTextureAlbedo()->Bind(m_SamplerSlots["albedo"]);
        m_BaseMaterial_M1911->GetTextureNormal()->Bind(m_SamplerSlots["normal"]);
        m_BaseMaterial_M1911->GetTextureMetallic()->Bind(m_SamplerSlots["metalness"]);
        m_BaseMaterial_M1911->GetTextureRoughness()->Bind(m_SamplerSlots["roughness"]);
        m_BaseMaterial_M1911->GetTextureAO()->Bind(m_SamplerSlots["ao"]);

        m_MeshAnimPBR_M1911->m_VertexArray->Bind();
        auto& materials = m_MeshAnimPBR_M1911->GetMaterials();

        int submeshIndex = 0;
        for (Hazel::Submesh* submesh : m_MeshAnimPBR_M1911->GetSubmeshes())
        {
            // Material
            auto material = materials[submesh->MaterialIndex];
            m_ShaderHybridAnimPBR->Bind();

            for (size_t i = 0; i < m_MeshAnimPBR_M1911->m_BoneTransforms.size(); i++)
            {
                std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
                m_ShaderHybridAnimPBR->setMat4(uniformName, m_MeshAnimPBR_M1911->m_BoneTransforms[i]);
            }

            m_ShaderHybridAnimPBR->setMat4("u_Transform", m_Transform_M1911 * submesh->Transform);

            glEnable(GL_DEPTH_TEST);
            glDrawElementsBaseVertex(GL_TRIANGLES, submesh->IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh->BaseIndex), submesh->BaseVertex);

            submeshIndex++;
        }
    }
    // END rendering the animated PBR model M1911

    // BEGIN rendering the animated PBR model BobLamp
    {
        m_BaseMaterial_BobLamp->GetTextureAlbedo()->Bind(m_SamplerSlots["albedo"]);
        m_BaseMaterial_BobLamp->GetTextureNormal()->Bind(m_SamplerSlots["normal"]);
        m_BaseMaterial_BobLamp->GetTextureMetallic()->Bind(m_SamplerSlots["metalness"]);
        m_BaseMaterial_BobLamp->GetTextureRoughness()->Bind(m_SamplerSlots["roughness"]);
        m_BaseMaterial_BobLamp->GetTextureAO()->Bind(m_SamplerSlots["ao"]);
 
        m_MeshAnimPBR_BobLamp->m_VertexArray->Bind();
        auto& materials = m_MeshAnimPBR_BobLamp->GetMaterials();

        int submeshIndex = 0;
        for (Hazel::Submesh* submesh : m_MeshAnimPBR_BobLamp->GetSubmeshes())
        {
            // Material
            auto material = materials[submesh->MaterialIndex];
            m_ShaderHybridAnimPBR->Bind();

            m_MeshAnimPBR_BobLamp->GetTextures()[submeshIndex]->Bind(m_SamplerSlots["albedo"]);

            for (size_t i = 0; i < m_MeshAnimPBR_BobLamp->m_BoneTransforms.size(); i++)
            {
                std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
                m_ShaderHybridAnimPBR->setMat4(uniformName, m_MeshAnimPBR_BobLamp->m_BoneTransforms[i]);
            }

            m_ShaderHybridAnimPBR->setMat4("u_Transform", m_Transform_BobLamp * submesh->Transform);

            glEnable(GL_DEPTH_TEST);
            glDrawElementsBaseVertex(GL_TRIANGLES, submesh->IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh->BaseIndex), submesh->BaseVertex);

            submeshIndex++;
        }
    }
    // END rendering the animated PBR model BobLamp

    // BEGIN rendering the animated PBR model Animated Boy
    {
        m_BaseMaterial_AnimBoy->GetTextureAlbedo()->Bind(m_SamplerSlots["albedo"]);
        m_BaseMaterial_AnimBoy->GetTextureNormal()->Bind(m_SamplerSlots["normal"]);
        m_BaseMaterial_AnimBoy->GetTextureMetallic()->Bind(m_SamplerSlots["metalness"]);
        m_BaseMaterial_AnimBoy->GetTextureRoughness()->Bind(m_SamplerSlots["roughness"]);
        m_BaseMaterial_AnimBoy->GetTextureAO()->Bind(m_SamplerSlots["ao"]);

        m_MeshAnimPBR_AnimBoy->m_VertexArray->Bind();
        auto& materials = m_MeshAnimPBR_AnimBoy->GetMaterials();

        int submeshIndex = 0;
        for (Hazel::Submesh* submesh : m_MeshAnimPBR_AnimBoy->GetSubmeshes())
        {
            // Material
            auto material = materials[submesh->MaterialIndex];
            m_ShaderHybridAnimPBR->Bind();

            for (size_t i = 0; i < m_MeshAnimPBR_AnimBoy->m_BoneTransforms.size(); i++)
            {
                std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
                m_ShaderHybridAnimPBR->setMat4(uniformName, m_MeshAnimPBR_AnimBoy->m_BoneTransforms[i]);
            }

            m_ShaderHybridAnimPBR->setMat4("u_Transform", m_Transform_AnimBoy * submesh->Transform);

            glEnable(GL_DEPTH_TEST);
            glDrawElementsBaseVertex(GL_TRIANGLES, submesh->IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh->BaseIndex), submesh->BaseVertex);

            submeshIndex++;
        }
    }
    // END rendering the animated PBR model Animated Boy

    // BEGIN main shader rendering
    m_ShaderMain->Bind();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, -2.5f, 0.0f));
    m_ShaderMain->setMat4("model", model);
    ResourceManager::GetTexture("crate")->Bind(textureSlots["diffuse"]);
    ResourceManager::GetTexture("crateNormal")->Bind(textureSlots["normal"]);
    m_ShaderMain->setFloat("tilingFactor", 0.1f);
    meshes["floor"]->Render();

    m_ShaderMain->setMat4("model", m_Transform_Cube);
    ResourceManager::GetTexture("crate")->Bind(textureSlots["diffuse"]);
    ResourceManager::GetTexture("crateNormal")->Bind(textureSlots["normal"]);
    m_ShaderMain->setFloat("tilingFactor", 1.0f);
    meshes["cube"]->Render();
    // END main shader rendering

    if (m_VisibleAABBs)
    {
        m_ShaderBasic->Bind();
        m_ShaderBasic->setMat4("projection", projectionMatrix);
        m_ShaderBasic->setMat4("view", m_CameraController->CalculateViewMatrix());
        m_ShaderBasic->setMat4("model", glm::mat4(1.0f));

        m_AABB_M1911->Draw();
        m_AABB_BobLamp->Draw();
        m_AABB_AnimBoy->Draw();
        m_AABB_Cube->Draw();
    }

    if (m_IsViewportEnabled)
    {
        m_RenderFramebuffer->Unbind();
    }
}

void SceneAnimPBR::SetupUniforms()
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

SceneAnimPBR::~SceneAnimPBR()
{
}
