#include "EnvMapEditorLayer.h"

#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Renderer/RenderPass.h"
#include "Hazel/Scene/SceneSerializer.h"
#include "Hazel/Script/ScriptEngine.h"
#include "Hazel/Utils/PlatformUtils.h"

#include "../../ImGuizmo/ImGuizmo.h"

#include "Core/MousePicker.h"
#include "Core/Util.h"
#include "EnvMap/EnvMapSceneRenderer.h"
#include "ImGui/ImGuiWrapper.h"
#include "Light/PointLight.h"
#include "Material/MaterialLibrary.h"
#include "Mesh/GeometryFactory.h"
#include "Renderer/RendererBasic.h"
#include "Shader/MoravaShaderLibrary.h"

#include <filesystem>


SelectionMode EnvMapEditorLayer::s_SelectionMode = SelectionMode::Entity;
Hazel::Ref<Hazel::HazelTexture2D> EnvMapEditorLayer::s_CheckerboardTexture;
Hazel::Ref<EnvMapMaterial> EnvMapEditorLayer::s_DefaultMaterial;
Hazel::Ref<EnvMapMaterial> EnvMapEditorLayer::s_LightMaterial;


EnvMapEditorLayer::EnvMapEditorLayer(const std::string& filepath, Scene* scene)
{
    glDebugMessageCallback(Util::OpenGLLogMessage, nullptr);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    EnvMapSharedData::s_SamplerSlots = std::map<std::string, unsigned int>();

    //  // PBR texture inputs
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("albedo",     1)); // uniform sampler2D u_AlbedoTexture
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("normal",     2)); // uniform sampler2D u_NormalTexture
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("metalness",  3)); // uniform sampler2D u_MetalnessTexture
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("roughness",  4)); // uniform sampler2D u_RoughnessTexture
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("emissive",   5)); // uniform sampler2D u_EmissiveTexture
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("ao",         6)); // uniform sampler2D u_AOTexture
    // Environment maps
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("radiance",   7)); // uniform samplerCube u_EnvRadianceTex
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("irradiance", 8)); // uniform samplerCube u_EnvIrradianceTex
    // BRDF LUT
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("BRDF_LUT",   9)); // uniform sampler2D u_BRDFLUTTexture
    // Shadow Map Directional Light
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("shadow",      10)); // uniform sampler2D u_ShadowMap
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("shadow_omni", 11)); // uniform samplerCube omniShadowMaps[i].shadowMap

    // Skybox.fs         - uniform samplerCube u_Texture;
    // SceneComposite.fs - uniform sampler2DMS u_Texture;
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("u_Texture",  1));

    EnvMapSharedData::s_SkyboxCube = Hazel::Ref<CubeSkybox>::Create();
    EnvMapSharedData::s_Quad = Hazel::Ref<Quad>::Create();

    EnvMapSharedData::s_EditorScene = Hazel::Ref<Hazel::HazelScene>::Create();
    EnvMapSharedData::s_EditorScene->SetSkyboxLod(0.1f);

    EnvMapSceneRenderer::Init(filepath, EnvMapSharedData::s_EditorScene.Raw());
    SetSkybox(EnvMapSceneRenderer::GetRadianceMap());

    SetupContextData(scene);

    // Create a default material
    s_DefaultMaterial = MaterialLibrary::CreateDefaultMaterial("MAT_DEF");
    MaterialLibrary::AddEnvMapMaterial(s_DefaultMaterial->GetUUID(), s_DefaultMaterial);

    // Create the light material
    s_LightMaterial = MaterialLibrary::CreateDefaultMaterial("MAT_LIGHT");
    // Load Hazel/Renderer/HazelTexture
    s_LightMaterial->GetAlbedoInput().TextureMap = ResourceManager::LoadHazelTexture2D("Textures/light_bulb.png");
    s_LightMaterial->GetAlbedoInput().UseTexture = true;
    MaterialLibrary::AddEnvMapMaterial(s_LightMaterial->GetUUID(), s_LightMaterial);

    Init(); // requires a valid Camera reference

    m_SceneHierarchyPanel = new Hazel::SceneHierarchyPanel(EnvMapSharedData::s_EditorScene);
    m_SceneHierarchyPanel->SetSelectionChangedCallback(std::bind(&EnvMapEditorLayer::SelectEntity, this, std::placeholders::_1));
    m_SceneHierarchyPanel->SetEntityDeletedCallback(std::bind(&EnvMapEditorLayer::OnEntityDeleted, this, std::placeholders::_1));
    m_SceneHierarchyPanel->SetContext(EnvMapSharedData::s_EditorScene); // already done in constructor
    Hazel::ScriptEngine::SetSceneContext(EnvMapSharedData::s_EditorScene);
    EnvMapSharedData::s_EditorScene->SetSelectedEntity({});

    s_CheckerboardTexture = Hazel::HazelTexture2D::Create("Textures/Hazel/Checkerboard.tga");
    m_PlayButtonTex = Hazel::HazelTexture2D::Create("Textures/Hazel/PlayButton.png");

    m_DisplayBoundingBoxes = false;
    m_DrawOnTopBoundingBoxes = true; // obsolete?
    m_DisplayLineElements = false;

    EnvMapSharedData::s_DisplayOutline = false;
    EnvMapSharedData::s_SkyboxExposureFactor = 1.0f;
    EnvMapSharedData::s_RadiancePrefilter = false;
    EnvMapSharedData::s_EnvMapRotation = 0.0f;
    EnvMapSharedData::s_DisplayHazelGrid = true;
    EnvMapSharedData::s_DisplayRay = false;

    Scene::s_ImGuizmoTransform = nullptr; // &GetMeshEntity()->Transform();
    Scene::s_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE;

    m_ResizeViewport = { 0.0f, 1.0f };

    SetupRenderFramebuffer();

    m_WindowTitleStatic = Application::Get()->GetWindow()->GetTitle();
    UpdateWindowTitle("New Scene");

    EnvMapSharedData::s_ShadowMapDirLight = Hazel::Ref<ShadowMap>::Create();
    EnvMapSharedData::s_ShadowMapDirLight->Init(scene->GetSettings().shadowMapWidth, scene->GetSettings().shadowMapHeight);

    m_LightDirection = glm::normalize(glm::vec3(0.05f, -0.85f, 0.05f));
    m_LightProjectionMatrix = glm::ortho(-64.0f, 64.0f, -64.0f, 64.0f, -64.0f, 64.0f);

    EnvMapSharedData::s_OmniShadowMapPointLight = Hazel::Ref<OmniShadowMap>::Create();
    EnvMapSharedData::s_OmniShadowMapPointLight->Init(scene->GetSettings().omniShadowMapWidth, scene->GetSettings().omniShadowMapHeight);

    EnvMapSharedData::s_OmniShadowMapSpotLight = Hazel::Ref<OmniShadowMap>::Create();
    EnvMapSharedData::s_OmniShadowMapSpotLight->Init(scene->GetSettings().omniShadowMapWidth, scene->GetSettings().omniShadowMapHeight);

    GeometryFactory::Quad::Create();
}

void EnvMapEditorLayer::Init()
{
    Application::Get()->GetWindow()->SetEventCallback(HZ_BIND_EVENT_FN(EnvMapEditorLayer::OnEvent));

    SetupShaders();

    // Hazel::ScriptEngine::Init(""); // TODO Assembly path

    // Temporary code Hazel LIVE! #004
    Hazel::HazelRenderer::Init();

    bool depthTest = true;
    Hazel::Renderer2D::Init();
}

EnvMapEditorLayer::~EnvMapEditorLayer()
{
    MaterialLibrary::Cleanup();

    delete EnvMapSharedData::s_RuntimeCamera;
    delete EnvMapSharedData::s_EditorCamera;

    GeometryFactory::Quad::Destroy();
}

void EnvMapEditorLayer::SetupContextData(Scene* scene)
{
    MaterialLibrary::Init();

    auto viewportWidth = m_ViewportBounds[1].x - m_ViewportBounds[0].x;
    auto viewportHeight = m_ViewportBounds[1].y - m_ViewportBounds[0].y;

    float fov = 60.0f;
    float aspectRatio = 1.778f; // 16/9
    EnvMapSharedData::s_EditorCamera = new Hazel::EditorCamera(fov, aspectRatio, 0.1f, 1000.0f);
    EnvMapSharedData::s_RuntimeCamera = new RuntimeCamera(scene->GetSettings().cameraPosition, scene->GetSettings().cameraStartYaw, scene->GetSettings().cameraStartPitch,
        fov, aspectRatio, scene->GetSettings().cameraMoveSpeed, 0.1f);

    EnvMapSharedData::s_EditorCamera->SetProjectionType(Hazel::SceneCamera::ProjectionType::Perspective);
    EnvMapSharedData::s_RuntimeCamera->SetProjectionType(Hazel::SceneCamera::ProjectionType::Perspective);

    EnvMapSharedData::s_EditorCamera->SetViewportSize((float)Application::Get()->GetWindow()->GetWidth(), (float)Application::Get()->GetWindow()->GetHeight());
    EnvMapSharedData::s_RuntimeCamera->SetViewportSize((float)Application::Get()->GetWindow()->GetWidth(), (float)Application::Get()->GetWindow()->GetHeight());

    Hazel::Entity cameraEntity = CreateEntity("Camera");
    cameraEntity.AddComponent<Hazel::CameraComponent>(*EnvMapSharedData::s_RuntimeCamera);

    EnvMapSharedData::s_ActiveCamera = EnvMapSharedData::s_RuntimeCamera;

    Log::GetLogger()->debug("cameraEntity UUID: {0}", cameraEntity.GetUUID());

    // auto mapGenerator = CreateEntity("Map Generator");
    // mapGenerator.AddComponent<Hazel::ScriptComponent>("Example.MapGenerator");

    // Hazel::HazelMesh* meshQuad = new Hazel::HazelMesh("Models/Primitives/quad.obj", m_ShaderHazelPBR, nullptr, false);

    m_DirectionalLightEntity = CreateEntity("Directional Light");
    auto& tc = m_DirectionalLightEntity.GetComponent<Hazel::TransformComponent>();
    // tc.Rotation = EnvMapSceneRenderer::GetActiveLight().Direction;
    tc.Rotation = glm::normalize(glm::vec3(-0.05f, -0.85f, -0.05f));
    // m_DirectionalLightEntity.AddComponent<Hazel::MeshComponent>(meshQuad);
    auto& dlc = m_DirectionalLightEntity.AddComponent<Hazel::DirectionalLightComponent>();

    EnvMapSharedData::s_PointLightEntity = CreateEntity("Point Light");
    // m_PointLightEntity.AddComponent<Hazel::MeshComponent>(meshQuad);
    auto& plc = EnvMapSharedData::s_PointLightEntity.AddComponent<Hazel::PointLightComponent>();

    EnvMapSharedData::s_SpotLightEntity = CreateEntity("Spot Light");
    // m_SpotLightEntity.AddComponent<Hazel::MeshComponent>(meshQuad);
    auto& slc = EnvMapSharedData::s_SpotLightEntity.AddComponent<Hazel::SpotLightComponent>();
    auto& sltc = EnvMapSharedData::s_SpotLightEntity.GetComponent<Hazel::TransformComponent>();
    sltc.Rotation = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));
}

