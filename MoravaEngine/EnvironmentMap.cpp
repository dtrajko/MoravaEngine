#include "EnvironmentMap.h"

#include "Hazel/Renderer/HazelRenderer.h"
#include "Hazel/Renderer/RenderPass.h"
#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Renderer/Renderer2D.h"
#include "Hazel/Scene/SceneSerializer.h"
#include "Hazel/Script/ScriptEngine.h"
#include "Hazel/Utils/PlatformUtils.h"

#include "ImGuiWrapper.h"
#include "ImGuizmo.h"
#include "RendererBasic.h"
#include "SceneHazelEnvMap.h"
#include "MousePicker.h"
#include "ShaderLibrary.h"

#include <filesystem>


TextureInfo EnvironmentMap::s_TextureInfoDefault;
std::map<std::string, TextureInfo> EnvironmentMap::s_TextureInfo;
SelectionMode EnvironmentMap::s_SelectionMode = SelectionMode::Entity;
Hazel::Ref<Hazel::HazelTexture2D> EnvironmentMap::s_CheckerboardTexture;

std::map<MaterialUUID, EnvMapMaterial*> EnvironmentMap::s_EnvMapMaterials; // MaterialUUID, EnvMapMaterial*
std::map<SubmeshUUID, MaterialUUID> EnvironmentMap::s_SubmeshMaterialUUIDs; // SubmeshUUID, MaterialUUID


EnvironmentMap::EnvironmentMap(const std::string& filepath, Scene* scene)
{
    m_SamplerSlots = new std::map<std::string, unsigned int>();

    //  // PBR texture inputs
    m_SamplerSlots->insert(std::make_pair("albedo",     1)); // uniform sampler2D u_AlbedoTexture
    m_SamplerSlots->insert(std::make_pair("normal",     2)); // uniform sampler2D u_NormalTexture
    m_SamplerSlots->insert(std::make_pair("metalness",  3)); // uniform sampler2D u_MetalnessTexture
    m_SamplerSlots->insert(std::make_pair("roughness",  4)); // uniform sampler2D u_RoughnessTexture
    m_SamplerSlots->insert(std::make_pair("emissive",   5)); // uniform sampler2D u_EmissiveTexture
    m_SamplerSlots->insert(std::make_pair("ao",         6)); // uniform sampler2D u_AOTexture
    // Environment maps
    m_SamplerSlots->insert(std::make_pair("radiance",   7)); // uniform samplerCube u_EnvRadianceTex
    m_SamplerSlots->insert(std::make_pair("irradiance", 8)); // uniform samplerCube u_EnvIrradianceTex
    // BRDF LUT
    m_SamplerSlots->insert(std::make_pair("BRDF_LUT",   9)); // uniform sampler2D u_BRDFLUTTexture

    // Skybox.fs         - uniform samplerCube u_Texture;
    // SceneComposite.fs - uniform sampler2DMS u_Texture;
    m_SamplerSlots->insert(std::make_pair("u_Texture",  1));

    m_SkyboxCube = new CubeSkybox();
    m_Quad = new Quad();

    float fov = 60.0f;
    float aspectRatio = 1.778f; // 16/9
    m_EditorCamera = new Hazel::EditorCamera(fov, aspectRatio, 0.1f, 1000.0f);
    m_RuntimeCamera = new RuntimeCamera(scene->GetSettings().cameraPosition, scene->GetSettings().cameraStartYaw, scene->GetSettings().cameraStartPitch, 
        fov, aspectRatio, scene->GetSettings().cameraMoveSpeed, 0.1f);
    m_ActiveCamera = m_RuntimeCamera; // m_RuntimeCamera m_EditorCamera;

    m_EditorScene = Hazel::Ref<Hazel::HazelScene>::Create();
    m_EditorScene->SetSkyboxLOD(0.1f);

    m_SceneRenderer = new Hazel::SceneRenderer(filepath, m_EditorScene.Raw());
    SetSkybox(m_SceneRenderer->s_Data.SceneData.SceneEnvironment.RadianceMap);

    Init(); // requires a valid Camera reference

    m_SceneHierarchyPanel = new Hazel::SceneHierarchyPanel(m_EditorScene);
    m_SceneHierarchyPanel->SetSelectionChangedCallback(std::bind(&EnvironmentMap::SelectEntity, this, std::placeholders::_1));
    m_SceneHierarchyPanel->SetEntityDeletedCallback(std::bind(&EnvironmentMap::OnEntityDeleted, this, std::placeholders::_1));
    m_SceneHierarchyPanel->SetContext(m_EditorScene); // already done in constructor
    Hazel::ScriptEngine::SetSceneContext(m_EditorScene);
    m_EditorScene->SetSelectedEntity({});

    s_CheckerboardTexture = Hazel::HazelTexture2D::Create("Textures/Hazel/Checkerboard.tga");
    m_PlayButtonTex = Hazel::HazelTexture2D::Create("Textures/Hazel/PlayButton.png");

    m_DisplayHazelGrid = true;
    m_DisplayBoundingBoxes = false;
    m_DisplayRay = false;
    m_DrawOnTopBoundingBoxes = true; // obsolete?
    m_DisplayLineElements = false;

    Scene::s_ImGuizmoTransform = nullptr; // &GetMeshEntity()->Transform();
    Scene::s_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE;

    m_IsViewportEnabled = true;

    m_ResizeViewport = { 0.0f, 1.0f };

    SetupRenderFramebuffer();
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

    // Hazel::ScriptEngine::Init(""); // TODO Assembly path

    // Temporary code Hazel LIVE! #004
    Hazel::HazelRenderer::Init();

    bool depthTest = true;
    Hazel::Renderer2D::Init();
}

void EnvironmentMap::SetupContextData()
{
    // Setup default texture info
    s_TextureInfoDefault = {};
    s_TextureInfoDefault.albedo    = "Textures/PBR/non_reflective/albedo.png";
    s_TextureInfoDefault.normal    = "Textures/PBR/non_reflective/normal.png";
    s_TextureInfoDefault.metallic  = "Textures/PBR/non_reflective/metallic.png";
    s_TextureInfoDefault.roughness = "Textures/PBR/non_reflective/roughness.png";
    s_TextureInfoDefault.emissive  = "Textures/PBR/non_reflective/emissive.png";
    s_TextureInfoDefault.ao        = "Textures/PBR/non_reflective/ao.png";

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
    m_DirectionalLightEntity.AddComponent<Hazel::DirectionalLightComponent>();
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
        m_ShaderHazelPBR = ShaderLibrary::Get("HazelPBR_Anim");
    }
    else {
        isAnimated = false;
        m_ShaderHazelPBR = ShaderLibrary::Get("HazelPBR_Static");
    }

    Log::GetLogger()->debug("EnvironmentMap::LoadMesh: fullPath '{0}' fileName '{1}' fileNameNoExt '{2}'", fullPath, fileName, fileNameNoExt);

    Hazel::HazelMesh* mesh = new Hazel::HazelMesh(fullPath, m_ShaderHazelPBR, nullptr, isAnimated);

    mesh->SetTimeMultiplier(1.0f);

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
    LoadEnvMapMaterials(mesh, meshEntity);

    return meshEntity;
}

