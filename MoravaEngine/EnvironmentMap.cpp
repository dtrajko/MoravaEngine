#include "EnvironmentMap.h"

#include "Hazel/Renderer/RenderPass.h"
#include "Hazel/Renderer/HazelMaterial.h"
#include "Hazel/Renderer/VertexArray.h"
#include "Hazel/Renderer/Buffer.h"
#include "Hazel/Renderer/HazelRenderer.h"
#include "Hazel/Renderer/Renderer2D.h"
#include "Hazel/Scene/Components.h"
#include "Hazel/Scene/Entity.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "ImGuiWrapper.h"
#include "ImGuizmo.h"
#include "Framebuffer.h"
#include "RendererBasic.h"
#include "Log.h"
#include "Application.h"
#include "Util.h"
#include "Input.h"
#include "ResourceManager.h"
#include "Math.h"
#include "SceneHazelEnvMap.h"


EnvironmentMap::EnvironmentMap(const std::string& filepath, Scene* scene)
{
    m_SamplerSlots = new std::map<std::string, unsigned int>();

    //  // PBR texture inputs
    m_SamplerSlots->insert(std::make_pair("albedo", 1)); // uniform sampler2D u_AlbedoTexture
    m_SamplerSlots->insert(std::make_pair("normal", 2)); // uniform sampler2D u_NormalTexture
    m_SamplerSlots->insert(std::make_pair("metalness", 3)); // uniform sampler2D u_MetalnessTexture
    m_SamplerSlots->insert(std::make_pair("roughness", 4)); // uniform sampler2D u_RoughnessTexture
    m_SamplerSlots->insert(std::make_pair("ao", 5)); // uniform sampler2D u_AOTexture
    // Environment maps
    m_SamplerSlots->insert(std::make_pair("radiance", 6)); // uniform samplerCube u_EnvRadianceTex
    m_SamplerSlots->insert(std::make_pair("irradiance", 7)); // uniform samplerCube u_EnvIrradianceTex
    // BRDF LUT
    m_SamplerSlots->insert(std::make_pair("BRDF_LUT", 8)); // uniform sampler2D u_BRDFLUTTexture

    // Skybox.fs         - uniform samplerCube u_Texture;
    // SceneComposite.fs - uniform sampler2DMS u_Texture;
    m_SamplerSlots->insert(std::make_pair("u_Texture", 1));

    m_SceneRenderer = new Hazel::SceneRenderer(filepath, scene);

    m_SkyboxCube = new CubeSkybox();
    m_Quad = new Quad();

    SetSkybox(m_SceneRenderer->s_Data.SceneData.SceneEnvironment.RadianceMap);

    m_EditorCamera = new Hazel::EditorCamera(45.0f, 1.778f, 0.1f, 1000.0f);
    m_RuntimeCamera = new RuntimeCamera(scene->GetSettings().cameraPosition, scene->GetSettings().cameraStartYaw, scene->GetSettings().cameraStartPitch, 
        45.0f, 1.778f, scene->GetSettings().cameraMoveSpeed, 0.1f);
    m_ActiveCamera = m_RuntimeCamera; // m_RuntimeCamera m_EditorCamera;

    Init(); // requires a valid Camera reference

    m_CheckerboardTexture = Hazel::HazelTexture2D::Create("Textures/Hazel/Checkerboard.tga");

    m_SceneHierarchyPanel = new Hazel::SceneHierarchyPanel(scene);

    m_DisplayHazelGrid = true;
    m_DisplayBoundingBoxes = false;

    Scene::s_ImGuizmoTransform = nullptr; // &GetMeshEntity()->Transform();
    Scene::s_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE;
}

void EnvironmentMap::Init()
{
    Application::Get()->GetWindow()->SetEventCallback(HZ_BIND_EVENT_FN(EnvironmentMap::OnEvent));

    SetupShaders();

    bool isMultisample = false;

    FramebufferSpecification geoFramebufferSpec;
    geoFramebufferSpec.Width = 1280;
    geoFramebufferSpec.Height = 720;
    geoFramebufferSpec.attachmentType = AttachmentType::Texture;
    geoFramebufferSpec.attachmentFormat = AttachmentFormat::RGBA16F;
    geoFramebufferSpec.Samples = 8;
    geoFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

    isMultisample = geoFramebufferSpec.Samples > 1;

    Hazel::RenderPassSpecification geoRenderPassSpec;
    geoRenderPassSpec.TargetFramebuffer = new Framebuffer(geoFramebufferSpec);
    geoRenderPassSpec.TargetFramebuffer->CreateAttachment(geoFramebufferSpec);

    FramebufferSpecification geoFramebufferDepthSpec;
    geoFramebufferDepthSpec = geoFramebufferSpec;
    geoFramebufferDepthSpec.attachmentType = AttachmentType::Texture;
    geoFramebufferDepthSpec.attachmentFormat = AttachmentFormat::Depth_24_Stencil_8;

    geoRenderPassSpec.TargetFramebuffer->CreateAttachment(geoFramebufferDepthSpec);
    Log::GetLogger()->debug("Generating the GEO RenderPass framebuffer with AttachmentFormat::RGBA16F");
    geoRenderPassSpec.TargetFramebuffer->Generate(geoFramebufferSpec.Width, geoFramebufferSpec.Height);
    m_SceneRenderer->s_Data.GeoPass = Hazel::RenderPass::Create(geoRenderPassSpec);

    FramebufferSpecification compFramebufferSpec;
    compFramebufferSpec.Width = 1280;
    compFramebufferSpec.Height = 720;
    compFramebufferSpec.attachmentType = AttachmentType::Texture;
    compFramebufferSpec.attachmentFormat = AttachmentFormat::RGBA8;
    compFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };

    isMultisample = compFramebufferSpec.Samples > 1;

    Hazel::RenderPassSpecification compRenderPassSpec;
    compRenderPassSpec.TargetFramebuffer = new Framebuffer(compFramebufferSpec);
    compRenderPassSpec.TargetFramebuffer->CreateAttachment(compFramebufferSpec);
    compRenderPassSpec.TargetFramebuffer->CreateAttachmentDepth(compFramebufferSpec.Width, compFramebufferSpec.Height, isMultisample,
        AttachmentType::Renderbuffer, AttachmentFormat::Depth);
    Log::GetLogger()->debug("Generating the COMPOSITE RenderPass framebuffer with AttachmentFormat::RGBA8");
    compRenderPassSpec.TargetFramebuffer->Generate(compFramebufferSpec.Width, compFramebufferSpec.Height);
    m_SceneRenderer->s_Data.CompositePass = Hazel::RenderPass::Create(compRenderPassSpec);

    m_SceneRenderer->s_Data.BRDFLUT = Hazel::HazelTexture2D::Create("Textures/Hazel/BRDF_LUT.tga");

    SetupContextData();

    // Temporary code Hazel LIVE! #004
    Hazel::HazelRenderer::Init();

    bool depthTest = true;
    Hazel::Renderer2D::Init();
}

void EnvironmentMap::SetupContextData()
{
    // Setup default texture info
    m_TextureInfoDefault = {};
    m_TextureInfoDefault.albedo = "Textures/PBR/non_reflective/albedo.png";
    m_TextureInfoDefault.normal = "Textures/PBR/non_reflective/normal.png";
    m_TextureInfoDefault.metallic = "Textures/PBR/non_reflective/metallic.png";
    m_TextureInfoDefault.roughness = "Textures/PBR/non_reflective/roughness.png";
    m_TextureInfoDefault.ao = "Textures/PBR/non_reflective/ao.png";

    m_CameraEntity = CreateEntity("Camera");
    auto viewportWidth = m_ViewportBounds[1].x - m_ViewportBounds[0].x;
    auto viewportHeight = m_ViewportBounds[1].y - m_ViewportBounds[0].y;

    m_EditorCamera->SetProjectionType(Hazel::SceneCamera::ProjectionType::Perspective);
    m_RuntimeCamera->SetProjectionType(Hazel::SceneCamera::ProjectionType::Perspective);

    m_EditorCamera->SetViewportSize((float)Application::Get()->GetWindow()->GetWidth(), (float)Application::Get()->GetWindow()->GetHeight());
    m_RuntimeCamera->SetViewportSize((float)Application::Get()->GetWindow()->GetWidth(), (float)Application::Get()->GetWindow()->GetHeight());

    m_CameraEntity.AddComponent<Hazel::CameraComponent>(m_ActiveCamera);

    auto mapGenerator = CreateEntity("Map Generator");
    mapGenerator.AddComponent<Hazel::ScriptComponent>("Example.MapGenerator");

    m_DirectionalLightEntity = CreateEntity("Directional Light");
    auto& tc = m_DirectionalLightEntity.GetComponent<Hazel::TransformComponent>();
    tc.Rotation = m_SceneRenderer->s_Data.SceneData.ActiveLight.Direction;
}