void EnvMapEditorLayer::SetupRenderFramebuffer()
{
    uint32_t width = Application::Get()->GetWindow()->GetWidth();
    uint32_t height = Application::Get()->GetWindow()->GetHeight();

    m_RenderFramebuffer = Hazel::Ref<Framebuffer>::Create(width, height);
    m_RenderFramebuffer->AddColorAttachmentSpecification(width, height, AttachmentType::Texture, AttachmentFormat::Color);
    m_RenderFramebuffer->AddDepthAttachmentSpecification(width, height, AttachmentType::Texture, AttachmentFormat::Depth);
    m_RenderFramebuffer->Generate(width, height);

    // post processing framebuffer
    m_PostProcessingFramebuffer = Hazel::Ref<Framebuffer>::Create(width, height);
    m_PostProcessingFramebuffer->AddColorAttachmentSpecification(width, height, AttachmentType::Texture, AttachmentFormat::Color);
    m_PostProcessingFramebuffer->Generate(width, height);
}

Hazel::Entity EnvMapEditorLayer::CreateEntity(const std::string& name)
{
    // Both NoECS and ECS
    Hazel::Entity entity = EnvMapSharedData::s_EditorScene->CreateEntity(name, EnvMapSharedData::s_EditorScene);

    return entity;
}

Hazel::Entity EnvMapEditorLayer::LoadEntity(std::string fullPath)
{
    std::string fileName = Util::GetFileNameFromFullPath(fullPath);
    std::string fileNameNoExt = Util::StripExtensionFromFileName(fileName);

    bool isAnimated = false;
    EnvMapSharedData::s_ShaderHazelPBR = MoravaShaderLibrary::Get("HazelPBR_Static");

    Log::GetLogger()->debug("EnvMapEditorLayer::LoadMesh: fullPath '{0}' fileName '{1}' fileNameNoExt '{2}'", fullPath, fileName, fileNameNoExt);

    Hazel::HazelMesh* mesh = new Hazel::HazelMesh(fullPath, EnvMapSharedData::s_ShaderHazelPBR, Hazel::Ref<Hazel::HazelMaterial>(), isAnimated);

    mesh->SetTimeMultiplier(1.0f);

    EnvMapSceneRenderer::CreateDrawCommand(fileNameNoExt, mesh);

    Hazel::Entity meshEntity = CreateEntity(fileNameNoExt);
    // m_MeshEntity: NoECS version
    meshEntity.AddComponent<Hazel::MeshComponent>(mesh);
    meshEntity.AddComponent<Hazel::ScriptComponent>("Example.Script");

    EnvMapSceneRenderer::SubmitEntityEnvMap(meshEntity);
    // LoadEnvMapMaterials(mesh, meshEntity);

    return meshEntity;
}

Hazel::CameraComponent EnvMapEditorLayer::GetMainCameraComponent()
{
    auto mainCameraEntity = EnvMapSharedData::s_EditorScene->GetMainCameraEntity();
    if (mainCameraEntity && mainCameraEntity.HasComponent<Hazel::CameraComponent>())
    {
        auto mainCameraComponent = mainCameraEntity.GetComponent<Hazel::CameraComponent>();
        return mainCameraComponent;
    }
    return {};
}

void EnvMapEditorLayer::AddSubmeshToSelectionContext(SelectedSubmesh submesh)
{
    EntitySelection::s_SelectionContext.push_back(submesh);

    if (EntitySelection::s_SelectionContext.size() && EntitySelection::s_SelectionContext[0].Mesh != nullptr) {
        Log::GetLogger()->debug("SelectionContext[0].Mesh->MeshName: '{0}'", EntitySelection::s_SelectionContext[0].Mesh->MeshName);
    }
}

void EnvMapEditorLayer::ShowBoundingBoxes(bool showBoundingBoxes, bool showBoundingBoxesOnTop)
{
}

void EnvMapEditorLayer::SetupShaders()
{
    Hazel::Ref<MoravaShader> shaderHazelPBR_Static = MoravaShader::Create("Shaders/Hazel/HazelPBR_Static.vs", "Shaders/Hazel/HazelPBR.fs");
    Log::GetLogger()->info("EnvMapEditorLayer: m_ShaderHazelPBR_Static compiled [programID={0}]", shaderHazelPBR_Static->GetProgramID());

    Hazel::Ref<MoravaShader> shaderHazelPBR_Anim = MoravaShader::Create("Shaders/Hazel/HazelPBR_Anim.vs", "Shaders/Hazel/HazelPBR.fs");
    Log::GetLogger()->info("EnvMapEditorLayer: m_ShaderHazelPBR_Anim compiled [programID={0}]", shaderHazelPBR_Anim->GetProgramID());

    Hazel::Ref<MoravaShader> shaderRenderer2D_Line = MoravaShader::Create("Shaders/Hazel/Renderer2D_Line.vs", "Shaders/Hazel/Renderer2D_Line.fs");
    Log::GetLogger()->info("EnvMapEditorLayer: m_ShaderRenderer2D_Line compiled [programID={0}]", shaderRenderer2D_Line->GetProgramID());

    EnvMapSharedData::s_ShaderOutline = MoravaShader::Create("Shaders/Hazel/Outline.vs", "Shaders/Hazel/Outline.fs");
    Log::GetLogger()->info("EnvMapEditorLayer: shaderOutline compiled [programID={0}]", EnvMapSharedData::s_ShaderOutline->GetProgramID());

    m_ShaderShadow = MoravaShader::Create("Shaders/directional_shadow_map.vert", "Shaders/directional_shadow_map.frag");
    Log::GetLogger()->info("EnvMapEditorLayer: m_ShaderShadow compiled [programID={0}]", m_ShaderShadow->GetProgramID());

    m_ShaderOmniShadow = MoravaShader::Create("Shaders/omni_shadow_map.vert", "Shaders/omni_shadow_map.geom", "Shaders/omni_shadow_map.frag");
    Log::GetLogger()->info("EnvMapEditorLayer: m_ShaderOmniShadow compiled [programID={0}]", m_ShaderOmniShadow->GetProgramID());

    m_ShaderPostProcessing = MoravaShader::Create("Shaders/env_map_post_processing.vert", "Shaders/env_map_post_processing.frag");
    Log::GetLogger()->info("EnvMapEditorLayer: m_ShaderPostProcessing compiled [programID={0}]", m_ShaderPostProcessing->GetProgramID());

    m_ShaderBloomBlur = MoravaShader::Create("Shaders/env_map_post_processing_bloom_blur.vert", "Shaders/env_map_post_processing_bloom_blur.frag");
    Log::GetLogger()->info("EnvMapEditorLayer: m_ShaderBloomBlur compiled [programID={0}]", m_ShaderBloomBlur->GetProgramID());

    ResourceManager::AddShader("Hazel/HazelPBR_Static", shaderHazelPBR_Static);
    ResourceManager::AddShader("Hazel/HazelPBR_Anim", shaderHazelPBR_Anim);
    ResourceManager::AddShader("Hazel/Renderer2D_Line", shaderRenderer2D_Line);
    ResourceManager::AddShader("Hazel/Outline", EnvMapSharedData::s_ShaderOutline);
    ResourceManager::AddShader("directional_shadow_map", m_ShaderShadow);
    ResourceManager::AddShader("env_map_post_processing", m_ShaderPostProcessing);
    ResourceManager::AddShader("env_map_post_processing_bloom_blur", m_ShaderBloomBlur);

    MoravaShaderLibrary::Add(shaderHazelPBR_Static);
    MoravaShaderLibrary::Add(shaderHazelPBR_Anim);
    MoravaShaderLibrary::Add(shaderRenderer2D_Line);
    MoravaShaderLibrary::Add(EnvMapSharedData::s_ShaderOutline);
    MoravaShaderLibrary::Add(m_ShaderShadow);
    MoravaShaderLibrary::Add(m_ShaderPostProcessing);
    MoravaShaderLibrary::Add(m_ShaderBloomBlur);
}

