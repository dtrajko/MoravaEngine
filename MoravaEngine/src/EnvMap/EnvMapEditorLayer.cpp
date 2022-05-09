#include "EnvMapEditorLayer.h"

#include "H2M/Renderer/RendererAPI_H2M.h"
#include "H2M/Renderer/RenderPassH2M.h"
#include "H2M/Renderer/Renderer2D_H2M.h"
#include "H2M/Scene/ComponentsH2M.h"
#include "H2M/Scene/SceneH2M.h"
#include "H2M/Scene/SceneSerializerH2M.h"
#include "H2M/Utilities/FileDialogsH2M.h"

#include "Core/MousePicker.h"
#include "Core/Util.h"
#include "EnvMap/EnvMapSceneRenderer.h"
#include "ImGui/ImGuiWrapper.h"
#include "Light/PointLight.h"
#include "Material/MaterialLibrary.h"
#include "Mesh/GeometryFactory.h"
#include "Renderer/RendererBasic.h"
#include "Shader/MoravaShaderLibrary.h"

#include "cross-platform/ImGuizmo/ImGuizmo.h"

#include <filesystem>


SelectionMode EnvMapEditorLayer::s_SelectionMode = SelectionMode::Entity;
H2M::RefH2M<H2M::Texture2D_H2M> EnvMapEditorLayer::s_CheckerboardTexture;
H2M::RefH2M<EnvMapMaterial> EnvMapEditorLayer::s_DefaultMaterial;
H2M::RefH2M<EnvMapMaterial> EnvMapEditorLayer::s_LightMaterial;

static const std::filesystem::path g_AssetPath = ".";

EnvMapEditorLayer::EnvMapEditorLayer(const std::string& filepath, Scene* scene)
{
    m_Filepath = filepath;
    m_Scene = scene;

    Init();
    OnAttach();
}

void EnvMapEditorLayer::Init()
{
    glDebugMessageCallback(Util::OpenGLLogMessage, nullptr);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    // glEnable(GL_LINE_SMOOTH);

    EnvMapSharedData::s_Scene = m_Scene;

    EnvMapSharedData::s_SamplerSlots = std::map<std::string, unsigned int>();

    // PBR texture inputs
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("albedo",       1)); // uniform sampler2D u_AlbedoTexture
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("normal",       2)); // uniform sampler2D u_NormalTexture
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("metalness",    3)); // uniform sampler2D u_MetalnessTexture
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("roughness",    4)); // uniform sampler2D u_RoughnessTexture
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("emissive",     5)); // uniform sampler2D u_EmissiveTexture
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("ao",           6)); // uniform sampler2D u_AOTexture
    // Environment maps
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("radiance",     7)); // uniform samplerCube u_EnvRadianceTex
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("irradiance",   8)); // uniform samplerCube u_EnvIrradianceTex
    // BRDF LUT
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("BRDF_LUT",     9)); // uniform sampler2D u_BRDFLUTTexture
    // Shadow Map Directional Light
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("shadow",      10)); // uniform sampler2D u_ShadowMap
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("shadow_omni", 11)); // uniform samplerCube omniShadowMaps[i].shadowMap

    // Skybox.fs         - uniform samplerCube u_Texture;
    // SceneComposite.fs - uniform sampler2DMS u_Texture;
    EnvMapSharedData::s_SamplerSlots.insert(std::make_pair("u_Texture", 1));

    EnvMapSharedData::s_SkyboxCube = H2M::RefH2M<CubeSkybox>::Create();
    EnvMapSharedData::s_Quad = H2M::RefH2M<Quad>::Create();

    m_EditorScene = H2M::RefH2M<H2M::SceneH2M>::Create();
    m_EditorScene->SetSkyboxLod(0.0f);

    m_ActiveScene = m_EditorScene;

    EnvMapSceneRenderer::Init(m_Filepath, m_ActiveScene.Raw(), this);
    SetSkybox(EnvMapSceneRenderer::GetRadianceMap());

    SetupContextData(m_Scene);

    // Create a default material
    s_DefaultMaterial = MaterialLibrary::CreateDefaultMaterial("MAT_DEF");
    MaterialLibrary::AddEnvMapMaterial(s_DefaultMaterial->GetUUID(), s_DefaultMaterial);

    // Create the light material
    s_LightMaterial = MaterialLibrary::CreateDefaultMaterial("MAT_LIGHT");
    // Load Hazel/Renderer/HazelTexture
    s_LightMaterial->GetAlbedoInput().TextureMap = ResourceManager::LoadTexture2D_H2M("Textures/light_bulb.png", true);
    s_LightMaterial->GetAlbedoInput().UseTexture = true;
    MaterialLibrary::AddEnvMapMaterial(s_LightMaterial->GetUUID(), s_LightMaterial);

    Application::Get()->GetWindow()->SetEventCallback(H2M_BIND_EVENT_FN(EnvMapEditorLayer::OnEvent));

    SetupShaders();

    bool depthTest = true;
    H2M::Renderer2D_H2M::InitObsolete();

    m_SceneHierarchyPanel = new H2M::SceneHierarchyPanelH2M(m_EditorScene);
    m_SceneHierarchyPanel->SetSelectionChangedCallback(std::bind(&EnvMapEditorLayer::SelectEntity, this, std::placeholders::_1));
    m_SceneHierarchyPanel->SetEntityDeletedCallback(std::bind(&EnvMapEditorLayer::OnEntityDeleted, this, std::placeholders::_1));
    m_SceneHierarchyPanel->SetContext(m_ActiveScene); // already done in constructor
    m_EditorScene->SetSelectedEntity({});

    m_ContentBrowserPanel = new H2M::ContentBrowserPanelH2M();

    m_MaterialEditorPanel = new MaterialEditorPanel();

    m_DisplayBoundingBoxes = false;
    m_DrawOnTopBoundingBoxes = true; // obsolete?
    m_DisplayLineElements = false;

    EnvMapSharedData::s_DisplayOutline = false;
    EnvMapSharedData::s_DisplayWireframe = false;
    EnvMapSharedData::s_SkyboxExposureFactor = 0.5f;
    EnvMapSharedData::s_RadiancePrefilter = false;
    EnvMapSharedData::s_EnvMapRotation = 180.0f;
    EnvMapSharedData::s_DisplayHazelGrid = true;
    EnvMapSharedData::s_DisplayRay = false;

    Scene::s_ImGuizmoTransform = nullptr; // &GetMeshEntity()->Transform();
    Scene::s_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE;

    m_ResizeViewport = { 0.0f, 1.0f };

    SetupRenderFramebuffer();

    m_WindowTitleStatic = Application::Get()->GetWindow()->GetTitle();
    UpdateWindowTitle("New Scene");

    EnvMapSharedData::s_ShadowMapDirLight = H2M::RefH2M<ShadowMap>::Create();
    EnvMapSharedData::s_ShadowMapDirLight->Init(m_Scene->GetSettings().shadowMapWidth, m_Scene->GetSettings().shadowMapHeight);

    m_LightDirection = glm::normalize(glm::vec3(0.05f, -0.85f, 0.05f));
    m_LightProjectionMatrix = glm::ortho(-64.0f, 64.0f, -64.0f, 64.0f, -64.0f, 64.0f);

    EnvMapSharedData::s_OmniShadowMapPointLight = H2M::RefH2M<OmniShadowMap>::Create();
    EnvMapSharedData::s_OmniShadowMapPointLight->Init(m_Scene->GetSettings().omniShadowMapWidth, m_Scene->GetSettings().omniShadowMapHeight);

    EnvMapSharedData::s_OmniShadowMapSpotLight = H2M::RefH2M<OmniShadowMap>::Create();
    EnvMapSharedData::s_OmniShadowMapSpotLight->Init(m_Scene->GetSettings().omniShadowMapWidth, m_Scene->GetSettings().omniShadowMapHeight);

    GeometryFactory::Quad::Create();
}

void EnvMapEditorLayer::OnAttach()
{
    s_CheckerboardTexture = H2M::Texture2D_H2M::Create("Textures/Hazel/Checkerboard.tga", false);
    m_PlayButtonTex = H2M::Texture2D_H2M::Create("Textures/Hazel/PlayButton.png", false);

    m_IconPlay = H2M::Texture2D_H2M::Create("Resources/Icons/PlayButton.png", false);
    m_IconStop = H2M::Texture2D_H2M::Create("Resources/Icons/StopButton.png", false);
}

EnvMapEditorLayer::~EnvMapEditorLayer()
{
    MaterialLibrary::Cleanup();

    delete m_RuntimeCamera;
    delete m_EditorCamera;

    GeometryFactory::Quad::Destroy();
}

void EnvMapEditorLayer::OnDetach()
{
}

void EnvMapEditorLayer::SetupContextData(Scene* scene)
{
    MaterialLibrary::Init();

    auto viewportWidth = m_ViewportBounds[1].x - m_ViewportBounds[0].x;
    auto viewportHeight = m_ViewportBounds[1].y - m_ViewportBounds[0].y;

    float fov = 60.0f;
    float aspectRatio = 1.778f; // 16/9
    m_EditorCamera = new H2M::EditorCameraH2M(fov, aspectRatio, 0.1f, 1000.0f);
    m_RuntimeCamera = new RuntimeCamera(scene->GetSettings().cameraPosition, scene->GetSettings().cameraStartYaw, scene->GetSettings().cameraStartPitch,
        fov, aspectRatio, scene->GetSettings().cameraMoveSpeed, 0.1f);

    m_EditorCamera->SetProjectionType(H2M::SceneCameraH2M::ProjectionType::Perspective);
    m_RuntimeCamera->SetProjectionType(H2M::SceneCameraH2M::ProjectionType::Perspective);

    m_EditorCamera->SetViewportSize((float)Application::Get()->GetWindow()->GetWidth(), (float)Application::Get()->GetWindow()->GetHeight());
    m_RuntimeCamera->SetViewportSize((float)Application::Get()->GetWindow()->GetWidth(), (float)Application::Get()->GetWindow()->GetHeight());

    H2M::EntityH2M cameraEntity = CreateEntity("Camera");
    cameraEntity.AddComponent<H2M::CameraComponentH2M>(*m_RuntimeCamera);
    m_ActiveCamera = m_RuntimeCamera;

    // cameraEntity.AddComponent<H2M::CameraComponentH2M>(*m_EditorCamera);
    // m_ActiveCamera = m_EditorCamera;

    Log::GetLogger()->debug("cameraEntity UUID: {0}", cameraEntity.GetUUID());

    // auto mapGenerator = CreateEntity("Map Generator");
    // mapGenerator.AddComponent<H2M::ScriptComponent>("Example.MapGenerator");

    // H2M::MeshH2M* meshQuad = new H2M::MeshH2M("Models/Primitives/quad.obj", m_ShaderHazelPBR, nullptr, false);

    SetupLights();
}

void EnvMapEditorLayer::SetupLights()
{
    H2M::EntityH2M directionalLightEntity = CreateEntity("Directional Light");
    auto& dlc = directionalLightEntity.AddComponent<H2M::DirectionalLightComponentH2M>();
    auto& tc = directionalLightEntity.GetComponent<H2M::TransformComponentH2M>();
    tc.Rotation = glm::normalize(glm::vec3(-0.05f, -0.85f, -0.05f));
    tc.Translation = glm::vec3(0.0f, 15.0f, 0.0f);

    // H2M::EntityH2M pointLightEntity = CreateEntity("Point Light");
    // auto& plc = pointLightEntity.AddComponent<H2M::PointLightComponentH2M>();

    // H2M::EntityH2M spotLightEntity = CreateEntity("Spot Light");
    // auto& slc = spotLightEntity.AddComponent<H2M::SpotLightComponentH2M>();
    // auto& sltc = spotLightEntity.GetComponent<H2M::TransformComponentH2M>();
    // sltc.Rotation = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));
}

