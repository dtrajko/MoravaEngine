#include "EnvironmentMap.h"

#include "Hazel/Renderer/RenderPass.h"
#include "Hazel/Renderer/HazelMaterial.h"
#include "Hazel/Renderer/VertexArray.h"
#include "Hazel/Renderer/Buffer.h"
#include "Hazel/Renderer/HazelRenderer.h"
#include "Hazel/Renderer/Renderer2D.h"
#include "Hazel/Scene/Components.h"
#include "Hazel/Scene/Entity.h"

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
    m_SceneRenderer->s_Data.SceneData.SceneCamera = scene->GetCamera();
    SetSkybox(m_SceneRenderer->s_Data.SceneData.SceneEnvironment.RadianceMap);

    Init();

    m_CheckerboardTexture = Hazel::HazelTexture2D::Create("Textures/Hazel/Checkerboard.tga");

    m_DisplayHazelGrid = true;

    m_DisplayBoundingBoxes = false;

    Scene::s_ImGuizmoTransform = nullptr;
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

    Ref<Hazel::HazelMesh> mesh = CreateRef<Hazel::HazelMesh>(fullPath, m_ShaderHazelPBR, nullptr, isAnimated);

    mesh->SetTimeMultiplier(1.0f);

    // m_MeshEntity: NoECS version
    Hazel::Entity meshEntity = CreateEntity(fileNameNoExt);
    meshEntity.AddComponent<Hazel::MeshComponent>(mesh);
    meshEntity.AddComponent<Hazel::ScriptComponent>("Example.Script");

    return meshEntity;
}