void EnvMapEditorLayer::UpdateUniforms()
{
    /**** BEGIN Shaders/Hazel/SceneComposite ****/
    Hazel::Ref<MoravaShader> shaderComposite = EnvMapSceneRenderer::GetShaderComposite();
    shaderComposite->Bind();
    shaderComposite->SetInt("u_Texture", EnvMapSharedData::s_SamplerSlots.at("u_Texture"));

    // ERROR: OpenGLMoravaShader::SetFloat() failed [name='u_Exposure', location='-1']
    // shaderComposite->SetFloat("u_Exposure", GetMainCameraComponent().Camera.GetExposure());

    /**** END Shaders/Hazel/SceneComposite ****/

    /**** BEGIN Shaders/Hazel/Skybox ****/
    EnvMapSceneRenderer::s_ShaderSkybox->Bind();
    EnvMapSceneRenderer::s_ShaderSkybox->SetInt("u_Texture", EnvMapSharedData::s_SamplerSlots.at("u_Texture"));
    EnvMapSceneRenderer::s_ShaderSkybox->SetFloat("u_TextureLod", EnvMapSharedData::s_EditorScene->GetSkyboxLod());
    // apply exposure to Shaders/Hazel/Skybox, considering that Shaders/Hazel/SceneComposite is not yet enabled
    EnvMapSceneRenderer::s_ShaderSkybox->SetFloat("u_Exposure", GetMainCameraComponent().Camera.GetExposure() * EnvMapSharedData::s_SkyboxExposureFactor); // originally used in Shaders/Hazel/SceneComposite
    /**** END Shaders/Hazel/Skybox ****/

    /**** BEGIN Shaders/Hazel/Outline ****/
    EnvMapSharedData::s_ShaderOutline->Bind();
    glm::mat4 viewProj = EnvMapSceneRenderer::GetViewProjection();
    EnvMapSharedData::s_ShaderOutline->SetMat4("u_ViewProjection", viewProj);
    /**** BEGIN Shaders/Hazel/Outline ****/
}

void EnvMapEditorLayer::SetSkybox(Hazel::Ref<Hazel::HazelTextureCube> skybox)
{
    m_SkyboxTexture = skybox;
    m_SkyboxTexture->Bind(EnvMapSharedData::s_SamplerSlots.at("u_Texture"));
}

void EnvMapEditorLayer::OnUpdate(float timestep)
{
    switch (m_SceneState)
    {
    case SceneState::Edit:
        if (m_ViewportPanelFocused) {
            EnvMapSharedData::s_EditorCamera->OnUpdate(timestep);
        }
        EnvMapSharedData::s_EditorScene->OnRenderEditor(timestep, *EnvMapSharedData::s_EditorCamera);

        if (m_DrawOnTopBoundingBoxes)
        {
            // Hazel::HazelRenderer::BeginRenderPass(Hazel::SceneRenderer::GetFinalRenderPass(), false);
            // auto viewProj = s_EditorCamera->GetViewProjection();
            // Hazel::Renderer2D::BeginScene(viewProj, false);
            // // TODO: Renderer::DrawAABB(m_MeshEntity.GetComponent<MeshComponent>(), m_MeshEntity.GetComponent<TransformComponent>());
            // Hazel::Renderer2D::EndScene();
            // Hazel::HazelRenderer::EndRenderPass();
        }
        break;
    case SceneState::Play:
        if (m_ViewportPanelFocused) {
            EnvMapSharedData::s_EditorCamera->OnUpdate(timestep);
        }
        EnvMapSharedData::s_RuntimeScene->OnUpdate(timestep);
        EnvMapSharedData::s_RuntimeScene->OnRenderRuntime(timestep);
        break;
    case SceneState::Pause:
        if (m_ViewportPanelFocused) {
            EnvMapSharedData::s_EditorCamera->OnUpdate(timestep);
        }
        EnvMapSharedData::s_RuntimeScene->OnRenderRuntime(timestep);
        break;
    }

    CameraSyncECS();

    if (m_DirectionalLightEntity.HasComponent<Hazel::TransformComponent>())
    {
        auto& tc = m_DirectionalLightEntity.GetComponent<Hazel::TransformComponent>();
        EnvMapSceneRenderer::GetActiveLight().Direction = glm::eulerAngles(glm::quat(tc.Rotation));

        m_LightDirection = glm::eulerAngles(glm::quat(tc.Rotation));
        EnvMapSharedData::s_DirLightTransform = Util::CalculateLightTransform(m_LightProjectionMatrix, m_LightDirection);
    }

    OnUpdateEditor(EnvMapSharedData::s_EditorScene, timestep);
    // OnUpdateRuntime(s_RuntimeScene, timestep);
}

void EnvMapEditorLayer::OnUpdateEditor(Hazel::Ref<Hazel::HazelScene> scene, float timestep)
{
    EnvMapSharedData::s_EditorScene = scene;

    EnvMapSceneRenderer::BeginScene(EnvMapSharedData::s_EditorScene.Raw(), { GetMainCameraComponent().Camera, GetMainCameraComponent().Camera.GetViewMatrix() });

    UpdateUniforms();

    // Update HazelMesh List
    auto meshEntities = EnvMapSharedData::s_EditorScene->GetAllEntitiesWith<Hazel::MeshComponent>();
    for (auto entt : meshEntities)
    {
        Hazel::Entity entity{ entt, EnvMapSharedData::s_EditorScene.Raw() };
        Hazel::Ref<Hazel::HazelMesh> mesh = entity.GetComponent<Hazel::MeshComponent>().Mesh;
        if (mesh)
        {
            mesh->OnUpdate(timestep, false);
        }
    }

    EnvMapSharedData::s_ActiveCamera->OnUpdate(timestep);

    GetMainCameraComponent().Camera.SetViewMatrix(EnvMapSharedData::s_ActiveCamera->GetViewMatrix());
    GetMainCameraComponent().Camera.SetProjectionMatrix(EnvMapSharedData::s_ActiveCamera->GetProjectionMatrix());

    auto viewMatrix1 = EnvMapSharedData::s_ActiveCamera->GetViewMatrix();
    auto projectionMatrix1 = EnvMapSharedData::s_ActiveCamera->GetProjectionMatrix();

    auto viewMatrix2 = GetMainCameraComponent().Camera.GetViewMatrix();
    auto projectionMatrix2 = GetMainCameraComponent().Camera.GetProjectionMatrix();

    Scene::s_ImGuizmoTransform = &m_CurrentlySelectedTransform; // moved from SceneHazelEnvMap

    m_ViewportWidth = m_ViewportBounds[1].x - m_ViewportBounds[0].x;
    m_ViewportHeight = m_ViewportBounds[1].y - m_ViewportBounds[0].y;

    if (m_ViewportWidth > 0.0f && m_ViewportHeight > 0.0f) {
        EnvMapSharedData::s_ActiveCamera->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
    }
}

void EnvMapEditorLayer::OnUpdateRuntime(Hazel::Ref<Hazel::HazelScene> scene, float timestep)
{
    EnvMapSharedData::s_EditorScene = scene;

    EnvMapSceneRenderer::BeginScene(EnvMapSharedData::s_EditorScene.Raw(), { GetMainCameraComponent().Camera, GetMainCameraComponent().Camera.GetViewMatrix() });

    UpdateUniforms();

    // Update HazelMesh List
    auto meshEntities = EnvMapSharedData::s_EditorScene->GetAllEntitiesWith<Hazel::MeshComponent>();
    for (auto entt : meshEntities)
    {
        Hazel::Entity entity{ entt, EnvMapSharedData::s_EditorScene.Raw() };
        Hazel::Ref<Hazel::HazelMesh> mesh = entity.GetComponent<Hazel::MeshComponent>().Mesh;

        mesh->OnUpdate(timestep, false);
    }

    Scene::s_ImGuizmoTransform = &m_CurrentlySelectedTransform; // moved from SceneHazelEnvMap

    m_ViewportWidth = m_ViewportBounds[1].x - m_ViewportBounds[0].x;
    m_ViewportHeight = m_ViewportBounds[1].y - m_ViewportBounds[0].y;

    if (m_ViewportWidth > 0.0f && m_ViewportHeight > 0.0f) {
        EnvMapSharedData::s_ActiveCamera->SetViewportSize(m_ViewportWidth, m_ViewportHeight);
    }
}

void EnvMapEditorLayer::OnScenePlay()
{
    EntitySelection::s_SelectionContext.clear();

    m_SceneState = SceneState::Play;

    if (m_ReloadScriptOnPlay) {
        Hazel::ScriptEngine::ReloadAssembly("assets/scripts/ExampleApp.dll");
    }

    EnvMapSharedData::s_RuntimeScene = Hazel::Ref<Hazel::HazelScene>::Create();
    EnvMapSharedData::s_EditorScene->CopyTo(EnvMapSharedData::s_RuntimeScene);

    EnvMapSharedData::s_RuntimeScene->OnRuntimeStart();
    m_SceneHierarchyPanel->SetContext(EnvMapSharedData::s_RuntimeScene);
}

void EnvMapEditorLayer::OnSceneStop()
{
    EnvMapSharedData::s_RuntimeScene->OnRuntimeStop();
    m_SceneState = SceneState::Edit;

    // Unload runtime scene
    EnvMapSharedData::s_RuntimeScene = nullptr;

    EntitySelection::s_SelectionContext.clear();
    Hazel::ScriptEngine::SetSceneContext(EnvMapSharedData::s_EditorScene);
    m_SceneHierarchyPanel->SetContext(EnvMapSharedData::s_EditorScene);
}