void EnvMapEditorLayer::SetupRenderFramebuffer()
{
    uint32_t width = Application::Get()->GetWindow()->GetWidth();
    uint32_t height = Application::Get()->GetWindow()->GetHeight();

    m_RenderFramebuffer = MoravaFramebuffer::Create(width, height);
    m_RenderFramebuffer->AddColorAttachmentSpecification(width, height, AttachmentType::Texture, AttachmentFormat::Color);
    // m_RenderFramebuffer->AddColorAttachmentSpecification(width, height, AttachmentType::Texture, AttachmentFormat::Color);
    m_RenderFramebuffer->AddColorAttachmentSpecification(width, height, AttachmentType::Texture, AttachmentFormat::RED_INTEGER);
    m_RenderFramebuffer->AddDepthAttachmentSpecification(width, height, AttachmentType::Texture, AttachmentFormat::Depth);
    m_RenderFramebuffer->Generate(width, height);

    // post processing framebuffer
    m_PostProcessingFramebuffer = MoravaFramebuffer::Create(width, height);
    m_PostProcessingFramebuffer->AddColorAttachmentSpecification(width, height, AttachmentType::Texture, AttachmentFormat::Color);
    m_PostProcessingFramebuffer->Generate(width, height);

    // Framebuffer with multiple attachments Hazel2D style
    // H2M::FramebufferSpecificationH2M framebufferSpecH2M;
    // framebufferSpecH2M.Width = width;
    // framebufferSpecH2M.Height = height;
    // framebufferSpecH2M.Attachments = { H2M::ImageFormatH2M::RGBA8, H2M::ImageFormatH2M::RED_INTEGER, H2M::ImageFormatH2M::Depth };
    // m_RenderFramebufferTempH2M = H2M::FramebufferH2M::Create(framebufferSpecH2M);
}

H2M::EntityH2M EnvMapEditorLayer::CreateEntity(const std::string& name)
{
    // Both NoECS and ECS
    H2M::EntityH2M entity = m_EditorScene->CreateEntity(name, m_EditorScene);

    return entity;
}

H2M::EntityH2M EnvMapEditorLayer::LoadEntity(std::string fullPath)
{
    std::string fileName = Util::GetFileNameFromFullPath(fullPath);
    std::string fileNameNoExt = Util::StripExtensionFromFileName(fileName);

    bool isAnimated = false;
    EnvMapSharedData::s_ShaderHazelPBR = MoravaShaderLibrary::Get("HazelPBR_Static");

    Log::GetLogger()->debug("EnvMapEditorLayer::LoadMesh: fullPath '{0}' fileName '{1}' fileNameNoExt '{2}'", fullPath, fileName, fileNameNoExt);

    H2M::RefH2M<H2M::MeshH2M> mesh = H2M::RefH2M<H2M::MeshH2M>::Create(fullPath, EnvMapSharedData::s_ShaderHazelPBR, H2M::RefH2M<H2M::MaterialH2M>(), isAnimated);

    mesh->SetTimeMultiplier(1.0f);

    EnvMapSceneRenderer::CreateDrawCommand(fileNameNoExt, mesh);

    H2M::EntityH2M meshEntity = CreateEntity(fileNameNoExt);
    // m_MeshEntity: NoECS version
    meshEntity.AddComponent<H2M::MeshComponentH2M>(mesh);

    EnvMapSceneRenderer::SubmitEntityEnvMap(meshEntity);
    // LoadEnvMapMaterials(mesh, meshEntity);

    return meshEntity;
}

H2M::CameraComponentH2M EnvMapEditorLayer::GetMainCameraComponent()
{
    auto mainCameraEntity = m_EditorScene->GetMainCameraEntity();
    if (mainCameraEntity && mainCameraEntity.HasComponent<H2M::CameraComponentH2M>())
    {
        auto mainCameraComponent = mainCameraEntity.GetComponent<H2M::CameraComponentH2M>();
        return mainCameraComponent;
    }
    return {};
}

void EnvMapEditorLayer::AddSubmeshToSelectionContext(SelectedSubmesh submesh)
{
    EntitySelection::s_SelectionContext.push_back(submesh);

    if (EntitySelection::s_SelectionContext.size() && EntitySelection::s_SelectionContext[0].Mesh)
    {
        Log::GetLogger()->debug("SelectionContext[0].Mesh->MeshName: '{0}'", EntitySelection::s_SelectionContext[0].Mesh->MeshName);
    }
}

void EnvMapEditorLayer::ShowBoundingBoxes(bool showBoundingBoxes, bool showBoundingBoxesOnTop)
{
}

void EnvMapEditorLayer::SetupShaders()
{
    H2M::RefH2M<MoravaShader> shaderHazelPBR_Static = MoravaShader::Create("Shaders/Hazel/HazelPBR_Static.vs", "Shaders/Hazel/HazelPBR.fs");
    Log::GetLogger()->info("EnvMapEditorLayer: m_ShaderHazelPBR_Static compiled [programID={0}]", shaderHazelPBR_Static->GetProgramID());

    // PBR shader with the support for Cascaded Shadow Maps
    // H2M::RefH2M<MoravaShader> shaderHazelPBR_Static = MoravaShader::Create("Shaders/Hazel/HazelPBR_Static.vs", "Shaders/Hazel/HazelPBR_Static_CSM.fs");
    // Log::GetLogger()->info("EnvMapEditorLayer: m_ShaderHazelPBR_Static compiled [programID={0}]", shaderHazelPBR_Static->GetProgramID());

    H2M::RefH2M<MoravaShader> shaderHazelPBR_Anim = MoravaShader::Create("Shaders/Hazel/HazelPBR_Anim.vs", "Shaders/Hazel/HazelPBR.fs");
    Log::GetLogger()->info("EnvMapEditorLayer: m_ShaderHazelPBR_Anim compiled [programID={0}]", shaderHazelPBR_Anim->GetProgramID());

    H2M::RefH2M<MoravaShader> shaderRenderer2D_Line = MoravaShader::Create("Shaders/Hazel/Renderer2D_Line.vs", "Shaders/Hazel/Renderer2D_Line.fs");
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
    H2M::RefH2M<MoravaShader> shaderComposite = EnvMapSceneRenderer::GetShaderComposite();
    shaderComposite->Bind();
    shaderComposite->SetInt("u_Texture", EnvMapSharedData::s_SamplerSlots.at("u_Texture"));

    // ERROR: OpenGLMoravaShader::SetFloat() failed [name='u_Exposure', location='-1']
    // shaderComposite->SetFloat("u_Exposure", GetMainCameraComponent().Camera.GetExposure());

    /**** END Shaders/Hazel/SceneComposite ****/

    /**** BEGIN Shaders/Hazel/Skybox ****/
    EnvMapSceneRenderer::s_ShaderSkybox->Bind();
    EnvMapSceneRenderer::s_ShaderSkybox->SetInt("u_Texture", EnvMapSharedData::s_SamplerSlots.at("u_Texture"));
    EnvMapSceneRenderer::s_ShaderSkybox->SetFloat("u_TextureLod", m_EditorScene->GetSkyboxLod());
    // apply exposure to Shaders/Hazel/Skybox, considering that Shaders/Hazel/SceneComposite is not yet enabled
    EnvMapSceneRenderer::s_ShaderSkybox->SetFloat("u_Exposure", GetMainCameraComponent().Camera.GetExposure() * EnvMapSharedData::s_SkyboxExposureFactor); // originally used in Shaders/Hazel/SceneComposite
    /**** END Shaders/Hazel/Skybox ****/

    /**** BEGIN Shaders/Hazel/Outline ****/
    EnvMapSharedData::s_ShaderOutline->Bind();
    glm::mat4 viewProj = EnvMapSceneRenderer::GetViewProjection();
    EnvMapSharedData::s_ShaderOutline->SetMat4("u_ViewProjection", viewProj);
    /**** BEGIN Shaders/Hazel/Outline ****/
}

void EnvMapEditorLayer::SetSkybox(H2M::RefH2M<H2M::TextureCubeH2M> skybox)
{
    m_SkyboxTexture = skybox;
    m_SkyboxTexture->Bind(EnvMapSharedData::s_SamplerSlots.at("u_Texture"));
}

void EnvMapEditorLayer::OnUpdate(float ts)
{
    switch (m_SceneState)
    {
        case SceneState::Edit:
        {
            if (m_ViewportFocused)
            {
                // m_CameraContoller.OnUpdate(ts);
                m_EditorCamera->OnUpdate(ts);
            }

            m_ActiveScene->OnUpdateEditor(ts, *m_EditorCamera);

            // Calculate mouse coordinates within the viewport
            // auto [mx, my] = ImGui::GetMousePos();
            // mx -= m_ViewportBounds[0].x;
            // my -= m_ViewportBounds[0].y;
            // glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
            // int mouseX = (int)mx;
            // int mouseY = (int)my;

            m_EditorScene->OnRenderEditor(H2M::RefH2M<H2M::SceneRendererH2M>(), ts, *m_EditorCamera);

            if (m_DrawOnTopBoundingBoxes)
            {
                // H2M::RendererH2M::BeginRenderPass(H2M::SceneRendererH2M::GetFinalRenderPass(), false);
                // auto viewProj = s_EditorCamera->GetViewProjection();
                // H2M::Renderer2D::BeginScene(viewProj, false);
                // // TODO: Renderer::DrawAABB(m_MeshEntity.GetComponent<MeshComponentH2M>(), m_MeshEntity.GetComponent<TransformComponentH2M>());
                // H2M::Renderer2D::EndScene();
                // H2M::RendererH2M::EndRenderPass();
            }
            break;
        }
        case SceneState::Play:
        {
            if (m_ViewportFocused)
            {
                m_EditorCamera->OnUpdate(ts);
            }

            m_ActiveScene->OnUpdateRuntime(ts);

            // m_RuntimeScene->OnUpdate(timestep);
            m_ActiveScene->OnRenderRuntime(H2M::RefH2M<H2M::SceneRendererH2M>(), ts);
            break;
        }
        case SceneState::Pause:
        {
            if (m_ViewportFocused)
            {
                m_EditorCamera->OnUpdate(ts);
            }

            m_ActiveScene->OnRenderRuntime(H2M::RefH2M<H2M::SceneRendererH2M>(), ts);
            break;
        }
    }

    CameraSyncECS();

    if (GetDirectionalLightEntities().size())
    {
        if (GetDirectionalLightEntities().at(0).HasComponent<H2M::TransformComponentH2M>())
        {
            auto& tc = GetDirectionalLightEntities().at(0).GetComponent<H2M::TransformComponentH2M>();
            EnvMapSceneRenderer::GetActiveLight().Direction = glm::eulerAngles(glm::quat(tc.Rotation));

            m_LightDirection = glm::eulerAngles(glm::quat(tc.Rotation));
            EnvMapSharedData::s_DirLightTransform = Util::CalculateLightTransform(m_LightProjectionMatrix, m_LightDirection);
        }
    }

    OnUpdateEditor(m_EditorScene, ts);
    // OnUpdateRuntime(s_RuntimeScene, timestep);
}