void EnvironmentMap::LoadEnvMapMaterials(Hazel::Ref<Hazel::HazelMesh> mesh, Hazel::Entity entity)
{
    //  for (auto material : m_EnvMapMaterials) {
    //      delete material.second;
    //  }
    //  
    //  m_EnvMapMaterials.clear();

    std::vector<Hazel::Submesh>& submeshes = mesh->GetSubmeshes();

    for (Hazel::Submesh& submesh : submeshes)
    {
        std::string materialUUID = Hazel::HazelMesh::GetSubmeshMaterialUUID(mesh, submesh, entity);

        Log::GetLogger()->debug("EnvironmentMap::LoadEnvMapMaterials materialUUID = '{0}'", materialUUID);

        if (s_EnvMapMaterials.contains(materialUUID)) {
            continue;
        }

        EnvMapMaterial* envMapMaterial = CreateDefaultMaterial(materialUUID);
        s_EnvMapMaterials.insert(std::make_pair(materialUUID, envMapMaterial));
    }

    //  // If no submeshes, add a default material for entity
    //  if (submeshes.empty())
    //  {
    //      EnvMapMaterial* envMapMaterial = CreateDefaultMaterial(meshName);
    //      m_EnvMapMaterials.insert(std::make_pair(meshName, envMapMaterial));
    //  }

    for (auto& material : s_EnvMapMaterials)
    {
        Log::GetLogger()->debug("EnvironmentMap::LoadEnvMapMaterials material name: '{0}' UUID: '{1}'", material.second->GetName(), material.first);
    }
}

void EnvironmentMap::AddMaterialFromComponent(Hazel::Entity entity)
{
    // If entity contains MaterialComponent, load generic material for the entire entity (all submeshes)
    if (entity.HasComponent<Hazel::MaterialComponent>())
    {
        if (entity.GetComponent<Hazel::MaterialComponent>().Material != nullptr)
        {
            std::string materialName = NewMaterialName();
            auto material = entity.GetComponent<Hazel::MaterialComponent>().Material;
            material->SetName(materialName);

            if (!s_EnvMapMaterials.contains(material->GetName()))
            {
                s_EnvMapMaterials.insert(std::make_pair(material->GetUUID(), material));
            }
        }
    }
}

std::string EnvironmentMap::NewMaterialName()
{
    std::string materialName = "MAT_UNDEFINED";
    unsigned int materialIndex = 0;
    bool newNameCreated = false;
    while (!newNameCreated)
    {
        materialName = "MAT_" + std::to_string(materialIndex);
        if (!s_EnvMapMaterials.contains(materialName))
        {
            newNameCreated = true;
        }
        else {
            materialIndex++;
        }
    }

    return materialName;
}

void EnvironmentMap::AddSubmeshToSelectionContext(SelectedSubmesh submesh)
{
    EntitySelection::s_SelectionContext.push_back(submesh);

    if (EntitySelection::s_SelectionContext.size() && EntitySelection::s_SelectionContext[0].Mesh != nullptr) {
        Log::GetLogger()->debug("SelectionContext[0].Mesh->MeshName: '{0}'", EntitySelection::s_SelectionContext[0].Mesh->MeshName);
    }
}

void EnvironmentMap::RenameMaterial(EnvMapMaterial* envMapMaterial, const std::string newName)
{
    Log::GetLogger()->debug("EnvironmentMap::RenameMaterial from '{0}' to '{1}'", envMapMaterial->GetName(), newName);

    std::string oldName = envMapMaterial->GetName();
    MaterialUUID materialUUID = envMapMaterial->GetUUID();

    // TODO: Make sure that the new material name is not already taken
    for (auto emm_it = s_EnvMapMaterials.cbegin(); emm_it != s_EnvMapMaterials.cend(); emm_it++) {
        if (emm_it->second->GetName() == newName) {
            Log::GetLogger()->error("Material name is already taken [Name: '{0}', UUID: '{1}']!", newName, emm_it->second->GetUUID());
            return;
        }
    }

    // TODO: Rename object attribute
    envMapMaterial->SetName(newName);

    // TODO: Rename in s_EnvMapMaterials
    for (auto emm_it = s_EnvMapMaterials.cbegin(); emm_it != s_EnvMapMaterials.cend();) {
        if (emm_it->second->GetName() == oldName) {
            emm_it->second->SetName(newName);
            Log::GetLogger()->error("s_EnvMapMaterials: '{0}' => '{1}'", oldName, s_EnvMapMaterials.find(materialUUID)->second->GetName());
            break;
        }
        else {
            ++emm_it;
        }
    }
}

void EnvironmentMap::ShowBoundingBoxes(bool showBoundingBoxes, bool showBoundingBoxesOnTop)
{
}