void EnvMapEditorLayer::UpdateWindowTitle(const std::string& sceneName)
{
    m_WindowTitleDynamic = sceneName + " - " + Application::GetPlatformName() + " (" + Application::GetConfigurationName() + ")";
    std::string newTitle = m_WindowTitleDynamic + " - " + m_WindowTitleStatic;
    Application::Get()->GetWindow()->SetTitle(newTitle);
}

/**
 * Update active camera with ECS camera parameter values
 **/
void EnvMapEditorLayer::CameraSyncECS()
{
    EnvMapSharedData::s_ActiveCamera->SetAspectRatio(GetMainCameraComponent().Camera.GetAspectRatio());
    EnvMapSharedData::s_ActiveCamera->SetExposure(GetMainCameraComponent().Camera.GetExposure());
    EnvMapSharedData::s_ActiveCamera->SetProjectionType(GetMainCameraComponent().Camera.GetProjectionType());

    // perspective
    EnvMapSharedData::s_ActiveCamera->SetPerspectiveVerticalFOV(GetMainCameraComponent().Camera.GetPerspectiveVerticalFOV());
    EnvMapSharedData::s_ActiveCamera->SetPerspectiveNearClip(GetMainCameraComponent().Camera.GetPerspectiveNearClip());
    EnvMapSharedData::s_ActiveCamera->SetPerspectiveFarClip(GetMainCameraComponent().Camera.GetPerspectiveFarClip());

    // ortho
    EnvMapSharedData::s_ActiveCamera->SetOrthographicNearClip(GetMainCameraComponent().Camera.GetOrthographicNearClip());
    EnvMapSharedData::s_ActiveCamera->SetOrthographicFarClip(GetMainCameraComponent().Camera.GetOrthographicFarClip());

    // s_ActiveCamera->SetPosition(GetMainCameraComponent().Camera.GetPosition());
    // s_ActiveCamera->SetPitch(GetMainCameraComponent().Camera.GetPitch());
    // s_ActiveCamera->SetYaw(GetMainCameraComponent().Camera.GetYaw());
    // s_ActiveCamera->SetProjectionMatrix(GetMainCameraComponent().Camera.GetProjectionMatrix());
}

void EnvMapEditorLayer::UpdateImGuizmo(Window* mainWindow)
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
    auto meshEntities = EnvMapSharedData::s_EditorScene->GetAllEntitiesWith<Hazel::MeshComponent>();
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
                glm::value_ptr(EnvMapSharedData::s_ActiveCamera->GetViewMatrix()),
                glm::value_ptr(EnvMapSharedData::s_ActiveCamera->GetProjection()),
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
                glm::value_ptr(EnvMapSharedData::s_ActiveCamera->GetViewMatrix()),
                glm::value_ptr(EnvMapSharedData::s_ActiveCamera->GetProjection()),
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

Ref<Hazel::Entity> EnvMapEditorLayer::GetMeshEntity()
{
    Ref<Hazel::Entity> meshEntity;
    auto meshEntities = EnvMapSharedData::s_EditorScene->GetAllEntitiesWith<Hazel::MeshComponent>();
    if (meshEntities.size()) {
        for (auto entt : meshEntities)
        {
            meshEntity = CreateRef<Hazel::Entity>(entt, EnvMapSharedData::s_EditorScene.Raw());
        }
        return meshEntity;
    }
    return nullptr;
}

float& EnvMapEditorLayer::GetSkyboxLOD()
{
    return EnvMapSharedData::s_EditorScene->GetSkyboxLod();
}

void EnvMapEditorLayer::SetViewportBounds(glm::vec2* viewportBounds)
{
    m_ViewportBounds[0] = viewportBounds[0];
    m_ViewportBounds[1] = viewportBounds[1];
}

void EnvMapEditorLayer::SetSkyboxLOD(float LOD)
{
    EnvMapSharedData::s_EditorScene->SetSkyboxLod(LOD);
}

Hazel::Ref<MoravaShader> EnvMapEditorLayer::GetShaderPBR_Anim()
{
    return MoravaShaderLibrary::Get("HazelPBR_Anim");
}

Hazel::Ref<MoravaShader> EnvMapEditorLayer::GetShaderPBR_Static()
{
    return MoravaShaderLibrary::Get("HazelPBR_Static");
}

void EnvMapEditorLayer::DrawIndexed(uint32_t count, Hazel::PrimitiveType type, bool depthTest)
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

