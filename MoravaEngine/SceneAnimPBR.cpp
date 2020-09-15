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

    SetCamera();
    SetLightManager();
    SetupShaders();
    SetupTextureSlots();
    SetupTextures();
    SetupMaterials();
    SetupFramebuffers();
    SetupMeshes();
    SetupModels();

    m_CubeTransform = glm::mat4(1.0f);
    m_CubeTransform = glm::translate(m_CubeTransform, glm::vec3(-6.0f, 2.0f, 6.0f));
    
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
}

void SceneAnimPBR::SetupMeshes()
{
    Block* floor = new Block(glm::vec3(30.0f, 5.0f, 30.0f));
    meshes.insert(std::make_pair("floor", floor));

    Block* cube = new Block(glm::vec3(1.0f, 1.0f, 1.0f));
    meshes.insert(std::make_pair("cube", cube));

    float materialSpecular = 0.0f;
    float materialShininess = 0.0f;

    Log::GetLogger()->info("-- BEGIN loading the animated PBR model M1911 --");

    // M1911
    TextureInfo textureInfoM1911 = {};
    textureInfoM1911.albedo    = "Models/m1911/m1911_color.png";
    textureInfoM1911.normal    = "Models/m1911/m1911_normal.png";
    textureInfoM1911.metallic  = "Models/m1911/m1911_metalness.png";
    textureInfoM1911.roughness = "Models/m1911/m1911_roughness.png";
    textureInfoM1911.ao        = "Textures/PBR/silver/ao.png";
    // textureInfoM1911.albedo    = "Textures/PBR/gold/albedo.png";
    // textureInfoM1911.normal    = "Textures/PBR/gold/normal.png";
    // textureInfoM1911.metallic  = "Textures/PBR/gold/metallic.png";
    // textureInfoM1911.roughness = "Textures/PBR/gold/roughness.png";
    // textureInfoM1911.ao        = "Textures/PBR/gold/ao.png";

    m_BaseMaterialM1911 = new Material(textureInfoM1911, materialSpecular, materialShininess);
    m_MeshAnimPBRM1911 = new Hazel::MeshAnimPBR("Models/m1911/m1911.fbx", m_ShaderHybridAnimPBR, m_BaseMaterialM1911);
    m_Transform_M1911 = glm::mat4(1.0f);

    m_MeshAnimPBRM1911->SetTimeMultiplier(1.0f);

    Log::GetLogger()->info("-- END loading the animated PBR model M1911 --");

    Log::GetLogger()->info("-- BEGIN loading the animated PBR model BobLamp --");

    // BobLamp
    TextureInfo textureInfoBobLamp = {};
    textureInfoBobLamp.albedo    = "Textures/plain.png";
    textureInfoBobLamp.normal    = "Textures/PBR/non_reflective/normal.png";
    textureInfoBobLamp.metallic  = "Textures/PBR/non_reflective/metallic.png";
    textureInfoBobLamp.roughness = "Textures/PBR/non_reflective/roughness.png";
    textureInfoBobLamp.ao        = "Textures/PBR/non_reflective/ao.png";

    m_BaseMaterialBob = new Material(textureInfoBobLamp, materialSpecular, materialShininess);
    m_MeshAnimPBRBob = new Hazel::MeshAnimPBR("Models/OGLdev/BobLamp/boblampclean.md5mesh", m_ShaderHybridAnimPBR, m_BaseMaterialBob);
    m_Transform_BobLamp = glm::mat4(1.0f);

    m_MeshAnimPBRBob->SetTimeMultiplier(1.0f);

    Log::GetLogger()->info("-- END loading the animated PBR model BobLamp --");

    Log::GetLogger()->info("-- BEGIN loading the animated PBR model Animated Boy --");

    // Animated Boy
    TextureInfo textureInfoAnimBoy = {};
    textureInfoAnimBoy.albedo    = "Models/ThinMatrix/AnimatedCharacter/AnimatedCharacterDiffuse.png";
    textureInfoAnimBoy.normal    = "Textures/PBR/non_reflective/normal.png";
    textureInfoAnimBoy.metallic  = "Textures/PBR/non_reflective/metallic.png";
    textureInfoAnimBoy.roughness = "Textures/PBR/non_reflective/roughness.png";
    textureInfoAnimBoy.ao        = "Textures/PBR/non_reflective/ao.png";

    m_BaseMaterialBoy = new Material(textureInfoAnimBoy, materialSpecular, materialShininess);
    m_MeshAnimPBRBoy = new Hazel::MeshAnimPBR("Models/ThinMatrix/AnimatedCharacter/AnimatedCharacter.dae", m_ShaderHybridAnimPBR, m_BaseMaterialBoy);
    m_Transform_Boy = glm::mat4(1.0f);

    m_MeshAnimPBRBoy->SetTimeMultiplier(800.0f);

    Log::GetLogger()->info("-- END loading the animated PBR model Animated Boy --");
}