Hazel::Entity EnvironmentMap::LoadEntity(std::string fullPath)
{
    std::string fileName = Util::GetFileNameFromFullPath(fullPath);
    std::string fileNameNoExt = Util::StripExtensionFromFileName(fileName);

    // A bit unfortunate hard-coded mesh type selection by model name
    // TODO: detect automatically mesh type in HazelMesh constructor
    bool isAnimated;
    if (fileNameNoExt == "m1911") {
        isAnimated = true;
        m_ShaderHazelPBR = m_ShaderHazelPBR_Anim;
    }
    else {
        isAnimated = false;
        m_ShaderHazelPBR = m_ShaderHazelPBR_Static;
    }

    Log::GetLogger()->debug("EnvironmentMap::LoadMesh: fullPath '{0}' fileName '{1}' fileNameNoExt '{2}'", fullPath, fileName, fileNameNoExt);

    Hazel::HazelMesh* mesh = new Hazel::HazelMesh(fullPath, m_ShaderHazelPBR, nullptr, isAnimated);

    ((Hazel::HazelMesh*)mesh)->SetTimeMultiplier(1.0f);

    // m_SceneRenderer->s_Data.DrawList.clear(); // doesn't work for multiple meshes on the scene
    Hazel::SceneRendererData::DrawCommand drawCommand;

    drawCommand.Name = fileNameNoExt;
    drawCommand.Mesh = mesh;
    drawCommand.Transform = glm::mat4(1.0f);

    m_SceneRenderer->s_Data.DrawList.push_back(drawCommand);

    // m_MeshEntity: NoECS version
    Hazel::Entity meshEntity = CreateEntity(drawCommand.Name);
    meshEntity.AddComponent<Hazel::MeshComponent>(Hazel::Ref<Hazel::HazelMesh>(mesh));
    meshEntity.AddComponent<Hazel::ScriptComponent>("Example.Script");

    SubmitEntity(meshEntity);
    LoadEnvMapMaterials(mesh);

    return meshEntity;
}

void EnvironmentMap::LoadEnvMapMaterials(Mesh* mesh)
{
    //  for (auto material : m_EnvMapMaterials) {
    //      delete material.second;
    //  }
    //  
    //  m_EnvMapMaterials.clear();

    std::vector<Hazel::Submesh>& submeshes = ((Hazel::HazelMesh*)mesh)->GetSubmeshes();

    for (Hazel::Submesh& submesh : submeshes)
    {
        std::string materialName = Hazel::HazelMesh::GetSubmeshMaterialName(mesh, submesh);

        Log::GetLogger()->debug("EnvironmentMap::LoadEnvMapMaterials materialName = '{0}'", materialName);

        if (m_EnvMapMaterials.contains(materialName)) {
            continue;
        }

        EnvMapMaterial* envMapMaterial = CreateDefaultMaterial(materialName);
        m_EnvMapMaterials.insert(std::make_pair(materialName, envMapMaterial));
    }

    //  // If no submeshes, add a default material for entity
    //  if (submeshes.empty())
    //  {
    //      EnvMapMaterial* envMapMaterial = CreateDefaultMaterial(meshName);
    //      m_EnvMapMaterials.insert(std::make_pair(meshName, envMapMaterial));
    //  }

    for (auto& material : m_EnvMapMaterials)
    {
        Log::GetLogger()->debug("EnvironmentMap::LoadEnvMapMaterials material name: '{0}'", material.first);
    }
}

EnvMapMaterial* EnvironmentMap::CreateDefaultMaterial(std::string materialName)
{
    EnvMapMaterial* envMapMaterial = new EnvMapMaterial();

    TextureInfo textureInfo;
    if (m_TextureInfo.contains(materialName)) {
        textureInfo = m_TextureInfo.at(materialName);
    }
    else {
        textureInfo = m_TextureInfoDefault;
    }

    // Load Hazel/Renderer/HazelTexture
    envMapMaterial->GetAlbedoInput().TextureMap = ResourceManager::LoadHazelTexture2D(textureInfo.albedo);
    envMapMaterial->GetAlbedoInput().UseTexture = true;
    envMapMaterial->GetNormalInput().TextureMap = ResourceManager::LoadHazelTexture2D(textureInfo.normal);
    envMapMaterial->GetNormalInput().UseTexture = true;
    envMapMaterial->GetMetalnessInput().TextureMap = ResourceManager::LoadHazelTexture2D(textureInfo.metallic);
    envMapMaterial->GetMetalnessInput().UseTexture = true;
    envMapMaterial->GetRoughnessInput().TextureMap = ResourceManager::LoadHazelTexture2D(textureInfo.roughness);
    envMapMaterial->GetRoughnessInput().UseTexture = true;
    envMapMaterial->GetAOInput().TextureMap = ResourceManager::LoadHazelTexture2D(textureInfo.ao);
    envMapMaterial->GetAOInput().UseTexture = true;

    return envMapMaterial;
}

void EnvironmentMap::SetupShaders()
{
    m_ShaderHazelPBR_Static = new Shader("Shaders/Hazel/HazelPBR_Static.vs", "Shaders/Hazel/HazelPBR.fs");
    Log::GetLogger()->info("EnvironmentMap: m_ShaderHazelPBR_Static compiled [programID={0}]", m_ShaderHazelPBR_Static->GetProgramID());

    m_ShaderHazelPBR_Anim = new Shader("Shaders/Hazel/HazelPBR_Anim.vs", "Shaders/Hazel/HazelPBR.fs");
    Log::GetLogger()->info("EnvironmentMap: m_ShaderHazelPBR_Anim compiled [programID={0}]", m_ShaderHazelPBR_Anim->GetProgramID());

    m_ShaderRenderer2D_Line = new Shader("Shaders/Hazel/Renderer2D_Line.vs", "Shaders/Hazel/Renderer2D_Line.fs");
    Log::GetLogger()->info("EnvironmentMap: m_ShaderRenderer2D_Line compiled [programID={0}]", m_ShaderRenderer2D_Line->GetProgramID());

    ResourceManager::AddShader("Hazel/HazelPBR_Static", m_ShaderHazelPBR_Static);
    ResourceManager::AddShader("Hazel/HazelPBR_Anim", m_ShaderHazelPBR_Anim);
    ResourceManager::AddShader("Hazel/Renderer2D_Line", m_ShaderRenderer2D_Line);
}

void EnvironmentMap::UpdateUniforms()
{
    /**** BEGIN Shaders/Hazel/SceneComposite ****/
    m_SceneRenderer->GetShaderComposite()->Bind();
    m_SceneRenderer->GetShaderComposite()->setInt("u_Texture", m_SamplerSlots->at("u_Texture"));
    m_SceneRenderer->GetShaderComposite()->setFloat("u_Exposure", m_ActiveCamera->GetExposure());
    /**** END Shaders/Hazel/SceneComposite ****/

    /**** BEGIN Shaders/Hazel/Skybox ****/
    m_SceneRenderer->GetShaderSkybox()->Bind();
    m_SceneRenderer->GetShaderSkybox()->setInt("u_Texture", m_SamplerSlots->at("u_Texture"));
    m_SceneRenderer->GetShaderSkybox()->setFloat("u_TextureLod", ((Hazel::HazelScene*)m_SceneRenderer->s_Data.ActiveScene)->GetSkyboxLOD());
    // apply exposure to Shaders/Hazel/Skybox, considering that Shaders/Hazel/SceneComposite is not yet enabled
    m_SceneRenderer->GetShaderSkybox()->setFloat("u_Exposure", m_ActiveCamera->GetExposure() * m_SkyboxExposureFactor); // originally used in Shaders/Hazel/SceneComposite
    /**** END Shaders/Hazel/Skybox ****/
}