void EnvMapEditorLayer::OnImGuiRender(Window* mainWindow, Scene* scene)
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

    if (m_ShowWindowAssetManager)
    {
        ImGui::Begin("Asset Manager", &m_ShowWindowAssetManager);
        {
            DrawAssetManager();
        }
        ImGui::End();
    }

    if (m_ShowWindowMaterialEditor)
    {
        ImGui::Begin("Material Editor", &m_ShowWindowMaterialEditor);
        {
            DrawMaterialEditor();
        }
        ImGui::End();
    }

    if (m_ShowWindowPostProcessing)
    {
        ImGui::Begin("Post Processing Effects", &m_ShowWindowPostProcessing);
        {
            if (ImGui::CollapsingHeader("Display Info", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Post Processing Enabled", &m_PostProcessingEnabled);

                ImGui::Separator();

                ImGui::RadioButton("Default colors", &m_PostProcessingEffect, 0);
                ImGui::RadioButton("Invert colors", &m_PostProcessingEffect, 1);
                ImGui::RadioButton("Grayscale", &m_PostProcessingEffect, 2);
                ImGui::RadioButton("Nightvision", &m_PostProcessingEffect, 3);
                ImGui::RadioButton("Kernel sharpen", &m_PostProcessingEffect, 4);
                ImGui::RadioButton("Kernel blur", &m_PostProcessingEffect, 5);
                ImGui::RadioButton("Shades of gray", &m_PostProcessingEffect, 6);
                ImGui::RadioButton("8-bit Colors", &m_PostProcessingEffect, 7);
                ImGui::RadioButton("Gaussian Blur", &m_PostProcessingEffect, 8);
            }
        }
        ImGui::End();
    }

    if (m_ShowWindowShaderManager)
    {
        // Shaders
        ImGui::Begin("Shader Manager", &m_ShowWindowShaderManager);
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
    }

    ImGui::Begin("Settings");
    {
        if (ImGui::CollapsingHeader("Display Info"))
        {
            char buffer[100];
            sprintf(buffer, "Can Viewport Receive Events ? %s", ImGuiWrapper::CanViewportReceiveEvents() ? "YES" : "NO");
            ImGui::Text(buffer);

            ImGui::Separator();

            ImGui::Checkbox("Display Outline", &EnvMapSharedData::s_DisplayOutline);
            ImGui::Checkbox("Display Bounding Boxes", &m_DisplayBoundingBoxes);
            ImGui::Checkbox("Display Hazel Grid", &EnvMapSharedData::s_DisplayHazelGrid);
            ImGui::Checkbox("Display Line Elements", &m_DisplayLineElements);
            ImGui::Checkbox("Display Ray", &EnvMapSharedData::s_DisplayRay);

            bool eventLoggingEnabled = Application::Get()->GetWindow()->GetEventLogging();
            if (ImGui::Checkbox("Enable Event Logging", &eventLoggingEnabled)) {
                Application::Get()->GetWindow()->SetEventLogging(eventLoggingEnabled);
            }

            float fovDegrees = GetMainCameraComponent().Camera.GetPerspectiveVerticalFOV();
            if (ImGui::DragFloat("FOV", &fovDegrees, 1.0f, -60.0f, 180.0f)) {
                GetMainCameraComponent().Camera.SetPerspectiveVerticalFOV(fovDegrees);
            }

            ImGui::Separator();

            ImGui::Text("Scene Settings");
            ImGui::Checkbox("Enable Shadows",       &scene->sceneSettings.enableShadows);
            ImGui::Checkbox("Enable Omni Shadows",  &scene->sceneSettings.enableOmniShadows);
            ImGui::Checkbox("Enable Water Effects", &scene->sceneSettings.enableWaterEffects);
            ImGui::Checkbox("Enable Particles",     &scene->sceneSettings.enableParticles);
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
        ImVec2 imageSize(96.0f, 96.0f);

        ImGui::Text("Viewport");
        ImGui::Image((void*)(intptr_t)m_RenderFramebuffer->GetTextureAttachmentColor()->GetID(), imageSize);

        ImGui::Text("Post Processing");
        ImGui::Image((void*)(intptr_t)m_PostProcessingFramebuffer->GetTextureAttachmentColor()->GetID(), imageSize);

        ImGui::Text("Equirectangular");
        ImGui::Image((void*)(intptr_t)EnvMapSceneRenderer::GetEnvEquirect()->GetID(), imageSize);

        ImGui::Text("Shadow Map");
        ImGui::Image((void*)(intptr_t)EnvMapSharedData::s_ShadowMapDirLight->GetTextureID(), imageSize);
        // ImGui::Image((void*)(intptr_t)LightManager::directionalLight.GetShadowMap()->GetTextureID(), imageSize);
    }
    ImGui::End();

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
        Hazel::Ref<Framebuffer> targetFramebuffer = EnvMapSceneRenderer::GetCompositePass()->GetSpecification().TargetFramebuffer;
        glm::ivec2 colorAttachmentSize = glm::ivec2(
            targetFramebuffer->GetTextureAttachmentColor()->GetWidth(),
            targetFramebuffer->GetTextureAttachmentColor()->GetHeight());
        glm::ivec2 depthAttachmentSize = glm::ivec2(
            targetFramebuffer->GetAttachmentDepth()->GetWidth(),
            targetFramebuffer->GetAttachmentDepth()->GetHeight());

        ImGui::SliderInt2("Color Attachment Size", glm::value_ptr(colorAttachmentSize), 0, 2048);
        ImGui::SliderInt2("Depth Attachment Size", glm::value_ptr(depthAttachmentSize), 0, 2048);
    }
    ImGui::End();

    // TheCherno ImGui Viewport displaying the framebuffer content
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });

    ImGui::Begin("Viewport");
    {
        m_ViewportPanelMouseOver = ImGui::IsWindowHovered();
        m_ViewportPanelFocused = ImGui::IsWindowFocused();

        ImGuiWrapper::SetViewportEnabled(true);
        ImGuiWrapper::SetViewportHovered(m_ViewportPanelMouseOver);
        ImGuiWrapper::SetViewportFocused(m_ViewportPanelFocused);

        // Calculate Viewport bounds (used in EnvMapEditorLayer::CastRay)
        auto viewportOffset = ImGui::GetCursorPos(); // includes tab bar
        auto viewportSize = ImGui::GetContentRegionAvail();

        //  Hazel::EnvMapSceneRenderer::SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
        //  EnvMapSharedData::s_EditorScene->SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
        //  if (s_RuntimeScene) {
        //      s_RuntimeScene->SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
        //  }
        //  
        //  s_EditorCamera->SetProjectionMatrix(glm::perspectiveFov(glm::radians(45.0f), viewportSize.x, viewportSize.y, 0.1f, 10000.0f));
        //  s_EditorCamera->SetViewportSize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
        //  ImGui::Image((void*)Hazel::EnvMapSceneRenderer::GetFinalColorBufferID(), viewportSize, { 0, 1 }, { 1, 0 });

        ImVec2 screen_pos = ImGui::GetCursorScreenPos();

        m_ImGuiViewport.X = (int)(ImGui::GetWindowPos().x - m_ImGuiViewportMain.x);
        m_ImGuiViewport.Y = (int)(ImGui::GetWindowPos().y - m_ImGuiViewportMain.y);
        m_ImGuiViewport.Width = (int)ImGui::GetWindowWidth();
        m_ImGuiViewport.Height = (int)ImGui::GetWindowHeight();
        m_ImGuiViewport.MouseX = (int)ImGui::GetMousePos().x;
        m_ImGuiViewport.MouseY = (int)ImGui::GetMousePos().y;

        glm::vec2 viewportPanelSize = glm::vec2(viewportSize.x, viewportSize.y);

        ResizeViewport(viewportPanelSize, m_RenderFramebuffer);
        ResizeViewport(viewportPanelSize, m_PostProcessingFramebuffer);

        uint64_t textureID;
        if (m_PostProcessingEnabled) {
            textureID = m_PostProcessingFramebuffer->GetTextureAttachmentColor()->GetID();
        }
        else {
            textureID = m_RenderFramebuffer->GetTextureAttachmentColor()->GetID();
        }
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

        m_ViewportEnvMapFocused = ImGui::IsWindowFocused(); // Not in use, use m_ViewportPanelFocused instead
        m_ViewportEnvMapHovered = ImGui::IsWindowHovered(); // Not in use, use m_ViewportPanelMouseOver instead

        ImVec2 viewportPanelSizeImGuiEnvMap = ImGui::GetContentRegionAvail();
        glm::vec2 viewportPanelSizeEnvMap = glm::vec2(viewportPanelSizeImGuiEnvMap.x, viewportPanelSizeImGuiEnvMap.y);

        // Currently resize can only work with a single (main) viewport
        // ResizeViewport(viewportPanelSizeEnvMap, m_EnvMapEditorLayer->GetSceneRenderer()->s_Data.CompositePass->GetSpecification().TargetFramebuffer);
        Hazel::Ref<Framebuffer> targetFramebuffer = EnvMapSceneRenderer::GetCompositePass()->GetSpecification().TargetFramebuffer;
        uint64_t textureID = targetFramebuffer->GetTextureAttachmentColor()->GetID();
        ImGui::Image((void*)(intptr_t)textureID, ImVec2{ m_ViewportEnvMapSize.x, m_ViewportEnvMapSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
    }
    ImGui::End();

    ImGui::PopStyleVar();

    uint32_t id = 0;
    auto meshEntities = EnvMapSharedData::s_EditorScene->GetAllEntitiesWith<Hazel::MeshComponent>();
    for (auto entt : meshEntities)
    {
        Hazel::Entity entity = { entt, EnvMapSharedData::s_EditorScene.Raw() };
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
        //  float physics2DGravity = EnvMapSharedData::s_EditorScene->GetPhysics2DGravity();
        //  if (ImGuiWrapper::Property("Gravity", physics2DGravity, -10000.0f, 10000.0f, PropertyFlag::DragProperty))
        //  {
        //      EnvMapSharedData::s_EditorScene->SetPhysics2DGravity(physics2DGravity);
        //  }

        if (ImGui::ImageButton((ImTextureID)(uint64_t)(m_PlayButtonTex->GetID()), ImVec2(32, 32), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), ImVec4(0.9f, 0.9f, 0.9f, 1.0f)))
        {
            OnScenePlay();
        }
    }
    else if (m_SceneState == SceneState::Play)
    {
        //  float physics2DGravity = s_RuntimeScene->GetPhysics2DGravity();
        //  
        //  if (ImGuiWrapper::Property("Gravity", physics2DGravity, -10000.0f, 10000.0f, PropertyFlag::DragProperty))
        //  {
        //      s_RuntimeScene->SetPhysics2DGravity(physics2DGravity);
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
        const char* label = EnvMapSharedData::s_ActiveCamera == EnvMapSharedData::s_EditorCamera ? "EDITOR [ Editor Camera ]" : "RUNTIME [ Runtime Camera ]";
        if (ImGui::Button(label))
        {
            EnvMapSharedData::s_ActiveCamera = (EnvMapSharedData::s_ActiveCamera == EnvMapSharedData::s_EditorCamera) ?
                (Hazel::HazelCamera*)EnvMapSharedData::s_RuntimeCamera :
                (Hazel::HazelCamera*)EnvMapSharedData::s_EditorCamera;
        }
    }
    ImGui::End();

    ImGui::Begin("Transform");
    {
        if (EntitySelection::s_SelectionContext.size())
        {
            glm::mat4 transformImGui;

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
                transformImGui = selectedSubmesh.Mesh->Transform;
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
                    selectedSubmesh.Mesh->Transform = Math::CreateTransform(translation, rotationDegrees, scale);
                }
            }
        }
    }
    ImGui::End();

    ImGui::Begin("Camera");
    {
        if (ImGui::CollapsingHeader("Display Info", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto pitch = GetMainCameraComponent().Camera.GetPitch();
            if (ImGui::DragFloat("Pitch", &pitch, 1.0f, -89.0f, 89.0f)) {
                GetMainCameraComponent().Camera.SetPitch(pitch);
            }

            auto yaw = GetMainCameraComponent().Camera.GetYaw();
            if (ImGui::DragFloat("Yaw", &yaw, 1.0f, -180.0f, 180.0f)) {
                GetMainCameraComponent().Camera.SetYaw(yaw);
            }

            auto fov = GetMainCameraComponent().Camera.GetPerspectiveVerticalFOV();
            if(ImGui::DragFloat("FOV", &fov, 1.0f, 10.0f, 150.0f)) {
                GetMainCameraComponent().Camera.SetPerspectiveVerticalFOV(fov);
            }

            float nearPlane = GetMainCameraComponent().Camera.GetPerspectiveNearClip();
            if (ImGui::DragFloat("Near Plane", &nearPlane, 0.1f, -10.0f, 100.0f))
            {
                GetMainCameraComponent().Camera.SetPerspectiveNearClip(nearPlane);
            }

            float farPlane = GetMainCameraComponent().Camera.GetPerspectiveFarClip();
            if (ImGui::DragFloat("Far Plane", &farPlane, 1.0f, -100.0f, 5000.0f))
            {
                GetMainCameraComponent().Camera.SetPerspectiveFarClip(farPlane);
            }

            char buffer[100];
            sprintf(buffer, "Aspect Ratio  %.2f", EnvMapSharedData::s_ActiveCamera->GetAspectRatio());
            ImGui::Text(buffer);
            sprintf(buffer, "Position    X %.2f Y %.2f Z %.2f", 
                EnvMapSharedData::s_ActiveCamera->GetPosition().x,
                EnvMapSharedData::s_ActiveCamera->GetPosition().y,
                EnvMapSharedData::s_ActiveCamera->GetPosition().z);
            ImGui::Text(buffer);
            sprintf(buffer, "Direction   X %.2f Y %.2f Z %.2f",
                EnvMapSharedData::s_ActiveCamera->GetDirection().x,
                EnvMapSharedData::s_ActiveCamera->GetDirection().y,
                EnvMapSharedData::s_ActiveCamera->GetDirection().z);
            ImGui::Text(buffer);
            sprintf(buffer, "Front       X %.2f Y %.2f Z %.2f",
                EnvMapSharedData::s_ActiveCamera->GetFront().x,
                EnvMapSharedData::s_ActiveCamera->GetFront().y,
                EnvMapSharedData::s_ActiveCamera->GetFront().z);
            ImGui::Text(buffer);
            sprintf(buffer, "Up          X %.2f Y %.2f Z %.2f",
                EnvMapSharedData::s_ActiveCamera->GetUp().x,
                EnvMapSharedData::s_ActiveCamera->GetUp().y,
                EnvMapSharedData::s_ActiveCamera->GetUp().z);
            ImGui::Text(buffer);
            sprintf(buffer, "Right       X %.2f Y %.2f Z %.2f",
                EnvMapSharedData::s_ActiveCamera->GetRight().x,
                EnvMapSharedData::s_ActiveCamera->GetRight().y,
                EnvMapSharedData::s_ActiveCamera->GetRight().z);
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
                        EnvMapSceneRenderer::SetEnvironment(EnvMapSceneRenderer::Load(filename));
                    }
                }

                float skyboxLOD = GetSkyboxLOD();
                bool valueChanged = ImGui::DragFloat("Skybox LOD", &skyboxLOD, 0.01f, 0.0f, 2.0f, "%.2f");
                if (valueChanged) {
                    SetSkyboxLOD(skyboxLOD);
                }

                ImGui::Columns(2);
                ImGui::AlignTextToFramePadding();

                Hazel::HazelLight light = EnvMapSceneRenderer::GetActiveLight();
                Hazel::HazelLight lightPrev = light;

                ImGuiWrapper::Property("Light Direction", light.Direction, -180.0f, 180.0f, PropertyFlag::SliderProperty);
                ImGuiWrapper::Property("Light Radiance", light.Radiance, PropertyFlag::ColorProperty);
                ImGuiWrapper::Property("Light Multiplier", light.Multiplier, 0.01f, 0.0f, 5.0f, PropertyFlag::DragProperty);
                ImGuiWrapper::Property("Exposure", GetMainCameraComponent().Camera.GetExposure(), 0.01f, 0.0f, 40.0f, PropertyFlag::DragProperty);
                ImGuiWrapper::Property("Skybox Exposure Factor", EnvMapSharedData::s_SkyboxExposureFactor, 0.01f, 0.0f, 10.0f, PropertyFlag::DragProperty);

                ImGuiWrapper::Property("Radiance Prefiltering", EnvMapSharedData::s_RadiancePrefilter);
                ImGuiWrapper::Property("Env Map Rotation", EnvMapSharedData::s_EnvMapRotation, 1.0f, -360.0f, 360.0f, PropertyFlag::DragProperty);

                if (m_SceneState == SceneState::Edit) {
                    //  float physics2DGravity = EnvMapSharedData::s_EditorScene->GetPhysics2DGravity();
                    //  if (ImGuiWrapper::Property("Gravity", physics2DGravity, -10000.0f, 10000.0f, PropertyFlag::DragProperty)) {
                    //      EnvMapSharedData::s_EditorScene->SetPhysics2DGravity(physics2DGravity);
                    //  }
                }
                else if (m_SceneState == SceneState::Play) {
                    //  float physics2DGravity = s_RuntimeScene->GetPhysics2DGravity();
                    //  if (ImGuiWrapper::Property("Gravity", physics2DGravity, -10000.0f, 10000.0f, PropertyFlag::DragProperty)) {
                    //      s_RuntimeScene->SetPhysics2DGravity(physics2DGravity);
                    //  }
                }

                EnvMapSceneRenderer::SetActiveLight(light);

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

                auto meshEntities = EnvMapSharedData::s_EditorScene->GetAllEntitiesWith<Hazel::MeshComponent>();
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
            submeshUUID = MaterialLibrary::GetSubmeshUUID(entity, selectedSubmesh.Mesh);
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
        for (auto& material : MaterialLibrary::s_EnvMapMaterials) {
            materialNameStrings.push_back(material.second->GetName());
        }

        std::string submeshMaterialName = materialNameStrings.size() ? materialNameStrings[0] : "N/A";

        MaterialUUID materialUUID;
        if (MaterialLibrary::s_SubmeshMaterialUUIDs.find(submeshUUID) != MaterialLibrary::s_SubmeshMaterialUUIDs.end()) {
            materialUUID = MaterialLibrary::s_SubmeshMaterialUUIDs.at(submeshUUID);
        }
        int selectedMaterial = -1;

        if (ImGui::BeginCombo("Material", submeshMaterialName.c_str()))
        {
            size_t emm_index = 0;
            for (auto emm_it = MaterialLibrary::s_EnvMapMaterials.begin(); emm_it != MaterialLibrary::s_EnvMapMaterials.end(); emm_it++)
            {
                bool is_selected = (submeshMaterialName == materialNameStrings[emm_index]);
                if (ImGui::Selectable(materialNameStrings.at(emm_index).c_str(), is_selected))
                {
                    submeshMaterialName = materialNameStrings[emm_index];
                    materialUUID = emm_it->second->GetUUID();
                    if (meshName != "N/A" && submeshMaterialName != "N/A" && submeshUUID != "N/A")
                    {
                        auto sm_it = MaterialLibrary::s_SubmeshMaterialUUIDs.find(submeshUUID);
                        if (sm_it != MaterialLibrary::s_SubmeshMaterialUUIDs.end()) {
                            sm_it->second = materialUUID;
                            Log::GetLogger()->debug("s_SubmeshMaterialUUIDs UPDATE [ SubmeshUUID: '{0}' => MaterialUUID: '{1}', Items: {2} ]",
                                submeshUUID, materialUUID, MaterialLibrary::s_SubmeshMaterialUUIDs.size());
                            break;
                        }
                        else {
                            MaterialLibrary::s_SubmeshMaterialUUIDs.insert(std::make_pair(submeshUUID, materialUUID));
                            Log::GetLogger()->debug("s_SubmeshMaterialUUIDs INSERT [ SubmeshUUID: '{0}' => MaterialUUID: '{1}', Items: {2} ]",
                                submeshUUID, materialUUID, MaterialLibrary::s_SubmeshMaterialUUIDs.size());
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

    //  ImGui::SetNextWindowSize({ 0.0f, 22.0f }, {});
    //  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 10, 5 });
    //  ImGui::Begin("Status bar");
    //  {
    //      char buffer[100];
    //      snprintf(buffer, sizeof(buffer), "Frame Time: %.2fms\n", Timer::Get()->GetDeltaTime() * 1000.0f);
    //      m_StatusBarMessage = buffer;
    //      ImGui::Text(buffer);
    //  }
    //  ImGui::End();
    //  ImGui::PopStyleVar();

    m_SceneHierarchyPanel->OnImGuiRender();

    ImGui::ShowMetricsWindow();

    ImVec2 workPos = ImGui::GetMainViewport()->GetWorkPos();
    m_WorkPosImGui = glm::vec2(workPos.x, workPos.y);
}

// Demonstrate using DockSpace() to create an explicit docking node within an existing window.
// Note that you already dock windows into each others _without_ a DockSpace() by just moving windows 
// from their title bar (or by holding SHIFT if io.ConfigDockingWithShift is set).
// DockSpace() is only useful to construct to a central location for your application.
void EnvMapEditorLayer::ShowExampleAppDockSpace(bool* p_open, Window* mainWindow)
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
                bool pOpen = false;
                p_open = &pOpen;
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

        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Asset Manager", "Ctrl+Space")) {
                m_ShowWindowAssetManager = !m_ShowWindowAssetManager;
            }

            if (ImGui::MenuItem("Material Editor", "Ctrl+M")) {
                m_ShowWindowMaterialEditor = !m_ShowWindowMaterialEditor;
            }

            if (ImGui::MenuItem("Post Processing Effects", "Ctrl+P")) {
                m_ShowWindowPostProcessing = !m_ShowWindowPostProcessing;
            }

            if (ImGui::MenuItem("Shader Manager", "Ctrl+S")) {
                m_ShowWindowShaderManager = !m_ShowWindowShaderManager;
            }

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

void EnvMapEditorLayer::NewScene()
{
    OnNewScene(m_ViewportMainSize);
}
 
void EnvMapEditorLayer::OpenScene()
{
    // auto app = Application::Get();
    std::string filepath = Hazel::FileDialogs::OpenFile("Hazel Scene (*.hsc)\0*.hsc\0");
    if (!filepath.empty())
    {
        Hazel::Ref<Hazel::HazelScene> newScene = Hazel::Ref<Hazel::HazelScene>::Create();
        Hazel::SceneSerializer serializer(newScene);
        serializer.Deserialize(filepath);
        EnvMapSharedData::s_EditorScene = newScene;
        std::filesystem::path path = filepath;
        UpdateWindowTitle(path.filename().string());
        m_SceneHierarchyPanel->SetContext(EnvMapSharedData::s_EditorScene);
        Hazel::ScriptEngine::SetSceneContext(EnvMapSharedData::s_EditorScene);

        EnvMapSharedData::s_EditorScene->SetSelectedEntity({});
        EntitySelection::s_SelectionContext.clear();

        m_SceneFilePath = filepath;

        OnNewScene(m_ViewportMainSize);
    }
}

void EnvMapEditorLayer::SaveScene()
{
    if (!m_SceneFilePath.empty()) {
        Hazel::SceneSerializer serializer(EnvMapSharedData::s_EditorScene);
        serializer.Serialize(m_SceneFilePath);
    }
}

void EnvMapEditorLayer::SaveSceneAs()
{
    auto app = Application::Get();
    std::string filepath = app->SaveFile("Hazel Scene (*.hsc)\0*.hsc\0");
    if (!filepath.empty())
    {
        Hazel::SceneSerializer serializer(EnvMapSharedData::s_EditorScene);
        serializer.Serialize(filepath);

        std::filesystem::path path = filepath;
        UpdateWindowTitle(path.filename().string());
        m_SceneFilePath = filepath;
    }
}

void EnvMapEditorLayer::OnNewScene(glm::vec2 viewportSize)
{
    // m_SceneRenderer->s_Data.ActiveScene = new Hazel::HazelScene();
    EnvMapSharedData::s_EditorScene->OnViewportResize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
    m_SceneHierarchyPanel->SetContext(EnvMapSharedData::s_EditorScene);
}

void EnvMapEditorLayer::DrawAssetManager()
{
}

void EnvMapEditorLayer::DrawMaterialEditor()
{
    unsigned int materialIndex = 0;
    for (auto material_it = MaterialLibrary::s_EnvMapMaterials.begin(); material_it != MaterialLibrary::s_EnvMapMaterials.end();)
    {
        Hazel::Ref<EnvMapMaterial> material = material_it->second;
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
            auto envMapMaterialSrc = MaterialLibrary::s_EnvMapMaterials.at(materialUUID);
            Hazel::Ref<EnvMapMaterial> envMapMaterialDst = Hazel::Ref<EnvMapMaterial>::Create(MaterialLibrary::NewMaterialName(), envMapMaterialSrc);
            MaterialLibrary::AddEnvMapMaterial(envMapMaterialDst->GetUUID(), envMapMaterialDst);
        }

        if (materialDelete) {
            material_it = MaterialLibrary::s_EnvMapMaterials.erase(material_it++);
        }
        else {
            ++material_it;
        }
    }

    // Right-click on blank space
    if (ImGui::BeginPopupContextWindow(0, 1, false))
    {
        if (ImGui::MenuItem("Create a Material"))
        {
            MaterialLibrary::AddNewMaterial("");
        }
        ImGui::EndPopup();
    }
}

void EnvMapEditorLayer::SelectEntity(Hazel::Entity e)
{
}

void EnvMapEditorLayer::SubmitMesh(Hazel::HazelMesh* mesh, const glm::mat4& transform, Material* overrideMaterial)
{
    auto& materials = mesh->GetMaterials();
    for (Hazel::Submesh& submesh : mesh->GetSubmeshes())
    {
        // Material
        auto material = materials[submesh.MaterialIndex];

        for (size_t i = 0; i < mesh->m_BoneTransforms.size(); i++)
        {
            std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
            EnvMapSharedData::s_ShaderHazelPBR->SetMat4(uniformName, mesh->m_BoneTransforms[i]);
        }

        EnvMapSharedData::s_ShaderHazelPBR->SetMat4("u_Transform", transform * submesh.Transform);

        if (material->GetFlag(Hazel::HazelMaterialFlag::DepthTest)) { // TODO: Fix Material flags
            glEnable(GL_DEPTH_TEST);
        }
        else {
            glDisable(GL_DEPTH_TEST);
        }

        glDrawElementsBaseVertex(GL_TRIANGLES, submesh.GetIndexCount(), GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh.BaseIndex), submesh.BaseVertex);
    }
}

void EnvMapEditorLayer::ResizeViewport(glm::vec2 viewportPanelSize, Hazel::Ref<Framebuffer> renderFramebuffer)
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

void EnvMapEditorLayer::OnEvent(Event& e)
{
    if (m_SceneState == SceneState::Edit)
    {
        if (m_ViewportPanelMouseOver) {
            GetMainCameraComponent().Camera.OnEvent(e);
        }

        EnvMapSharedData::s_EditorScene->OnEvent(e);
    }
    else if (m_SceneState == SceneState::Play)
    {
        EnvMapSharedData::s_RuntimeScene->OnEvent(e);
    }

    // EnvMapSharedData::s_ActiveCamera->OnEvent(e);

    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<KeyPressedEvent>(HZ_BIND_EVENT_FN(EnvMapEditorLayer::OnKeyPressedEvent));
    dispatcher.Dispatch<MouseButtonPressedEvent>(HZ_BIND_EVENT_FN(EnvMapEditorLayer::OnMouseButtonPressed));
}

bool EnvMapEditorLayer::OnKeyPressedEvent(KeyPressedEvent& e)
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
                    EnvMapSharedData::s_EditorScene->DestroyEntity(selectedEntity);
                    EntitySelection::s_SelectionContext.clear();
                    EnvMapSharedData::s_EditorScene->SetSelectedEntity({});
                    m_SceneHierarchyPanel->SetSelected({});
                }
                break;
        }
    }

    if (Input::IsKeyPressed(MORAVA_KEY_LEFT_CONTROL))
    {
        switch (e.GetKeyCode())
        {
            case (int)KeyCode::B:
                // Toggle bounding boxes
                m_UIShowBoundingBoxes = !m_UIShowBoundingBoxes;
                ShowBoundingBoxes(m_UIShowBoundingBoxes, m_UIShowBoundingBoxesOnTop);
                break;
            case (int)KeyCode::D:
                if (EntitySelection::s_SelectionContext.size()) {
                    Hazel::Entity selectedEntity = EntitySelection::s_SelectionContext[0].Entity;
                    EnvMapSharedData::s_EditorScene->DuplicateEntity(selectedEntity);
                }
                break;
            case (int)KeyCode::G:
                // Toggle grid
                EnvMapSceneRenderer::GetOptions().ShowGrid = !EnvMapSceneRenderer::GetOptions().ShowGrid;
                break;
            case (int)KeyCode::O:
                OpenScene();
                break;
            case (int)KeyCode::S:
                SaveScene();
                break;

                // Toggle ImGui windows
            case (int)KeyCode::Space:
                // Left CTRL + Space: Toggle Asset Manager
                m_ShowWindowAssetManager = !m_ShowWindowAssetManager;
                break;
            case (int)KeyCode::M:
                // Left CTRL + M: Toggle Material Editor
                m_ShowWindowMaterialEditor = !m_ShowWindowMaterialEditor;
                break;
            case (int)KeyCode::P:
                // Left CTRL + P: Toggle Post Processing Effects
                m_ShowWindowPostProcessing = !m_ShowWindowPostProcessing;
                break;
            case (int)KeyCode::H:
                // Left CTRL + H: Toggle Shader Manager
                m_ShowWindowShaderManager = !m_ShowWindowShaderManager;
                break;
        }

        if (Input::IsKeyPressed(MORAVA_KEY_LEFT_SHIFT))
        {
            switch (e.GetKeyCode())
            {
            case (int)KeyCode::S:
                SaveSceneAs();
                break;
            }
        }
    }

    return false;
}