void EnvMapEditorLayer::OnUpdateEditor(H2M::RefH2M<H2M::SceneH2M> scene, float timestep)
{
    // m_EditorScene = scene;
    // m_ActiveScene = scene;

    EnvMapSceneRenderer::BeginScene(m_EditorScene.Raw(), H2M::SceneRendererCameraH2M{ GetMainCameraComponent().Camera, GetMainCameraComponent().Camera.GetViewMatrix() });

    UpdateUniforms();

    // Update HazelMesh List
    auto meshEntities = m_ActiveScene->GetAllEntitiesWith<H2M::MeshComponentH2M>();
    for (auto entt : meshEntities)
    {
        H2M::EntityH2M entity{ entt, m_ActiveScene.Raw() };
        H2M::RefH2M<H2M::MeshH2M> mesh = entity.GetComponent<H2M::MeshComponentH2M>().Mesh;
        if (mesh)
        {
            mesh->OnUpdate(timestep, false);
        }
    }

    m_ActiveCamera->OnUpdate(timestep);

    GetMainCameraComponent().Camera.SetViewMatrix(m_ActiveCamera->GetViewMatrix());
    GetMainCameraComponent().Camera.SetProjectionMatrix(m_ActiveCamera->GetProjectionMatrix());

    auto viewMatrix1 = m_ActiveCamera->GetViewMatrix();
    auto projectionMatrix1 = m_ActiveCamera->GetProjectionMatrix();

    auto viewMatrix2 = GetMainCameraComponent().Camera.GetViewMatrix();
    auto projectionMatrix2 = GetMainCameraComponent().Camera.GetProjectionMatrix();

    Scene::s_ImGuizmoTransform = &m_CurrentlySelectedTransform; // moved from SceneHazelEnvMap

    // m_ViewportSize.x = m_ViewportBounds[1].x - m_ViewportBounds[0].x;
    // m_ViewportSize.y = m_ViewportBounds[1].y - m_ViewportBounds[0].y;

    if (m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f)
    {
        m_ActiveCamera->SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
    }
}

void EnvMapEditorLayer::OnUpdateRuntime(H2M::RefH2M<H2M::SceneH2M> scene, float timestep)
{
    m_EditorScene = scene;

    EnvMapSceneRenderer::BeginScene(m_EditorScene.Raw(), { GetMainCameraComponent().Camera, GetMainCameraComponent().Camera.GetViewMatrix() });

    UpdateUniforms();

    // Update HazelMesh List
    auto meshEntities = m_EditorScene->GetAllEntitiesWith<H2M::MeshComponentH2M>();
    for (auto entt : meshEntities)
    {
        H2M::EntityH2M entity{ entt, m_EditorScene.Raw() };
        H2M::RefH2M<H2M::MeshH2M> mesh = entity.GetComponent<H2M::MeshComponentH2M>().Mesh;

        mesh->OnUpdate(timestep, false);
    }

    Scene::s_ImGuizmoTransform = &m_CurrentlySelectedTransform; // moved from SceneHazelEnvMap

    // m_ViewportSize.x = m_ViewportBounds[1].x - m_ViewportBounds[0].x;
    // m_ViewportSize.y = m_ViewportBounds[1].y - m_ViewportBounds[0].y;

    if (m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f)
    {
        m_ActiveCamera->SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
    }
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
    m_ActiveCamera->SetAspectRatio(GetMainCameraComponent().Camera.GetAspectRatio());
    m_ActiveCamera->SetExposure(GetMainCameraComponent().Camera.GetExposure());
    m_ActiveCamera->SetProjectionType(GetMainCameraComponent().Camera.GetProjectionType());

    // perspective
    m_ActiveCamera->SetPerspectiveVerticalFOV(GetMainCameraComponent().Camera.GetPerspectiveVerticalFOV());
    m_ActiveCamera->SetPerspectiveNearClip(GetMainCameraComponent().Camera.GetPerspectiveNearClip());
    m_ActiveCamera->SetPerspectiveFarClip(GetMainCameraComponent().Camera.GetPerspectiveFarClip());

    // ortho
    m_ActiveCamera->SetOrthographicNearClip(GetMainCameraComponent().Camera.GetOrthographicNearClip());
    m_ActiveCamera->SetOrthographicFarClip(GetMainCameraComponent().Camera.GetOrthographicFarClip());

    // s_ActiveCamera->SetPosition(GetMainCameraComponent().Camera.GetPosition());
    // s_ActiveCamera->SetPitch(GetMainCameraComponent().Camera.GetPitch());
    // s_ActiveCamera->SetYaw(GetMainCameraComponent().Camera.GetYaw());
    // s_ActiveCamera->SetProjectionMatrix(GetMainCameraComponent().Camera.GetProjectionMatrix());
}

H2M::EntityH2M EnvMapEditorLayer::GetMeshEntity()
{
    H2M::EntityH2M meshEntity;
    auto meshEntities = m_EditorScene->GetAllEntitiesWith<H2M::MeshComponentH2M>();
    if (meshEntities.size()) {
        for (auto entt : meshEntities)
        {
            meshEntity = H2M::EntityH2M(entt, m_EditorScene.Raw());
        }
        return meshEntity;
    }
    return {};
}

float& EnvMapEditorLayer::GetSkyboxLOD()
{
    return m_EditorScene->GetSkyboxLod();
}

void EnvMapEditorLayer::SetViewportBounds(glm::vec2* viewportBounds)
{
    m_ViewportBounds[0] = viewportBounds[0];
    m_ViewportBounds[1] = viewportBounds[1];
}

void EnvMapEditorLayer::SetSkyboxLOD(float LOD)
{
    m_EditorScene->SetSkyboxLod(LOD);
}

H2M::RefH2M<MoravaShader> EnvMapEditorLayer::GetShaderPBR_Anim()
{
    return MoravaShaderLibrary::Get("HazelPBR_Anim");
}

H2M::RefH2M<MoravaShader> EnvMapEditorLayer::GetShaderPBR_Static()
{
    return MoravaShaderLibrary::Get("HazelPBR_Static");
}