void SceneAnimPBR::SetupModels()
{
}

void SceneAnimPBR::SetupFramebuffers()
{
}

void SceneAnimPBR::Update(float timestep, Window& mainWindow)
{
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
    m_MeshAnimPBRM1911->OnUpdate(deltaTime, false);
    m_MeshAnimPBRBob->OnUpdate(deltaTime, false);
    m_MeshAnimPBRBoy->OnUpdate(deltaTime, false);

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

    ImGui::Begin("Light");
    {
        ImGui::SliderFloat3("Light Position", glm::value_ptr(m_LightPosition), -100.0f, 100.0f);
        ImGui::ColorEdit3("Light Color", glm::value_ptr(m_LightColor));
    }
    ImGui::End();

    ImGui::Begin("Textures");
    {
        ImVec2 imageSize(128.0f, 128.0f);

        ImGui::Text("Environment Cubemap");
        ImGui::Image((void*)(intptr_t)m_MaterialWorkflowPBR->GetEnvironmentCubemap(), imageSize);

        ImGui::Text("Irradiance Map");
        ImGui::Image((void*)(intptr_t)m_MaterialWorkflowPBR->GetIrradianceMap(), imageSize);

        ImGui::Text("Prefilter Map");
        ImGui::Image((void*)(intptr_t)m_MaterialWorkflowPBR->GetPrefilterMap(), imageSize);

        ImGui::Text("BRDF LUT");
        ImGui::Image((void*)(intptr_t)m_MaterialWorkflowPBR->GetBRDF_LUT_Texture(), imageSize);
    }
    ImGui::End();

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
    }
    ImGui::End();

    ImGui::Begin("ImGuizmo");
    {
        glm::mat4 projectionMatrix = RendererBasic::GetProjectionMatrix();
        bool editTransformDecomposition = true;

        EditTransform(glm::value_ptr(m_CameraController->CalculateViewMatrix()),
            glm::value_ptr(projectionMatrix),
            glm::value_ptr(m_CubeTransform), editTransformDecomposition);
    }
    ImGui::End();
}