bool EnvMapEditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
{
    auto [mx, my] = Input::GetMousePosition();
    if (e.GetMouseButton() == (int)Mouse::ButtonLeft && !ImGuizmo::IsUsing() && !ImGuizmo::IsOver() && !Input::IsKeyPressed(Key::LeftAlt))
    {
        auto [mouseX, mouseY] = GetMouseViewportSpace();
        if (mouseX > -1.0f && mouseX < 1.0f && mouseY > -1.0f && mouseY < 1.0f)
        {
            auto [origin, direction] = CastRay(mouseX, mouseY);

            EntitySelection::s_SelectionContext.clear();

            auto meshEntities = EnvMapSharedData::s_EditorScene->GetAllEntitiesWith<Hazel::MeshComponent>();
            for (auto e : meshEntities)
            {
                Hazel::Entity entity = { e, EnvMapSharedData::s_EditorScene.Raw() };
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
                    auto transform = entity.GetComponent<Hazel::TransformComponent>().GetTransform();
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
                m_CurrentlySelectedTransform = EntitySelection::s_SelectionContext[0].Mesh->Transform;
                OnSelected(EntitySelection::s_SelectionContext[0]);
            }
            else {
                Ref<Hazel::Entity> meshEntity = GetMeshEntity();
                if (meshEntity) {
                    m_CurrentlySelectedTransform = meshEntity->Transform().GetTransform();
                }
            }
        }
    }
    return false;
}