EnvMapMaterial* EnvironmentMap::CreateDefaultMaterial(std::string materialName)
{
    EnvMapMaterial* envMapMaterial = new EnvMapMaterial(materialName);

    TextureInfo textureInfo;
    if (s_TextureInfo.contains(materialName)) {
        textureInfo = s_TextureInfo.at(materialName);
    }
    else {
        textureInfo = s_TextureInfoDefault;
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
    envMapMaterial->GetEmissiveInput().TextureMap = ResourceManager::LoadHazelTexture2D(textureInfo.emissive);
    envMapMaterial->GetEmissiveInput().UseTexture = true;
    envMapMaterial->GetAOInput().TextureMap = ResourceManager::LoadHazelTexture2D(textureInfo.ao);
    envMapMaterial->GetAOInput().UseTexture = true;

    return envMapMaterial;
}

void EnvironmentMap::SetupShaders()
{
    Ref<Shader> shaderHazelPBR_Static = CreateRef<Shader>("Shaders/Hazel/HazelPBR_Static.vs", "Shaders/Hazel/HazelPBR.fs");
    Log::GetLogger()->info("EnvironmentMap: m_ShaderHazelPBR_Static compiled [programID={0}]", shaderHazelPBR_Static->GetProgramID());

    Ref<Shader> shaderHazelPBR_Anim = CreateRef<Shader>("Shaders/Hazel/HazelPBR_Anim.vs", "Shaders/Hazel/HazelPBR.fs");
    Log::GetLogger()->info("EnvironmentMap: m_ShaderHazelPBR_Anim compiled [programID={0}]", shaderHazelPBR_Anim->GetProgramID());

    m_ShaderRenderer2D_Line = CreateRef<Shader>("Shaders/Hazel/Renderer2D_Line.vs", "Shaders/Hazel/Renderer2D_Line.fs");
    Log::GetLogger()->info("EnvironmentMap: m_ShaderRenderer2D_Line compiled [programID={0}]", m_ShaderRenderer2D_Line->GetProgramID());

    ResourceManager::AddShader("Hazel/HazelPBR_Static", shaderHazelPBR_Static);
    ResourceManager::AddShader("Hazel/HazelPBR_Anim", shaderHazelPBR_Anim);
    ResourceManager::AddShader("Hazel/Renderer2D_Line", m_ShaderRenderer2D_Line);

    ShaderLibrary::Add(shaderHazelPBR_Static);
    ShaderLibrary::Add(shaderHazelPBR_Anim);
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
    m_SceneRenderer->GetShaderSkybox()->setFloat("u_TextureLod", m_EditorScene->GetSkyboxLOD());
    // apply exposure to Shaders/Hazel/Skybox, considering that Shaders/Hazel/SceneComposite is not yet enabled
    m_SceneRenderer->GetShaderSkybox()->setFloat("u_Exposure", m_ActiveCamera->GetExposure() * m_SkyboxExposureFactor); // originally used in Shaders/Hazel/SceneComposite
    /**** END Shaders/Hazel/Skybox ****/
}

void EnvironmentMap::UpdateShaderPBRUniforms(Ref<Shader> shaderHazelPBR, EnvMapMaterial* envMapMaterial)
{
    /**** BEGIN Shaders/Hazel/HazelPBR_Anim / Shaders/Hazel/HazelPBR_Static ***/

    shaderHazelPBR->Bind();

    shaderHazelPBR->setInt("u_AlbedoTexture",    m_SamplerSlots->at("albedo"));
    shaderHazelPBR->setInt("u_NormalTexture",    m_SamplerSlots->at("normal"));
    shaderHazelPBR->setInt("u_MetalnessTexture", m_SamplerSlots->at("metalness"));
    shaderHazelPBR->setInt("u_RoughnessTexture", m_SamplerSlots->at("roughness"));
    shaderHazelPBR->setInt("u_EmissiveTexture",  m_SamplerSlots->at("emissive"));
    shaderHazelPBR->setInt("u_AOTexture",        m_SamplerSlots->at("ao"));

    shaderHazelPBR->setVec3("u_AlbedoColor", envMapMaterial->GetAlbedoInput().Color);
    shaderHazelPBR->setFloat("u_Metalness",  envMapMaterial->GetMetalnessInput().Value);
    shaderHazelPBR->setFloat("u_Roughness",  envMapMaterial->GetRoughnessInput().Value);
    shaderHazelPBR->setFloat("u_Emissive",   envMapMaterial->GetEmissiveInput().Value);
    shaderHazelPBR->setFloat("u_AO",         envMapMaterial->GetAOInput().Value);

    shaderHazelPBR->setFloat("u_EnvMapRotation", m_EnvMapRotation);

    shaderHazelPBR->setFloat("u_RadiancePrefilter",  m_RadiancePrefilter ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_AlbedoTexToggle",    envMapMaterial->GetAlbedoInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_NormalTexToggle",    envMapMaterial->GetNormalInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_MetalnessTexToggle", envMapMaterial->GetMetalnessInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_RoughnessTexToggle", envMapMaterial->GetRoughnessInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_EmissiveTexToggle",  envMapMaterial->GetEmissiveInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_AOTexToggle",        envMapMaterial->GetAOInput().UseTexture ? 1.0f : 0.0f);

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
    for (auto item : s_EnvMapMaterials) {
        delete item.second;
    }
    s_EnvMapMaterials.clear();

    delete m_SceneRenderer;

    delete m_RuntimeCamera;
    delete m_EditorCamera;
}

Hazel::Entity EnvironmentMap::CreateEntity(const std::string& name)
{
    // Both NoECS and ECS
    Hazel::Entity entity = m_EditorScene->CreateEntity(name, m_EditorScene);

    return entity;
}

void EnvironmentMap::OnUpdate(Scene* scene, float timestep)
{
    switch (m_SceneState)
    {
    case SceneState::Edit:
        if (m_ViewportPanelFocused) {
            m_EditorCamera->OnUpdate(timestep);
        }
        m_EditorScene->OnRenderEditor(timestep, *m_EditorCamera);
        break;
    case SceneState::Play:
        if (m_ViewportPanelFocused) {
            m_EditorCamera->OnUpdate(timestep);
        }
        m_RuntimeScene->OnUpdate(timestep);
        m_RuntimeScene->OnRenderRuntime(timestep);
        break;
    case SceneState::Pause:
        if (m_ViewportPanelFocused) {
            m_EditorCamera->OnUpdate(timestep);
        }
        m_RuntimeScene->OnRenderRuntime(timestep);
        break;
    }

    // CameraSyncECS(); TODO

    if (m_DirectionalLightEntity.HasComponent<Hazel::TransformComponent>())
    {
        auto& tc = m_DirectionalLightEntity.GetComponent<Hazel::TransformComponent>();
        m_SceneRenderer->s_Data.SceneData.ActiveLight.Direction = glm::eulerAngles(glm::quat(tc.Rotation));
    }

    OnUpdateEditor(scene, timestep);
    // OnUpdateRuntime(scene, timestep);
}

void EnvironmentMap::OnUpdateEditor(Scene* scene, float timestep)
{
    m_EditorScene = scene;

    m_SceneRenderer->BeginScene(m_EditorScene.Raw());

    UpdateUniforms();

    // Update HazelMesh List
    auto meshEntities = m_EditorScene->GetAllEntitiesWith<Hazel::MeshComponent>();
    for (auto entt : meshEntities)
    {
        Hazel::Entity entity{ entt, m_EditorScene.Raw() };
        Hazel::Ref<Hazel::HazelMesh> mesh = entity.GetComponent<Hazel::MeshComponent>().Mesh;
        if (mesh) {
            mesh->OnUpdate(timestep, false);
        }
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
    m_EditorScene = scene;

    m_SceneRenderer->BeginScene(m_EditorScene.Raw());

    UpdateUniforms();

    // Update HazelMesh List
    auto meshEntities = m_EditorScene->GetAllEntitiesWith<Hazel::MeshComponent>();
    for (auto entt : meshEntities)
    {
        Hazel::Entity entity{ entt, m_EditorScene.Raw() };
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

    if (m_ReloadScriptOnPlay) {
        Hazel::ScriptEngine::ReloadAssembly("assets/scripts/ExampleApp.dll");
    }

    m_RuntimeScene = Hazel::Ref<Hazel::HazelScene>::Create();
    m_EditorScene->CopyTo(m_RuntimeScene);

    m_RuntimeScene->OnRuntimeStart();
    m_SceneHierarchyPanel->SetContext(m_RuntimeScene);
}

void EnvironmentMap::OnSceneStop()
{
    m_RuntimeScene->OnRuntimeStop();
    m_SceneState = SceneState::Edit;

    // Unload runtime scene
    m_RuntimeScene = nullptr;

    EntitySelection::s_SelectionContext.clear();
    Hazel::ScriptEngine::SetSceneContext(m_EditorScene);
    m_SceneHierarchyPanel->SetContext(m_EditorScene);
}

void EnvironmentMap::UpdateWindowTitle(const std::string& sceneName)
{
    std::string title = sceneName + " - Hazelnut - " + Application::GetPlatformName() + " (" + Application::GetConfigurationName() + ")";
    Application::Get()->GetWindow()->SetTitle(title);
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
    auto meshEntities = m_EditorScene->GetAllEntitiesWith<Hazel::MeshComponent>();
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

        SelectedSubmesh selectedSubmesh = EntitySelection::s_SelectionContext[0];

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

        if (s_SelectionMode == SelectionMode::Entity || !selectedSubmesh.Mesh)
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
        else if (s_SelectionMode == SelectionMode::SubMesh)
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
    auto meshEntities = m_EditorScene->GetAllEntitiesWith<Hazel::MeshComponent>();
    if (meshEntities.size()) {
        for (auto entt : meshEntities)
        {
            meshEntity = CreateRef<Hazel::Entity>(entt, m_EditorScene.Raw());
        }
        return meshEntity;
    }
    return nullptr;
}

float& EnvironmentMap::GetSkyboxLOD()
{
    return m_EditorScene->GetSkyboxLOD();
}

void EnvironmentMap::SetViewportBounds(glm::vec2* viewportBounds)
{
    m_ViewportBounds[0] = viewportBounds[0];
    m_ViewportBounds[1] = viewportBounds[1];
}

void EnvironmentMap::SetSkyboxLOD(float LOD)
{
    m_EditorScene->SetSkyboxLOD(LOD);
}

Ref<Shader> EnvironmentMap::GetShaderPBR_Anim()
{
    return ShaderLibrary::Get("HazelPBR_Anim");
}

Ref<Shader> EnvironmentMap::GetShaderPBR_Static()
{
    return ShaderLibrary::Get("HazelPBR_Static");
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

void EnvironmentMap::OnImGuiRender(Window* mainWindow)
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

    ImGui::Begin("Settings");
    {
        if (ImGui::CollapsingHeader("Display Info"))
        {
            ImGui::Checkbox("Display Bounding Boxes", &m_DisplayBoundingBoxes);
            ImGui::Checkbox("Display Hazel Grid", &m_DisplayHazelGrid);
            ImGui::Checkbox("Display Line Elements", &m_DisplayLineElements);
            ImGui::Checkbox("Display Ray", &m_DisplayRay);

            ImGui::Separator();

            bool eventLoggingEnabled = Application::Get()->GetWindow()->GetEventLogging();
            if (ImGui::Checkbox("Enable Event Logging", &eventLoggingEnabled)) {
                Application::Get()->GetWindow()->SetEventLogging(eventLoggingEnabled);
            }

            float fovDegrees = m_ActiveCamera->GetPerspectiveVerticalFOV();
            if (ImGui::SliderFloat("FOV", &fovDegrees, -60.0f, 180.0f)) {
                m_ActiveCamera->SetPerspectiveVerticalFOV(fovDegrees);
            }
        }
    }
    ImGui::End();

    Application::Get()->OnImGuiRender();

    // ImGui::ShowMetricsWindow();

    m_ImGuiViewportMain.x = ImGui::GetMainViewport()->GetWorkPos().x;
    m_ImGuiViewportMain.y = ImGui::GetMainViewport()->GetWorkPos().y;

    MousePicker* mp = MousePicker::Get();

    ImGui::Begin("Mouse Picker");
    {
        if (ImGui::CollapsingHeader("Display Info"))
        {
            char buffer[100];

            sprintf(buffer, "Main Window [ X %i Y %i ]", (int)m_ImGuiViewportMain.x, (int)m_ImGuiViewportMain.y);
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

    ImGui::Begin("Framebuffers");
    {
        ImVec2 imageSize(128.0f, 128.0f);

        if (m_IsViewportEnabled)
        {
            ImGui::Text("Viewport");
            ImGui::Image((void*)(intptr_t)m_RenderFramebuffer->GetTextureAttachmentColor()->GetID(), imageSize);

            ImGui::Text("Equirectangular");
            ImGui::Image((void*)(intptr_t)m_SceneRenderer->GetEnvEquirect()->GetID(), imageSize);
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
                m_SceneRenderer->s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetWidth(),
                m_SceneRenderer->s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetHeight());
            glm::ivec2 depthAttachmentSize = glm::ivec2(
                m_SceneRenderer->s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetAttachmentDepth()->GetWidth(),
                m_SceneRenderer->s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetAttachmentDepth()->GetHeight());

            ImGui::SliderInt2("Color Attachment Size", glm::value_ptr(colorAttachmentSize), 0, 2048);
            ImGui::SliderInt2("Depth Attachment Size", glm::value_ptr(depthAttachmentSize), 0, 2048);
        }
        ImGui::End();
    }

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
            m_ViewportPanelMouseOver = ImGui::IsWindowHovered();
            m_ViewportPanelFocused = ImGui::IsWindowFocused();

            // Calculate Viewport bounds (used in EnvironmentMap::CastRay)
            auto viewportOffset = ImGui::GetCursorPos(); // includes tab bar
            auto viewportSize = ImGui::GetContentRegionAvail();
            //  Hazel::SceneRenderer::SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
            //  m_EditorScene->SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
            //  if (m_RuntimeScene) {
            //      m_RuntimeScene->SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
            //  }
            //  
            //  m_EditorCamera->SetProjectionMatrix(glm::perspectiveFov(glm::radians(45.0f), viewportSize.x, viewportSize.y, 0.1f, 10000.0f));
            //  m_EditorCamera->SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
            //  ImGui::Image((void*)Hazel::SceneRenderer::GetFinalColorBufferID(), viewportSize, { 0, 1 }, { 1, 0 });

            ImVec2 screen_pos = ImGui::GetCursorScreenPos();

            m_ImGuiViewport.X = (int)(ImGui::GetWindowPos().x - m_ImGuiViewportMain.x);
            m_ImGuiViewport.Y = (int)(ImGui::GetWindowPos().y - m_ImGuiViewportMain.y);
            m_ImGuiViewport.Width = (int)ImGui::GetWindowWidth();
            m_ImGuiViewport.Height = (int)ImGui::GetWindowHeight();
            m_ImGuiViewport.MouseX = (int)ImGui::GetMousePos().x;
            m_ImGuiViewport.MouseY = (int)ImGui::GetMousePos().y;

            glm::vec2 viewportPanelSize = glm::vec2(viewportSize.x, viewportSize.y);

            ResizeViewport(viewportPanelSize, m_RenderFramebuffer);

            uint64_t textureID = m_RenderFramebuffer->GetTextureAttachmentColor()->GetID();
            ImGui::Image((void*)(intptr_t)textureID, ImVec2{ m_ViewportMainSize.x, m_ViewportMainSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

            UpdateImGuizmo(mainWindow);

            auto windowSize = ImGui::GetWindowSize();
            ImVec2 minBound = ImGui::GetWindowPos();

            minBound.x += viewportOffset.x;
            // minBound.y += viewportOffset.y;

            ImVec2 maxBound = { minBound.x + windowSize.x, minBound.y + windowSize.y };
            m_ViewportBounds[0] = { minBound.x, minBound.y };
            m_ViewportBounds[1] = { maxBound.x, maxBound.y };

            SetViewportBounds(m_ViewportBounds);
            m_AllowViewportCameraEvents = ImGui::IsMouseHoveringRect(minBound, maxBound); // EditorLayer
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
            uint64_t textureID = m_SceneRenderer->s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetID();
            ImGui::Image((void*)(intptr_t)textureID, ImVec2{ m_ViewportEnvMapSize.x, m_ViewportEnvMapSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
        }
        ImGui::End();

        ImGui::PopStyleVar();
    }

    uint32_t id = 0;
    auto meshEntities = m_EditorScene->GetAllEntitiesWith<Hazel::MeshComponent>();
    for (auto entt : meshEntities)
    {
        Hazel::Entity entity = { entt, m_EditorScene.Raw() };
        auto& meshComponent = entity.GetComponent<Hazel::MeshComponent>();
        if (meshComponent.Mesh) {
            meshComponent.Mesh->OnImGuiRender(++id);
        }
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0.8f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
    ImGui::Begin("Toolbar");

    if (m_SceneState == SceneState::Edit)
    {
        //  float physics2DGravity = m_EditorScene->GetPhysics2DGravity();
        //  if (ImGuiWrapper::Property("Gravity", physics2DGravity, -10000.0f, 10000.0f, PropertyFlag::DragProperty))
        //  {
        //      m_EditorScene->SetPhysics2DGravity(physics2DGravity);
        //  }

        if (ImGui::ImageButton((ImTextureID)(uint64_t)(m_PlayButtonTex->GetID()), ImVec2(32, 32), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), ImVec4(0.9f, 0.9f, 0.9f, 1.0f)))
        {
            OnScenePlay();
        }
    }
    else if (m_SceneState == SceneState::Play)
    {
        //  float physics2DGravity = m_RuntimeScene->GetPhysics2DGravity();
        //  
        //  if (ImGuiWrapper::Property("Gravity", physics2DGravity, -10000.0f, 10000.0f, PropertyFlag::DragProperty))
        //  {
        //      m_RuntimeScene->SetPhysics2DGravity(physics2DGravity);
        //  }

        if (ImGui::ImageButton((ImTextureID)(uint64_t)(m_PlayButtonTex->GetID()), ImVec2(32, 32), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(1.0f, 1.0f, 1.0f, 0.2f)))
        {
            OnSceneStop();
        }
    }
    ImGui::SameLine();
    if (ImGui::ImageButton((ImTextureID)(uint64_t)(m_PlayButtonTex->GetID()), ImVec2(32, 32), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), ImVec4(1.0f, 1.0f, 1.0f, 0.6f)))
    {
        MORAVA_CORE_INFO("PLAY!");
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

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

            SelectedSubmesh selectedSubmesh = EntitySelection::s_SelectionContext[0];

            // Entity transform
            auto& tc = selectedSubmesh.Entity.GetComponent<Hazel::TransformComponent>();
            glm::mat4 entityTransform = tc.GetTransform();

            if (s_SelectionMode == SelectionMode::Entity || !selectedSubmesh.Mesh)
            {
                transformImGui = entityTransform;
            }
            else if (s_SelectionMode == SelectionMode::SubMesh)
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

            bool isTranslationChanged = ImGuiWrapper::DrawVec3Control("Translation", translation, 0.0f, 80.0f);
            bool isRotationChanged = ImGuiWrapper::DrawVec3Control("Rotation", rotationDegrees, 0.0f, 80.0f);
            bool isScaleChanged = ImGuiWrapper::DrawVec3Control("Scale", scale, 1.0f, 80.0f);

            if (isTranslationChanged || isRotationChanged || isScaleChanged)
            {
                rotationRadians = glm::radians(rotationDegrees);

                if (s_SelectionMode == SelectionMode::Entity || !selectedSubmesh.Mesh)
                {
                    glm::vec3 deltaRotation = rotationRadians - tc.Rotation;
                    tc.Translation = translation;
                    tc.Rotation += deltaRotation;
                    tc.Scale = scale;
                }
                else if (s_SelectionMode == SelectionMode::SubMesh)
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

                ImGuiWrapper::Property("Light Direction", light.Direction, -180.0f, 180.0f, PropertyFlag::SliderProperty);
                ImGuiWrapper::Property("Light Radiance", light.Radiance, PropertyFlag::ColorProperty);
                ImGuiWrapper::Property("Light Multiplier", light.Multiplier, 0.0f, 5.0f, PropertyFlag::SliderProperty);
                ImGuiWrapper::Property("Exposure", m_ActiveCamera->GetExposure(), 0.0f, 40.0f, PropertyFlag::SliderProperty);
                ImGuiWrapper::Property("Skybox Exposure Factor", m_SkyboxExposureFactor, 0.0f, 10.0f, PropertyFlag::SliderProperty);

                ImGuiWrapper::Property("Radiance Prefiltering", m_RadiancePrefilter);
                ImGuiWrapper::Property("Env Map Rotation", m_EnvMapRotation, -360.0f, 360.0f);

                if (m_SceneState == SceneState::Edit) {
                    //  float physics2DGravity = m_EditorScene->GetPhysics2DGravity();
                    //  if (ImGuiWrapper::Property("Gravity", physics2DGravity, -10000.0f, 10000.0f, PropertyFlag::DragProperty)) {
                    //      m_EditorScene->SetPhysics2DGravity(physics2DGravity);
                    //  }
                }
                else if (m_SceneState == SceneState::Play) {
                    //  float physics2DGravity = m_RuntimeScene->GetPhysics2DGravity();
                    //  if (ImGuiWrapper::Property("Gravity", physics2DGravity, -10000.0f, 10000.0f, PropertyFlag::DragProperty)) {
                    //      m_RuntimeScene->SetPhysics2DGravity(physics2DGravity);
                    //  }
                }

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
                    std::string fullPath = Application::Get()->OpenFile();
                    if (fullPath != "")
                    {
                        Hazel::Entity entity = LoadEntity(fullPath);
                    }
                }

                auto meshEntities = m_EditorScene->GetAllEntitiesWith<Hazel::MeshComponent>();
                if (meshEntities.size())
                {
                    meshEntity = GetMeshEntity();
                    auto& meshComponent = meshEntity->GetComponent<Hazel::MeshComponent>();
                    if (meshComponent.Mesh) {
                        ImGui::SameLine();
                        ImGui::Checkbox("Is Animated", &meshComponent.Mesh->IsAnimated());
                    }
                }
            }
        }
    }
    ImGui::End();
    /**** END Environment Map Scene Settings ****/

    ImGui::Begin("EnvMap Materials");
    {
        unsigned int materialIndex = 0;
        for (auto material_it = s_EnvMapMaterials.cbegin(); material_it != s_EnvMapMaterials.cend();)
        {
            EnvMapMaterial* material = material_it->second;
            std::string materialName = material->GetName();
            MaterialUUID materialUUID = material->GetUUID();

            // Material section
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)materialIndex++, flags, materialName.c_str());

            bool materialDelete = false;
            bool materialClone = false;

            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Delete Material"))
                {
                    materialDelete = true;
                }

                if (ImGui::MenuItem("Clone Material"))
                {
                    materialClone = true;
                }

                ImGui::EndPopup();
            }

            if (opened)
            {
                ImGuiWrapper::DrawMaterialUI(material, s_CheckerboardTexture);

                ImGui::TreePop();
            }

            if (materialClone) {
                auto envMapMaterialSrc = s_EnvMapMaterials.at(materialUUID);
                EnvMapMaterial* envMapMaterialDst = new EnvMapMaterial(NewMaterialName(), envMapMaterialSrc);
                s_EnvMapMaterials.insert(std::make_pair(envMapMaterialDst->GetUUID(), envMapMaterialDst));
            }

            if (materialDelete) {
                material_it = s_EnvMapMaterials.erase(material_it++);
            }
            else {
                ++material_it;
            }
        }
    }

    // Right-click on blank space
    if (ImGui::BeginPopupContextWindow(0, 1, false))
    {
        if (ImGui::MenuItem("Create a Material"))
        {
            std::string materialName = NewMaterialName();
            EnvMapMaterial* envMapMaterial = CreateDefaultMaterial(materialName);
            s_EnvMapMaterials.insert(std::make_pair(envMapMaterial->GetUUID(), envMapMaterial));
        }
        ImGui::EndPopup();
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
        const char* label = s_SelectionMode == SelectionMode::Entity ? "Entity" : "Mesh";
        if (ImGui::Button(label))
        {
            s_SelectionMode = s_SelectionMode == SelectionMode::Entity ? SelectionMode::SubMesh : SelectionMode::Entity;
        }

        std::string entityTag = "N/A";
        std::string meshName = "N/A";
        SubmeshUUID submeshUUID = "N/A";
        Hazel::Entity* entity = nullptr;

        if (EntitySelection::s_SelectionContext.size())
        {
            SelectedSubmesh selectedSubmesh = EntitySelection::s_SelectionContext[0];

            entity = &selectedSubmesh.Entity;
            entityTag = selectedSubmesh.Entity.GetComponent<Hazel::TagComponent>().Tag;
            meshName = (selectedSubmesh.Mesh) ? selectedSubmesh.Mesh->MeshName : "N/A";
            submeshUUID = GetSubmeshUUID(entity, selectedSubmesh.Mesh);
        }

        ImGui::Text("Selected Entity: ");
        ImGui::SameLine();
        ImGui::Text(entityTag.c_str());

        ImGui::Text("Selected Mesh: ");
        ImGui::SameLine();
        ImGui::Text(meshName.c_str());

        // Drop down for selecting a material for a specific submesh
        std::vector<std::string> materialNameStrings;
        int index = 0;
        for (auto& material : s_EnvMapMaterials) {
            materialNameStrings.push_back(material.second->GetName());
        }

        std::string submeshMaterialName = materialNameStrings.size() ? materialNameStrings[0] : "N/A";

        MaterialUUID materialUUID;
        if (s_SubmeshMaterialUUIDs.contains(submeshUUID)) {
            materialUUID = s_SubmeshMaterialUUIDs.at(submeshUUID);
        }
        int selectedMaterial = -1;

        if (ImGui::BeginCombo("Material", submeshMaterialName.c_str()))
        {
            size_t emm_index = 0;
            for (auto emm_it = s_EnvMapMaterials.begin(); emm_it != s_EnvMapMaterials.end(); emm_it++)
            {
                bool is_selected = (submeshMaterialName == materialNameStrings[emm_index]);
                if (ImGui::Selectable(materialNameStrings.at(emm_index).c_str(), is_selected))
                {
                    submeshMaterialName = materialNameStrings[emm_index];
                    materialUUID = emm_it->second->GetUUID();
                    if (meshName != "N/A" && submeshMaterialName != "N/A" && submeshUUID != "N/A")
                    {
                        auto sm_it = s_SubmeshMaterialUUIDs.find(submeshUUID);
                        if (sm_it != s_SubmeshMaterialUUIDs.end()) {
                            sm_it->second = materialUUID;
                            Log::GetLogger()->debug("s_SubmeshMaterialUUIDs UPDATE [ SubmeshUUID: '{0}' => MaterialUUID: '{1}', Items: {2} ]",
                                submeshUUID, materialUUID, s_SubmeshMaterialUUIDs.size());
                        }
                        else {
                            s_SubmeshMaterialUUIDs.insert(std::make_pair(submeshUUID, materialUUID));
                            Log::GetLogger()->debug("s_SubmeshMaterialUUIDs INSERT [ SubmeshUUID: '{0}' => MaterialUUID: '{1}', Items: {2} ]",
                                submeshUUID, materialUUID, s_SubmeshMaterialUUIDs.size());
                        }
                    }
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
                emm_index++;
            }
            ImGui::EndCombo();
        }
    }
    ImGui::End();

    ImGui::Begin("Help");
    {
        ImGui::Text("* Left ALT + Left Mouse Button - Pan");
        ImGui::Text("* Left ALT + Middle Mouse Button - Rotate/Orbit");
        ImGui::Text("* Left ALT + Right Mouse Button - Zoom");
        ImGui::Text("* Left SHIFT + R - Toggle Wireframe");
    }
    ImGui::End();

    m_SceneHierarchyPanel->OnImGuiRender();

    ImGui::ShowMetricsWindow();

    ImVec2 workPos = ImGui::GetMainViewport()->GetWorkPos();
    m_WorkPosImGui = glm::vec2(workPos.x, workPos.y);
}

// Demonstrate using DockSpace() to create an explicit docking node within an existing window.
// Note that you already dock windows into each others _without_ a DockSpace() by just moving windows 
// from their title bar (or by holding SHIFT if io.ConfigDockingWithShift is set).
// DockSpace() is only useful to construct to a central location for your application.
void EnvironmentMap::ShowExampleAppDockSpace(bool* p_open, Window* mainWindow)
{
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
            if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                NewScene();
            }

            if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {
                OpenScene();
            }

            if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                SaveScene();
            }

            if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S")) {
                SaveSceneAs();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit")) {
                p_open = false;
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

        if (ImGui::BeginMenu("Script"))
        {
            if (ImGui::MenuItem("Reload C# Assembly")) {
                Hazel::ScriptEngine::ReloadAssembly("assets/scripts/ExampleApp.dll");
            }

            ImGui::MenuItem("Reload assembly on play", nullptr, &m_ReloadScriptOnPlay);
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

void EnvironmentMap::NewScene()
{
    OnNewScene(m_ViewportMainSize);
}
 
void EnvironmentMap::OpenScene()
{
    // auto app = Application::Get();
    std::string filepath = Hazel::FileDialogs::OpenFile("Hazel Scene (*.hsc)\0*.hsc\0");
    if (!filepath.empty())
    {
        Hazel::Ref<Hazel::HazelScene> newScene = Hazel::Ref<Hazel::HazelScene>::Create();
        Hazel::SceneSerializer serializer(newScene);
        serializer.Deserialize(filepath);
        m_EditorScene = newScene;
        std::filesystem::path path = filepath;
        UpdateWindowTitle(path.filename().string());
        m_SceneHierarchyPanel->SetContext(m_EditorScene);
        Hazel::ScriptEngine::SetSceneContext(m_EditorScene);

        m_EditorScene->SetSelectedEntity({});
        EntitySelection::s_SelectionContext.clear();

        m_SceneFilePath = filepath;

        OnNewScene(m_ViewportMainSize);
    }
}

void EnvironmentMap::SaveScene()
{
    if (!m_SceneFilePath.empty()) {
        Hazel::SceneSerializer serializer(m_EditorScene);
        serializer.Serialize(m_SceneFilePath);
    }
}

void EnvironmentMap::SaveSceneAs()
{
    auto app = Application::Get();
    std::string filepath = app->SaveFile("Hazel Scene (*.hsc)\0*.hsc\0");
    Hazel::SceneSerializer serializer(m_EditorScene);
    serializer.Serialize(filepath);

    std::filesystem::path path = filepath;
    UpdateWindowTitle(path.filename().string());
    m_SceneFilePath = filepath;
}

void EnvironmentMap::OnNewScene(glm::vec2 viewportSize)
{
    // m_SceneRenderer->s_Data.ActiveScene = new Hazel::HazelScene();
    m_EditorScene->OnViewportResize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
    m_SceneHierarchyPanel->SetContext(m_EditorScene);
}

void EnvironmentMap::SelectEntity(Hazel::Entity e)
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
    m_SceneRenderer->GetShaderSkybox()->setFloat("u_TextureLod", m_EditorScene->GetSkyboxLOD());
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

SubmeshUUID EnvironmentMap::GetSubmeshUUID(Hazel::Entity* entity, Hazel::Submesh* submesh)
{
    SubmeshUUID submeshUUID = "E_" + std::to_string(entity->GetHandle()) + "_S_" + submesh->MeshName;
    // Log::GetLogger()->debug("EnvironmentMap::GetSubmeshUUID: '{0}'", submeshUUID);
    return submeshUUID;
}

void EnvironmentMap::ResizeViewport(glm::vec2 viewportPanelSize, Framebuffer* renderFramebuffer)
{
    float currentTimestamp = Timer::Get()->GetCurrentTimestamp();

    // Cooldown
    if (currentTimestamp - m_ResizeViewport.lastTime < m_ResizeViewport.cooldown) return;
    m_ResizeViewport.lastTime = currentTimestamp;

    if (viewportPanelSize != m_ViewportMainSize && viewportPanelSize.x > 0 && viewportPanelSize.y > 0)
    {
        renderFramebuffer->Resize((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);
        m_ViewportMainSize = glm::vec2(viewportPanelSize.x, viewportPanelSize.y);
    }
}

void EnvironmentMap::SetupRenderFramebuffer()
{
    if (!m_IsViewportEnabled) return;

    uint32_t width = Application::Get()->GetWindow()->GetWidth();
    uint32_t height = Application::Get()->GetWindow()->GetHeight();

    m_RenderFramebuffer = new Framebuffer(width, height);
    m_RenderFramebuffer->AddAttachmentSpecification(width, height, AttachmentType::Texture, AttachmentFormat::Color);
    m_RenderFramebuffer->AddAttachmentSpecification(width, height, AttachmentType::Renderbuffer, AttachmentFormat::Depth);
    m_RenderFramebuffer->Generate(width, height);
}

void EnvironmentMap::OnEvent(Event& e)
{
    if (m_SceneState == SceneState::Edit)
    {
        if (m_ViewportPanelMouseOver) {
            m_EditorCamera->OnEvent(e);
        }

        m_EditorScene->OnEvent(e);
    }
    else if (m_SceneState == SceneState::Play)
    {
        m_RuntimeScene->OnEvent(e);
    }

    // m_ActiveCamera->OnEvent(e);

    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<KeyPressedEvent>(HZ_BIND_EVENT_FN(EnvironmentMap::OnKeyPressedEvent));
    dispatcher.Dispatch<MouseButtonPressedEvent>(HZ_BIND_EVENT_FN(EnvironmentMap::OnMouseButtonPressed));
}

bool EnvironmentMap::OnKeyPressedEvent(KeyPressedEvent& e)
{
    if (m_ViewportPanelFocused)
    {
        switch (e.GetKeyCode())
        {
            case (int)KeyCode::Q:
                Scene::s_ImGuizmoType = -1;
                break;

            /**** BEGIN UpdateImGuizmo
            case (int)KeyCode::W:
                Scene::s_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE;
                break;
            case (int)KeyCode::E:
                Scene::s_ImGuizmoType = ImGuizmo::OPERATION::ROTATE;
                break;
            case (int)KeyCode::R:
                Scene::s_ImGuizmoType = ImGuizmo::OPERATION::SCALE;
                break;
            END UpdateImGuizmo ****/

            case (int)KeyCode::Delete:
                if (EntitySelection::s_SelectionContext.size())
                {
                    Hazel::Entity selectedEntity = EntitySelection::s_SelectionContext[0].Entity;
                    m_EditorScene->DestroyEntity(selectedEntity);
                    EntitySelection::s_SelectionContext.clear();
                    m_EditorScene->SetSelectedEntity({});
                    m_SceneHierarchyPanel->SetSelected({});
                }
                break;
        }
    }

    if (Input::IsKeyPressed(MORAVA_KEY_LEFT_CONTROL))
    {
        switch (e.GetKeyCode())
        {
            case (int)KeyCode::G:
                // Toggle grid
                Hazel::SceneRenderer::GetOptions().ShowGrid = !Hazel::SceneRenderer::GetOptions().ShowGrid;
                break;
            case (int)KeyCode::B:
                // Toggle bounding boxes
                m_UIShowBoundingBoxes = !m_UIShowBoundingBoxes;
                ShowBoundingBoxes(m_UIShowBoundingBoxes, m_UIShowBoundingBoxesOnTop);
                break;
            case (int)KeyCode::D:
                if (EntitySelection::s_SelectionContext.size()) {
                    Hazel::Entity selectedEntity = EntitySelection::s_SelectionContext[0].Entity;
                    m_EditorScene->DuplicateEntity(selectedEntity);
                }
                break;
            case (int)KeyCode::S:
                SaveScene();
                break;
        }
    }

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

            auto meshEntities = m_EditorScene->GetAllEntitiesWith<Hazel::MeshComponent>();
            for (auto e : meshEntities)
            {
                Hazel::Entity entity = { e, m_EditorScene.Raw() };
                auto mesh = entity.GetComponent<Hazel::MeshComponent>().Mesh;
                if (!mesh) {
                    continue;
                }

                std::vector<Hazel::Submesh>& submeshes = mesh->GetSubmeshes();
                float lastT = std::numeric_limits<float>::max(); // Distance between camera and intersection in CastRay
                // for (Hazel::Submesh& submesh : submeshes)
                for (uint32_t i = 0; i < submeshes.size(); i++)
                {
                    Hazel::Submesh* submesh = &submeshes[i];
                    auto& transform = entity.GetComponent<Hazel::TransformComponent>().GetTransform();
                    Hazel::Ray ray = {
                        glm::inverse(transform * submesh->Transform) * glm::vec4(origin, 1.0f),
                        glm::inverse(glm::mat3(transform) * glm::mat3(submesh->Transform)) * direction
                    };

                    float t;
                    bool intersects = ray.IntersectsAABB(submesh->BoundingBox, t);
                    if (intersects)
                    {
                        const auto& triangleCache = ((Hazel::HazelMesh*)mesh.Raw())->GetTriangleCache(i);
                        if (triangleCache.size())
                        {
                            for (const auto& triangle : triangleCache)
                            {
                                if (ray.IntersectsTriangle(triangle.V0.Position, triangle.V1.Position, triangle.V2.Position, t))
                                {
                                    AddSubmeshToSelectionContext({ entity, submesh, t });

                                    Log::GetLogger()->debug("Adding submesh to selection context. Submesh Name: '{0}', selection size: '{1}'", 
                                        submesh->MeshName, EntitySelection::s_SelectionContext.size());
                                    break;
                                }
                            }
                        }
                        else {
                            AddSubmeshToSelectionContext({ entity, submesh, t });
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
                if (meshEntity) {
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
    m_SceneHierarchyPanel->SetSelected(selectionContext.Entity);
    m_EditorScene->SetSelectedEntity(selectionContext.Entity);
}

void EnvironmentMap::OnEntityDeleted(Hazel::Entity e)
{
    if (EntitySelection::s_SelectionContext.size())
    {
        if (EntitySelection::s_SelectionContext[0].Entity == e) {
            EntitySelection::s_SelectionContext.clear();
            m_EditorScene->SetSelectedEntity({});
        }
    }
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

    // auto meshEntities = m_EditorScene->GetAllEntitiesWith<Hazel::MeshComponent>();
    auto meshEntities = m_SceneHierarchyPanel->GetContext()->GetAllEntitiesWith<Hazel::MeshComponent>();

    // Render all entities with mesh component
    if (meshEntities.size())
    {
        for (auto entt : meshEntities)
        {
            Hazel::Entity entity = { entt, m_SceneHierarchyPanel->GetContext().Raw() };
            auto& meshComponent = entity.GetComponent<Hazel::MeshComponent>();

            if (meshComponent.Mesh)
            {
                m_ShaderHazelPBR = meshComponent.Mesh->IsAnimated() ? ShaderLibrary::Get("HazelPBR_Anim") : ShaderLibrary::Get("HazelPBR_Static");

                m_ShaderHazelPBR->Bind();
                {
                    EnvMapMaterial* envMapMaterial = nullptr;
                    std::string materialUUID;

                    for (Hazel::Submesh& submesh : meshComponent.Mesh->GetSubmeshes())
                    {
                        materialUUID = Hazel::HazelMesh::GetSubmeshMaterialUUID(meshComponent.Mesh.Raw(), submesh, entity);

                        // load submesh materials for each specific submesh from the s_EnvMapMaterials list
                        if (s_EnvMapMaterials.contains(materialUUID)) {
                            envMapMaterial = s_EnvMapMaterials.at(materialUUID);
                            UpdateShaderPBRUniforms(m_ShaderHazelPBR, envMapMaterial);
                        }

                        glm::mat4 entityTransform = entity.GetComponent<Hazel::TransformComponent>().GetTransform();
                        submesh.Render(meshComponent.Mesh.Raw(), m_ShaderHazelPBR, entityTransform, samplerSlot, s_EnvMapMaterials, entity);
                    }
                }
                m_ShaderHazelPBR->Unbind();
            }
        }
    }

    Hazel::Renderer2D::BeginScene(viewProj, true);
    {
        RendererBasic::SetLineThickness(2.0f);

        if (m_DisplayRay)
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
                    glm::vec4 color = s_SelectionMode == SelectionMode::Entity ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) : glm::vec4(0.2f, 0.9f, 0.2f, 1.0f);
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

void EnvironmentMap::OnRender(Framebuffer* framebuffer, Window* mainWindow)
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
    }

    OnRenderEditor(framebuffer);
    // OnRenderRuntime(framebuffer)

    if (m_IsViewportEnabled)
    {
        m_RenderFramebuffer->Unbind();
    }
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