void SceneAnimPBR::EditTransform(const float* cameraView, float* cameraProjection, float* matrix, bool editTransformDecomposition)
{
    static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
    static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::LOCAL);
    static bool useSnap = false;
    static float snap[3] = { 1.f, 1.f, 1.f };
    static float bounds[] = { -0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f };
    static float boundsSnap[] = { 0.1f, 0.1f, 0.1f };
    static bool boundSizing = false;
    static bool boundSizingSnap = false;

    if (editTransformDecomposition)
    {
        if (ImGui::IsKeyPressed(90))
            mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyPressed(69))
            mCurrentGizmoOperation = ImGuizmo::ROTATE;
        if (ImGui::IsKeyPressed(82)) // r Key
            mCurrentGizmoOperation = ImGuizmo::SCALE;
        if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
            mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
            mCurrentGizmoOperation = ImGuizmo::ROTATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
            mCurrentGizmoOperation = ImGuizmo::SCALE;
        float matrixTranslation[3], matrixRotation[3], matrixScale[3];
        ImGuizmo::DecomposeMatrixToComponents(matrix, matrixTranslation, matrixRotation, matrixScale);
        ImGui::InputFloat3("Tr", matrixTranslation, 3);
        ImGui::InputFloat3("Rt", matrixRotation, 3);
        ImGui::InputFloat3("Sc", matrixScale, 3);
        ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, matrix);

        if (mCurrentGizmoOperation != ImGuizmo::SCALE)
        {
            if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
                mCurrentGizmoMode = ImGuizmo::LOCAL;
            ImGui::SameLine();
            if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
                mCurrentGizmoMode = ImGuizmo::WORLD;
        }
        if (ImGui::IsKeyPressed(83))
            useSnap = !useSnap;
        ImGui::Checkbox("", &useSnap);
        ImGui::SameLine();

        switch (mCurrentGizmoOperation)
        {
        case ImGuizmo::TRANSLATE:
            ImGui::InputFloat3("Snap", &snap[0]);
            break;
        case ImGuizmo::ROTATE:
            ImGui::InputFloat("Angle Snap", &snap[0]);
            break;
        case ImGuizmo::SCALE:
            ImGui::InputFloat("Scale Snap", &snap[0]);
            break;
        }
        ImGui::Checkbox("Bound Sizing", &boundSizing);
        if (boundSizing)
        {
            ImGui::PushID(3);
            ImGui::Checkbox("", &boundSizingSnap);
            ImGui::SameLine();
            ImGui::InputFloat3("Snap", boundsSnap);
            ImGui::PopID();
        }
    }
    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(
        ImGui::GetWindowPos().x,
        ImGui::GetWindowPos().y,
        (float)Application::Get()->GetWindow()->GetBufferWidth(),
        (float)Application::Get()->GetWindow()->GetBufferHeight()
    );
    ImGuizmo::Manipulate(cameraView, cameraProjection, mCurrentGizmoOperation, mCurrentGizmoMode, matrix, NULL, useSnap ? &snap[0] : NULL, boundSizing ? bounds : NULL, boundSizingSnap ? boundsSnap : NULL);
}

float SceneAnimPBR::GetSnapValue()
{
    if (m_GizmoType == ImGuizmo::OPERATION::TRANSLATE) return 0.5f;
    else if (m_GizmoType == ImGuizmo::OPERATION::ROTATE) return 45.0f;
    else if (m_GizmoType == ImGuizmo::OPERATION::SCALE) return 0.5f;
    else return 0.0f;
}