void EnvMapEditorLayer::OnSelected(const SelectedSubmesh& selectionContext)
{
    // TODO: move to SceneHazelEnvMap
    m_SceneHierarchyPanel->SetSelected(selectionContext.Entity);
    EnvMapSharedData::s_EditorScene->SetSelectedEntity(selectionContext.Entity);
}

void EnvMapEditorLayer::OnEntityDeleted(Hazel::Entity e)
{
    if (EntitySelection::s_SelectionContext.size())
    {
        if (EntitySelection::s_SelectionContext[0].Entity == e) {
            EntitySelection::s_SelectionContext.clear();
            EnvMapSharedData::s_EditorScene->SetSelectedEntity({});
        }
    }
}

std::pair<float, float> EnvMapEditorLayer::GetMouseViewportSpace()
{
    auto [mx, my] = ImGui::GetMousePos(); // Input::GetMousePosition();
    mx -= m_ViewportBounds[0].x;
    my -= m_ViewportBounds[0].y;
    m_ViewportWidth = m_ViewportBounds[1].x - m_ViewportBounds[0].x;
    m_ViewportHeight = m_ViewportBounds[1].y - m_ViewportBounds[0].y;

    return { (mx / m_ViewportWidth) * 2.0f - 1.0f, ((my / m_ViewportHeight) * 2.0f - 1.0f) * -1.0f };
}

