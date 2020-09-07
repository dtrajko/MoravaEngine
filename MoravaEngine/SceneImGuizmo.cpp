#include "SceneImGuizmo.h"
#include "ImGuiWrapper.h"
#include "../cross-platform/ImGuizmo/ImGuizmo.h"
#include "RendererBasic.h"
#include "ResourceManager.h"
#include "Block.h"
#include "Application.h"
#include "Shader.h"
#include "Hazel/Renderer/MeshAnimPBR.h"


SceneImGuizmo::SceneImGuizmo()
{
    sceneSettings.cameraPosition = glm::vec3(0.0f, 6.0f, 8.0f);
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
    SetupTextureSlots();
    SetupTextures();
    SetupFramebuffers();
    SetupMeshes();
    SetupModels();

    m_CubeTransform = glm::mat4(1.0f);
    m_CubeTransform = glm::translate(m_CubeTransform, glm::vec3(6.0f, 1.5f, -6.0f));
    
    //  // PBR texture inputs
    m_SamplerSlots.insert(std::make_pair("albedo"        , 1)); // uniform sampler2D u_AlbedoTexture
    m_SamplerSlots.insert(std::make_pair("normal"        , 2)); // uniform sampler2D u_NormalTexture
    m_SamplerSlots.insert(std::make_pair("metalness"     , 3)); // uniform sampler2D u_MetalnessTexture
    m_SamplerSlots.insert(std::make_pair("roughness"     , 4)); // uniform sampler2D u_RoughnessTexture
    // Environment maps
    m_SamplerSlots.insert(std::make_pair("env_radiance"  , 5)); // uniform samplerCube u_EnvRadianceTex
    m_SamplerSlots.insert(std::make_pair("env_irradiance", 6)); // uniform samplerCube u_EnvIrradianceTex
    // BRDF LUT
    m_SamplerSlots.insert(std::make_pair("BRDF_LUT"      , 7)); // uniform sampler2D u_BRDFLUTTexture

    // HDR / Environment map
    m_TextureCubemaps = CreateEnvironmentMap("Textures/HDR/birchwood_4k.hdr");

    m_BRDF_LUT = new Texture("Textures/Hazel/BRDF_LUT.tga");
}

void SceneImGuizmo::SetupTextures()
{
    ResourceManager::LoadTexture("crate", "Textures/crate.png");
    ResourceManager::LoadTexture("crateNormal", "Textures/crateNormal.png");
}

void SceneImGuizmo::SetupTextureSlots()
{
}

void SceneImGuizmo::SetupMeshes()
{
    Block* floor = new Block(glm::vec3(16.0f, 0.5f, 16.0f));
    meshes.insert(std::make_pair("floor", floor));

    Block* cube = new Block(glm::vec3(1.0f, 1.0f, 1.0f));
    meshes.insert(std::make_pair("cube", cube));

    Log::GetLogger()->info("-- BEGIN loading the animated PBR model M1911 --");

    m_ShaderHazelAnimPBR = new Shader("Shaders/Hazel/HazelPBR_Anim.vs", "Shaders/Hazel/HazelPBR_Anim.fs");
    Log::GetLogger()->info("SceneImGuizmo: m_ShaderHazelAnimPBR compiled [programID={0}]", m_ShaderHazelAnimPBR->GetProgramID());

    m_ShaderHDR = new Shader("Shaders/Hazel/HDR.vs", "Shaders/Hazel/HDR.fs");
    Log::GetLogger()->info("SceneImGuizmo: m_ShaderHDR compiled [programID={0}]", m_ShaderHDR->GetProgramID());

    m_ShaderEquirectangularConversion = new Shader("Shaders/Hazel/EquirectangularToCubeMap.cs");
    Log::GetLogger()->info("SceneImGuizmo: m_ShaderEquirectangularConversion compiled [programID={0}]", m_ShaderEquirectangularConversion->GetProgramID());

    m_ShaderEnvFiltering = new Shader("Shaders/Hazel/EnvironmentMipFilter.cs");
    Log::GetLogger()->info("SceneImGuizmo: m_ShaderEnvFiltering compiled [programID={0}]", m_ShaderEnvFiltering->GetProgramID());

    m_ShaderEnvIrradiance = new Shader("Shaders/Hazel/EnvironmentIrradiance.cs");
    Log::GetLogger()->info("SceneImGuizmo: m_ShaderEnvIrradiance compiled [programID={0}]", m_ShaderEnvIrradiance->GetProgramID());

    // M1911
    TextureInfo textureInfoM1911 = {};
    textureInfoM1911.albedo    = "Models/m1911/m1911_color.png";
    textureInfoM1911.normal    = "Models/m1911/m1911_normal.png";
    textureInfoM1911.metallic  = "Models/m1911/m1911_metalness.png";
    textureInfoM1911.roughness = "Models/m1911/m1911_roughness.png";
    textureInfoM1911.ao        = "Textures/plain.png";

    float materialSpecular  = 1.0f;
    float materialShininess = 256.0f;

    m_BaseMaterial = new Material(textureInfoM1911, materialSpecular, materialShininess);
    m_MeshAnimPBR = new Hazel::MeshAnimPBR("Models/m1911/m1911.fbx", m_ShaderHazelAnimPBR, m_BaseMaterial);

    m_M1911_Transform = glm::mat4(1.0f);

    Log::GetLogger()->info("-- END loading the animated PBR model M1911 --");
}