void EnvironmentMap::UpdateShaderPBRUniforms(Shader* shaderHazelPBR, EnvMapMaterial* envMapMaterial)
{
    /**** BEGIN Shaders/Hazel/HazelPBR_Anim / Shaders/Hazel/HazelPBR_Static ***/

    shaderHazelPBR->Bind();

    shaderHazelPBR->setInt("u_AlbedoTexture", m_SamplerSlots->at("albedo"));
    shaderHazelPBR->setInt("u_NormalTexture", m_SamplerSlots->at("normal"));
    shaderHazelPBR->setInt("u_MetalnessTexture", m_SamplerSlots->at("metalness"));
    shaderHazelPBR->setInt("u_RoughnessTexture", m_SamplerSlots->at("roughness"));
    shaderHazelPBR->setInt("u_AOTexture", m_SamplerSlots->at("ao"));

    shaderHazelPBR->setVec3("u_AlbedoColor", envMapMaterial->GetAlbedoInput().Color);
    shaderHazelPBR->setFloat("u_Metalness", envMapMaterial->GetMetalnessInput().Value);
    shaderHazelPBR->setFloat("u_Roughness", envMapMaterial->GetRoughnessInput().Value);
    shaderHazelPBR->setFloat("u_AO", envMapMaterial->GetAOInput().Value);

    shaderHazelPBR->setFloat("u_EnvMapRotation", m_EnvMapRotation);

    shaderHazelPBR->setFloat("u_RadiancePrefilter", m_RadiancePrefilter ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_AlbedoTexToggle", envMapMaterial->GetAlbedoInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_NormalTexToggle", envMapMaterial->GetNormalInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_MetalnessTexToggle", envMapMaterial->GetMetalnessInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_RoughnessTexToggle", envMapMaterial->GetRoughnessInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_AOTexToggle", envMapMaterial->GetAOInput().UseTexture ? 1.0f : 0.0f);
    // apply exposure to Shaders/Hazel/HazelPBR_Anim, considering that Shaders/Hazel/SceneComposite is not yet enabled
    shaderHazelPBR->setFloat("u_Exposure", m_ActiveCamera->GetExposure()); // originally used in Shaders/Hazel/SceneComposite

    shaderHazelPBR->setFloat("u_TilingFactor", envMapMaterial->GetTilingFactor());

    shaderHazelPBR->setMat4("u_ViewProjectionMatrix", m_ActiveCamera->GetViewProjection());
    shaderHazelPBR->setVec3("u_CameraPosition", m_ActiveCamera->GetPosition());

    // Environment (TODO: don't do this per mesh)
    shaderHazelPBR->setInt("u_EnvRadianceTex", m_SamplerSlots->at("radiance"));
    shaderHazelPBR->setInt("u_EnvIrradianceTex", m_SamplerSlots->at("irradiance"));
    shaderHazelPBR->setInt("u_BRDFLUTTexture", m_SamplerSlots->at("BRDF_LUT"));

    // Set lights (TODO: move to light environment and don't do per mesh)
    shaderHazelPBR->setVec3("lights.Direction", m_SceneRenderer->s_Data.SceneData.ActiveLight.Direction);
    shaderHazelPBR->setVec3("lights.Radiance", m_SceneRenderer->s_Data.SceneData.ActiveLight.Radiance);
    shaderHazelPBR->setFloat("lights.Multiplier", m_SceneRenderer->s_Data.SceneData.ActiveLight.Multiplier);

    shaderHazelPBR->Validate();

    /**** END Shaders/Hazel/HazelPBR_Anim / Shaders/Hazel/HazelPBR_Static ***/
}

void EnvironmentMap::SetSkybox(Hazel::Ref<Hazel::HazelTextureCube> skybox)
{
    m_SkyboxTexture = skybox;
    m_SkyboxTexture->Bind(m_SamplerSlots->at("u_Texture"));
}

EnvironmentMap::~EnvironmentMap()
{
    for (auto material : m_EnvMapMaterials) {
        delete material.second;
    }

    m_EnvMapMaterials.clear();

    delete m_SceneRenderer;

    delete m_RuntimeCamera;
    delete m_EditorCamera;
}

Hazel::Entity EnvironmentMap::CreateEntity(const std::string& name)
{
    // Both NoECS and ECS
    Hazel::Entity entity = ((Hazel::HazelScene*)m_SceneRenderer->s_Data.ActiveScene)->CreateEntity(name, (const Hazel::HazelScene&)m_SceneRenderer->s_Data.ActiveScene);

    return entity;
}

void EnvironmentMap::OnUpdate(Scene* scene, float timestep)
{
    switch (m_SceneState)
    {
    case SceneState::Edit:
        // m_EditorCamera->OnUpdate(timestep);
        // m_ActiveScene->OnRenderEditor(timestep, m_EditorCamera);
        break;
    case SceneState::Play:
        // m_EditorCamera->OnUpdate(timestep);
        // m_ActiveScene->OnUpdate(timestep);
        // m_ActiveScene->OnRenderRuntime(timestep);
        break;
    case SceneState::Pause:
        // m_EditorCamera->OnUpdate(timestep);
        // m_ActiveScene->OnRenderRuntime(timestep);
        break;
    }

    // CameraSyncECS(); TODO

    auto& tc = m_DirectionalLightEntity.GetComponent<Hazel::TransformComponent>();
    m_SceneRenderer->s_Data.SceneData.ActiveLight.Direction = glm::eulerAngles(glm::quat(tc.Rotation));

    OnUpdateEditor(scene, timestep);
    // OnUpdateRuntime(scene, timestep);
}

void EnvironmentMap::OnUpdateEditor(Scene* scene, float timestep)
{
    m_SceneRenderer->s_Data.ActiveScene = scene;

    m_SceneRenderer->BeginScene((Scene*)m_SceneRenderer->s_Data.ActiveScene);

    UpdateUniforms();

    // Update HazelMesh List
    auto meshEntities = m_SceneRenderer->s_Data.ActiveScene->GetAllEntitiesWith<Hazel::MeshComponent>();
    for (auto entt : meshEntities)
    {
        Hazel::Entity entity{ entt, m_SceneRenderer->s_Data.ActiveScene };
        Hazel::Ref<Hazel::HazelMesh> mesh = entity.GetComponent<Hazel::MeshComponent>().Mesh;
        mesh->OnUpdate(timestep, false);
    }

    m_ActiveCamera->OnUpdate(timestep);

    Scene::s_ImGuizmoTransform = m_CurrentlySelectedTransform; // moved from SceneHazelEnvMap

    m_ViewportWidth = m_ViewportBounds[1].x - m_ViewportBounds[0].x;
    m_ViewportHeight = m_ViewportBounds[1].y - m_ViewportBounds[0].y;

    if (m_ViewportWidth > 0.0f && m_ViewportHeight > 0.0f) {
        m_ActiveCamera->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
    }
}

void EnvironmentMap::OnUpdateRuntime(Scene* scene, float timestep)
{
    m_SceneRenderer->s_Data.ActiveScene = scene;

    m_SceneRenderer->BeginScene((Scene*)m_SceneRenderer->s_Data.ActiveScene);

    UpdateUniforms();

    // Update HazelMesh List
    auto meshEntities = m_SceneRenderer->s_Data.ActiveScene->GetAllEntitiesWith<Hazel::MeshComponent>();
    for (auto entt : meshEntities)
    {
        Hazel::Entity entity{ entt, m_SceneRenderer->s_Data.ActiveScene };
        Hazel::Ref<Hazel::HazelMesh> mesh = entity.GetComponent<Hazel::MeshComponent>().Mesh;
        mesh->OnUpdate(timestep, false);
    }

    Scene::s_ImGuizmoTransform = m_CurrentlySelectedTransform; // moved from SceneHazelEnvMap

    m_ViewportWidth = m_ViewportBounds[1].x - m_ViewportBounds[0].x;
    m_ViewportHeight = m_ViewportBounds[1].y - m_ViewportBounds[0].y;

    if (m_ViewportWidth > 0.0f && m_ViewportHeight > 0.0f) {
        m_ActiveCamera->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
    }
}

void EnvironmentMap::OnScenePlay()
{
    EntitySelection::s_SelectionContext.clear();

    m_SceneState = SceneState::Play;

    m_RuntimeScene = Hazel::Ref<Hazel::HazelScene>::Create();
    m_EditorScene->CopyTo(m_RuntimeScene);

    m_ActiveScene = m_RuntimeScene;

    m_ActiveScene->OnRuntimeStart();
}

void EnvironmentMap::OnSceneStop()
{
    m_ActiveScene->OnRuntimeStop();
    m_SceneState = SceneState::Edit;
    m_ActiveScene = m_EditorScene;

    // Unload runtime scene
    m_RuntimeScene = nullptr;
}

void EnvironmentMap::CameraSyncECS()
{
    glm::vec3 cameraPosition = m_CameraEntity.GetComponent<Hazel::TransformComponent>().Translation;
    m_ActiveCamera->SetPosition(cameraPosition);
}

void EnvironmentMap::UpdateImGuizmo(Window* mainWindow)
{
    // BEGIN ImGuizmo

    // ImGizmo switching modes
    if (Input::IsKeyPressed(Key::D1))
        Scene::s_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE;

    if (Input::IsKeyPressed(Key::D2))
        Scene::s_ImGuizmoType = ImGuizmo::OPERATION::ROTATE;

    if (Input::IsKeyPressed(Key::D3))
        Scene::s_ImGuizmoType = ImGuizmo::OPERATION::SCALE;

    if (Input::IsKeyPressed(Key::D4))
        Scene::s_ImGuizmoType = -1;

    // Dirty fix: m_SelectionContext not decremented when mesh entity is removed from the scene
    size_t selectionContextSize = EntitySelection::s_SelectionContext.size();
    auto meshEntities = m_SceneRenderer->s_Data.ActiveScene->GetAllEntitiesWith<Hazel::MeshComponent>();
    if (selectionContextSize > meshEntities.size()) {
        selectionContextSize = meshEntities.size();
    }

    // ImGuizmo
    if (Scene::s_ImGuizmoType != -1 && EntitySelection::s_SelectionContext.size())
    {
        float rw = (float)ImGui::GetWindowWidth();
        float rh = (float)ImGui::GetWindowHeight();
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, rw, rh);

        auto& selectedSubmesh = EntitySelection::s_SelectionContext[0];

        // Entity transform
        auto& tc = selectedSubmesh.Entity.GetComponent<Hazel::TransformComponent>();
        glm::mat4 entityTransform = tc.GetTransform();

        // Snapping
        bool snap = Input::IsKeyPressed(Key::LeftControl);
        float snapValue = 1.0f; // Snap to 0.5m for translation/scale
        // Snap to 45 degrees for rotation
        if (Scene::s_ImGuizmoType == ImGuizmo::OPERATION::ROTATE) {
            snapValue = 45.0f;
        }

        float snapValues[3] = { snapValue, snapValue, snapValue };

        if (m_SelectionMode == SelectionMode::Entity || !selectedSubmesh.Mesh)
        {
            ImGuizmo::Manipulate(
                glm::value_ptr(m_ActiveCamera->GetViewMatrix()),
                glm::value_ptr(m_ActiveCamera->GetProjection()),
                (ImGuizmo::OPERATION)Scene::s_ImGuizmoType,
                ImGuizmo::LOCAL,
                glm::value_ptr(entityTransform),
                nullptr,
                snap ? snapValues : nullptr);

            if (ImGuizmo::IsUsing())
            {
                glm::vec3 translation, rotation, scale;
                Math::DecomposeTransform(entityTransform, translation, rotation, scale);

                glm::vec3 deltaRotation = rotation - tc.Rotation;
                tc.Translation = translation;
                tc.Rotation += deltaRotation;
                tc.Scale = scale;
            }
        }
        else if (m_SelectionMode == SelectionMode::SubMesh)
        {
            auto aabb = selectedSubmesh.Mesh->BoundingBox;

            glm::vec3 aabbCenterOffset = glm::vec3(
                aabb.Min.x + ((aabb.Max.x - aabb.Min.x) / 2.0f),
                aabb.Min.y + ((aabb.Max.y - aabb.Min.y) / 2.0f),
                aabb.Min.z + ((aabb.Max.z - aabb.Min.z) / 2.0f)
            );

            glm::mat4 submeshTransform = selectedSubmesh.Mesh->Transform;
            submeshTransform = glm::translate(submeshTransform, aabbCenterOffset);
            glm::mat4 transformBase = entityTransform * submeshTransform;

            ImGuizmo::Manipulate(
                glm::value_ptr(m_ActiveCamera->GetViewMatrix()),
                glm::value_ptr(m_ActiveCamera->GetProjection()),
                (ImGuizmo::OPERATION)Scene::s_ImGuizmoType,
                ImGuizmo::LOCAL,
                glm::value_ptr(transformBase),
                nullptr,
                snap ? snapValues : nullptr);

            if (ImGuizmo::IsUsing())
            {
                submeshTransform = glm::inverse(entityTransform) * transformBase;
                submeshTransform = glm::translate(submeshTransform, -aabbCenterOffset);
                selectedSubmesh.Mesh->Transform = submeshTransform;
            }
        }
    }
    // END ImGuizmo
}