std::pair<glm::vec3, glm::vec3> EnvMapEditorLayer::CastRay(float mx, float my)
{
    glm::vec4 mouseClipPos = { mx, my, -1.0f, 1.0f };

    glm::mat4 projectionMatrix = EnvMapSharedData::s_ActiveCamera->GetProjection();
    glm::mat4 viewMatrix = EnvMapSharedData::s_ActiveCamera->GetViewMatrix();

    auto inverseProj = glm::inverse(projectionMatrix);
    auto inverseView = glm::inverse(glm::mat3(viewMatrix));

    glm::vec4 ray = inverseProj * mouseClipPos;
    glm::vec3 rayPos = EnvMapSharedData::s_ActiveCamera->GetPosition();
    glm::vec3 rayDir = inverseView * glm::vec3(ray); // inverseView * glm::vec3(ray)

    Log::GetLogger()->debug("EnvMapEditorLayer::CastRay | MousePosition [ {0} {1} ]", mx, my);
    Log::GetLogger()->debug("EnvMapEditorLayer::CastRay | m_ViewportBounds[0] [ {0} {1} ]", m_ViewportBounds[0].x, m_ViewportBounds[0].y);
    Log::GetLogger()->debug("EnvMapEditorLayer::CastRay | m_ViewportBounds[1] [ {0} {1} ]", m_ViewportBounds[1].x, m_ViewportBounds[1].y);
    Log::GetLogger()->debug("EnvMapEditorLayer::CastRay | mouseClipPos [ {0} {1} ]", mouseClipPos.x, mouseClipPos.y);

    return { rayPos, rayDir };
}

std::vector<glm::mat4> EnvMapEditorLayer::CalculateLightTransform(glm::mat4 lightProj, glm::vec3 position)
{
    std::vector<glm::mat4> lightMatrices = std::vector<glm::mat4>();
    // X axis
    lightMatrices.push_back(lightProj * glm::lookAt(position, position + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
    lightMatrices.push_back(lightProj * glm::lookAt(position, position + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
    // Y axis
    lightMatrices.push_back(lightProj * glm::lookAt(position, position + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
    lightMatrices.push_back(lightProj * glm::lookAt(position, position + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
    // Z axis
    lightMatrices.push_back(lightProj * glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
    lightMatrices.push_back(lightProj * glm::lookAt(position, position + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

    return lightMatrices;
}

void EnvMapEditorLayer::OnRenderShadow(Window* mainWindow)
{
    EnvMapSharedData::s_ShadowMapDirLight->BindForWriting();

    glViewport(0, 0, EnvMapSharedData::s_ShadowMapDirLight->GetShadowWidth(), EnvMapSharedData::s_ShadowMapDirLight->GetShadowHeight());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);
    RendererBasic::DisableCulling();

    m_ShaderShadow->Bind();

    m_ShaderShadow->SetMat4("u_DirLightTransform", EnvMapSharedData::s_DirLightTransform);

    RenderSubmeshesShadowPass(m_ShaderShadow);

    m_ShaderShadow->Unbind();
    EnvMapSharedData::s_ShadowMapDirLight->Unbind((unsigned int)mainWindow->GetWidth(), (unsigned int)mainWindow->GetHeight());
    RendererBasic::SetDefaultFramebuffer((unsigned int)mainWindow->GetWidth(), (unsigned int)mainWindow->GetHeight());
}

void EnvMapEditorLayer::OnRenderShadowOmni(Window* mainWindow)
{
    // render all point and spot lights here, create a loop for multiple lights
    RenderShadowOmniSingleLight(mainWindow, EnvMapSharedData::s_PointLightEntity, EnvMapSharedData::s_OmniShadowMapPointLight);
    RenderShadowOmniSingleLight(mainWindow, EnvMapSharedData::s_SpotLightEntity, EnvMapSharedData::s_OmniShadowMapSpotLight);
}

void EnvMapEditorLayer::RenderShadowOmniSingleLight(Window* mainWindow, Hazel::Entity lightEntity, Hazel::Ref<OmniShadowMap> omniShadowMap)
{
    omniShadowMap->BindForWriting();

    glViewport(0, 0, omniShadowMap->GetShadowWidth(), omniShadowMap->GetShadowHeight());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);
    RendererBasic::EnableCulling();

    m_ShaderOmniShadow->Bind();

    glm::vec3 lightPosition = glm::vec3(0.0f);
    if (lightEntity.HasComponent<Hazel::TransformComponent>())
    {
        auto& tc = lightEntity.GetComponent<Hazel::TransformComponent>();
        lightPosition = tc.Translation;
    }

    float farPlane = 1000.0f;
    if (lightEntity.HasComponent<Hazel::PointLightComponent>())
    {
        auto& plc = lightEntity.GetComponent<Hazel::PointLightComponent>();
        farPlane = plc.FarPlane;
    }

    m_ShaderOmniShadow->SetFloat3("lightPosition", lightPosition);
    m_ShaderOmniShadow->SetFloat("farPlane", farPlane);

    float aspect = (float)omniShadowMap->GetShadowWidth() / (float)omniShadowMap->GetShadowHeight();
    float nearPlane = 0.01f;
    glm::mat4 lightProj = glm::perspective(glm::radians(90.0f), aspect, nearPlane, farPlane);
    std::vector<glm::mat4> lightMatrices = CalculateLightTransform(lightProj, lightPosition);
    for (unsigned int i = 0; i < lightMatrices.size(); i++) {
        m_ShaderOmniShadow->SetMat4("lightMatrices[" + std::to_string(i) + "]", lightMatrices[i]);
    }

    RenderSubmeshesShadowPass(m_ShaderOmniShadow);

    m_ShaderOmniShadow->Unbind();
    omniShadowMap->Unbind((unsigned int)mainWindow->GetWidth(), (unsigned int)mainWindow->GetHeight());
    RendererBasic::SetDefaultFramebuffer((unsigned int)mainWindow->GetWidth(), (unsigned int)mainWindow->GetHeight());
}

void EnvMapEditorLayer::RenderSubmeshesShadowPass(Hazel::Ref<MoravaShader> shader)
{
    // Rendering all meshes (submeshes) on the scene to a shadow framebuffer
    auto meshEntities = EnvMapSharedData::s_EditorScene->GetAllEntitiesWith<Hazel::MeshComponent>();
    // Render all entities with mesh component
    if (meshEntities.size())
    {
        for (auto entt : meshEntities)
        {
            Hazel::Entity entity = { entt, EnvMapSharedData::s_EditorScene.Raw() };
            auto& meshComponent = entity.GetComponent<Hazel::MeshComponent>();

            glm::mat4 entityTransform = glm::mat4(1.0f);
            if (entity && entity.HasComponent<Hazel::TransformComponent>()) {
                entityTransform = entity.GetComponent<Hazel::TransformComponent>().GetTransform();
            }

            if (meshComponent.Mesh)
            {
                for (Hazel::Submesh& submesh : meshComponent.Mesh->GetSubmeshes())
                {
                    // Render Submesh
                    meshComponent.Mesh->m_VertexBuffer->Bind();
                    meshComponent.Mesh->m_Pipeline->Bind();
                    meshComponent.Mesh->m_IndexBuffer->Bind();

                    shader->SetMat4("model", entityTransform * submesh.Transform);

                    for (size_t i = 0; i < meshComponent.Mesh->m_BoneTransforms.size(); i++)
                    {
                        std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
                        shader->SetMat4(uniformName, meshComponent.Mesh->m_BoneTransforms[i]);
                    }

                    shader->SetBool("u_Animated", meshComponent.Mesh->IsAnimated());

                    glDrawElementsBaseVertex(GL_TRIANGLES, submesh.IndexCount, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * submesh.BaseIndex), submesh.BaseVertex);
                }
            }
        }
    }
}

void EnvMapEditorLayer::OnRender(Window* mainWindow)
{
    /**** BEGIN Render to Main Viewport ****/
    m_RenderFramebuffer->Bind();
    m_RenderFramebuffer->Clear(); // Clear the window

    EnvMapSceneRenderer::GetGeoPass()->GetSpecification().TargetFramebuffer = m_RenderFramebuffer;

    OnRenderEditor();
    // OnRenderRuntime()

    m_RenderFramebuffer->Unbind();
}

void EnvMapEditorLayer::PostProcessing(Window* mainWindow)
{
    if (!m_PostProcessingEnabled) return;

    /**** BEGIN Render to Main Viewport ****/
    m_PostProcessingFramebuffer->Bind();
    m_PostProcessingFramebuffer->Clear(); // Clear the window

    RendererBasic::Clear(0.0f, 1.0f, 1.0f, 1.0f);

    EnvMapSceneRenderer::GetGeoPass()->GetSpecification().TargetFramebuffer = m_PostProcessingFramebuffer;

    m_RenderFramebuffer->GetTextureAttachmentColor()->Bind(1);

    // render to post processing framebuffer here

    {
        // learnopengl post processing
        m_ShaderPostProcessing->Bind();
        m_ShaderPostProcessing->SetInt("u_AlbedoMap", 1);
        m_ShaderPostProcessing->SetInt("u_Effect", m_PostProcessingEffect);

        // 0: Default Colors
        // 1: Invert Colors
        // 2: Grayscale
        // 3: Nightvision
        // 4: Kernel(kernel_sharpen)
        // 5: Kernel(kernel_blur)
        // 6: Shades Of Gray
        // 7: 8-Bit Colors
        // 8: Gaussian Blur
    }

    {
        // Hazel bloom blur post processing
        // m_ShaderBloomBlur->Bind();
        // m_ShaderBloomBlur->SetInt("u_Texture", 1);
        // m_ShaderBloomBlur->SetBool("u_Horizontal", false);
    }

    glBindVertexArray(GeometryFactory::Quad::GetVAO());
    glDrawArrays(GL_TRIANGLES, 0, 6);

    m_ShaderPostProcessing->Unbind();
    m_ShaderBloomBlur->Unbind();

    m_PostProcessingFramebuffer->Unbind();
}

void EnvMapEditorLayer::OnRenderEditor()
{
    EnvMapSceneRenderer::GeometryPass();
    EnvMapSceneRenderer::CompositePass();
}

void EnvMapEditorLayer::OnRenderRuntime()
{
    EnvMapSceneRenderer::GeometryPass();
    EnvMapSceneRenderer::CompositePass();
}