void SceneImGuizmo::SetupModels()
{
}

void SceneImGuizmo::SetupFramebuffers()
{
}

void SceneImGuizmo::Update(float timestep, Window& mainWindow)
{
    glm::vec3 u_AlbedoColor = glm::vec3(1.0f, 1.0f, 1.0f);
    float u_Roughness = 0.5f;

    struct Light
    {
        glm::vec3 Direction = LightManager::directionalLight.GetDirection();
        glm::vec3 Radiance = glm::vec3(0.0f, 0.0f, 0.0f);
        float Multiplier = 1.0f;
    } lights;

    m_ShaderHazelAnimPBR->Bind();
    m_ShaderHazelAnimPBR->setVec3("u_AlbedoColor", u_AlbedoColor);
    m_ShaderHazelAnimPBR->setFloat("u_Roughness", u_Roughness);
    m_ShaderHazelAnimPBR->setVec3("lights.Direction", lights.Direction);
    m_ShaderHazelAnimPBR->setVec3("lights.Radiance", lights.Radiance);
    m_ShaderHazelAnimPBR->setFloat("lights.Multiplier", lights.Multiplier);

    m_ShaderHazelAnimPBR->setFloat("u_AlbedoTexToggle",    m_AlbedoTexToggle ? 1.0f : 0.0f);
    m_ShaderHazelAnimPBR->setFloat("u_NormalTexToggle",    m_NormalTexToggle ? 1.0f : 0.0f);
    m_ShaderHazelAnimPBR->setFloat("u_MetalnessTexToggle", m_MetalnessTexToggle ? 1.0f : 0.0f);
    m_ShaderHazelAnimPBR->setFloat("u_RoughnessTexToggle", m_RoughnessTexToggle ? 1.0f : 0.0f);
    m_ShaderHazelAnimPBR->setFloat("u_EnvMapRotation", m_EnvMapRotation);

    if (m_BaseMaterial->GetTextureAlbedo()->IsLoaded())
        m_ShaderHazelAnimPBR->setInt("u_AlbedoTexture", m_BaseMaterial->GetTextureAlbedo()->GetID());

    if (m_BaseMaterial->GetTextureNormal()->IsLoaded())
        m_ShaderHazelAnimPBR->setInt("u_NormalTexture", m_BaseMaterial->GetTextureNormal()->GetID());

    if (m_BaseMaterial->GetTextureMetallic()->IsLoaded())
        m_ShaderHazelAnimPBR->setInt("u_MetalnessTexture", m_BaseMaterial->GetTextureMetallic()->GetID());

    if (m_BaseMaterial->GetTextureRoughness()->IsLoaded())
        m_ShaderHazelAnimPBR->setInt("u_RoughnessTexture", m_BaseMaterial->GetTextureRoughness()->GetID());

    m_ShaderHazelAnimPBR->setMat4("u_ViewProjectionMatrix", RendererBasic::GetProjectionMatrix() * m_CameraController->CalculateViewMatrix());
    m_ShaderHazelAnimPBR->setVec3("u_CameraPosition", m_Camera->GetPosition());

    m_ShaderHazelAnimPBR->setInt("u_EnvRadianceTex",   0 /* s_Data.SceneData.SceneEnvironment.RadianceMap */);
    m_ShaderHazelAnimPBR->setInt("u_EnvIrradianceTex", 0 /* s_Data.SceneData.SceneEnvironment.IrradianceMap */);
    m_ShaderHazelAnimPBR->setInt("u_BRDFLUTTexture",   0 /* s_Data.BRDFLUT */ );

    m_MeshAnimPBR->OnUpdate(timestep * 0.001f);
}