void EnvironmentMap::SubmitEntity(Hazel::Entity entity)
{
    auto mesh = entity.GetComponent<Hazel::MeshComponent>().Mesh;
    if (!mesh) {
        return;
    }

    auto& transform = entity.GetComponent<Hazel::TransformComponent>().GetTransform();

    auto name = entity.GetComponent<Hazel::TagComponent>().Tag;
    m_SceneRenderer->s_Data.DrawList.push_back({ name, mesh.Raw(), entity.GetMaterial(), transform });
}

Ref<Hazel::Entity> EnvironmentMap::GetMeshEntity()
{
    Ref<Hazel::Entity> meshEntity;
    auto meshEntities = m_SceneRenderer->s_Data.ActiveScene->GetAllEntitiesWith<Hazel::MeshComponent>();
    if (meshEntities.size()) {
        for (auto entt : meshEntities)
        {
            meshEntity = CreateRef<Hazel::Entity>(entt, m_SceneRenderer->s_Data.ActiveScene);
        }
        return meshEntity;
    }
    return nullptr;
}

float& EnvironmentMap::GetSkyboxLOD()
{
    return ((Hazel::HazelScene*)m_SceneRenderer->s_Data.ActiveScene)->GetSkyboxLOD();
}

void EnvironmentMap::SetViewportBounds(glm::vec2* viewportBounds)
{
    m_ViewportBounds[0] = viewportBounds[0];
    m_ViewportBounds[1] = viewportBounds[1];
}

void EnvironmentMap::SetSkyboxLOD(float LOD)
{
    ((Hazel::HazelScene*)m_SceneRenderer->s_Data.ActiveScene)->SetSkyboxLOD(LOD);
}

void EnvironmentMap::DrawIndexed(uint32_t count, Hazel::PrimitiveType type, bool depthTest)
{
    if (!depthTest)
        glDisable(GL_DEPTH_TEST);

    GLenum glPrimitiveType = 0;
    switch (type)
    {
    case Hazel::PrimitiveType::Triangles:
        glPrimitiveType = GL_TRIANGLES;
        break;
    case Hazel::PrimitiveType::Lines:
        glPrimitiveType = GL_LINES;
        break;
    }

    glDrawElements(glPrimitiveType, count, GL_UNSIGNED_INT, nullptr);

    if (!depthTest)
        glEnable(GL_DEPTH_TEST);
}