void SceneAnimPBR::Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
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
        m_BaseMaterialM1911->GetTextureAlbedo()->Bind(m_SamplerSlots["albedo"]);
        m_BaseMaterialM1911->GetTextureNormal()->Bind(m_SamplerSlots["normal"]);
        m_BaseMaterialM1911->GetTextureMetallic()->Bind(m_SamplerSlots["metalness"]);
        m_BaseMaterialM1911->GetTextureRoughness()->Bind(m_SamplerSlots["roughness"]);
        m_BaseMaterialM1911->GetTextureAO()->Bind(m_SamplerSlots["ao"]);

        m_MeshAnimPBRM1911->m_VertexArray->Bind();
        auto& materials = m_MeshAnimPBRM1911->GetMaterials();

        int submeshIndex = 0;
        for (Hazel::Submesh* submesh : m_MeshAnimPBRM1911->GetSubmeshes())
        {
            // Material
            auto material = materials[submesh->MaterialIndex];
            m_ShaderHybridAnimPBR->Bind();

            for (size_t i = 0; i < m_MeshAnimPBRM1911->m_BoneTransforms.size(); i++)
            {
                std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
                m_ShaderHybridAnimPBR->setMat4(uniformName, m_MeshAnimPBRM1911->m_BoneTransforms[i]);
            }

            glm::mat4 transform = m_Transform_M1911 * submesh->Transform;
            transform = glm::translate(transform, glm::vec3(0.0f, 40.0f, 50.0f));
            transform = glm::scale(transform, glm::vec3(40.0f));
            m_ShaderHybridAnimPBR->setMat4("u_Transform", transform);

            glEnable(GL_DEPTH_TEST);
            glDrawElementsBaseVertex(GL_TRIANGLES, submesh->IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh->BaseIndex), submesh->BaseVertex);

            submeshIndex++;
        }
    }
    // END rendering the animated PBR model M1911

    // BEGIN rendering the animated PBR model BobLamp
    {
        m_BaseMaterialBob->GetTextureAlbedo()->Bind(m_SamplerSlots["albedo"]);
        m_BaseMaterialBob->GetTextureNormal()->Bind(m_SamplerSlots["normal"]);
        m_BaseMaterialBob->GetTextureMetallic()->Bind(m_SamplerSlots["metalness"]);
        m_BaseMaterialBob->GetTextureRoughness()->Bind(m_SamplerSlots["roughness"]);
        m_BaseMaterialBob->GetTextureAO()->Bind(m_SamplerSlots["ao"]);
 
        m_MeshAnimPBRBob->m_VertexArray->Bind();
        auto& materials = m_MeshAnimPBRBob->GetMaterials();

        int submeshIndex = 0;
        for (Hazel::Submesh* submesh : m_MeshAnimPBRBob->GetSubmeshes())
        {
            // Material
            auto material = materials[submesh->MaterialIndex];
            m_ShaderHybridAnimPBR->Bind();

            m_MeshAnimPBRBob->GetTextures()[submeshIndex]->Bind(m_SamplerSlots["albedo"]);

            for (size_t i = 0; i < m_MeshAnimPBRBob->m_BoneTransforms.size(); i++)
            {
                std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
                m_ShaderHybridAnimPBR->setMat4(uniformName, m_MeshAnimPBRBob->m_BoneTransforms[i]);
            }

            glm::mat4 transform = m_Transform_BobLamp * submesh->Transform;
            transform = glm::translate(transform, glm::vec3(5.0f, 5.0f, 0.0f));
            transform = glm::scale(transform, glm::vec3(0.2f));
            m_ShaderHybridAnimPBR->setMat4("u_Transform", transform);

            glEnable(GL_DEPTH_TEST);
            glDrawElementsBaseVertex(GL_TRIANGLES, submesh->IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh->BaseIndex), submesh->BaseVertex);

            submeshIndex++;
        }
    }
    // END rendering the animated PBR model BobLamp

    // BEGIN rendering the animated PBR model Animated Boy
    {
        m_BaseMaterialBoy->GetTextureAlbedo()->Bind(m_SamplerSlots["albedo"]);
        m_BaseMaterialBoy->GetTextureNormal()->Bind(m_SamplerSlots["normal"]);
        m_BaseMaterialBoy->GetTextureMetallic()->Bind(m_SamplerSlots["metalness"]);
        m_BaseMaterialBoy->GetTextureRoughness()->Bind(m_SamplerSlots["roughness"]);
        m_BaseMaterialBoy->GetTextureAO()->Bind(m_SamplerSlots["ao"]);

        m_MeshAnimPBRBoy->m_VertexArray->Bind();
        auto& materials = m_MeshAnimPBRBoy->GetMaterials();

        int submeshIndex = 0;
        for (Hazel::Submesh* submesh : m_MeshAnimPBRBoy->GetSubmeshes())
        {
            // Material
            auto material = materials[submesh->MaterialIndex];
            m_ShaderHybridAnimPBR->Bind();

            for (size_t i = 0; i < m_MeshAnimPBRBoy->m_BoneTransforms.size(); i++)
            {
                std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
                m_ShaderHybridAnimPBR->setMat4(uniformName, m_MeshAnimPBRBoy->m_BoneTransforms[i]);
            }

            glm::mat4 transform = m_Transform_Boy * submesh->Transform;
            transform = glm::translate(transform, glm::vec3(-5.0f, 5.0f, 0.0f));
            transform = glm::scale(transform, glm::vec3(0.8f));
            m_ShaderHybridAnimPBR->setMat4("u_Transform", transform);

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

    m_ShaderMain->setMat4("model", m_CubeTransform);
    ResourceManager::GetTexture("crate")->Bind(textureSlots["diffuse"]);
    ResourceManager::GetTexture("crateNormal")->Bind(textureSlots["normal"]);
    m_ShaderMain->setFloat("tilingFactor", 1.0f);
    meshes["cube"]->Render();
    // END main shader rendering
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