std::pair<TextureCubemapLite*, TextureCubemapLite*> SceneImGuizmo::CreateEnvironmentMap(const std::string& filepath)
{
    const uint32_t cubemapSize = 2048;
    const uint32_t irradianceMapSize = 32;

    m_EnvUnfiltered = new TextureCubemapLite(cubemapSize, cubemapSize);
    m_EnvEquirect = new Texture(filepath.c_str(), false);

    // HZ_CORE_ASSERT(envEquirect->GetFormat() == TextureFormat::Float16, "Texture is not HDR!");

    m_ShaderEquirectangularConversion->Bind();
    m_EnvEquirect->Bind();
    glBindImageTexture(0, m_EnvUnfiltered->GetID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glDispatchCompute(cubemapSize / 32, cubemapSize / 32, 6);
    glGenerateTextureMipmap(m_EnvUnfiltered->GetID());

    m_EnvFiltered = new TextureCubemapLite(cubemapSize, cubemapSize);

    glCopyImageSubData(m_EnvFiltered->GetID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
        m_EnvFiltered->GetID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
        m_EnvFiltered->GetWidth(), m_EnvFiltered->GetHeight(), 6);

    m_ShaderEnvFiltering->Bind();
    m_EnvFiltered->Bind(0);

    const float deltaRoughness = 1.0f / glm::max((float)(m_EnvFiltered->GetMipLevelCount() - 1.0f), 1.0f);
    for (unsigned int level = 1, size = cubemapSize / 2; level < m_EnvFiltered->GetMipLevelCount(); level++, size /= 2) // <= ?
    {
        const GLuint numGroups = glm::max((unsigned int)1, size / 32);
        glBindImageTexture(0, m_EnvFiltered->GetID(), level, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        glProgramUniform1f(m_ShaderEnvFiltering->GetProgramID(), 0, level * deltaRoughness);
        glDispatchCompute(numGroups, numGroups, 6);
    }

    m_IrradianceMap = new TextureCubemapLite(irradianceMapSize, irradianceMapSize);
    m_ShaderEnvIrradiance->Bind();
    m_EnvFiltered->Bind(0);
    glBindImageTexture(0, m_IrradianceMap->GetID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glDispatchCompute(m_IrradianceMap->GetWidth() / 32, m_IrradianceMap->GetHeight() / 32, 6);
    glGenerateTextureMipmap(m_IrradianceMap->GetID());

    return { m_EnvFiltered, m_IrradianceMap };
}

void SceneImGuizmo::UpdateImGui(float timestep, Window& mainWindow)
{
    glm::mat4 projectionMatrix = RendererBasic::GetProjectionMatrix();
    bool editTransformDecomposition = true;

    EditTransform(glm::value_ptr(m_CameraController->CalculateViewMatrix()),
        glm::value_ptr(projectionMatrix),
        glm::value_ptr(m_CubeTransform), editTransformDecomposition);
}

void SceneImGuizmo::EditTransform(const float* cameraView, float* cameraProjection, float* matrix, bool editTransformDecomposition)
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

float SceneImGuizmo::GetSnapValue()
{
    if (m_GizmoType == ImGuizmo::OPERATION::TRANSLATE) return 0.5f;
    else if (m_GizmoType == ImGuizmo::OPERATION::ROTATE) return 45.0f;
    else if (m_GizmoType == ImGuizmo::OPERATION::SCALE) return 0.5f;
    else return 0.0f;
}

void SceneImGuizmo::Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
    Shader* shaderMain = shaders["main"];

    glm::mat4 model = glm::mat4(1.0f);

    shaderMain->Bind();
    model = glm::mat4(1.0f);
    shaderMain->setMat4("model", model);
    ResourceManager::GetTexture("crate")->Bind(textureSlots["diffuse"]);
    ResourceManager::GetTexture("crateNormal")->Bind(textureSlots["normal"]);
    meshes["floor"]->Render();

    shaderMain->setMat4("model", m_CubeTransform);
    ResourceManager::GetTexture("crate")->Bind(textureSlots["diffuse"]);
    ResourceManager::GetTexture("crateNormal")->Bind(textureSlots["normal"]);
    meshes["cube"]->Render();

    // Render M1911
    m_MeshAnimPBR->m_VertexArray->Bind();
    auto& materials = m_MeshAnimPBR->GetMaterials();

    m_ShaderHazelAnimPBR->Bind();

    // Toggles
    m_ShaderHazelAnimPBR->setFloat("u_RadiancePrefilter",  1.0f);
    m_ShaderHazelAnimPBR->setFloat("u_AlbedoTexToggle",    1.0f);
    m_ShaderHazelAnimPBR->setFloat("u_NormalTexToggle",    1.0f);
    m_ShaderHazelAnimPBR->setFloat("u_MetalnessTexToggle", 1.0f);
    m_ShaderHazelAnimPBR->setFloat("u_RoughnessTexToggle", 1.0f);

    // m_BaseMaterial->BindTextures(1);
    m_BaseMaterial->GetTextureAlbedo()->Bind(m_SamplerSlots["albedo"]);
    m_ShaderHazelAnimPBR->setInt("u_AlbedoTexture", m_SamplerSlots["albedo"]);

    m_BaseMaterial->GetTextureNormal()->Bind(m_SamplerSlots["normal"]);
    m_ShaderHazelAnimPBR->setInt("u_NormalTexture", m_SamplerSlots["normal"]);

    m_BaseMaterial->GetTextureMetallic()->Bind(m_SamplerSlots["metalness"]);
    m_ShaderHazelAnimPBR->setInt("u_MetalnessTexture", m_SamplerSlots["metalness"]);

    m_BaseMaterial->GetTextureRoughness()->Bind(m_SamplerSlots["roughness"]);
    m_ShaderHazelAnimPBR->setInt("u_RoughnessTexture", m_SamplerSlots["roughness"]);

    m_EnvFiltered->Bind(m_SamplerSlots["env_radiance"]);
    m_ShaderHazelAnimPBR->setInt("u_EnvRadianceTex", m_SamplerSlots["env_radiance"]);

    m_IrradianceMap->Bind(m_SamplerSlots["env_irradiance"]);
    m_ShaderHazelAnimPBR->setInt("u_EnvIrradianceTex", m_SamplerSlots["env_irradiance"]);

    m_BRDF_LUT->Bind(m_SamplerSlots["BRDF_LUT"]);
    m_ShaderHazelAnimPBR->setInt("u_BRDFLUTTexture", m_SamplerSlots["BRDF_LUT"]);

    int submeshIndex = 0;
    for (Hazel::Submesh* submesh : m_MeshAnimPBR->GetSubmeshes())
    {
        // Material
        auto material = materials[submesh->MaterialIndex];
        m_ShaderHazelAnimPBR->Bind();
        
        for (size_t i = 0; i < m_MeshAnimPBR->m_BoneTransforms.size(); i++)
        {
            std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
            m_ShaderHazelAnimPBR->setMat4(uniformName, m_MeshAnimPBR->m_BoneTransforms[i]);
        }

        glm::mat4 transform = m_M1911_Transform * submesh->Transform;
        transform = glm::translate(transform, glm::vec3(0.0f, 25.0f, 0.0f));
        transform = glm::scale(transform, glm::vec3(20.0f));

        m_ShaderHazelAnimPBR->setMat4("u_Transform", transform);
        
        glEnable(GL_DEPTH_TEST);

        //  LOG_INFO("submeshIndex: {0}, submesh->IndexCount: {1}, submesh->BaseIndex: {2}, submesh->BaseVertex: {3}",
        //      submeshIndex, submesh->IndexCount, submesh->BaseIndex, submesh->BaseVertex);

        glDrawElementsBaseVertex(GL_TRIANGLES, submesh->IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh->BaseIndex), submesh->BaseVertex);

        submeshIndex++;
    }
}

SceneImGuizmo::~SceneImGuizmo()
{
    delete m_EnvUnfiltered;
    delete m_EnvEquirect;
    delete m_EnvFiltered;
    delete m_IrradianceMap;
    delete m_BRDF_LUT;
}