EnvMapMaterial* EnvironmentMap::CreateDefaultMaterial(const std::string& nodeName)
{
    EnvMapMaterial* envMapMaterial = new EnvMapMaterial();

    TextureInfo textureInfo;
    if (m_TextureInfo.contains(nodeName)) {
        textureInfo = m_TextureInfo.at(nodeName);
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
    m_SceneRenderer->GetShaderComposite()->setInt("u_Texture", 1);
    m_SceneRenderer->GetShaderComposite()->setFloat("u_Exposure", m_SceneRenderer->s_Data.SceneData.SceneCamera->GetExposure());
    /**** END Shaders/Hazel/SceneComposite ****/

    /**** BEGIN Shaders/Hazel/Skybox ****/
    m_SceneRenderer->GetShaderSkybox()->Bind();
    m_SceneRenderer->GetShaderSkybox()->setInt("u_Texture", 1);
    m_SceneRenderer->GetShaderSkybox()->setFloat("u_TextureLod", 0.0f);
    m_SceneRenderer->GetShaderSkybox()->setFloat("u_Exposure", m_SceneRenderer->s_Data.SceneData.SceneCamera->GetExposure() * m_SkyboxExposureFactor); // originally used in Shaders/Hazel/SceneComposite
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
    shaderHazelPBR->setFloat("u_Exposure", m_SceneRenderer->s_Data.SceneData.SceneCamera->GetExposure()); // originally used in Shaders/Hazel/SceneComposite

    glm::mat4 viewProjection = RendererBasic::GetProjectionMatrix() * ((Scene*)m_SceneRenderer->s_Data.ActiveScene)->GetCameraController()->CalculateViewMatrix();

    shaderHazelPBR->setMat4("u_ViewProjectionMatrix", viewProjection);
    shaderHazelPBR->setVec3("u_CameraPosition", m_SceneRenderer->s_Data.SceneData.SceneCamera->GetPosition());

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

void EnvironmentMap::SetSkybox(Hazel::HazelTextureCube* skybox)
{
    Log::GetLogger()->debug("EnvironmentMap::SetSkybox u_Texture: {0}", 1);
    m_SkyboxTexture = skybox;
    m_SkyboxTexture->Bind(1);
}

EnvironmentMap::~EnvironmentMap()
{
    for (auto material : m_EnvMapMaterials) {
        delete material.second;
    }

    m_EnvMapMaterials.clear();

    delete m_SceneRenderer;
}

Hazel::Entity EnvironmentMap::CreateEntity(const std::string& name)
{
    // Both NoECS and ECS
    Hazel::Entity entity = ((Hazel::HazelScene*)m_SceneRenderer->s_Data.ActiveScene)->CreateEntity(name, (const Hazel::HazelScene&)m_SceneRenderer->s_Data.ActiveScene);

    return entity;
}

void EnvironmentMap::Update(Scene* scene, float timestep)
{
    m_SceneRenderer->s_Data.ActiveScene = scene;

    m_SceneRenderer->BeginScene((Scene*)m_SceneRenderer->s_Data.ActiveScene);

    UpdateUniforms();
}

Hazel::Entity* EnvironmentMap::GetMeshEntity()
{
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

void EnvironmentMap::RenderHazelSkybox()
{
    RendererBasic::DisableCulling();
    RendererBasic::DisableDepthTest();

    // Hazel Skybox
    m_SceneRenderer->GetShaderSkybox()->Bind();

    glm::mat4 viewProjection = RendererBasic::GetProjectionMatrix() * ((Scene*)m_SceneRenderer->s_Data.ActiveScene)->GetCameraController()->CalculateViewMatrix();
    m_SceneRenderer->GetShaderSkybox()->setMat4("u_InverseVP", glm::inverse(viewProjection));
    m_SceneRenderer->s_Data.SceneData.SceneEnvironment.RadianceMap->Bind(1);
    Hazel::HazelRenderer::SubmitFullscreenQuad(nullptr);

    m_SceneRenderer->GetShaderSkybox()->Unbind();
}

void EnvironmentMap::OnEvent(Event& e)
{
    if (m_AllowViewportCameraEvents) {
        ((Scene*)m_SceneRenderer->s_Data.ActiveScene)->GetCamera()->OnEvent(e);
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

void EnvironmentMap::GeometryPassTemporary()
{
    glm::mat4 projectionMatrix = RendererBasic::GetProjectionMatrix();
    glm::mat4 viewMatrix = ((Scene*)m_SceneRenderer->s_Data.ActiveScene)->GetCameraController()->CalculateViewMatrix();
    glm::mat4 viewProj = projectionMatrix * viewMatrix;

    m_SceneRenderer->s_Data.SceneData.SceneEnvironment.RadianceMap->Bind(m_SamplerSlots->at("radiance"));
    m_SceneRenderer->s_Data.SceneData.SceneEnvironment.IrradianceMap->Bind(m_SamplerSlots->at("irradiance"));
    m_SceneRenderer->s_Data.BRDFLUT->Bind(m_SamplerSlots->at("BRDF_LUT"));

    uint32_t samplerSlot = m_SamplerSlots->at("albedo");

    RenderHazelSkybox();
}

void EnvironmentMap::CompositePassTemporary(Framebuffer* framebuffer)
{
    m_SceneRenderer->GetShaderComposite()->Bind();
    framebuffer->GetTextureAttachmentColor()->Bind(1);
    m_SceneRenderer->GetShaderComposite()->setInt("u_Texture", 1);
    m_SceneRenderer->GetShaderComposite()->setFloat("u_Exposure", m_SceneRenderer->s_Data.SceneData.SceneCamera->GetExposure());
    m_SceneRenderer->s_Data.GeoPass->GetSpecification().TargetFramebuffer->Bind();
    m_SceneRenderer->GetShaderComposite()->setInt("u_TextureSamples", m_SceneRenderer->s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetSpecification().Samples);
    Hazel::HazelRenderer::SubmitFullscreenQuad(nullptr);
}

void EnvironmentMap::Render(Framebuffer* framebuffer)
{
    GeometryPassTemporary();
    CompositePassTemporary(framebuffer);
}