void EnvMapEditorLayer::OnImGuiRender(Window* mainWindow, Scene* scene)
{
    // if (m_FullscreenEnabled) return;

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
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.9f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.60f, 0.6f, 0.6f, 1.0f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);

    if (m_ShowWindowSceneHierarchy)
    {
        EnvMapSceneRenderer::OnImGuiRender();

        m_SceneHierarchyPanel->OnImGuiRender(&m_ShowWindowSceneHierarchy);

        /****************************************************************************************************/

        uint32_t id = 0;
        auto meshEntities = m_ActiveScene->GetAllEntitiesWith<H2M::MeshComponentH2M>();
        for (auto entt : meshEntities)
        {
            H2M::EntityH2M entity = { entt, m_ActiveScene.Raw() };
            auto& meshComponent = entity.GetComponent<H2M::MeshComponentH2M>();
            if (meshComponent.Mesh)
            {
                meshComponent.Mesh->OnImGuiRender(++id, &m_ShowWindowSceneHierarchy);
            }
        }
    }

    if (m_ShowWindowAssetManager)
    {
        m_ContentBrowserPanel->OnImGuiRender(m_ShowWindowAssetManager);
    }

    if (m_ShowWindowMaterialEditor)
    {
        m_MaterialEditorPanel->OnImGuiRender(&m_ShowWindowMaterialEditor);
    }

    if (m_ShowWindowMaterialEditor)
    {
        DisplaySubmeshMaterialSelector(&m_ShowWindowMaterialEditor);
    }

    if (m_ShowWindowTransform)
    {
        /////////////////////////////////////////////////////////
        //// TRANSFORM
        /////////////////////////////////////////////////////////
        ImGui::Begin("Transform", &m_ShowWindowTransform);
        {
            if (EntitySelection::s_SelectionContext.size())
            {
                glm::mat4 transformImGui;

                SelectedSubmesh selectedSubmesh = EntitySelection::s_SelectionContext[0];

                // Entity transform
                auto& tc = selectedSubmesh.Entity.GetComponent<H2M::TransformComponentH2M>();
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

        /////////////////////////////////////////////////////////
        //// TOOLBAR
        /////////////////////////////////////////////////////////
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 4));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.8f, 0.8f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));

            //  ImGui::Begin("Toolbar", &m_ShowWindowTransform);
            //  {
            //      if (m_SceneState == SceneState::Edit)
            //      {
            //          float physics2DGravity = m_EditorScene->GetPhysics2DGravity();
            //          if (ImGuiWrapper::Property("Gravity", physics2DGravity, -10000.0f, 10000.0f, PropertyFlag::DragProperty))
            //          {
            //              m_EditorScene->SetPhysics2DGravity(physics2DGravity);
            //          }
            //  
            //          if (ImGui::ImageButton((ImTextureID)(uint64_t)(m_PlayButtonTex->GetID()), ImVec2(32, 32), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), ImVec4(0.9f, 0.9f, 0.9f, 1.0f)))
            //          {
            //              OnScenePlay();
            //          }
            //      }
            //      else if (m_SceneState == SceneState::Play)
            //      {
            //          float physics2DGravity = m_RuntimeScene->GetPhysics2DGravity();
            //          
            //          if (ImGuiWrapper::Property("Gravity", physics2DGravity, -10000.0f, 10000.0f, PropertyFlag::DragProperty))
            //          {
            //              m_RuntimeScene->SetPhysics2DGravity(physics2DGravity);
            //          }
            //  
            //          if (ImGui::ImageButton((ImTextureID)(uint64_t)(m_PlayButtonTex->GetID()), ImVec2(32, 32), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(1.0f, 1.0f, 1.0f, 0.2f)))
            //          {
            //              OnSceneStop();
            //          }
            //      }
            //      ImGui::SameLine();
            //      if (ImGui::ImageButton((ImTextureID)(uint64_t)(m_PlayButtonTex->GetID()), ImVec2(32, 32), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), ImVec4(1.0f, 1.0f, 1.0f, 0.6f)))
            //      {
            //          MORAVA_CORE_INFO("PLAY!");
            //      }
            //  }
            //  ImGui::End();

            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
            ImGui::PopStyleVar();
            ImGui::PopStyleVar();
        }

        /////////////////////////////////////////////////////////
        //// SWITCH STATE
        /////////////////////////////////////////////////////////
        ImGui::Begin("Switch State", &m_ShowWindowTransform);
        {
            const char* sceneState = "Scene State: UNDEFINED";
            switch (m_SceneState)
            {
                case SceneState::Edit:
                    sceneState = "Scene State: EDIT";
                    break;
                case SceneState::Play:
                    sceneState = "Scene State: PLAY";
                    break;
                case SceneState::Pause:
                    sceneState = "Scene State: PAUSE";
                    break;
                case SceneState::Simulate:
                    sceneState = "Scene State: SIMULATE";
                    break;
            }
            ImGui::Text(sceneState);

            ImGui::Separator();

            const char* label = m_ActiveCamera == m_EditorCamera ? "EDITOR [ Editor Camera ]" : "RUNTIME [ Runtime Camera ]";
            if (ImGui::Button(label))
            {
                m_ActiveCamera = (m_ActiveCamera == m_EditorCamera) ?
                    (H2M::CameraH2M*)m_RuntimeCamera :
                    (H2M::CameraH2M*)m_EditorCamera;
            }
        }
        ImGui::End();

        /////////////////////////////////////////////////////////
        //// CAMERA
        /////////////////////////////////////////////////////////
        ImGui::Begin("Camera");
        {
            if (ImGui::CollapsingHeader("Display Info", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto pitch = GetMainCameraComponent().Camera.GetPitch();
                if (ImGui::DragFloat("Pitch", &pitch, 1.0f, -89.0f, 89.0f))
                {
                    GetMainCameraComponent().Camera.SetPitch(pitch);
                }

                auto yaw = GetMainCameraComponent().Camera.GetYaw();
                if (ImGui::DragFloat("Yaw", &yaw, 1.0f, -180.0f, 180.0f))
                {
                    GetMainCameraComponent().Camera.SetYaw(yaw);
                }

                auto fov = GetMainCameraComponent().Camera.GetPerspectiveVerticalFOV();
                if (ImGui::DragFloat("FOV", &fov, 1.0f, 10.0f, 150.0f))
                {
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
                sprintf(buffer, "Aspect Ratio  %.2f", m_ActiveCamera->GetAspectRatio());
                ImGui::Text(buffer);
                sprintf(buffer, "Position    X %.2f Y %.2f Z %.2f",
                    m_ActiveCamera->GetPosition().x,
                    m_ActiveCamera->GetPosition().y,
                    m_ActiveCamera->GetPosition().z);
                ImGui::Text(buffer);
                sprintf(buffer, "Direction   X %.2f Y %.2f Z %.2f",
                    m_ActiveCamera->GetDirection().x,
                    m_ActiveCamera->GetDirection().y,
                    m_ActiveCamera->GetDirection().z);
                ImGui::Text(buffer);
                sprintf(buffer, "Front       X %.2f Y %.2f Z %.2f",
                    m_ActiveCamera->GetFront().x,
                    m_ActiveCamera->GetFront().y,
                    m_ActiveCamera->GetFront().z);
                ImGui::Text(buffer);
                sprintf(buffer, "Up          X %.2f Y %.2f Z %.2f",
                    m_ActiveCamera->GetUp().x,
                    m_ActiveCamera->GetUp().y,
                    m_ActiveCamera->GetUp().z);
                ImGui::Text(buffer);
                sprintf(buffer, "Right       X %.2f Y %.2f Z %.2f",
                    m_ActiveCamera->GetRight().x,
                    m_ActiveCamera->GetRight().y,
                    m_ActiveCamera->GetRight().z);
                ImGui::Text(buffer);
            }
        }
        ImGui::End();

        /////////////////////////////////////////////////////////
        //// ENVIRONMENT
        /////////////////////////////////////////////////////////

        /**** BEGIN Environment ****/
        ImGui::Begin("Environment");
        {
            if (ImGui::CollapsingHeader("Display Info", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
            {
                {
                    ImGui::Columns(2);

                    ImGui::InputText("##envmapfilepath", (char*)m_EnvMapFilename.c_str(), 256, ImGuiInputTextFlags_ReadOnly);

                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                        {
                            std::wstring itemPath = std::wstring((const wchar_t*)payload->Data);
                            size_t itemSize = payload->DataSize;
                            Log::GetLogger()->debug("END DRAG & DROP FILE '{0}', size: {1}", Util::to_str(itemPath.c_str()).c_str(), itemSize);

                            m_EnvMapFilename = std::string{ itemPath.begin(), itemPath.end() };
                            if (m_EnvMapFilename != "")
                            {
                                EnvMapSceneRenderer::SetEnvironment(EnvMapSceneRenderer::Load(m_EnvMapFilename));
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }

                    ImGui::NextColumn();

                    if (ImGui::Button("Load Environment Map"))
                    {
                        m_EnvMapFilename = Application::Get()->OpenFile("*.hdr");
                        if (m_EnvMapFilename != "")
                        {
                            EnvMapSceneRenderer::SetEnvironment(EnvMapSceneRenderer::Load(m_EnvMapFilename));
                        }
                    }

                    ImGui::NextColumn();

                    ImGui::AlignTextToFramePadding();

                    float skyboxLOD = GetSkyboxLOD();
                    if (ImGuiWrapper::Property("Skybox LOD", skyboxLOD, 0.01f, 0.0f, 2.0f, PropertyFlag::DragProperty))
                    {
                        SetSkyboxLOD(skyboxLOD);
                    }

                    H2M::LightH2M light = EnvMapSceneRenderer::GetActiveLight();
                    H2M::LightH2M lightPrev = light;

                    ImGuiWrapper::Property("Light Direction", light.Direction, -180.0f, 180.0f, PropertyFlag::DragProperty);
                    ImGuiWrapper::Property("Light Radiance", light.Radiance, PropertyFlag::ColorProperty);
                    ImGuiWrapper::Property("Light Multiplier", light.Multiplier, 0.01f, 0.0f, 5.0f, PropertyFlag::DragProperty);
                    ImGuiWrapper::Property("Exposure", GetMainCameraComponent().Camera.GetExposure(), 0.01f, 0.0f, 40.0f, PropertyFlag::DragProperty);
                    ImGuiWrapper::Property("Skybox Exposure Factor", EnvMapSharedData::s_SkyboxExposureFactor, 0.01f, 0.0f, 10.0f, PropertyFlag::DragProperty);

                    ImGuiWrapper::Property("Radiance Prefiltering", EnvMapSharedData::s_RadiancePrefilter);
                    ImGuiWrapper::Property("Env Map Rotation", EnvMapSharedData::s_EnvMapRotation, 1.0f, -360.0f, 360.0f, PropertyFlag::DragProperty);

                    if (m_SceneState == SceneState::Edit)
                    {
                        float physics2DGravity = m_EditorScene->GetPhysics2DGravity();
                        if (ImGuiWrapper::Property("Gravity", physics2DGravity, -10000.0f, 10000.0f, PropertyFlag::DragProperty)) {
                            m_EditorScene->SetPhysics2DGravity(physics2DGravity);
                        }
                    }
                    else if (m_SceneState == SceneState::Play)
                    {
                        float physics2DGravity = m_ActiveScene->GetPhysics2DGravity();
                        if (ImGuiWrapper::Property("Gravity", physics2DGravity, -10000.0f, 10000.0f, PropertyFlag::DragProperty)) {
                            m_ActiveScene->SetPhysics2DGravity(physics2DGravity);
                        }
                    }

                    EnvMapSceneRenderer::SetActiveLight(light);

                    if (light.Direction != lightPrev.Direction)
                    {
                        if (GetDirectionalLightEntities().size())
                        {
                            auto& tc = GetDirectionalLightEntities().at(0).GetComponent<H2M::TransformComponentH2M>();
                            tc.Rotation = glm::eulerAngles(glm::quat(glm::radians(light.Direction)));
                            lightPrev = light;
                        }
                    }

                    ImGui::Columns(1);
                }

                ImGui::Separator();

                {
                    ImGui::Text("Mesh");

                    H2M::EntityH2M meshEntity = {};
                    std::string meshFullPath = "None";

                    std::string fileName = Util::GetFileNameFromFullPath(meshFullPath);
                    ImGui::Text(fileName.c_str()); ImGui::SameLine();
                    if (ImGui::Button("...##Mesh"))
                    {
                        std::string fullPath = Application::Get()->OpenFile();
                        if (fullPath != "")
                        {
                            H2M::EntityH2M entity = LoadEntity(fullPath);
                        }
                    }

                    auto meshEntities = m_ActiveScene->GetAllEntitiesWith<H2M::MeshComponentH2M>();
                    if (meshEntities.size())
                    {
                        meshEntity = GetMeshEntity();
                        auto& meshComponent = meshEntity.GetComponent<H2M::MeshComponentH2M>();
                        if (meshComponent.Mesh)
                        {
                            ImGui::SameLine();
                            ImGui::Checkbox("Is Animated", &meshComponent.Mesh->IsAnimated());
                        }
                    }
                }
            }
        }
        ImGui::End();
        /**** END Environment ****/
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
            try {
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
            }
            catch (const std::runtime_error& e) {
                Log::GetLogger()->error("ImGui Exception: '{0}'", e.what());
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

    if (m_ShowWindowFramebuffers)
    {
        ImGui::Begin("Framebuffers", &m_ShowWindowFramebuffers);
        {
            ImVec2 imageSize(128.0f, 128.0f);

            ImGui::Text("Viewport");
            ImGui::Image((void*)(intptr_t)m_RenderFramebuffer->GetTextureAttachmentColor()->GetID(), imageSize);

            ImGui::Text("Render framebuffer 2nd attachment");
            ImGui::Image((void*)(intptr_t)m_RenderFramebuffer->GetTextureAttachmentColor(1)->GetID(), imageSize);

            ImGui::Text("Post Processing");
            ImGui::Image((void*)(intptr_t)m_PostProcessingFramebuffer->GetTextureAttachmentColor()->GetID(), imageSize);

            ImGui::Text("Equirectangular");
            ImGui::Image((void*)(intptr_t)EnvMapSceneRenderer::GetEnvEquirect()->GetID(), imageSize);

            ImGui::Text("Shadow Map");
            ImGui::Image((void*)(intptr_t)EnvMapSharedData::s_ShadowMapDirLight->GetTextureID(), imageSize);
            // ImGui::Image((void*)(intptr_t)LightManager::directionalLight.GetShadowMap()->GetTextureID(), imageSize);
        }
        ImGui::End();
    }

    if (m_ShowWindowSettings)
    {
        ImGui::Begin("Settings", &m_ShowWindowSettings);
        {
            if (ImGui::CollapsingHeader("Display Info"))
            {
                char buffer[100];
                sprintf(buffer, "Can Viewport Receive Events ? %s", ImGuiWrapper::CanViewportReceiveEvents() ? "YES" : "NO");
                ImGui::Text(buffer);

                ImGui::Separator();

                ImGui::Checkbox("Display Outline", &EnvMapSharedData::s_DisplayOutline);
                ImGui::Checkbox("Display Wireframe", &EnvMapSharedData::s_DisplayWireframe);
                ImGui::Checkbox("Display Bounding Boxes", &m_DisplayBoundingBoxes);
                ImGui::Checkbox("Display Hazel Grid", &EnvMapSharedData::s_DisplayHazelGrid);
                ImGui::Checkbox("Display Line Elements", &m_DisplayLineElements);
                ImGui::Checkbox("Display Ray", &EnvMapSharedData::s_DisplayRay);

                bool eventLoggingEnabled = Application::Get()->GetWindow()->GetEventLogging();
                if (ImGui::Checkbox("Enable Event Logging", &eventLoggingEnabled)) {
                    Application::Get()->GetWindow()->SetEventLogging(eventLoggingEnabled);
                }

                if (ImGui::Checkbox("Show Physics Colliders", &m_ShowPhysicsColliders));

                float fovDegrees = GetMainCameraComponent().Camera.GetPerspectiveVerticalFOV();
                if (ImGui::DragFloat("FOV", &fovDegrees, 1.0f, -60.0f, 180.0f)) {
                    GetMainCameraComponent().Camera.SetPerspectiveVerticalFOV(fovDegrees);
                }

                ImGui::Separator();

                ImGui::Text("Scene Settings");
                ImGui::Checkbox("Enable Shadows", &scene->sceneSettings.enableShadows);
                ImGui::Checkbox("Enable Omni Shadows", &scene->sceneSettings.enableOmniShadows);
                ImGui::Checkbox("Enable Water Effects", &scene->sceneSettings.enableWaterEffects);
                ImGui::Checkbox("Enable Particles", &scene->sceneSettings.enableParticles);
            }
        }
        ImGui::End();
    }

    if (m_ShowWindowRendererStats)
    {
        Application::Get()->OnImGuiRender(&m_ShowWindowRendererStats);
    }

    if (m_ShowWindowHelp)
    {
        ImGui::Begin("Help", &m_ShowWindowHelp);
        {
            ImGui::Text("* Left ALT + Left Mouse Button - Pan");
            ImGui::Text("* Left ALT + Middle Mouse Button - Rotate/Orbit");
            ImGui::Text("* Left ALT + Right Mouse Button - Zoom");
            ImGui::Text("* Left CTRL + P - Capture a screenshot");
            ImGui::Text("* Left SHIFT + R - Toggle Wireframe");
        }
        ImGui::End();
    }

    if (m_ShowWindowMousePicker)
    {
        ImGui::Begin("Mouse Picker", &m_ShowWindowMousePicker);
        {
            if (ImGui::CollapsingHeader("Display Info"))
            {
                MousePicker* mp = MousePicker::Get();

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
    }

    if (m_ShowWindowViewportInfo)
    {
        ImGui::Begin("Viewport Info", &m_ShowWindowViewportInfo);
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
   
    if (m_ShowViewportBounds)
    {
        ImGui::Begin("Viewport Bounds", &m_ShowViewportBounds);
        {
            char buffer[100];

            sprintf(buffer, "Render framebuffer size [ %i %i ]", (int)m_ViewportSize.x, (int)m_ViewportSize.y);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "ViewportBounds Min [ %i %i ]", (int)m_ViewportBounds[0].x, (int)m_ViewportBounds[0].y);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "ViewportBounds Max [ %i %i ]", (int)m_ViewportBounds[1].x, (int)m_ViewportBounds[1].y);
            ImGui::Text(buffer);
            ImGui::Separator();

            glm::vec2 viewportBoundsDiff = m_ViewportBounds[1] - m_ViewportBounds[0];
            sprintf(buffer, "ViewportBounds Diff [ %i %i ]", (int)viewportBoundsDiff.x, (int)viewportBoundsDiff.y);
            ImGui::Text(buffer);
            ImGui::Separator();

            m_MouseCoordsInViewport = GetMouseCoordsInViewport();
            sprintf(buffer, "Mouse Coordinates in Viewport [ %i %i ]", m_MouseCoordsInViewport.first, m_MouseCoordsInViewport.second);
            ImGui::Text(buffer);
            ImGui::Separator();

            m_MouseCoordsInViewportFlipY = GetMouseCoordsInViewportFlipY();
            sprintf(buffer, "Mouse Coordinates in Viewport Flip Y [ %i %i ]", m_MouseCoordsInViewportFlipY.first, m_MouseCoordsInViewportFlipY.second);
            ImGui::Text(buffer);
            ImGui::Separator();

            sprintf(buffer, "EntityID [ %i ]", m_EntityID);
            ImGui::Text(buffer);
            ImGui::Separator();            
        }
        ImGui::End();
    }

    if (m_ShowWindowImGuiMetrics)
    {
        ImGui::ShowMetricsWindow();
    }

    m_ImGuiViewportMain.x = ImGui::GetMainViewport()->Pos.x;
    m_ImGuiViewportMain.y = ImGui::GetMainViewport()->Pos.y;

    // TheCherno ImGui Viewport displaying the framebuffer content
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
    ImGui::Begin("Viewport");
    {
        auto viewportOffset = ImGui::GetCursorPos(); // Includes tab bar

        m_ViewportFocused = ImGui::IsWindowFocused();
        m_ViewportHovered = ImGui::IsWindowHovered();

        ImGuiWrapper::SetViewportEnabled(true);
        ImGuiWrapper::SetViewportHovered(m_ViewportHovered);
        ImGuiWrapper::SetViewportFocused(m_ViewportFocused);

        // Calculate Viewport bounds (used in EnvMapEditorLayer::CastRay)
        auto viewportPanelSize = ImGui::GetContentRegionAvail();
        m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

        ImVec2 screen_pos = ImGui::GetCursorScreenPos();

        m_ImGuiViewport.X = (int)(ImGui::GetWindowPos().x - m_ImGuiViewportMain.x);
        m_ImGuiViewport.Y = (int)(ImGui::GetWindowPos().y - m_ImGuiViewportMain.y);
        m_ImGuiViewport.Width = (int)ImGui::GetWindowWidth();
        m_ImGuiViewport.Height = (int)ImGui::GetWindowHeight();
        m_ImGuiViewport.MouseX = (int)ImGui::GetMousePos().x;
        m_ImGuiViewport.MouseY = (int)ImGui::GetMousePos().y;

        ResizeViewport();

        uint64_t textureID;
        if (m_PostProcessingEnabled)
        {
            textureID = m_PostProcessingFramebuffer->GetTextureAttachmentColor()->GetID();
        }
        else
        {
            textureID = m_RenderFramebuffer->GetTextureAttachmentColor()->GetID();
        }
        ImGui::Image((void*)(intptr_t)textureID, ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

        // BEGIN Viewport Drop target
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                const wchar_t* path = (const wchar_t*)payload->Data;
                OpenScene(std::filesystem::path(g_AssetPath) / path);
            }
            ImGui::EndDragDropTarget();
        }
        // END Viewport Drop target

        // BEGIN Calculate Viewport bounds
        {
            auto windowSize = ImGui::GetWindowSize();
            ImVec2 minBound = ImGui::GetWindowPos();

            minBound.x += viewportOffset.x;
            minBound.y += viewportOffset.y;

            // ImVec2 maxBound = { minBound.x + windowSize.x, minBound.y + windowSize.y };
            ImVec2 maxBound = { minBound.x + m_ViewportSize.x, minBound.y + m_ViewportSize.y };
            m_ViewportBounds[0] = { minBound.x, minBound.y };
            m_ViewportBounds[1] = { maxBound.x, maxBound.y };

            // SetViewportBounds(m_ViewportBounds);
            m_AllowViewportCameraEvents = ImGui::IsMouseHoveringRect(minBound, maxBound); // EditorLayer
        }
        // END Calculate Viewport bounds

        UpdateImGuizmo(mainWindow);
    }
    ImGui::End();
    ImGui::PopStyleVar();

    UI_Toolbar();

    ImVec2 workPos = ImGui::GetMainViewport()->Pos;
    m_WorkPosImGui = glm::vec2(workPos.x, workPos.y);
}

void EnvMapEditorLayer::UI_Toolbar()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    auto& colors = ImGui::GetStyle().Colors;
    auto& buttonHovered = colors[ImGuiCol_ButtonHovered];
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
    auto& buttonActive = colors[ImGuiCol_ButtonActive];
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));

    ImGui::Begin("##toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    float size = ImGui::GetWindowHeight() - 8.0f;

    H2M::RefH2M<H2M::Texture2D_H2M> icon = m_SceneState == SceneState::Edit ? m_IconPlay : m_IconStop;

    ImGui::SameLine((ImGui::GetWindowContentRegionMax().x * 0.5f) - (size * 0.5f));

    if (ImGui::ImageButton((ImTextureID)(uint64_t)(icon->GetID()), ImVec2(size, size), ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), ImVec4(0.9f, 0.9f, 0.9f, 1.0f)))
    {
        if (m_SceneState == SceneState::Edit)
        {
            OnScenePlay();
        }
        else if (m_SceneState == SceneState::Play)
        {
            OnSceneStop();
        }
    }

    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
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
            if (ImGui::MenuItem("Scene Hierarchy", "Ctrl+H")) {
                m_ShowWindowSceneHierarchy = !m_ShowWindowSceneHierarchy;
            }

            if (ImGui::MenuItem("Transform", "Ctrl+T")) {
                m_ShowWindowTransform = !m_ShowWindowTransform;
            }

            if (ImGui::MenuItem("Asset Manager", "Ctrl+Space")) {
                m_ShowWindowAssetManager = !m_ShowWindowAssetManager;
            }

            if (ImGui::MenuItem("Material Editor", "Ctrl+M")) {
                m_ShowWindowMaterialEditor = !m_ShowWindowMaterialEditor;
            }

            if (ImGui::MenuItem("Post Processing Effects", "Ctrl+P")) {
                m_ShowWindowPostProcessing = !m_ShowWindowPostProcessing;
            }

            if (ImGui::MenuItem("Shader Manager", "Ctrl+V")) {
                m_ShowWindowShaderManager = !m_ShowWindowShaderManager;
            }

            if (ImGui::MenuItem("Framebuffers", "Ctrl+F")) {
                m_ShowWindowFramebuffers = !m_ShowWindowFramebuffers;
            }

            if (ImGui::MenuItem("Settings", "Ctrl+Z")) {
                m_ShowWindowSettings = !m_ShowWindowSettings;
            }

            if (ImGui::MenuItem("Renderer Stats")) {
                m_ShowWindowRendererStats = !m_ShowWindowRendererStats;
            }

            if (ImGui::MenuItem("Help")) {
                m_ShowWindowHelp = !m_ShowWindowHelp;
            }

            if (ImGui::MenuItem("Mouse Picker")) {
                m_ShowWindowMousePicker = !m_ShowWindowMousePicker;
            }

            if (ImGui::MenuItem("Viewport Bounds")) {
                m_ShowViewportBounds = !m_ShowViewportBounds;
            }

            if (ImGui::MenuItem("Viewport Info")) {
                m_ShowWindowViewportInfo = !m_ShowWindowViewportInfo;
            }

            if (ImGui::MenuItem("Dear ImGui Metrics")) {
                m_ShowWindowImGuiMetrics = !m_ShowWindowImGuiMetrics;
            }

            //  if (ImGui::MenuItem("Fullscreen", "Ctrl+Enter")) {
            //      m_FullscreenEnabled = !m_FullscreenEnabled;
            //  }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Script"))
        {
            if (ImGui::MenuItem("Reload C# Assembly")) {
                // H2M::ScriptEngine::ReloadAssembly("assets/scripts/ExampleApp.dll");
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

void EnvMapEditorLayer::UpdateImGuizmo(Window* mainWindow)
{
    // BEGIN ImGuizmo

    // ImGizmo switching modes
    if (Input::IsKeyPressed(KeyH2M::D1))
    {
        Scene::s_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE;
    }

    if (Input::IsKeyPressed(KeyH2M::D2))
    {
        Scene::s_ImGuizmoType = ImGuizmo::OPERATION::ROTATE;
    }

    if (Input::IsKeyPressed(KeyH2M::D3))
    {
        Scene::s_ImGuizmoType = ImGuizmo::OPERATION::SCALE;
    }

    if (Input::IsKeyPressed(KeyH2M::D4))
    {
        Scene::s_ImGuizmoType = -1;
    }

    // Dirty fix: m_SelectionContext not decremented when mesh entity is removed from the scene
    size_t selectionContextSize = EntitySelection::s_SelectionContext.size();
    auto meshEntities = m_EditorScene->GetAllEntitiesWith<H2M::MeshComponentH2M>();
    if (selectionContextSize > meshEntities.size())
    {
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
        auto& tc = selectedSubmesh.Entity.GetComponent<H2M::TransformComponentH2M>();
        glm::mat4 entityTransform = tc.GetTransform();

        // Snapping
        bool snap = Input::IsKeyPressed(KeyH2M::LeftControl);
        float snapValue = 1.0f; // Snap to 0.5m for translation/scale
        // Snap to 45 degrees for rotation
        if (Scene::s_ImGuizmoType == ImGuizmo::OPERATION::ROTATE)
        {
            snapValue = 45.0f;
        }

        float snapValues[3] = { snapValue, snapValue, snapValue };

        if (s_SelectionMode == SelectionMode::Entity || !selectedSubmesh.Mesh)
        {
            ImGuizmo::Manipulate(
                glm::value_ptr(m_ActiveCamera->GetViewMatrix()),
                glm::value_ptr(m_ActiveCamera->GetProjectionMatrix()),
                (ImGuizmo::OPERATION)Scene::s_ImGuizmoType,
                ImGuizmo::WORLD,
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
                glm::value_ptr(m_ActiveCamera->GetProjectionMatrix()),
                (ImGuizmo::OPERATION)Scene::s_ImGuizmoType,
                ImGuizmo::WORLD,
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

void EnvMapEditorLayer::DisplaySubmeshMaterialSelector(bool* p_open)
{
    /////////////////////////////////////////////////////////
    //// SELECTION
    /////////////////////////////////////////////////////////
    ImGui::Begin("Selection", p_open);
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
        H2M::EntityH2M entity = {};

        if (EntitySelection::s_SelectionContext.size())
        {
            SelectedSubmesh selectedSubmesh = EntitySelection::s_SelectionContext[0];

            entity = selectedSubmesh.Entity;
            entityTag = selectedSubmesh.Entity.GetComponent<H2M::TagComponentH2M>().Tag;
            meshName = (selectedSubmesh.Mesh) ? selectedSubmesh.Mesh->MeshName : "N/A";
            if (selectedSubmesh.Mesh)
            {
                submeshUUID = MaterialLibrary::GetSubmeshUUID(entity, selectedSubmesh.Mesh);
            }
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
}

void EnvMapEditorLayer::UpdateSubmeshMaterialMap(H2M::EntityH2M entity, H2M::SubmeshH2M* submesh)
{
    SubmeshUUID submeshUUID = MaterialLibrary::GetSubmeshUUID(entity, submesh);

    MaterialUUID materialUUID;
    if (MaterialLibrary::s_SubmeshMaterialUUIDs.find(submeshUUID) != MaterialLibrary::s_SubmeshMaterialUUIDs.end()) {
        materialUUID = MaterialLibrary::s_SubmeshMaterialUUIDs.at(submeshUUID);
    }

    auto sm_it = MaterialLibrary::s_SubmeshMaterialUUIDs.find(submeshUUID);
    if (sm_it != MaterialLibrary::s_SubmeshMaterialUUIDs.end()) {
        sm_it->second = materialUUID;
        Log::GetLogger()->debug("s_SubmeshMaterialUUIDs UPDATE [ SubmeshUUID: '{0}' => MaterialUUID: '{1}', Items: {2} ]",
            submeshUUID, materialUUID, MaterialLibrary::s_SubmeshMaterialUUIDs.size());
        return;
    }
    else {
        MaterialLibrary::s_SubmeshMaterialUUIDs.insert(std::make_pair(submeshUUID, materialUUID));
        Log::GetLogger()->debug("s_SubmeshMaterialUUIDs INSERT [ SubmeshUUID: '{0}' => MaterialUUID: '{1}', Items: {2} ]",
            submeshUUID, materialUUID, MaterialLibrary::s_SubmeshMaterialUUIDs.size());
    }
}

void EnvMapEditorLayer::NewScene()
{
    m_ActiveScene = H2M::RefH2M<H2M::SceneH2M>::Create();
    m_ActiveScene->OnRuntimeStart();
    m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
    m_SceneHierarchyPanel->SetContext(m_ActiveScene);
    m_EditorScenePath = std::filesystem::path();

    OnNewScene(m_ViewportSize);

    // SetupLights();
}

void EnvMapEditorLayer::OpenScene()
{
    if (m_SceneState != SceneState::Edit)
    {
        OnSceneStop();
    }

    std::string filepath = H2M::FileDialogsH2M::OpenFile("Hazel Scene (*.hazel)\0*.hazel\0");

    if (!filepath.empty())
    {
        OpenScene(filepath);
    }
}

void EnvMapEditorLayer::OpenScene(const std::filesystem::path& path)
{
    if (m_SceneState != SceneState::Edit)
    {
        OnSceneStop();
    }

    if (path.extension().string() != ".hazel")
    {
        Log::GetLogger()->warn("Could not load {0} - not a scene file", path.filename().string());
        return;
    }

    H2M::RefH2M<H2M::SceneH2M> newScene = H2M::RefH2M<H2M::SceneH2M>::Create();
    H2M::SceneSerializerH2M serializer(newScene);
    if (serializer.Deserialize(path.string()))
    {
        m_EditorScene = newScene;
        m_EditorScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
        m_SceneHierarchyPanel->SetContext(m_EditorScene);

        m_ActiveScene = m_EditorScene;
        m_EditorScenePath = path;

        // Create MeshH2M objects for each deserialized MeshComponentH2M
        bool isAnimated = false;
        auto meshEntities = m_EditorScene->GetAllEntitiesWith<H2M::MeshComponentH2M>();
        for (auto entt : meshEntities)
        {
            H2M::EntityH2M entity{ entt, m_EditorScene.Raw() };
            auto& meshComponent = entity.GetComponent<H2M::MeshComponentH2M>();
            meshComponent.Mesh = H2M::RefH2M<H2M::MeshH2M>::Create(meshComponent.FilePath,
                EnvMapSharedData::s_ShaderHazelPBR, s_DefaultMaterial, isAnimated);
        }
    }
}

void EnvMapEditorLayer::SaveScene()
{
    if (!m_EditorScenePath.empty())
    {
        SerializeScene(m_ActiveScene, m_EditorScenePath);
    }
    else
    {
        SaveSceneAs();
    }
}

void EnvMapEditorLayer::SaveSceneAs()
{
    std::string filepath = H2M::FileDialogsH2M::SaveFile("Hazel Scene (*.hazel)\0*.hazel\0");
    if (!filepath.empty())
    {
        SerializeScene(m_ActiveScene, filepath);
        m_EditorScenePath = filepath;
    }
}

void EnvMapEditorLayer::SerializeScene(H2M::RefH2M<H2M::SceneH2M> scene, const std::filesystem::path& path)
{
    H2M::SceneSerializerH2M serializer(scene);
    serializer.Serialize(path.string());
}

void EnvMapEditorLayer::OnScenePlay()
{
#if 1 // Old -- without scene copy -- working

    m_SceneState = SceneState::Play;
    m_ActiveScene->OnRuntimeStart();

    m_SceneHierarchyPanel->SetContext(m_ActiveScene); // m_RuntimeScene

    EntitySelection::s_SelectionContext.clear();

    // m_ActiveScene = H2M::RefH2M<H2M::SceneH2M>::Create();

#else // New -- with scene copy -- not working

    m_SceneState = SceneState::Play;

    m_ActiveScene = H2M::SceneH2M::Copy(m_EditorScene);
    m_ActiveScene->OnRuntimeStart();

    m_SceneHierarchyPanel->SetContext(m_ActiveScene); // m_RuntimeScene
    EntitySelection::s_SelectionContext.clear();

#endif

    //  if (m_ReloadScriptOnPlay)
    //  {
    //      H2M::ScriptEngine::ReloadAssembly("assets/scripts/ExampleApp.dll");
    //  }
    //     
    //  m_RuntimeScene = H2M::RefH2M<H2M::SceneH2M>::Create();
    //  m_EditorScene->CopyTo(m_RuntimeScene);
    //  m_RuntimeScene->OnRuntimeStart();
}

void EnvMapEditorLayer::OnSceneStop()
{
    m_SceneState = SceneState::Edit;
    m_ActiveScene->OnRuntimeStop();

    m_ActiveScene = m_EditorScene;

    m_SceneHierarchyPanel->SetContext(m_ActiveScene); // m_EditorScene

    // EntitySelection::s_SelectionContext.clear();
    // m_RuntimeScene->OnRuntimeStop();
    // Unload runtime scene
    // H2M::ScriptEngine::SetSceneContext(m_EditorScene);
}

void EnvMapEditorLayer::OnDuplicateEntity()
{
    if (m_SceneState != SceneState::Edit) return;

    H2M::EntityH2M selectedEntity = m_SceneHierarchyPanel->GetSelectedEntity();
    if (selectedEntity)
    {
        m_EditorScene->DuplicateEntity(selectedEntity);
    }
}

void EnvMapEditorLayer::OnNewScene(glm::vec2 viewportSize)
{
    // m_SceneRenderer->s_Data.ActiveScene = new H2M::SceneH2M();
    m_EditorScene->OnViewportResize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
    m_SceneHierarchyPanel->SetContext(m_EditorScene);
}

void EnvMapEditorLayer::SelectEntity(H2M::EntityH2M e)
{
}

void EnvMapEditorLayer::SubmitMesh(H2M::MeshH2M* mesh, const glm::mat4& transform, Material* overrideMaterial)
{
    auto& materials = mesh->GetMaterials();
    for (H2M::RefH2M<H2M::SubmeshH2M> submesh : mesh->GetSubmeshes())
    {
        // Material
        auto material = materials[submesh->MaterialIndex];

        for (size_t i = 0; i < mesh->GetBoneTransforms().size(); i++)
        {
            std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
            EnvMapSharedData::s_ShaderHazelPBR->SetMat4(uniformName, mesh->GetBoneTransforms()[i]);
        }

        EnvMapSharedData::s_ShaderHazelPBR->SetMat4("u_Transform", transform * submesh->Transform);

        if (material->GetFlag(H2M::MaterialFlagH2M::DepthTest)) { // TODO: Fix Material flags
            RendererBasic::EnableDepthTest();
        }
        else {
            RendererBasic::DisableDepthTest();
        }

        RendererBasic::DrawIndexed(submesh->GetIndexCount(), 0, submesh->BaseVertex, (void*)(sizeof(uint32_t) * submesh->BaseIndex));
    }
}

void EnvMapEditorLayer::ResizeViewport()
{
    float currentTimestamp = Timer::Get()->GetCurrentTimestamp();

    // Cooldown
    if (currentTimestamp - m_ResizeViewport.lastTime < m_ResizeViewport.cooldown) return;
    m_ResizeViewport.lastTime = currentTimestamp;

    if (m_ViewportSize != m_ViewportSizePrevious && m_ViewportSize.x > 0 && m_ViewportSize.y > 0)
    {
        m_RenderFramebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
        m_PostProcessingFramebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
        // m_RenderFramebufferTempH2M->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

        m_ViewportSizePrevious = glm::vec2(m_ViewportSize.x, m_ViewportSize.y);
    }
}

void EnvMapEditorLayer::OnEvent(H2M::EventH2M& e)
{
    if (m_SceneState == SceneState::Edit)
    {
        if (m_ViewportHovered)
        {
            GetMainCameraComponent().Camera.OnEvent(e);
        }

        m_EditorScene->OnEvent(e);
    }
    else if (m_SceneState == SceneState::Play)
    {
        m_ActiveScene->OnEvent(e);
    }

    // m_ActiveCamera->OnEvent(e);

    H2M::EventDispatcherH2M dispatcher(e);
    dispatcher.Dispatch<H2M::KeyPressedEventH2M>(H2M_BIND_EVENT_FN(EnvMapEditorLayer::OnKeyPressedEvent));
    dispatcher.Dispatch<H2M::MouseButtonPressedEventH2M>(H2M_BIND_EVENT_FN(EnvMapEditorLayer::OnMouseButtonPressed));
    dispatcher.Dispatch<H2M::MouseScrolledEventH2M>(H2M_BIND_EVENT_FN(EnvMapEditorLayer::OnMouseScrolled));
}

bool EnvMapEditorLayer::OnKeyPressedEvent(H2M::KeyPressedEventH2M& e)
{
    bool control = Input::IsKeyPressed(KeyH2M::LeftControl) || Input::IsKeyPressed(KeyH2M::RightControl);
    bool shift = Input::IsKeyPressed(KeyH2M::LeftShift) || Input::IsKeyPressed(KeyH2M::RightShift);

    if (m_ViewportFocused)
    {
        switch (e.GetKeyCode())
        {
            case (int)KeyCodeH2M::Q:
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

            case (int)KeyCodeH2M::Delete:
                if (EntitySelection::s_SelectionContext.size())
                {
                    H2M::EntityH2M selectedEntity = EntitySelection::s_SelectionContext[0].Entity;
                    m_EditorScene->DestroyEntity(selectedEntity);
                    EntitySelection::s_SelectionContext.clear();
                    m_EditorScene->SetSelectedEntity({});
                    m_SceneHierarchyPanel->SetSelected({});
                }
                break;
        }
    }

    if (Input::IsKeyPressed(H2M_KEY_LEFT_CONTROL))
    {
        switch (e.GetKeyCode())
        {
            case (int)KeyCodeH2M::B:
                // Toggle bounding boxes
                m_UIShowBoundingBoxes = !m_UIShowBoundingBoxes;
                ShowBoundingBoxes(m_UIShowBoundingBoxes, m_UIShowBoundingBoxesOnTop);
                break;
            case (int)KeyCodeH2M::D:
                if (EntitySelection::s_SelectionContext.size())
                {
                    H2M::EntityH2M selectedEntity = EntitySelection::s_SelectionContext[0].Entity;
                    m_EditorScene->DuplicateEntity(selectedEntity);
                }
                break;
            case (int)KeyCodeH2M::G:
                // Toggle grid
                EnvMapSceneRenderer::GetOptions().ShowGrid = !EnvMapSceneRenderer::GetOptions().ShowGrid;
                break;

            //  case (int)KeyCodeH2M::D:
            //      OnDuplicateEntity();
            //      break;
            //  Already defined in SceneHazelEnvMap::OnKeyPressed
            //  case (int)KeyCodeH2M::O:
            //      OpenScene();
            //      break;
            //  Already defined in SceneHazelEnvMap::OnKeyPressed
            //  case (int)KeyCodeH2M::S:
            //      SaveScene();
            //      break;

                // Toggle ImGui windows
            case (int)KeyCodeH2M::H:
                // Left CTRL + H: Toggle Scene Hierarchy
                m_ShowWindowSceneHierarchy = !m_ShowWindowSceneHierarchy;
                break;
            case (int)KeyCodeH2M::T:
                // Left CTRL + T: Toggle Transform
                m_ShowWindowTransform = !m_ShowWindowTransform;
                break;
            case (int)KeyCodeH2M::Space:
                // Left CTRL + Space: Toggle Asset Manager
                m_ShowWindowAssetManager = !m_ShowWindowAssetManager;
                break;
            case (int)KeyCodeH2M::M:
                // Left CTRL + M: Toggle Material Editor
                m_ShowWindowMaterialEditor = !m_ShowWindowMaterialEditor;
                break;
            case (int)KeyCodeH2M::P:
                // Left CTRL + P: Toggle Post Processing Effects
                m_ShowWindowPostProcessing = !m_ShowWindowPostProcessing;
                break;
            case (int)KeyCodeH2M::V:
                // Left CTRL + H: Toggle Shader Manager
                m_ShowWindowShaderManager = !m_ShowWindowShaderManager;
                break;
            case (int)KeyCodeH2M::F:
                // Left CTRL + F: Toggle Framebuffers
                m_ShowWindowFramebuffers = !m_ShowWindowFramebuffers;
                break;
            case (int)KeyCodeH2M::X:
                // Left CTRL + X: Toggle Settings
                m_ShowWindowSettings = !m_ShowWindowSettings;
                break;
            //  case (int)KeyCode::Enter:
            //      // Left CTRL + Enter: Toggle Fullscreen
            //      m_FullscreenEnabled = !m_FullscreenEnabled;
            //      break;
        }

        //  Already defined in SceneHazelEnvMap::OnKeyPressed
        //  if (Input::IsKeyPressed(H2M_KEY_LEFT_SHIFT))
        //  {
        //      switch (e.GetKeyCode())
        //      {
        //          case (int)KeyCodeH2M::S:
        //              SaveSceneAs();
        //              break;
        //      }
        //  }
    }

    return false;
}

bool EnvMapEditorLayer::OnMouseButtonPressed(H2M::MouseButtonPressedEventH2M& e)
{
    auto [mx, my] = Input::GetMousePosition();
    if (e.GetMouseButton() == (int)MouseH2M::ButtonLeft && !ImGuizmo::IsUsing() && !ImGuizmo::IsOver() && !Input::IsKeyPressed(KeyH2M::LeftAlt))
    {
        auto [mouseX, mouseY] = GetMouseViewportSpace();

        if (mouseX > -1.0f && mouseX < 1.0f && mouseY > -1.0f && mouseY < 1.0f)
        {
            auto [origin, direction] = CastRay(mouseX, mouseY);

            EntitySelection::s_SelectionContext.clear();

            auto meshEntities = m_EditorScene->GetAllEntitiesWith<H2M::MeshComponentH2M>();
            for (auto e : meshEntities)
            {
                H2M::EntityH2M entity = { e, m_EditorScene.Raw() };
                auto mesh = entity.GetComponent<H2M::MeshComponentH2M>().Mesh;
                if (!mesh)
                {
                    continue;
                }

                std::vector<H2M::RefH2M<H2M::SubmeshH2M>>& submeshes = mesh->GetSubmeshes();
                float lastT = std::numeric_limits<float>::max(); // Distance between camera and intersection in CastRay
                // for (H2M::Submesh& submesh : submeshes)
                for (uint32_t i = 0; i < submeshes.size(); i++)
                {
                    H2M::RefH2M<H2M::SubmeshH2M> submesh = submeshes[i];
                    auto transform = entity.GetComponent<H2M::TransformComponentH2M>().GetTransform();
                    H2M::RayH2M ray = {
                        glm::inverse(transform * submesh->Transform) * glm::vec4(origin, 1.0f),
                        glm::inverse(glm::mat3(transform) * glm::mat3(submesh->Transform)) * direction
                    };

                    float t;
                    bool intersects = ray.IntersectsAABB(submesh->BoundingBox, t);
                    if (intersects)
                    {
                        const auto& triangleCache = ((H2M::MeshH2M*)mesh.Raw())->GetTriangleCache(i);
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
            if (EntitySelection::s_SelectionContext.size())
            {
                m_CurrentlySelectedTransform = EntitySelection::s_SelectionContext[0].Mesh->Transform;
                OnSelected(EntitySelection::s_SelectionContext[0]);
            }
            else {
                H2M::EntityH2M meshEntity = GetMeshEntity();
                if (meshEntity)
                {
                    m_CurrentlySelectedTransform = meshEntity.Transform().GetTransform();
                }
            }
        }
    }

    // BEGIN Mouse picking based on RED_INTEGER framebuffer attachment
    m_MouseCoordsInViewportFlipY = GetMouseCoordsInViewportFlipY();
    
    m_EntityID = -1;
    if (m_MouseCoordsInViewportFlipY.first > 0 && m_MouseCoordsInViewportFlipY.second > 0 &&
        m_MouseCoordsInViewportFlipY.first < m_ViewportSize.x && m_MouseCoordsInViewportFlipY.second < m_ViewportSize.y)
    {
        m_RenderFramebuffer->Bind(); // required
        m_EntityID = m_RenderFramebuffer->ReadPixel(1, m_MouseCoordsInViewportFlipY.first, m_MouseCoordsInViewportFlipY.second);
        m_RenderFramebuffer->Unbind();
    
        if (m_EntityID > -1 && m_EntityID < std::numeric_limits<short>::max())
        {
            H2M::EntityH2M entity = { (entt::entity)m_EntityID, m_EditorScene.Raw() };
            if (entity.IsValid())
            {
                SelectedSubmesh selectedSubmesh = SelectedSubmesh{ entity, H2M::RefH2M<H2M::SubmeshH2M>(), 0 };
                AddSubmeshToSelectionContext(selectedSubmesh);
                if (Scene::s_ImGuizmoType == -1)
                {
                    Scene::s_ImGuizmoType = ImGuizmo::OPERATION::TRANSLATE;
                }
            }
        }
    }
    // END Mouse picking based on RED_INTEGER framebuffer attachment

    return false;
}

/**
 * Camera Zoom in / Zoom out implemented for active camera on (EnvMap)EditorLayer level
 * Better way would be to implement the functionality on camera / camera controller level
 */
bool EnvMapEditorLayer::OnMouseScrolled(H2M::MouseScrolledEventH2M& e)
{
    float offsetX = e.GetXOffset();
    float offsetY = e.GetYOffset();

    if (abs(offsetY) < 0.1f || abs(offsetY) > 10.0f)
    {
        return false;
    }

    float velocity = m_CameraMoveSpeed * offsetY;

    m_ActiveCamera->SetPosition(m_ActiveCamera->GetPosition() + m_ActiveCamera->GetFront() * velocity);

    Log::GetLogger()->debug("OnMouseScrolled offsetX: {0} offsetY: {1}", offsetX, offsetY);

    return true;
}

void EnvMapEditorLayer::OnOverlayRender()
{
    //  glm::mat4 viewProj = m_EditorCamera->GetViewProjection();
    //  
    //  // H2M::Renderer2D_H2M::BeginScene(*m_EditorCamera);
    //  H2M::Renderer2D_H2M::BeginScene(viewProj, true);
    //  {
    //      auto view = m_ActiveScene->GetAllEntitiesWith<H2M::TransformComponentH2M, H2M::CircleCollider2DComponentH2M>();
    //  
    //      for (auto entity : view)
    //      {
    //          auto [tc, cc2d] = view.get<H2M::TransformComponentH2M, H2M::CircleCollider2DComponentH2M>(entity);
    //  
    //          glm::vec3 translation = tc.Translation + glm::vec3(cc2d.Offset, 0.001f);
    //          glm::vec3 scale = tc.Scale * glm::vec3(cc2d.Radius * 2.0f);
    //  
    //          glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation);
    //          transform = glm::scale(transform, scale);
    //  
    //          H2M::Renderer2D_H2M::DrawCircle(tc.GetTransform(), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 1.0f, 0.0f, (int)entity);
    //          // H2M::Renderer2D_H2M::DrawCircle(transform, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), /*thickness=*/0.05f, /*fade=*/0.005f, /*entityID=*/-1);
    //      }
    //  }
    //  H2M::Renderer2D_H2M::EndScene();
}

void EnvMapEditorLayer::OnSelected(const SelectedSubmesh& selectionContext)
{
    // TODO: move to SceneHazelEnvMap
    m_SceneHierarchyPanel->SetSelected(selectionContext.Entity);
    m_EditorScene->SetSelectedEntity(selectionContext.Entity);
}

void EnvMapEditorLayer::OnEntityDeleted(H2M::EntityH2M e)
{
    if (EntitySelection::s_SelectionContext.size())
    {
        if (EntitySelection::s_SelectionContext[0].Entity == e)
        {
            EntitySelection::s_SelectionContext.clear();
            m_EditorScene->SetSelectedEntity({});
        }
    }
}

std::pair<float, float> EnvMapEditorLayer::GetMouseViewportSpace()
{
    auto [mx, my] = ImGui::GetMousePos(); // Input::GetMousePosition();
    mx -= m_ViewportBounds[0].x;
    my -= m_ViewportBounds[0].y;
    // m_ViewportSize.x = m_ViewportBounds[1].x - m_ViewportBounds[0].x;
    // m_ViewportSize.y = m_ViewportBounds[1].y - m_ViewportBounds[0].y;

    return { (mx / m_ViewportSize.x) * 2.0f - 1.0f, ((my / m_ViewportSize.y) * 2.0f - 1.0f) * - 1.0f };
}

std::pair<int, int> EnvMapEditorLayer::GetMouseCoordsInViewport()
{
    auto [mouseX, mouseY] = GetMouseViewportSpace();
    int mouseCoordX = (mouseX * m_ViewportSize.x + m_ViewportSize.x) / 2;
    int mouseCoordY = (m_ViewportSize.y - mouseY * m_ViewportSize.y) / 2;
    mouseCoordX = mouseCoordX < 0 ? 0 : mouseCoordX;
    mouseCoordY = mouseCoordY < 0 ? 0 : mouseCoordY;
    mouseCoordX = mouseCoordX > m_ViewportSize.x ? m_ViewportSize.x : mouseCoordX;
    mouseCoordY = mouseCoordY > m_ViewportSize.y ? m_ViewportSize.y : mouseCoordY;

    return std::pair<int, int>(mouseCoordX, mouseCoordY);
}

std::pair<int, int> EnvMapEditorLayer::GetMouseCoordsInViewportFlipY()
{
    std::pair<int, int> mouseCoordsInViewport = GetMouseCoordsInViewport();
    int mouseCoordX = mouseCoordsInViewport.first;
    int mouseCoordY = m_ViewportSize.y - mouseCoordsInViewport.second;
    return std::pair<int, int>(mouseCoordX, mouseCoordY);
}

std::pair<glm::vec3, glm::vec3> EnvMapEditorLayer::CastRay(float mx, float my)
{
    glm::vec4 mouseClipPos = { mx, my, -1.0f, 1.0f };

    glm::mat4 projectionMatrix = m_ActiveCamera->GetProjectionMatrix();
    glm::mat4 viewMatrix = m_ActiveCamera->GetViewMatrix();

    auto inverseProj = glm::inverse(projectionMatrix);
    auto inverseView = glm::inverse(glm::mat3(viewMatrix));

    glm::vec4 ray = inverseProj * mouseClipPos;
    glm::vec3 rayPos = m_ActiveCamera->GetPosition();
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

    uint32_t width = EnvMapSharedData::s_ShadowMapDirLight->GetShadowWidth();
    uint32_t height = EnvMapSharedData::s_ShadowMapDirLight->GetShadowHeight();

    RendererBasic::SetViewportSize(width, height);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // TODO: create a graphics API-agnostic method in RendererBasic

    RendererBasic::DisableBlend();
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
    if (GetPointLightEntities().size())
    {
        RenderShadowOmniSingleLight(mainWindow, GetPointLightEntities().at(0), EnvMapSharedData::s_OmniShadowMapPointLight);
    }
    if (GetSpotLightEntities().size())
    {
        RenderShadowOmniSingleLight(mainWindow, GetSpotLightEntities().at(0), EnvMapSharedData::s_OmniShadowMapSpotLight);
    }
}

void EnvMapEditorLayer::OnRenderCascadedShadowMaps(Window* mainWindow)
{
    // Entry point for rendering meshes to cascaded shadow framebuffers
    // Triggers EnvMapSceneRenderer::ShadowMapPass(), a copy of H2M::SceneRendererH2M::ShadowMapPass()
    EnvMapSceneRenderer::ShadowMapPass();
}

void EnvMapEditorLayer::OnRenderWaterReflection(Window* mainWindow)
{
    // TODO: Render scene object to the water reflection framebuffer, color and depth attachments
}

void EnvMapEditorLayer::OnRenderWaterRefraction(Window* mainWindow)
{
    // TODO: Render scene object to the water refraction framebuffer, color and depth attachments
}

void EnvMapEditorLayer::RenderShadowOmniSingleLight(Window* mainWindow, H2M::EntityH2M lightEntity, H2M::RefH2M<OmniShadowMap> omniShadowMap)
{
    omniShadowMap->BindForWriting();

    uint32_t width = omniShadowMap->GetShadowWidth();
    uint32_t height = omniShadowMap->GetShadowHeight();

    RendererBasic::SetViewportSize(width, height);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    RendererBasic::DisableBlend();
    RendererBasic::EnableCulling();

    m_ShaderOmniShadow->Bind();

    glm::vec3 lightPosition = glm::vec3(0.0f);
    if (lightEntity.HasComponent<H2M::TransformComponentH2M>())
    {
        auto& tc = lightEntity.GetComponent<H2M::TransformComponentH2M>();
        lightPosition = tc.Translation;
    }

    float farPlane = 1000.0f;
    if (lightEntity.HasComponent<H2M::PointLightComponentH2M>())
    {
        auto& plc = lightEntity.GetComponent<H2M::PointLightComponentH2M>();
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

void EnvMapEditorLayer::RenderSubmeshesShadowPass(H2M::RefH2M<MoravaShader> shader)
{
    // Rendering all meshes (submeshes) on the scene to a shadow framebuffer
    auto meshEntities = m_EditorScene->GetAllEntitiesWith<H2M::MeshComponentH2M>();
    // Render all entities with mesh component
    if (meshEntities.size())
    {
        for (auto entt : meshEntities)
        {
            H2M::EntityH2M entity = { entt, m_EditorScene.Raw() };
            auto& meshComponent = entity.GetComponent<H2M::MeshComponentH2M>();

            glm::mat4 entityTransform = glm::mat4(1.0f);
            if (entity && entity.HasComponent<H2M::TransformComponentH2M>())
            {
                entityTransform = entity.GetComponent<H2M::TransformComponentH2M>().GetTransform();
            }

            if (meshComponent.Mesh && meshComponent.CastShadows)
            {
                for (H2M::RefH2M<H2M::SubmeshH2M> submesh : meshComponent.Mesh->GetSubmeshes())
                {
                    // Render Submesh
                    meshComponent.Mesh->GetVertexBuffer()->Bind();
                    meshComponent.Mesh->GetPipeline()->Bind();
                    meshComponent.Mesh->GetIndexBuffer()->Bind();

                    shader->SetMat4("model", entityTransform * submesh->Transform);

                    for (size_t i = 0; i < meshComponent.Mesh->GetBoneTransforms().size(); i++)
                    {
                        std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
                        shader->SetMat4(uniformName, meshComponent.Mesh->GetBoneTransforms()[i]);
                    }

                    shader->SetBool("u_Animated", meshComponent.Mesh->IsAnimated());

                    RendererBasic::DrawIndexed(submesh->IndexCount, 0, submesh->BaseVertex, (void*)(sizeof(uint32_t) * submesh->BaseIndex));
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

    // Clear entity ID attachment to -1
    m_RenderFramebuffer->ClearAttachment(1, -1);

    EnvMapSceneRenderer::GetGeoPass()->GetSpecification().TargetFramebuffer = m_RenderFramebuffer;

    OnRenderEditor();
    // OnRenderRuntime()

    // OnOverlayRender();

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

/****
void EnvMapEditorLayer::RenderFullscreen(Window* mainWindow)
{
    int windowWidth = Application::Get()->GetWindow()->GetWidth();
    int windowHeight = Application::Get()->GetWindow()->GetHeight();
    int monitorWidth = windowWidth;
    int monitorHeight = windowHeight;

    if (!m_FullscreenEnabled)
    {
        // glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        // glfwSetWindowMonitor(mainWindow->GetHandle(), NULL, 0, 0, windowWidth, windowHeight, GLFW_DONT_CARE);
        glViewport(0, 0, windowWidth, windowHeight);
        return;
    }

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    // glfwGetMonitorPhysicalSize(primaryMonitor, &monitorWidth, &monitorHeight);

    monitorWidth = 1920;
    monitorHeight = 1080;

    float aspectRatio = monitorWidth / monitorHeight;
    float degreesFOV = m_ActiveCamera->GetPerspectiveVerticalFOV();
    float nearPlane = m_ActiveCamera->GetPerspectiveNearClip();
    float farPlane = m_ActiveCamera->GetPerspectiveFarClip();
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(degreesFOV), aspectRatio, nearPlane, farPlane);
    RendererBasic::SetProjectionMatrix(projectionMatrix);

    glfwSetWindowMonitor(mainWindow->GetHandle(), primaryMonitor, 0, 0, monitorWidth, monitorHeight, GLFW_DONT_CARE);
    glViewport(0, 0, monitorWidth, monitorHeight);
    RendererBasic::Clear();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //  Log::GetLogger()->info("Fullscreen enabled: {0}, Monitor resolution: {1}x{2}, Window resolution: {3}x{4}",
    //      m_FullscreenEnabled, monitorWidth, monitorHeight, windowWidth, windowHeight);

    m_ShaderPostProcessing->Bind();
    {
        if (m_PostProcessingEnabled)
        {
            m_PostProcessingFramebuffer->GetTextureAttachmentColor()->Bind(1);
        }
        else {
            m_RenderFramebuffer->GetTextureAttachmentColor()->Bind(1);
        }

        m_ShaderPostProcessing->SetInt("u_AlbedoMap", 1);
        m_ShaderPostProcessing->SetInt("u_Effect", 0);

        glBindVertexArray(GeometryFactory::Quad::GetVAO());

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    m_ShaderPostProcessing->Unbind();
}
****/

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

std::vector<H2M::EntityH2M> EnvMapEditorLayer::GetDirectionalLightEntities()
{
    std::vector<H2M::EntityH2M> directionalLightEntities = std::vector<H2M::EntityH2M>();
    auto view = m_ActiveScene->GetRegistry().view<H2M::DirectionalLightComponentH2M>();
    for (auto entity : view)
    {
        directionalLightEntities.push_back(H2M::EntityH2M{ entity, m_ActiveScene.Raw() });
    }

    if (directionalLightEntities.size() == 0)
    {
        // Log::GetLogger()->warn("DirectionalLightEntity not found in scene registry!");
    }

    return directionalLightEntities;
}

std::vector<H2M::EntityH2M> EnvMapEditorLayer::GetPointLightEntities()
{
    std::vector<H2M::EntityH2M> pointLightEntities = std::vector<H2M::EntityH2M>();
    auto view = m_ActiveScene->GetRegistry().view<H2M::PointLightComponentH2M>();
    for (auto entity : view)
    {
        pointLightEntities.push_back(H2M::EntityH2M{ entity, m_ActiveScene.Raw() });
    }
    return pointLightEntities;
}

std::vector<H2M::EntityH2M> EnvMapEditorLayer::GetSpotLightEntities()
{
    std::vector<H2M::EntityH2M> spotLightEntities = std::vector<H2M::EntityH2M>();
    auto view = m_ActiveScene->GetRegistry().view<H2M::SpotLightComponentH2M>();
    for (auto entity : view)
    {
        spotLightEntities.push_back(H2M::EntityH2M{ entity, m_ActiveScene.Raw() });
    }
    return spotLightEntities;
}