void EnvironmentMap::OnImGuiRender()
{
    uint32_t id = 0;
    auto meshEntities = m_SceneRenderer->s_Data.ActiveScene->GetAllEntitiesWith<Hazel::MeshComponent>();
    for (auto entt : meshEntities)
    {
        Hazel::Entity entity = { entt, m_SceneRenderer->s_Data.ActiveScene };
        auto mesh = entity.GetComponent<Hazel::MeshComponent>().Mesh;
        mesh->OnImGuiRender(++id);
    }

    ImGui::Begin("Switch State");
    {
        const char* label = m_ActiveCamera == m_EditorCamera ? "EDITOR [ Editor Camera ]" : "RUNTIME [ Runtime Camera ]";
        if (ImGui::Button(label))
        {
            m_ActiveCamera = m_ActiveCamera == m_EditorCamera ? (Hazel::HazelCamera*)m_RuntimeCamera : (Hazel::HazelCamera*)m_EditorCamera;
        }
    }
    ImGui::End();

    ImGui::Begin("Transform");
    {
        if (EntitySelection::s_SelectionContext.size())
        {
            glm::mat4 transformImGui;
            glm::mat4 submeshTransform;
            glm::vec3 aabbCenterOffset;

            auto& selectedSubmesh = EntitySelection::s_SelectionContext[0];

            // Entity transform
            auto& tc = selectedSubmesh.Entity.GetComponent<Hazel::TransformComponent>();
            glm::mat4 entityTransform = tc.GetTransform();

            if (m_SelectionMode == SelectionMode::Entity || !selectedSubmesh.Mesh)
            {
                transformImGui = entityTransform;
            }
            else if (m_SelectionMode == SelectionMode::SubMesh)
            {
                auto aabb = selectedSubmesh.Mesh->BoundingBox;

                aabbCenterOffset = glm::vec3(
                    aabb.Min.x + ((aabb.Max.x - aabb.Min.x) / 2.0f),
                    aabb.Min.y + ((aabb.Max.y - aabb.Min.y) / 2.0f),
                    aabb.Min.z + ((aabb.Max.z - aabb.Min.z) / 2.0f)
                );

                submeshTransform = selectedSubmesh.Mesh->Transform;
                submeshTransform = glm::translate(submeshTransform, aabbCenterOffset);
                transformImGui = entityTransform * submeshTransform;
            }

            glm::vec3 translation, rotationRadians, scale;
            Math::DecomposeTransform(transformImGui, translation, rotationRadians, scale);
            glm::vec3 rotationDegrees = glm::degrees(rotationRadians);

            bool isTranslationChanged = ImGuiWrapper::DrawVec3Control("Translation", translation, 0.0f, 100.0f);
            bool isRotationChanged = ImGuiWrapper::DrawVec3Control("Rotation", rotationDegrees, 0.0f, 100.0f);
            bool isScaleChanged = ImGuiWrapper::DrawVec3Control("Scale", scale, 1.0f, 100.0f);

            if (isTranslationChanged || isRotationChanged || isScaleChanged)
            {
                rotationRadians = glm::radians(rotationDegrees);

                if (m_SelectionMode == SelectionMode::Entity || !selectedSubmesh.Mesh)
                {
                    glm::vec3 deltaRotation = rotationRadians - tc.Rotation;
                    tc.Translation = translation;
                    tc.Rotation += deltaRotation;
                    tc.Scale = scale;
                }
                else if (m_SelectionMode == SelectionMode::SubMesh)
                {
                    submeshTransform = glm::inverse(entityTransform) * transformImGui;
                    submeshTransform = glm::translate(submeshTransform, -aabbCenterOffset);
                    selectedSubmesh.Mesh->Transform = submeshTransform;
                }
            }
        }
    }
    ImGui::End();

    ImGui::Begin("Camera");
    {
        if (ImGui::CollapsingHeader("Display Info", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
        {
            char buffer[100];
            sprintf(buffer, "Pitch         %.2f", m_ActiveCamera->GetPitch());
            ImGui::Text(buffer);
            sprintf(buffer, "Yaw           %.2f", m_ActiveCamera->GetYaw());
            ImGui::Text(buffer);
            sprintf(buffer, "FOV           %.2f", glm::degrees(m_ActiveCamera->GetPerspectiveVerticalFOV()));
            ImGui::Text(buffer);
            sprintf(buffer, "Aspect Ratio  %.2f", glm::degrees(m_ActiveCamera->GetAspectRatio()));
            ImGui::Text(buffer);
            sprintf(buffer, "Position    X %.2f Y %.2f Z %.2f", m_ActiveCamera->GetPosition().x, m_ActiveCamera->GetPosition().y, m_ActiveCamera->GetPosition().z);
            ImGui::Text(buffer);
            sprintf(buffer, "Direction   X %.2f Y %.2f Z %.2f", m_ActiveCamera->GetDirection().x, m_ActiveCamera->GetDirection().y, m_ActiveCamera->GetDirection().z);
            ImGui::Text(buffer);
            sprintf(buffer, "Front       X %.2f Y %.2f Z %.2f", m_ActiveCamera->GetFront().x, m_ActiveCamera->GetFront().y, m_ActiveCamera->GetFront().z);
            ImGui::Text(buffer);
            sprintf(buffer, "Up          X %.2f Y %.2f Z %.2f", m_ActiveCamera->GetUp().x, m_ActiveCamera->GetUp().y, m_ActiveCamera->GetUp().z);
            ImGui::Text(buffer);
            sprintf(buffer, "Right       X %.2f Y %.2f Z %.2f", m_ActiveCamera->GetRight().x, m_ActiveCamera->GetRight().y, m_ActiveCamera->GetRight().z);
            ImGui::Text(buffer);
        }
    }
    ImGui::End();

    /**** BEGIN Environment Map Settings ****/
    ImGui::Begin("Environment Map Settings");
    {
        if (ImGui::CollapsingHeader("Display Info", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
        {
            {
                if (ImGui::Button("Load Environment Map"))
                {
                    std::string filename = Application::Get()->OpenFile("*.hdr");
                    if (filename != "") {
                        m_SceneRenderer->SetEnvironment(m_SceneRenderer->Load(filename));
                    }
                }

                float skyboxLOD = GetSkyboxLOD();
                if (ImGui::SliderFloat("Skybox LOD", &skyboxLOD, 0.0f, 2.0f))
                {
                    SetSkyboxLOD(skyboxLOD);
                }

                ImGui::Columns(2);
                ImGui::AlignTextToFramePadding();

                Hazel::HazelLight light = m_SceneRenderer->GetLight();
                Hazel::HazelLight lightPrev = light;

                ImGuiWrapper::Property("Light Direction", light.Direction, -180.0f, 180.0f);
                ImGuiWrapper::Property("Light Radiance", light.Radiance, PropertyFlag::ColorProperty);
                ImGuiWrapper::Property("Light Multiplier", light.Multiplier, 0.0f, 5.0f);
                ImGuiWrapper::Property("Exposure", m_ActiveCamera->GetExposure(), 0.0f, 40.0f);
                ImGuiWrapper::Property("Skybox Exposure Factor", m_SkyboxExposureFactor, 0.0f, 10.0f);
                ImGuiWrapper::Property("Radiance Prefiltering", m_RadiancePrefilter);
                ImGuiWrapper::Property("Env Map Rotation", m_EnvMapRotation, -360.0f, 360.0f);

                m_SceneRenderer->SetLight(light);

                if (light.Direction != lightPrev.Direction) {
                    auto& tc = m_DirectionalLightEntity.GetComponent<Hazel::TransformComponent>();
                    tc.Rotation = glm::eulerAngles(glm::quat(glm::radians(light.Direction)));
                    lightPrev = light;
                }

                ImGui::Columns(1);
            }

            ImGui::Separator();

            {
                ImGui::Text("Mesh");

                Ref<Hazel::Entity> meshEntity = nullptr;
                std::string meshFullPath = "None";

                std::string fileName = Util::GetFileNameFromFullPath(meshFullPath);
                ImGui::Text(fileName.c_str()); ImGui::SameLine();
                if (ImGui::Button("...##Mesh"))
                {
                    std::string fullPath = Application::Get()->OpenFile("");
                    if (fullPath != "")
                    {
                        Hazel::Entity entity = LoadEntity(fullPath);
                    }
                }

                auto meshEntities = m_SceneRenderer->s_Data.ActiveScene->GetAllEntitiesWith<Hazel::MeshComponent>();
                if (meshEntities.size())
                {
                    meshEntity = GetMeshEntity();
                    Hazel::Ref<Hazel::HazelMesh> meshAnimPBR = meshEntity->GetComponent<Hazel::MeshComponent>().Mesh;
                    ImGui::Checkbox("Is Animated", &meshAnimPBR->IsAnimated());
                }
            }

            ImGui::Separator();
        }
    }
    ImGui::End();
    /**** END Environment Map Scene Settings ****/

    ImGui::Begin("EnvMap Materials");
    {
        for (auto& material : m_EnvMapMaterials)
        {
            std::string materialName = material.first;

            // Material section
            if (ImGui::CollapsingHeader(materialName.c_str(), nullptr /*, ImGuiTreeNodeFlags_DefaultOpen */ ))
            {
                // BEGIN PBR Textures
                ImGui::Indent(10.0f);
                {
                    {
                        // Tiling Factor
                        ImGui::SliderFloat("Tiling Factor", &material.second->GetTilingFactor(), 0.0f, 20.0f);

                        // Albedo
                        std::string textureLabel = materialName + " Albedo";
                        if (ImGui::CollapsingHeader(textureLabel.c_str(), nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                            ImGui::Image(material.second->GetAlbedoInput().TextureMap ?
                                (void*)(intptr_t)material.second->GetAlbedoInput().TextureMap->GetID() :
                                (void*)(intptr_t)m_CheckerboardTexture->GetID(), ImVec2(64, 64));
                            ImGui::PopStyleVar();
                            if (ImGui::IsItemHovered())
                            {
                                if (material.second->GetAlbedoInput().TextureMap)
                                {
                                    ImGui::BeginTooltip();
                                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                    ImGui::TextUnformatted(material.second->GetAlbedoInput().TextureMap->GetPath().c_str());
                                    ImGui::PopTextWrapPos();
                                    ImGui::Image((void*)(intptr_t)material.second->GetAlbedoInput().TextureMap->GetID(), ImVec2(384, 384));
                                    ImGui::EndTooltip();
                                }
                                if (ImGui::IsItemClicked())
                                {
                                    std::string filename = Application::Get()->OpenFile("");
                                    if (filename != "")
                                        material.second->GetAlbedoInput().TextureMap = Hazel::HazelTexture2D::Create(filename, material.second->GetAlbedoInput().SRGB);
                                }
                            }
                            ImGui::SameLine();
                            ImGui::BeginGroup();

                            std::string checkboxLabel = "Use##" + materialName + "AlbedoMap";
                            ImGui::Checkbox(checkboxLabel.c_str(), &material.second->GetAlbedoInput().UseTexture);

                            std::string checkboxLabelSRGB = "sRGB##" + materialName + "AlbedoMap";
                            if (ImGui::Checkbox(checkboxLabelSRGB.c_str(), &material.second->GetAlbedoInput().SRGB))
                            {
                                if (material.second->GetAlbedoInput().TextureMap)
                                    material.second->GetAlbedoInput().TextureMap = Hazel::HazelTexture2D::Create(
                                        material.second->GetAlbedoInput().TextureMap->GetPath(),
                                        material.second->GetAlbedoInput().SRGB);
                            }
                            ImGui::EndGroup();
                            ImGui::SameLine();
                            std::string colorLabel = "Color##" + materialName + "Albedo";
                            ImGui::ColorEdit3(colorLabel.c_str(), glm::value_ptr(material.second->GetAlbedoInput().Color), ImGuiColorEditFlags_NoInputs);
                        }
                    }
                    {
                        // Normals
                        std::string textureLabel = materialName + " Normals";
                        if (ImGui::CollapsingHeader(textureLabel.c_str(), nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                            ImGui::Image(material.second->GetNormalInput().TextureMap ?
                                (void*)(intptr_t)material.second->GetNormalInput().TextureMap->GetID() :
                                (void*)(intptr_t)m_CheckerboardTexture->GetID(), ImVec2(64, 64));
                            ImGui::PopStyleVar();
                            if (ImGui::IsItemHovered())
                            {
                                if (material.second->GetNormalInput().TextureMap)
                                {
                                    ImGui::BeginTooltip();
                                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                    ImGui::TextUnformatted(material.second->GetNormalInput().TextureMap->GetPath().c_str());
                                    ImGui::PopTextWrapPos();
                                    ImGui::Image((void*)(intptr_t)material.second->GetNormalInput().TextureMap->GetID(), ImVec2(384, 384));
                                    ImGui::EndTooltip();
                                }
                                if (ImGui::IsItemClicked())
                                {
                                    std::string filename = Application::Get()->OpenFile("");
                                    if (filename != "")
                                        material.second->GetNormalInput().TextureMap = Hazel::HazelTexture2D::Create(filename);
                                }
                            }
                            ImGui::SameLine();
                            std::string checkboxLabel = "Use##" + materialName + "NormalMap";
                            ImGui::Checkbox(checkboxLabel.c_str(), &material.second->GetNormalInput().UseTexture);
                        }
                    }
                    {
                        // Metalness
                        std::string textureLabel = materialName + " Metalness";
                        if (ImGui::CollapsingHeader(textureLabel.c_str(), nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                            ImGui::Image(material.second->GetMetalnessInput().TextureMap ?
                                (void*)(intptr_t)material.second->GetMetalnessInput().TextureMap->GetID() :
                                (void*)(intptr_t)m_CheckerboardTexture->GetID(), ImVec2(64, 64));
                            ImGui::PopStyleVar();
                            if (ImGui::IsItemHovered())
                            {
                                if (material.second->GetMetalnessInput().TextureMap)
                                {
                                    ImGui::BeginTooltip();
                                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                    ImGui::TextUnformatted(material.second->GetMetalnessInput().TextureMap->GetPath().c_str());
                                    ImGui::PopTextWrapPos();
                                    ImGui::Image((void*)(intptr_t)material.second->GetMetalnessInput().TextureMap->GetID(), ImVec2(384, 384));
                                    ImGui::EndTooltip();
                                }
                                if (ImGui::IsItemClicked())
                                {
                                    std::string filename = Application::Get()->OpenFile("");
                                    if (filename != "")
                                        material.second->GetMetalnessInput().TextureMap = Hazel::HazelTexture2D::Create(filename);
                                }
                            }
                            ImGui::SameLine();
                            std::string checkboxLabel = "Use##" + materialName + "MetalnessMap";
                            ImGui::Checkbox(checkboxLabel.c_str(), &material.second->GetMetalnessInput().UseTexture);
                            ImGui::SameLine();
                            std::string sliderLabel = "Value##" + materialName + "MetalnessInput";
                            ImGui::SliderFloat(sliderLabel.c_str(), &material.second->GetMetalnessInput().Value, 0.0f, 1.0f);
                        }
                    }
                    {
                        // Roughness
                        std::string textureLabel = materialName + " Roughness";
                        if (ImGui::CollapsingHeader(textureLabel.c_str(), nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                            ImGui::Image(material.second->GetRoughnessInput().TextureMap ?
                                (void*)(intptr_t)material.second->GetRoughnessInput().TextureMap->GetID() :
                                (void*)(intptr_t)m_CheckerboardTexture->GetID(), ImVec2(64, 64));
                            ImGui::PopStyleVar();
                            if (ImGui::IsItemHovered())
                            {
                                if (material.second->GetRoughnessInput().TextureMap)
                                {
                                    ImGui::BeginTooltip();
                                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                    ImGui::TextUnformatted(material.second->GetRoughnessInput().TextureMap->GetPath().c_str());
                                    ImGui::PopTextWrapPos();
                                    ImGui::Image((void*)(intptr_t)material.second->GetRoughnessInput().TextureMap->GetID(), ImVec2(384, 384));
                                    ImGui::EndTooltip();
                                }
                                if (ImGui::IsItemClicked())
                                {
                                    std::string filename = Application::Get()->OpenFile("");
                                    if (filename != "")
                                        material.second->GetRoughnessInput().TextureMap = Hazel::HazelTexture2D::Create(filename);
                                }
                            }
                            ImGui::SameLine();
                            std::string checkboxLabel = "Use##" + materialName + "RoughnessMap";
                            ImGui::Checkbox(checkboxLabel.c_str(), &material.second->GetRoughnessInput().UseTexture);
                            ImGui::SameLine();
                            std::string sliderLabel = "Value##" + materialName + "RoughnessInput";
                            ImGui::SliderFloat(sliderLabel.c_str(), &material.second->GetRoughnessInput().Value, 0.0f, 1.0f);
                        }
                    }
                    {
                        // AO (Ambient Occlusion
                        std::string textureLabel = materialName + " AO";
                        if (ImGui::CollapsingHeader(textureLabel.c_str(), nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                            ImGui::Image(material.second->GetAOInput().TextureMap ?
                                (void*)(intptr_t)material.second->GetAOInput().TextureMap->GetID() :
                                (void*)(intptr_t)m_CheckerboardTexture->GetID(), ImVec2(64, 64));
                            ImGui::PopStyleVar();
                            if (ImGui::IsItemHovered())
                            {
                                if (material.second->GetAOInput().TextureMap)
                                {
                                    ImGui::BeginTooltip();
                                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                    ImGui::TextUnformatted(material.second->GetAOInput().TextureMap->GetPath().c_str());
                                    ImGui::PopTextWrapPos();
                                    ImGui::Image((void*)(intptr_t)material.second->GetAOInput().TextureMap->GetID(), ImVec2(384, 384));
                                    ImGui::EndTooltip();
                                }
                                if (ImGui::IsItemClicked())
                                {
                                    std::string filename = Application::Get()->OpenFile("");
                                    if (filename != "")
                                        material.second->GetAOInput().TextureMap = Hazel::HazelTexture2D::Create(filename);
                                }
                            }
                            ImGui::SameLine();
                            std::string checkboxLabel = "Use##" + materialName + "AOMap";
                            ImGui::Checkbox(checkboxLabel.c_str(), &material.second->GetAOInput().UseTexture);
                            ImGui::SameLine();
                            std::string sliderLabel = "Value##" + materialName + "AOInput";
                            ImGui::SliderFloat(sliderLabel.c_str(), &material.second->GetAOInput().Value, 0.0f, 1.0f);
                        }
                    }
                }
                ImGui::Unindent();
                // END PBR Textures
            }
        }
    }
    ImGui::End();

    // Shaders
    ImGui::Begin("Shaders");
    {
        if (ImGui::TreeNode("Shaders"))
        {
            auto shaders = ResourceManager::GetShaders();
            for (auto shader = shaders->begin(); shader != shaders->end(); shader++)
            {
                if (ImGui::TreeNode(shader->first.c_str()))
                {
                    std::string buttonName = "Reload##" + shader->first;
                    if (ImGui::Button(buttonName.c_str())) {
                        shader->second->Reload();
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }

        std::string buttonName = "Reload All";
        if (ImGui::Button(buttonName.c_str())) {
            auto shaders = ResourceManager::GetShaders();
            for (auto shader = shaders->begin(); shader != shaders->end(); shader++) {
                shader->second->Reload();
            }
        }
    }
    ImGui::End();

    ImGui::Separator();

    // Selection
    ImGui::Begin("Selection");
    {
        ImGui::Text("Selection Mode: ");
        ImGui::SameLine();
        const char* label = m_SelectionMode == SelectionMode::Entity ? "Entity" : "Mesh";
        if (ImGui::Button(label))
        {
            m_SelectionMode = m_SelectionMode == SelectionMode::Entity ? SelectionMode::SubMesh : SelectionMode::Entity;
        }

        const char* entityTag = "N/A";
        const char* meshName = "N/A";

        if (EntitySelection::s_SelectionContext.size())
        {
            auto selection = EntitySelection::s_SelectionContext[0];
            entityTag = selection.Entity.GetComponent<Hazel::TagComponent>().Tag.c_str();
            meshName = selection.Mesh ? selection.Mesh->MeshName.c_str() : "N/A";
        }

        ImGui::Text("Selected Entity: ");
        ImGui::SameLine();
        ImGui::Text(entityTag);

        ImGui::Text("Selected Mesh: ");
        ImGui::SameLine();
        ImGui::Text(meshName);
    }
    ImGui::End();

    ImGui::Begin("Help");
    {
        ImGui::Text("* Left ALT + Left Mouse Button - Pan");
        ImGui::Text("* Left ALT + Middle Mouse Button - Rotate/Orbit");
        ImGui::Text("* Left ALT + Right Mouse Button - Zoom");
    }
    ImGui::End();

    m_SceneHierarchyPanel->OnImGuiRender();

    ImGui::ShowMetricsWindow();

    ImVec2 workPos = ImGui::GetMainViewport()->GetWorkPos();
    m_WorkPosImGui = glm::vec2(workPos.x, workPos.y);
}

void EnvironmentMap::OnNewScene(glm::vec2 viewportSize)
{
    m_SceneRenderer->s_Data.ActiveScene = new Hazel::HazelScene();
    m_SceneRenderer->s_Data.ActiveScene->OnViewportResize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
    m_SceneHierarchyPanel->SetContext(m_SceneRenderer->s_Data.ActiveScene);
}

void EnvironmentMap::OnEntityDeleted(Hazel::Entity e)
{
}

void EnvironmentMap::SubmitMesh(Hazel::HazelMesh* mesh, const glm::mat4& transform, Material* overrideMaterial)
{
    auto& materials = mesh->GetMaterials();
    for (Hazel::Submesh& submesh : mesh->GetSubmeshes())
    {
        // Material
        auto material = materials[submesh.MaterialIndex];

        for (size_t i = 0; i < mesh->m_BoneTransforms.size(); i++)
        {
            std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
            m_ShaderHazelPBR->setMat4(uniformName, mesh->m_BoneTransforms[i]);
        }

        m_ShaderHazelPBR->setMat4("u_Transform", transform * submesh.Transform);

        if (material->GetFlag(MaterialFlag::DepthTest)) { // TODO: Fix Material flags
            glEnable(GL_DEPTH_TEST);
        }
        else {
            glDisable(GL_DEPTH_TEST);
        }

        glDisable(GL_DEPTH_TEST);

        glDrawElementsBaseVertex(GL_TRIANGLES, submesh.GetIndexCount(), GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh.BaseIndex), submesh.BaseVertex);
    }
}

void EnvironmentMap::RenderSkybox()
{
    RendererBasic::DisableCulling();
    RendererBasic::DisableDepthTest();

    // render skybox (render as last to prevent overdraw)
    m_SceneRenderer->GetShaderSkybox()->Bind();

    m_SceneRenderer->s_Data.SceneData.SceneEnvironment.RadianceMap->Bind(m_SamplerSlots->at("u_Texture"));

    glm::mat4 viewProjection = m_ActiveCamera->GetViewProjection();
    m_SceneRenderer->GetShaderSkybox()->setMat4("u_InverseVP", glm::inverse(viewProjection));

    m_SceneRenderer->GetShaderSkybox()->setInt("u_Texture", m_SamplerSlots->at("u_Texture"));
    m_SceneRenderer->GetShaderSkybox()->setFloat("u_TextureLod", ((Hazel::HazelScene*)m_SceneRenderer->s_Data.ActiveScene)->GetSkyboxLOD());
    m_SceneRenderer->GetShaderSkybox()->setFloat("u_Exposure", m_ActiveCamera->GetExposure() * m_SkyboxExposureFactor); // originally used in Shaders/Hazel/SceneComposite

    m_SkyboxCube->Render();

    m_SceneRenderer->GetShaderSkybox()->Unbind();
}

void EnvironmentMap::RenderHazelGrid()
{
    // Grid
    // -- Shaders/Hazel/Grid.vs
    // ---- uniform mat4 u_ViewProjection;
    // ---- uniform mat4 u_Transform;
    // -- Shaders/Hazel/Grid.fs
    // ---- uniform float u_Scale;
    // ---- uniform float u_Res;

    m_SceneRenderer->GetShaderGrid()->Bind();
    m_SceneRenderer->GetShaderGrid()->setFloat("u_Scale", m_SceneRenderer->m_GridScale);
    m_SceneRenderer->GetShaderGrid()->setFloat("u_Res", m_SceneRenderer->m_GridSize);

    m_SceneRenderer->GetShaderGrid()->setMat4("u_ViewProjection", m_ActiveCamera->GetViewProjection());

    bool depthTest = true;

    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::scale(transform, glm::vec3(16.0f, 1.0f, 16.0f));
    transform = glm::rotate(transform, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    m_SceneRenderer->GetShaderGrid()->setMat4("u_Transform", transform);

    m_Quad->Render();

    RendererBasic::EnableTransparency();
    RendererBasic::EnableMSAA();
}

void EnvironmentMap::OnEvent(Event& e)
{
    if (m_AllowViewportCameraEvents) {
        /* Camera.OnEvent */
        // ((Scene*)m_SceneRenderer->s_Data.ActiveScene)->GetCamera()->OnEvent(e);
    }

    m_ActiveCamera->OnEvent(e);

    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<KeyPressedEvent>(HZ_BIND_EVENT_FN(EnvironmentMap::OnKeyPressedEvent));
    dispatcher.Dispatch<MouseButtonPressedEvent>(HZ_BIND_EVENT_FN(EnvironmentMap::OnMouseButtonPressed));
}

bool EnvironmentMap::OnKeyPressedEvent(KeyPressedEvent& e)
{
    return false;
}

bool EnvironmentMap::OnMouseButtonPressed(MouseButtonPressedEvent& e)
{
    auto [mx, my] = Input::GetMousePosition();
    if (e.GetMouseButton() == (int)Mouse::ButtonLeft && !Input::IsKeyPressed(Key::LeftAlt) && !ImGuizmo::IsOver())
    {
        auto [mouseX, mouseY] = GetMouseViewportSpace();
        if (mouseX > -1.0f && mouseX < 1.0f && mouseY > -1.0f && mouseY < 1.0f)
        {
            auto [origin, direction] = CastRay(mouseX, mouseY);

            EntitySelection::s_SelectionContext.clear();

            auto meshEntities = m_SceneRenderer->s_Data.ActiveScene->GetAllEntitiesWith<Hazel::MeshComponent>();
            for (auto e : meshEntities)
            {
                Hazel::Entity entity = { e, m_SceneRenderer->s_Data.ActiveScene };
                auto mesh = entity.GetComponent<Hazel::MeshComponent>().Mesh;
                if (!mesh) {
                    continue;
                }

                auto& submeshes = mesh->GetSubmeshes();
                float lastT = std::numeric_limits<float>::max(); // Distance between camera and intersection in CastRay
                // for (Hazel::Submesh& submesh : submeshes)
                for (uint32_t i = 0; i < submeshes.size(); i++)
                {
                    auto& submesh = submeshes[i];
                    auto& transform = entity.GetComponent<Hazel::TransformComponent>().GetTransform();
                    Hazel::Ray ray = {
                        glm::inverse(transform * submesh.Transform) * glm::vec4(origin, 1.0f),
                        glm::inverse(glm::mat3(transform) * glm::mat3(submesh.Transform)) * direction
                    };

                    float t;
                    bool intersects = ray.IntersectsAABB(submesh.BoundingBox, t);
                    if (intersects)
                    {
                        const auto& triangleCache = ((Hazel::HazelMesh*)mesh.Raw())->GetTriangleCache(i);
                        if (triangleCache.size())
                        {
                            for (const auto& triangle : triangleCache)
                            {
                                if (ray.IntersectsTriangle(triangle.V0.Position, triangle.V1.Position, triangle.V2.Position, t))
                                {
                                    EntitySelection::s_SelectionContext.push_back({ entity, &submesh, t });
                                    break;
                                }
                            }
                        }
                        else {
                            EntitySelection::s_SelectionContext.push_back({ entity, &submesh, t });
                        }
                    }
                }
            }
            std::sort(EntitySelection::s_SelectionContext.begin(), EntitySelection::s_SelectionContext.end(), [](auto& a, auto& b) { return a.Distance < b.Distance; });

            // TODO: Handle mesh being deleted, etc
            if (EntitySelection::s_SelectionContext.size()) {
                m_CurrentlySelectedTransform = &EntitySelection::s_SelectionContext[0].Mesh->Transform;
                OnSelected(EntitySelection::s_SelectionContext[0]);
            }
            else {
                Ref<Hazel::Entity> meshEntity = GetMeshEntity();
                if (!meshEntity) {
                    m_CurrentlySelectedTransform = &meshEntity->Transform();
                }
            }
        }
    }
    return false;
}

void EnvironmentMap::OnSelected(const SelectedSubmesh& selectionContext)
{
    // TODO: move to SceneHazelEnvMap
    // m_SceneHierarchyPanel->SetSelected(selectionContext.Entity);
}

std::pair<float, float> EnvironmentMap::GetMouseViewportSpace()
{
    auto [mx, my] = ImGui::GetMousePos(); // Input::GetMousePosition();
    mx -= m_ViewportBounds[0].x;
    my -= m_ViewportBounds[0].y;
    m_ViewportWidth = m_ViewportBounds[1].x - m_ViewportBounds[0].x;
    m_ViewportHeight = m_ViewportBounds[1].y - m_ViewportBounds[0].y;

    return { (mx / m_ViewportWidth) * 2.0f - 1.0f, ((my / m_ViewportHeight) * 2.0f - 1.0f) * -1.0f };
}

std::pair<glm::vec3, glm::vec3> EnvironmentMap::CastRay(float mx, float my)
{
    glm::vec4 mouseClipPos = { mx, my, -1.0f, 1.0f };

    glm::mat4 projectionMatrix = m_ActiveCamera->GetProjection();
    glm::mat4 viewMatrix = m_ActiveCamera->GetViewMatrix();

    auto inverseProj = glm::inverse(projectionMatrix);
    auto inverseView = glm::inverse(glm::mat3(viewMatrix));

    glm::vec4 ray = inverseProj * mouseClipPos;
    glm::vec3 rayPos = m_ActiveCamera->GetPosition();
    glm::vec3 rayDir = inverseView * glm::vec3(ray); // inverseView * glm::vec3(ray)

    Log::GetLogger()->debug("EnvironmentMap::CastRay | MousePosition [ {0} {1} ]", mx, my);
    Log::GetLogger()->debug("EnvironmentMap::CastRay | m_ViewportBounds[0] [ {0} {1} ]", m_ViewportBounds[0].x, m_ViewportBounds[0].y);
    Log::GetLogger()->debug("EnvironmentMap::CastRay | m_ViewportBounds[1] [ {0} {1} ]", m_ViewportBounds[1].x, m_ViewportBounds[1].y);
    Log::GetLogger()->debug("EnvironmentMap::CastRay | mouseClipPos [ {0} {1} ]", mouseClipPos.x, mouseClipPos.y);

    return { rayPos, rayDir };
}

void EnvironmentMap::GeometryPassTemporary()
{
    RendererBasic::EnableTransparency();
    RendererBasic::EnableMSAA();

    glm::mat4 viewProj = m_ActiveCamera->GetViewProjection();

    m_SceneRenderer->s_Data.SceneData.SceneEnvironment.RadianceMap->Bind(m_SamplerSlots->at("radiance"));
    m_SceneRenderer->s_Data.SceneData.SceneEnvironment.IrradianceMap->Bind(m_SamplerSlots->at("irradiance"));
    m_SceneRenderer->s_Data.BRDFLUT->Bind(m_SamplerSlots->at("BRDF_LUT"));

    uint32_t samplerSlot = m_SamplerSlots->at("albedo");

    RenderSkybox();

    if (m_DisplayHazelGrid) {
        RenderHazelGrid();
    }

    auto meshEntities = m_SceneRenderer->s_Data.ActiveScene->GetAllEntitiesWith<Hazel::MeshComponent>();

    // Render all entities with mesh component
    if (meshEntities.size())
    {
        // Log::GetLogger()->debug("Rendering mesh entities, count = {0}", meshEntities.size());

        m_ShaderHazelPBR->Bind();

        for (auto entt : meshEntities)
        {
            Hazel::Entity entity = { entt, m_SceneRenderer->s_Data.ActiveScene };

            Hazel::Ref<Hazel::HazelMesh> hazelMesh = entity.GetComponent<Hazel::MeshComponent>().Mesh;
            m_ShaderHazelPBR = hazelMesh->IsAnimated() ? m_ShaderHazelPBR_Anim : m_ShaderHazelPBR_Static;

            for (Hazel::Submesh& submesh : hazelMesh->GetSubmeshes())
            {
                std::string materialName = Hazel::HazelMesh::GetSubmeshMaterialName(hazelMesh.Raw(), submesh);

                if (m_EnvMapMaterials.contains(materialName)) {
                    UpdateShaderPBRUniforms(m_ShaderHazelPBR, m_EnvMapMaterials.at(materialName));
                }

                glm::mat4 entityTransform = entity.GetComponent<Hazel::TransformComponent>().GetTransform();
                submesh.Render(hazelMesh.Raw(), m_ShaderHazelPBR, entityTransform, samplerSlot, m_EnvMapMaterials);
            }
        }

        m_ShaderHazelPBR->Unbind();
    }

    Hazel::Renderer2D::BeginScene(viewProj, true);
    {
        RendererBasic::SetLineThickness(2.0f);

        if (m_DrawOnTopBoundingBoxes)
        {
            glm::vec3 camPosition = m_ActiveCamera->GetPosition();
            Hazel::Renderer2D::DrawLine(m_NewRay, m_NewRay + glm::vec3(1.0f, 0.0f, 0.0f) * 100.0f, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
        }

        if (EntitySelection::s_SelectionContext.size()) {
            for (auto selection : EntitySelection::s_SelectionContext)
            {
                if (selection.Mesh) {
                    Hazel::Entity meshEntity = selection.Entity;
                    glm::mat4 transform = meshEntity.GetComponent<Hazel::TransformComponent>().GetTransform();
                    glm::vec4 color = m_SelectionMode == SelectionMode::Entity ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) : glm::vec4(0.2f, 0.9f, 0.2f, 1.0f);
                    Hazel::HazelRenderer::DrawAABB(selection.Mesh->BoundingBox, transform * selection.Mesh->Transform, color);
                }
            }
        }
    }
    Hazel::Renderer2D::EndScene();

    m_SceneRenderer->s_Data.GeoPass->GetSpecification().TargetFramebuffer->Bind();
}

void EnvironmentMap::CompositePassTemporary(Framebuffer* framebuffer)
{
    m_SceneRenderer->GetShaderComposite()->Bind();
    framebuffer->GetTextureAttachmentColor()->Bind(m_SamplerSlots->at("u_Texture"));
    m_SceneRenderer->GetShaderComposite()->setInt("u_Texture", m_SamplerSlots->at("u_Texture"));
    m_SceneRenderer->GetShaderComposite()->setFloat("u_Exposure", m_ActiveCamera->GetExposure());
    // m_ShaderComposite->setInt("u_TextureSamples", framebuffer->GetSpecification().Samples);
    m_SceneRenderer->GetShaderComposite()->setInt("u_TextureSamples", m_SceneRenderer->s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetSpecification().Samples);
    Hazel::HazelRenderer::SubmitFullscreenQuad(nullptr);
}

void EnvironmentMap::OnRender(Framebuffer* framebuffer)
{
    OnRenderEditor(framebuffer);
    // OnRenderRuntime(framebuffer)
}

void EnvironmentMap::OnRenderEditor(Framebuffer* framebuffer)
{
    GeometryPassTemporary();
    CompositePassTemporary(framebuffer);
}

void EnvironmentMap::OnRenderRuntime(Framebuffer* framebuffer)
{
    GeometryPassTemporary();
    CompositePassTemporary(framebuffer);
}
