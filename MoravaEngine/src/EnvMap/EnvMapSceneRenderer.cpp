#include "EnvMapSceneRenderer.h"

#include "Hazel/Core/Assert.h"
#include "Hazel/Platform/OpenGL/OpenGLRenderPass.h"
#include "Hazel/Platform/OpenGL/OpenGLShader.h"
#include "Hazel/Platform/OpenGL/OpenGLTexture.h"
#include "Hazel/Platform/Vulkan/VulkanRenderer.h"
#include "Hazel/Renderer/HazelRenderer.h"
#include "Hazel/Renderer/HazelShader.h"
#include "Hazel/Scene/HazelScene.h"

#include "Core/Application.h"
#include "Core/Log.h"
#include "Core/ResourceManager.h"
#include "EnvMap/EnvMapEditorLayer.h"
#include "Material/MaterialLibrary.h"
#include "Renderer/RendererBasic.h"
#include "Shader/MoravaShaderLibrary.h"


Hazel::Ref<MoravaShader> EnvMapSceneRenderer::s_ShaderEquirectangularConversion;
Hazel::Ref<MoravaShader> EnvMapSceneRenderer::s_ShaderEnvFiltering;
Hazel::Ref<MoravaShader> EnvMapSceneRenderer::s_ShaderEnvIrradiance;
Hazel::Ref<MoravaShader> EnvMapSceneRenderer::s_ShaderGrid;
Hazel::Ref<MoravaShader> EnvMapSceneRenderer::s_ShaderSkybox;
// Hazel::Ref<Hazel::HazelTextureCube> EnvMapSceneRenderer::s_EnvUnfiltered;
Hazel::Ref<Hazel::HazelTexture2D> EnvMapSceneRenderer::s_EnvEquirect;
// Hazel::Ref<Hazel::HazelTextureCube> EnvMapSceneRenderer::s_EnvFiltered;
// Hazel::Ref<Hazel::HazelTextureCube> EnvMapSceneRenderer::s_IrradianceMap;
float EnvMapSceneRenderer::s_GridScale = 16.025f;
float EnvMapSceneRenderer::s_GridSize = 0.025f;
uint32_t EnvMapSceneRenderer::s_FramebufferWidth = 1280;
uint32_t EnvMapSceneRenderer::s_FramebufferHeight = 720;

// variables from SceneRenderer
Hazel::Ref<Hazel::RenderCommandBuffer> EnvMapSceneRenderer::s_CommandBuffer;

EnvMapSceneRenderer::UBRendererData EnvMapSceneRenderer::s_RendererDataUB;

float EnvMapSceneRenderer::s_CascadeSplitLambda = 0.92f;
float EnvMapSceneRenderer::s_CascadeFarPlaneOffset = 50.0f;
float EnvMapSceneRenderer::s_CascadeNearPlaneOffset = -50.0f;

Hazel::Ref<Hazel::Pipeline> EnvMapSceneRenderer::s_GeometryPipeline;
Hazel::Ref<Hazel::Pipeline> EnvMapSceneRenderer::s_SelectedGeometryPipeline;
Hazel::Ref<Hazel::Pipeline> EnvMapSceneRenderer::s_GeometryWireframePipeline;
Hazel::Ref<Hazel::Pipeline> EnvMapSceneRenderer::s_GeometryWireframeOnTopPipeline;
Hazel::Ref<Hazel::Pipeline> EnvMapSceneRenderer::s_PreDepthPipeline;
Hazel::Ref<Hazel::Pipeline> EnvMapSceneRenderer::s_CompositePipeline;
Hazel::Ref<Hazel::Pipeline> EnvMapSceneRenderer::s_ShadowPassPipelines[4];
Hazel::Ref<Hazel::HazelMaterial> EnvMapSceneRenderer::s_ShadowPassMaterial;
Hazel::Ref<Hazel::HazelMaterial> EnvMapSceneRenderer::s_PreDepthMaterial;
Hazel::Ref<Hazel::Pipeline> EnvMapSceneRenderer::s_SkyboxPipeline;
Hazel::Ref<Hazel::HazelMaterial> EnvMapSceneRenderer::s_SkyboxMaterial;
Hazel::Ref<Hazel::Pipeline> EnvMapSceneRenderer::s_DOFPipeline;
Hazel::Ref<Hazel::HazelMaterial> EnvMapSceneRenderer::s_DOFMaterial;

SceneRendererOptions EnvMapSceneRenderer::s_Options;

Hazel::Ref<Hazel::HazelTexture2D> EnvMapSceneRenderer::s_BloomComputeTextures[3];

bool EnvMapSceneRenderer::s_ResourcesCreated = false;

BloomSettings EnvMapSceneRenderer::s_BloomSettings;
Hazel::Ref<Hazel::HazelTexture2D> EnvMapSceneRenderer::s_BloomDirtTexture;

EnvMapSceneRenderer::GPUTimeQueries EnvMapSceneRenderer::s_GPUTimeQueries;

Hazel::Ref<Hazel::Renderer2D> EnvMapSceneRenderer::s_Renderer2D;


struct EnvMapSceneRendererData
{
    const Hazel::HazelScene* ActiveScene = nullptr;
    struct SceneInfo
    {
        Hazel::SceneRendererCamera SceneCamera;

        // Resources
        Ref<Hazel::HazelMaterial> HazelSkyboxMaterial;
        Material* SkyboxMaterial;
        Hazel::Environment SceneEnvironment;
        Hazel::HazelDirLight ActiveLight;
    } SceneData;

    Hazel::Ref<Hazel::HazelTexture2D> BRDFLUT;

    Hazel::Ref<MoravaShader> CompositeShader;

    Hazel::Ref<Hazel::RenderPass> GeoPass;
    Hazel::Ref<Hazel::RenderPass> CompositePass;
    Hazel::Ref<Hazel::RenderPass> ActiveRenderPass;

    struct DrawCommand
    {
        std::string Name;
        Hazel::Ref<Hazel::HazelMesh> MeshPtr;
        Material* MaterialPtr;
        glm::mat4 Transform;
    };
    std::vector<DrawCommand> DrawList;
    std::vector<DrawCommand> SelectedMeshDrawList;

    // Grid
    Material* GridMaterial;
    // Ref<HazelShader> HazelGridShader;
    // Ref<MoravaShader> GridShader;
    Ref<Hazel::HazelMaterial> OutlineMaterial;

    Hazel::SceneRendererOptions Options;

    // Renderer data
    Hazel::RenderCommandQueue* m_CommandQueue;
};

static EnvMapSceneRendererData s_Data;

void EnvMapSceneRenderer::Init(std::string filepath, Hazel::HazelScene* scene)
{
    Hazel::HazelRenderer::Init();

    SetupShaders();

    s_Data.SceneData.SceneEnvironment = Load(filepath);
    SetEnvironment(s_Data.SceneData.SceneEnvironment);

    // Set lights
    s_Data.SceneData.ActiveLight.Direction = { 0.0f, -1.0f, 0.0f };
    s_Data.SceneData.ActiveLight.Radiance = { 1.0f, 1.0f, 1.0f };
    s_Data.SceneData.ActiveLight.Multiplier = 1.0f;

    // Grid
    // s_Data.OutlineMaterial = Hazel::Ref<HazelMaterial>::Create(s_ShaderGrid);
    s_Data.GridMaterial = new Material(s_ShaderGrid);
    s_ShaderGrid->Bind();
    s_ShaderGrid->SetFloat("u_Scale", s_GridScale);
    s_ShaderGrid->SetFloat("u_Res", s_GridSize);

    s_Data.SceneData.SkyboxMaterial = new Material(s_ShaderSkybox);
    s_Data.SceneData.SkyboxMaterial->SetFlag(MaterialFlag::DepthTest, true); // false

    // s_Data.ActiveScene->m_ViewportWidth = Application::Get()->GetWindow()->GetWidth();
    // s_Data.ActiveScene->m_ViewportHeight = Application::Get()->GetWindow()->GetHeight();

    bool isMultisample = false;

    FramebufferSpecification geoFramebufferSpec;
    geoFramebufferSpec.Width = s_FramebufferWidth;
    geoFramebufferSpec.Height = s_FramebufferHeight;
    geoFramebufferSpec.attachmentType = AttachmentType::Texture;
    geoFramebufferSpec.attachmentFormat = AttachmentFormat::RGBA16F;
    geoFramebufferSpec.Samples = 8;
    geoFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

    isMultisample = geoFramebufferSpec.Samples > 1;

    FramebufferSpecification geoFramebufferDepthSpec;
    geoFramebufferDepthSpec = geoFramebufferSpec;
    geoFramebufferDepthSpec.attachmentType = AttachmentType::Texture;
    geoFramebufferDepthSpec.attachmentFormat = AttachmentFormat::Depth_24_Stencil_8;

    Hazel::RenderPassSpecification geoRenderPassSpec;
    geoRenderPassSpec.TargetFramebuffer = MoravaFramebuffer::Create(geoFramebufferSpec);

    auto targetFramebufferGeo = static_cast<Hazel::Ref<MoravaFramebuffer>>(geoRenderPassSpec.TargetFramebuffer);

    targetFramebufferGeo->AddColorAttachment(geoFramebufferSpec);
    targetFramebufferGeo->AddDepthAttachment(geoFramebufferDepthSpec);
    Log::GetLogger()->debug("Generating the GEO RenderPass framebuffer with AttachmentFormat::RGBA16F");

    targetFramebufferGeo->Generate(geoFramebufferSpec.Width, geoFramebufferSpec.Height);

    s_Data.GeoPass = Hazel::RenderPass::Create(geoRenderPassSpec);

    FramebufferSpecification compFramebufferSpec;
    compFramebufferSpec.Width = s_FramebufferWidth;
    compFramebufferSpec.Height = s_FramebufferHeight;
    compFramebufferSpec.attachmentType = AttachmentType::Texture;
    compFramebufferSpec.attachmentFormat = AttachmentFormat::RGBA;
    compFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };

    isMultisample = compFramebufferSpec.Samples > 1;

    Hazel::RenderPassSpecification compRenderPassSpec;
    compRenderPassSpec.TargetFramebuffer = MoravaFramebuffer::Create(compFramebufferSpec);

    auto targetFramebufferComp = static_cast<Hazel::Ref<MoravaFramebuffer>>(compRenderPassSpec.TargetFramebuffer);

    targetFramebufferComp->AddColorAttachment(compFramebufferSpec);

    FramebufferSpecification compFramebufferDepthSpec;
    compFramebufferDepthSpec = compFramebufferSpec;
    compFramebufferDepthSpec.attachmentType = AttachmentType::Renderbuffer;
    compFramebufferDepthSpec.attachmentFormat = AttachmentFormat::Depth;
    targetFramebufferComp->AddDepthAttachment(compFramebufferDepthSpec);

    Log::GetLogger()->debug("Generating the COMPOSITE RenderPass framebuffer with AttachmentFormat::RGBA");
    targetFramebufferComp->Generate(compFramebufferSpec.Width, compFramebufferSpec.Height);
    s_Data.CompositePass = Hazel::RenderPass::Create(compRenderPassSpec);

    s_Data.BRDFLUT = Hazel::HazelTexture2D::Create("Textures/Hazel/BRDF_LUT.tga");

    s_Renderer2D = Hazel::Ref<Hazel::Renderer2D>::Create();
}

void EnvMapSceneRenderer::SetupShaders()
{
    MoravaShaderSpecification moravaShaderSpecification;
    moravaShaderSpecification.ShaderType = MoravaShaderSpecification::ShaderType::MoravaShader;
    moravaShaderSpecification.VertexShaderPath = "Shaders/Hazel/SceneComposite.vs";
    moravaShaderSpecification.FragmentShaderPath = "Shaders/Hazel/SceneComposite.fs";
    moravaShaderSpecification.ForceCompile = false;
    s_Data.CompositeShader = MoravaShader::Create(moravaShaderSpecification);
    Log::GetLogger()->info("EnvMapSceneRenderer: s_ShaderComposite compiled [programID={0}]", s_Data.CompositeShader->GetProgramID());

    s_ShaderEquirectangularConversion = MoravaShader::Create("Shaders/Hazel/EquirectangularToCubeMap.cs");
    Log::GetLogger()->info("EnvMapSceneRenderer: s_ShaderEquirectangularConversion compiled [programID={0}]", s_ShaderEquirectangularConversion->GetProgramID());

    s_ShaderEnvFiltering = MoravaShader::Create("Shaders/Hazel/EnvironmentMipFilter.cs");
    Log::GetLogger()->info("EnvMapSceneRenderer: s_ShaderEnvFiltering compiled [programID={0}]", s_ShaderEnvFiltering->GetProgramID());

    s_ShaderEnvIrradiance = MoravaShader::Create("Shaders/Hazel/EnvironmentIrradiance.cs");
    Log::GetLogger()->info("EnvMapSceneRenderer: s_ShaderEnvIrradiance compiled [programID={0}]", s_ShaderEnvIrradiance->GetProgramID());

    s_ShaderSkybox = MoravaShader::Create("Shaders/Hazel/Skybox.vs", "Shaders/Hazel/Skybox.fs");
    Log::GetLogger()->info("EnvMapSceneRenderer: s_ShaderSkybox compiled [programID={0}]", s_ShaderSkybox->GetProgramID());

    s_ShaderGrid = MoravaShader::Create("Shaders/Hazel/Grid.vs", "Shaders/Hazel/Grid.fs");
    Log::GetLogger()->info("EnvMapSceneRenderer: s_ShaderGrid compiled [programID={0}]", s_ShaderGrid->GetProgramID());

    ResourceManager::AddShader("Hazel/SceneComposite", s_Data.CompositeShader);
    ResourceManager::AddShader("Hazel/EquirectangularToCubeMap", s_ShaderEquirectangularConversion.Raw());
    ResourceManager::AddShader("Hazel/EnvironmentMipFilter", s_ShaderEnvFiltering.Raw());
    ResourceManager::AddShader("Hazel/EnvironmentIrradiance", s_ShaderEnvIrradiance.Raw());
    ResourceManager::AddShader("Hazel/Skybox", s_ShaderSkybox.Raw());
    ResourceManager::AddShader("Hazel/Grid", s_ShaderGrid.Raw());
}

// Moved from EnvironmentMap
Hazel::Environment EnvMapSceneRenderer::Load(const std::string& filepath)
{
    auto [radiance, irradiance] = CreateEnvironmentMap(filepath);
    // auto [radiance, irradiance] = Hazel::HazelRenderer::CreateEnvironmentMap(filepath);
    return { radiance, irradiance };
}

void EnvMapSceneRenderer::SetViewportSize(uint32_t width, uint32_t height)
{
    s_Data.GeoPass->GetSpecification().TargetFramebuffer->Resize(width, height, true);
    s_Data.CompositePass->GetSpecification().TargetFramebuffer->Resize(width, height, true);
}

void EnvMapSceneRenderer::BeginScene(Hazel::HazelScene* scene, const Hazel::SceneRendererCamera& camera)
{
    // HZ_CORE_ASSERT(!s_Data.ActiveScene, "");

    s_Data.ActiveScene = scene;

    s_Data.SceneData.SceneCamera = camera;
}

void EnvMapSceneRenderer::EndScene()
{
    HZ_CORE_ASSERT(s_Data.ActiveScene, "");

    s_Data.ActiveScene = nullptr;

    FlushDrawList();
}

void EnvMapSceneRenderer::SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform, Ref<Hazel::HazelMaterial> overrideMaterial)
{
}

void EnvMapSceneRenderer::SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform)
{
}

void EnvMapSceneRenderer::SubmitEntity(Hazel::Entity entity)
{
    // TODO: Culling, sorting, etc.

    auto& mesh = entity.GetComponent<Hazel::MeshComponent>().Mesh;
    if (!mesh) {
        return;
    }

    // TODO: s_Data.DrawList.push_back({ mesh, entity->GetMaterial(), entity->GetTransform() });
}

Hazel::SceneRendererCamera& EnvMapSceneRenderer::GetCamera()
{
    return s_Data.SceneData.SceneCamera;
}

static Ref<Hazel::HazelShader> equirectangularConversionShader, envFilteringShader, envIrradianceShader;

// Moved from EnvironmentMap
void EnvMapSceneRenderer::SetEnvironment(Hazel::Environment environment)
{
    s_Data.SceneData.SceneEnvironment = environment;
}

/****
 * Moved from EnvironmentMap
 * This version doesn't give satisfying results, OpenGLRenderer::CreateEnvironmentMap() is probably better
 ****/
std::pair<Hazel::Ref<Hazel::HazelTextureCube>, Hazel::Ref<Hazel::HazelTextureCube>> EnvMapSceneRenderer::CreateEnvironmentMap(const std::string& filepath)
{
    Log::GetLogger()->debug("ComputeEnvironmentMaps: {0}", Hazel::HazelRenderer::GetConfig().ComputeEnvironmentMaps);

    if (!Hazel::HazelRenderer::GetConfig().ComputeEnvironmentMaps)
    {
        return { Hazel::HazelRenderer::GetBlackCubeTexture(), Hazel::HazelRenderer::GetBlackCubeTexture() };
    }

    const uint32_t cubemapSize = Hazel::HazelRenderer::GetConfig().EnvironmentMapResolution;
    const uint32_t irradianceMapSize = 32;

    Hazel::Ref<Hazel::OpenGLTextureCube> envUnfiltered = Hazel::HazelTextureCube::Create(Hazel::HazelImageFormat::RGBA16F, cubemapSize, cubemapSize).As<Hazel::OpenGLTextureCube>();
    // Ref<OpenGLShader> equirectangularConversionShader = HazelRenderer::GetShaderLibrary()->Get("EquirectangularToCubeMap").As<OpenGLShader>();
    Hazel::Ref<Hazel::OpenGLShader> equirectangularConversionShader = ResourceManager::GetShader("Hazel/EquirectangularToCubeMap").As<Hazel::OpenGLShader>();
    s_EnvEquirect = Hazel::HazelTexture2D::Create(filepath);

    // HZ_CORE_ASSERT(envEquirect->GetFormat() == ImageFormat::RGBA32F, "Texture is not HDR!");
    if (s_EnvEquirect->GetFormat() != Hazel::HazelImageFormat::RGBA32F)
    {
        Log::GetLogger()->error("Texture is not HDR!");
    }

    equirectangularConversionShader->Bind();
    s_EnvEquirect->Bind(1);
    // HazelRenderer::Submit([envUnfiltered, cubemapSize, envEquirect]() {});
    {
        glBindImageTexture(0, envUnfiltered->GetRendererID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        glDispatchCompute(cubemapSize / 32, cubemapSize / 32, 6);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glGenerateTextureMipmap(envUnfiltered->GetRendererID());
    }

    // Ref<OpenGLShader> envFilteringShader = HazelRenderer::GetShaderLibrary()->Get("EnvironmentMipFilter").As<OpenGLShader>();
    Hazel::Ref<Hazel::OpenGLShader> envFilteringShader = ResourceManager::GetShader("Hazel/EnvironmentMipFilter").As<Hazel::OpenGLShader>();

    // s_EnvFiltered = Hazel::HazelTextureCube::Create(Hazel::HazelImageFormat::RGBA16F, cubemapSize, cubemapSize, true);
    Hazel::Ref<Hazel::OpenGLTextureCube> envFiltered = Hazel::HazelTextureCube::Create(Hazel::HazelImageFormat::RGBA16F, cubemapSize, cubemapSize).As<Hazel::OpenGLTextureCube>();

    // HazelRenderer::Submit([envUnfiltered, envFiltered]() {});
    {
        glCopyImageSubData(envUnfiltered->GetRendererID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
            envFiltered->GetRendererID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
            envFiltered->GetWidth(), envFiltered->GetHeight(), 6);
    }

    envFilteringShader->Bind();
    envFiltered->Bind(1);

    // HazelRenderer::Submit([envFilteringShader, envUnfiltered, envFiltered, cubemapSize]() {});
    {
        const float deltaRoughness = 1.0f / glm::max((float)(envFiltered->GetMipLevelCount() - 1.0f), 1.0f);
        for (uint32_t level = 1, size = cubemapSize / 2; level < envFiltered->GetMipLevelCount(); level++, size /= 2) // <= ?
        {
            Log::GetLogger()->debug("BEGIN EnvFiltering size {0} level {1}/{2}", size, level, envFiltered->GetMipLevelCount());

            glBindImageTexture(0, envFiltered->GetRendererID(), level, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);

            const GLint roughnessUniformLocation = glGetUniformLocation(envFilteringShader->GetRendererID(), "u_Uniforms.Roughness");
            // HZ_CORE_ASSERT(roughnessUniformLocation != -1);
            glUniform1f(roughnessUniformLocation, (float)level * deltaRoughness);

            // glProgramUniform1f(envFilteringShader->GetRendererID(), roughnessUniformLocation, (float)(level * deltaRoughness));

            const GLuint numGroups = glm::max(1u, size / 32);
            glDispatchCompute(numGroups, numGroups, 6);
            glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            Log::GetLogger()->debug("END EnvFiltering size {0} numGroups {1} level {2}/{3}", size, numGroups, level, envFiltered->GetMipLevelCount());
        }
    }

    // Ref<OpenGLShader> envIrradianceShader = HazelRenderer::GetShaderLibrary()->Get("EnvironmentIrradiance").As<OpenGLShader>();
    Hazel::Ref<Hazel::OpenGLShader> envIrradianceShader = ResourceManager::GetShader("Hazel/EnvironmentIrradiance").As<Hazel::OpenGLShader>();

    // s_IrradianceMap = Hazel::HazelTextureCube::Create(Hazel::HazelImageFormat::RGBA16F, irradianceMapSize, irradianceMapSize, true);
    Hazel::Ref<Hazel::OpenGLTextureCube> irradianceMap = Hazel::HazelTextureCube::Create(Hazel::HazelImageFormat::RGBA16F, irradianceMapSize, irradianceMapSize).As<Hazel::OpenGLTextureCube>();

    envIrradianceShader->Bind();
    envFiltered->Bind(1);

    // HazelRenderer::Submit([irradianceMap, envIrradianceShader]() {});
    {
        glBindImageTexture(0, irradianceMap->GetID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);

        const GLint samplesUniformLocation = glGetUniformLocation(envIrradianceShader->GetRendererID(), "u_Uniforms.Samples");
        // HZ_CORE_ASSERT(samplesUniformLocation != -1);
        const uint32_t samples = Hazel::HazelRenderer::GetConfig().IrradianceMapComputeSamples;
        glUniform1ui(samplesUniformLocation, samples);

        glDispatchCompute(irradianceMap->GetWidth() / 32, irradianceMap->GetHeight() / 32, 6);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glGenerateTextureMipmap(irradianceMap->GetID());
    }

    return { envFiltered, irradianceMap };
}

void EnvMapSceneRenderer::RenderSkybox()
{
    RendererBasic::DisableCulling();
    RendererBasic::DisableDepthTest();

    // render skybox (render as last to prevent overdraw)
    EnvMapSceneRenderer::s_ShaderSkybox->Bind();

    EnvMapSceneRenderer::GetRadianceMap()->Bind(EnvMapSharedData::s_SamplerSlots.at("u_Texture"));

    glm::mat4 viewProj = GetViewProjection();
    EnvMapSceneRenderer::s_ShaderSkybox->SetMat4("u_InverseVP", glm::inverse(viewProj));

    s_ShaderSkybox->SetInt("u_Texture", EnvMapSharedData::s_SamplerSlots.at("u_Texture"));
    s_ShaderSkybox->SetFloat("u_TextureLod", EnvMapSharedData::s_EditorScene->GetSkyboxLod());
    s_ShaderSkybox->SetFloat("u_Exposure", EnvMapEditorLayer::GetMainCameraComponent().Camera.GetExposure() * EnvMapSharedData::s_SkyboxExposureFactor); // originally used in Shaders/Hazel/SceneComposite

    EnvMapSharedData::s_SkyboxCube->Render();

    EnvMapSceneRenderer::s_ShaderSkybox->Unbind();
}

void EnvMapSceneRenderer::RenderHazelGrid()
{
    // Grid
    // -- Shaders/Hazel/Grid.vs
    // ---- uniform mat4 u_ViewProjection;
    // ---- uniform mat4 u_Transform;
    // -- Shaders/Hazel/Grid.fs
    // ---- uniform float u_Scale;
    // ---- uniform float u_Res;

    s_ShaderGrid->Bind();
    s_ShaderGrid->SetFloat("u_Scale", EnvMapSceneRenderer::s_GridScale);
    s_ShaderGrid->SetFloat("u_Res", EnvMapSceneRenderer::s_GridSize);

    glm::mat4 viewProj = GetViewProjection();
    s_ShaderGrid->SetMat4("u_ViewProjection", viewProj);

    bool depthTest = true;

    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::scale(transform, glm::vec3(16.0f, 1.0f, 16.0f));
    transform = glm::rotate(transform, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    s_ShaderGrid->SetMat4("u_Transform", transform);

    EnvMapSharedData::s_Quad->Render();

    RendererBasic::EnableTransparency();
    RendererBasic::EnableMSAA();
}

void EnvMapSceneRenderer::RenderOutline(Hazel::Ref<MoravaShader> shader, Hazel::Entity entity, const glm::mat4& entityTransform, Hazel::Submesh& submesh)
{
    if (!EnvMapSharedData::s_DisplayOutline) return;

    auto& meshComponent = entity.GetComponent<Hazel::MeshComponent>();

    // Render outline
    if (EntitySelection::s_SelectionContext.size()) {
        for (auto selection : EntitySelection::s_SelectionContext)
        {
            if (selection.Mesh && &submesh == selection.Mesh) {
                submesh.RenderOutline(meshComponent.Mesh, shader, entityTransform, entity);
            }
        }
    }
}

void EnvMapSceneRenderer::UpdateShaderPBRUniforms(Hazel::Ref<MoravaShader> shaderHazelPBR, Hazel::Ref<EnvMapMaterial> envMapMaterial)
{
    /**** BEGIN Shaders/Hazel/HazelPBR_Anim / Shaders/Hazel/HazelPBR_Static ***/

    shaderHazelPBR->Bind();

    shaderHazelPBR->SetInt("u_AlbedoTexture", EnvMapSharedData::s_SamplerSlots.at("albedo"));
    shaderHazelPBR->SetInt("u_NormalTexture", EnvMapSharedData::s_SamplerSlots.at("normal"));
    shaderHazelPBR->SetInt("u_MetalnessTexture", EnvMapSharedData::s_SamplerSlots.at("metalness"));
    shaderHazelPBR->SetInt("u_RoughnessTexture", EnvMapSharedData::s_SamplerSlots.at("roughness"));
    shaderHazelPBR->SetInt("u_EmissiveTexture", EnvMapSharedData::s_SamplerSlots.at("emissive"));
    shaderHazelPBR->SetInt("u_AOTexture", EnvMapSharedData::s_SamplerSlots.at("ao"));

    shaderHazelPBR->SetFloat3("u_MaterialUniforms.AlbedoColor", envMapMaterial->GetAlbedoInput().Color);
    shaderHazelPBR->SetFloat("u_MaterialUniforms.Metalness", envMapMaterial->GetMetalnessInput().Value);
    shaderHazelPBR->SetFloat("u_MaterialUniforms.Roughness", envMapMaterial->GetRoughnessInput().Value);
    shaderHazelPBR->SetFloat("u_MaterialUniforms.Emissive", envMapMaterial->GetEmissiveInput().Value);
    shaderHazelPBR->SetFloat("u_MaterialUniforms.AO", envMapMaterial->GetAOInput().Value);

    shaderHazelPBR->SetFloat("u_MaterialUniforms.EnvMapRotation", EnvMapSharedData::s_EnvMapRotation);

    // shaderHazelPBR->SetFloat("u_MaterialUniforms.RadiancePrefilter", EnvMapSharedData::s_RadiancePrefilter ? 1.0f : 0.0f);
    shaderHazelPBR->SetFloat("u_MaterialUniforms.AlbedoTexToggle", envMapMaterial->GetAlbedoInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->SetFloat("u_MaterialUniforms.NormalTexToggle", envMapMaterial->GetNormalInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->SetFloat("u_MaterialUniforms.MetalnessTexToggle", envMapMaterial->GetMetalnessInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->SetFloat("u_MaterialUniforms.RoughnessTexToggle", envMapMaterial->GetRoughnessInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->SetFloat("u_MaterialUniforms.EmissiveTexToggle", envMapMaterial->GetEmissiveInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->SetFloat("u_MaterialUniforms.AOTexToggle", envMapMaterial->GetAOInput().UseTexture ? 1.0f : 0.0f);

    // apply exposure to Shaders/Hazel/HazelPBR_Anim, considering that Shaders/Hazel/SceneComposite is not yet enabled
    shaderHazelPBR->SetFloat("u_Exposure", EnvMapEditorLayer::GetMainCameraComponent().Camera.GetExposure()); // originally used in Shaders/Hazel/SceneComposite

    shaderHazelPBR->SetFloat("u_TilingFactor", envMapMaterial->GetTilingFactor());

    glm::mat4 viewProj = GetViewProjection();
    shaderHazelPBR->SetMat4("u_ViewProjectionMatrix", viewProj);
    shaderHazelPBR->SetFloat3("u_CameraPosition", EnvMapSharedData::s_ActiveCamera->GetPosition());
    shaderHazelPBR->SetMat4("u_DirLightTransform", EnvMapSharedData::s_DirLightTransform);

    // Environment (TODO: don't do this per mesh)
    shaderHazelPBR->SetInt("u_EnvRadianceTex", EnvMapSharedData::s_SamplerSlots.at("radiance"));
    shaderHazelPBR->SetInt("u_EnvIrradianceTex", EnvMapSharedData::s_SamplerSlots.at("irradiance"));
    shaderHazelPBR->SetInt("u_BRDFLUTTexture", EnvMapSharedData::s_SamplerSlots.at("BRDF_LUT"));

    // Set lights (TODO: move to light environment and don't do per mesh)
    shaderHazelPBR->SetFloat3("u_DirectionalLights.Direction", EnvMapSceneRenderer::GetActiveLight().Direction);
    shaderHazelPBR->SetFloat3("u_DirectionalLights.Radiance", EnvMapSceneRenderer::GetActiveLight().Radiance);
    shaderHazelPBR->SetFloat("u_DirectionalLights.Multiplier", EnvMapSceneRenderer::GetActiveLight().Multiplier);

    shaderHazelPBR->SetInt("pointLightCount", 1);
    shaderHazelPBR->SetInt("spotLightCount", 1);

    // Point lights / Omni directional shadows
    if (EnvMapSharedData::s_PointLightEntity.HasComponent<Hazel::PointLightComponent>())
    {
        auto& plc = EnvMapSharedData::s_PointLightEntity.GetComponent<Hazel::PointLightComponent>();
        auto& tc = EnvMapSharedData::s_PointLightEntity.GetComponent<Hazel::TransformComponent>();
        shaderHazelPBR->SetBool("pointLights[0].base.enabled", plc.Enabled);
        shaderHazelPBR->SetFloat3("pointLights[0].base.color", plc.Color);
        shaderHazelPBR->SetFloat("pointLights[0].base.ambientIntensity", plc.AmbientIntensity);
        shaderHazelPBR->SetFloat("pointLights[0].base.diffuseIntensity", plc.DiffuseIntensity);
        shaderHazelPBR->SetFloat3("pointLights[0].position", tc.Translation);
        shaderHazelPBR->SetFloat("pointLights[0].constant", plc.Constant);
        shaderHazelPBR->SetFloat("pointLights[0].linear", plc.Linear);
        shaderHazelPBR->SetFloat("pointLights[0].exponent", plc.Exponent);
    }

    // Spot lights / Omni directional shadows
    if (EnvMapSharedData::s_SpotLightEntity.HasComponent<Hazel::SpotLightComponent>())
    {
        auto& slc = EnvMapSharedData::s_SpotLightEntity.GetComponent<Hazel::SpotLightComponent>();
        auto& tc = EnvMapSharedData::s_SpotLightEntity.GetComponent<Hazel::TransformComponent>();
        shaderHazelPBR->SetBool("spotLights[0].base.base.enabled", slc.Enabled);
        shaderHazelPBR->SetFloat3("spotLights[0].base.base.color", slc.Color);
        shaderHazelPBR->SetFloat("spotLights[0].base.base.ambientIntensity", slc.AmbientIntensity);
        shaderHazelPBR->SetFloat("spotLights[0].base.base.diffuseIntensity", slc.DiffuseIntensity);
        shaderHazelPBR->SetFloat3("spotLights[0].base.position", tc.Translation);
        shaderHazelPBR->SetFloat("spotLights[0].base.constant", slc.Constant);
        shaderHazelPBR->SetFloat("spotLights[0].base.linear", slc.Linear);
        shaderHazelPBR->SetFloat("spotLights[0].base.exponent", slc.Exponent);
        shaderHazelPBR->SetFloat3("spotLights[0].direction", tc.Rotation);
        shaderHazelPBR->SetFloat("spotLights[0].edge", slc.Edge);
    }

    shaderHazelPBR->Validate();
    shaderHazelPBR->Unbind();

    /**** END Shaders/Hazel/HazelPBR_Anim / Shaders/Hazel/HazelPBR_Static ***/
}

glm::mat4 EnvMapSceneRenderer::GetViewProjection()
{
    glm::mat4 viewProjECS = EnvMapEditorLayer::GetMainCameraComponent().Camera.GetViewProjection();
    glm::mat4 viewProj = EnvMapSharedData::s_ActiveCamera->GetViewProjection();
    return viewProj;
}

void EnvMapSceneRenderer::OnImGuiRender()
{
    // HZ_PROFILE_FUNC();

    ImGui::Begin("Scene Renderer");

    if (ImGui::TreeNode("Shaders"))
    {
        auto& shaders = Hazel::HazelShader::s_AllShaders;
        for (auto& shader : shaders)
        {
            if (ImGui::TreeNode(shader->GetName().c_str()))
            {
                std::string buttonName = "Reload##" + shader->GetName();
                if (ImGui::Button(buttonName.c_str()))
                    shader->Reload(true);
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }

    if (Hazel::UI::BeginTreeNode("Visualization"))
    {
        Hazel::UI::BeginPropertyGrid();
        Hazel::UI::Property("Show Light Complexity", s_RendererDataUB.ShowLightComplexity);
        Hazel::UI::Property("Show Shadow Cascades", s_RendererDataUB.ShowCascades);
        static int maxDrawCall = 1000;
        Hazel::UI::PropertySlider("Selected Draw", Hazel::VulkanRenderer::GetSelectedDrawCall(), -1, maxDrawCall);
        Hazel::UI::Property("Max Draw Call", maxDrawCall);
        Hazel::UI::EndPropertyGrid();
        Hazel::UI::EndTreeNode();
    }

    if (Hazel::UI::BeginTreeNode("Render Statistics"))
    {
        // uint32_t frameIndex = Hazel::HazelRenderer::GetCurrentFrameIndex();
        // ImGui::Text("GPU time: %.3fms", s_CommandBuffer->GetExecutionGPUTime(frameIndex));

        // ImGui::Text("Shadow Map Pass: %.3fms", s_CommandBuffer->GetExecutionGPUTime(frameIndex, s_GPUTimeQueries.ShadowMapPassQuery));
        // ImGui::Text("Depth Pre-Pass: %.3fms", s_CommandBuffer->GetExecutionGPUTime(frameIndex, s_GPUTimeQueries.DepthPrePassQuery));
        // ImGui::Text("Light Culling Pass: %.3fms", s_CommandBuffer->GetExecutionGPUTime(frameIndex, s_GPUTimeQueries.LightCullingPassQuery));
        // ImGui::Text("Geometry Pass: %.3fms", s_CommandBuffer->GetExecutionGPUTime(frameIndex, s_GPUTimeQueries.GeometryPassQuery));
        // ImGui::Text("HBAO Pass: %.3fms", s_CommandBuffer->GetExecutionGPUTime(frameIndex, s_GPUTimeQueries.HBAOPassQuery));
        // ImGui::Text("Bloom Pass: %.3fms", s_CommandBuffer->GetExecutionGPUTime(frameIndex, s_GPUTimeQueries.BloomComputePassQuery));
        // ImGui::Text("Jump Flood Pass: %.3fms", s_CommandBuffer->GetExecutionGPUTime(frameIndex, s_GPUTimeQueries.JumpFloodPassQuery));
        // ImGui::Text("Composite Pass: %.3fms", s_CommandBuffer->GetExecutionGPUTime(frameIndex, s_GPUTimeQueries.CompositePassQuery));

        if (Hazel::UI::BeginTreeNode("Pipeline Statistics"))
        {
            // const Hazel::PipelineStatistics& pipelineStats = s_CommandBuffer->GetPipelineStatistics(frameIndex);
            // ImGui::Text("Input Assembly Vertices: %llu", pipelineStats.InputAssemblyVertices);
            // ImGui::Text("Input Assembly Primitives: %llu", pipelineStats.InputAssemblyPrimitives);
            // ImGui::Text("Vertex Shader Invocations: %llu", pipelineStats.VertexShaderInvocations);
            // ImGui::Text("Clipping Invocations: %llu", pipelineStats.ClippingInvocations);
            // ImGui::Text("Clipping Primitives: %llu", pipelineStats.ClippingPrimitives);
            // ImGui::Text("Fragment Shader Invocations: %llu", pipelineStats.FragmentShaderInvocations);
            // ImGui::Text("Compute Shader Invocations: %llu", pipelineStats.ComputeShaderInvocations);
            Hazel::UI::EndTreeNode();
        }

        Hazel::UI::EndTreeNode();
    }

    if (Hazel::UI::BeginTreeNode("Bloom Settings"))
    {
        Hazel::UI::BeginPropertyGrid();
        Hazel::UI::Property("Bloom Enabled", s_BloomSettings.Enabled);
        Hazel::UI::Property("Threshold", s_BloomSettings.Threshold);
        Hazel::UI::Property("Knee", s_BloomSettings.Knee);
        Hazel::UI::Property("Upsample Scale", s_BloomSettings.UpsampleScale);
        Hazel::UI::Property("Intensity", s_BloomSettings.Intensity, 0.05f, 0.0f, 20.0f);
        Hazel::UI::Property("Dirt Intensity", s_BloomSettings.DirtIntensity, 0.05f, 0.0f, 20.0f);

        // TODO(Yan): move this to somewhere else
        // Hazel::UI::Image(s_BloomDirtTexture, ImVec2(64, 64));
        if (ImGui::IsItemHovered())
        {
            if (ImGui::IsItemClicked())
            {
                std::string filename = Application::Get()->OpenFile("");
                if (!filename.empty())
                {
                    s_BloomDirtTexture = Hazel::HazelTexture2D::Create(filename);
                }
            }
        }

        Hazel::UI::EndPropertyGrid();
        Hazel::UI::EndTreeNode();
    }

    if (Hazel::UI::BeginTreeNode("Horizon-Based Ambient Occlusion"))
    {
        Hazel::UI::BeginPropertyGrid();
        Hazel::UI::Property("Enable", s_Options.EnableHBAO);
        Hazel::UI::Property("Intensity", s_Options.HBAOIntensity, 0.05f, 0.1f, 4.0f);
        Hazel::UI::Property("Radius", s_Options.HBAORadius, 0.05f, 0.0f, 4.0f);
        Hazel::UI::Property("Bias", s_Options.HBAOBias, 0.02f, 0.0f, 0.95f);
        Hazel::UI::Property("Blur Sharpness", s_Options.HBAOBlurSharpness, 0.5f, 0.0f, 100.f);
        Hazel::UI::EndPropertyGrid();

        float size = ImGui::GetContentRegionAvailWidth();
        if (s_ResourcesCreated)
        {
            float size = ImGui::GetContentRegionAvailWidth();
            auto image = s_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetImage(1);
            Hazel::UI::Image(image, { size, size * (1.0f / image->GetAspectRatio()) }, { 0, 1 }, { 1, 0 });
        }
        Hazel::UI::EndTreeNode();
    }

    if (Hazel::UI::BeginTreeNode("Shadows"))
    {
        Hazel::UI::BeginPropertyGrid();
        Hazel::UI::Property("Soft Shadows", s_RendererDataUB.SoftShadows);
        Hazel::UI::Property("DirLight Size", s_RendererDataUB.LightSize, 0.01f);
        Hazel::UI::Property("Max Shadow Distance", s_RendererDataUB.MaxShadowDistance, 1.0f);
        Hazel::UI::Property("Shadow Fade", s_RendererDataUB.ShadowFade, 5.0f);
        Hazel::UI::EndPropertyGrid();
        if (Hazel::UI::BeginTreeNode("Cascade Settings"))
        {
            Hazel::UI::BeginPropertyGrid();
            Hazel::UI::Property("Cascade Fading", s_RendererDataUB.CascadeFading);
            Hazel::UI::Property("Cascade Transition Fade", s_RendererDataUB.CascadeTransitionFade, 0.05f, 0.0f, FLT_MAX);
            Hazel::UI::Property("Cascade Split", s_CascadeSplitLambda, 0.01f);
            Hazel::UI::Property("CascadeNearPlaneOffset", s_CascadeNearPlaneOffset, 0.1f, -FLT_MAX, 0.0f);
            Hazel::UI::Property("CascadeFarPlaneOffset", s_CascadeFarPlaneOffset, 0.1f, 0.0f, FLT_MAX);
            Hazel::UI::EndPropertyGrid();
            Hazel::UI::EndTreeNode();
        }

        if (Hazel::UI::BeginTreeNode("Shadow Map", false))
        {
            static int cascadeIndex = 0;
            // auto fb = s_ShadowPassPipelines[cascadeIndex]->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer;
            // auto image = fb->GetDepthImage();

            float size = ImGui::GetContentRegionAvailWidth(); // (float)fb->GetWidth() * 0.5f, (float)fb->GetHeight() * 0.5f
            Hazel::UI::BeginPropertyGrid();
            Hazel::UI::PropertySlider("Cascade Index", cascadeIndex, 0, 3);
            Hazel::UI::EndPropertyGrid();
            if (s_ResourcesCreated)
            {
                // Hazel::UI::Image(image, (uint32_t)cascadeIndex, { size, size }, { 0, 1 }, { 1, 0 });
            }
            Hazel::UI::EndTreeNode();
        }

        Hazel::UI::EndTreeNode();
    }

    if (Hazel::UI::BeginTreeNode("Compute Bloom"))
    {
        float size = ImGui::GetContentRegionAvailWidth();
        if (s_ResourcesCreated)
        {
            static int tex = 0;
            Hazel::UI::PropertySlider("Texture", tex, 0, 2);
            static int mip = 0;
            auto [mipWidth, mipHeight] = s_BloomComputeTextures[tex]->GetMipSize(mip);
            std::string label = fmt::format("Mip ({0}x{1})", mipWidth, mipHeight);
            Hazel::UI::PropertySlider(label.c_str(), mip, 0, s_BloomComputeTextures[tex]->GetMipLevelCount() - 1);
            Hazel::UI::ImageMip(s_BloomComputeTextures[tex]->GetImage(), mip, { size, size * (1.0f / s_BloomComputeTextures[tex]->GetImage()->GetAspectRatio()) }, { 0, 1 }, { 1, 0 });
        }
        Hazel::UI::EndTreeNode();
    }

    ImGui::End();
}

void EnvMapSceneRenderer::GeometryPass()
{
    RendererBasic::EnableTransparency();
    RendererBasic::EnableMSAA();

    glm::mat4 viewProj = GetViewProjection();

    GetRadianceMap()->Bind(EnvMapSharedData::s_SamplerSlots.at("radiance"));
    GetIrradianceMap()->Bind(EnvMapSharedData::s_SamplerSlots.at("irradiance"));
    GetBRDFLUT()->Bind(EnvMapSharedData::s_SamplerSlots.at("BRDF_LUT"));

    uint32_t samplerSlot = EnvMapSharedData::s_SamplerSlots.at("albedo");

    RenderSkybox();

    if (EnvMapSharedData::s_DisplayHazelGrid) {
        RenderHazelGrid();
    }

    auto meshEntities = EnvMapSharedData::s_EditorScene->GetAllEntitiesWith<Hazel::MeshComponent>();
    // auto meshEntities = m_SceneHierarchyPanel->GetContext()->GetAllEntitiesWith<Hazel::MeshComponent>();

    // Render all entities with mesh component
    if (meshEntities.size())
    {
        for (auto entt : meshEntities)
        {
            Hazel::Entity entity = { entt, EnvMapSharedData::s_EditorScene.Raw() };
            auto& meshComponent = entity.GetComponent<Hazel::MeshComponent>();

            if (meshComponent.Mesh)
            {
                glm::mat4 entityTransform = glm::mat4(1.0f);
                if (entity && entity.HasComponent<Hazel::TransformComponent>()) {
                    entityTransform = entity.GetComponent<Hazel::TransformComponent>().GetTransform();
                }

                EnvMapSharedData::s_ShaderHazelPBR = meshComponent.Mesh->IsAnimated() ? MoravaShaderLibrary::Get("HazelPBR_Anim") : MoravaShaderLibrary::Get("HazelPBR_Static");

                EnvMapSharedData::s_ShaderHazelPBR->Bind();

                EnvMapSharedData::s_ShaderHazelPBR->SetBool("u_ShowCascades", true);

                EnvMapSharedData::s_ShadowMapDirLight->ReadTexture(EnvMapSharedData::s_SamplerSlots.at("shadow"));
                EnvMapSharedData::s_ShaderHazelPBR->SetInt("u_ShadowMap", EnvMapSharedData::s_SamplerSlots.at("shadow"));

                {
                    EnvMapSharedData::s_OmniShadowMapPointLight->ReadTexture(EnvMapSharedData::s_SamplerSlots.at("shadow_omni"));
                    EnvMapSharedData::s_ShaderHazelPBR->SetInt("u_OmniShadowMaps[0].shadowMap", EnvMapSharedData::s_SamplerSlots.at("shadow_omni"));

                    float farPlane = 1000.0f;
                    if (EnvMapSharedData::s_PointLightEntity.HasComponent<Hazel::PointLightComponent>()) {
                        farPlane = EnvMapSharedData::s_PointLightEntity.GetComponent<Hazel::PointLightComponent>().FarPlane;
                    }
                    EnvMapSharedData::s_ShaderHazelPBR->SetFloat("u_OmniShadowMaps[0].farPlane", farPlane);
                }

                {
                    EnvMapSharedData::s_OmniShadowMapSpotLight->ReadTexture(EnvMapSharedData::s_SamplerSlots.at("shadow_omni") + 1);
                    EnvMapSharedData::s_ShaderHazelPBR->SetInt("u_OmniShadowMaps[1].shadowMap", EnvMapSharedData::s_SamplerSlots.at("shadow_omni") + 1);

                    float farPlane = 1000.0f;
                    if (EnvMapSharedData::s_SpotLightEntity.HasComponent<Hazel::SpotLightComponent>()) {
                        farPlane = EnvMapSharedData::s_SpotLightEntity.GetComponent<Hazel::SpotLightComponent>().FarPlane;
                    }
                    EnvMapSharedData::s_ShaderHazelPBR->SetFloat("u_OmniShadowMaps[1].farPlane", farPlane);
                }

                Hazel::Ref<EnvMapMaterial> envMapMaterial = Hazel::Ref<EnvMapMaterial>();
                std::string materialUUID;

                for (Hazel::Submesh& submesh : meshComponent.Mesh->GetSubmeshes())
                {
                    materialUUID = MaterialLibrary::GetSubmeshMaterialUUID(meshComponent.Mesh.Raw(), submesh, &entity);

                    RenderOutline(EnvMapSharedData::s_ShaderOutline, entity, entityTransform, submesh);

                    // Render Submesh
                    // load submesh materials for each specific submesh from the s_EnvMapMaterials list
                    if (MaterialLibrary::s_EnvMapMaterials.find(materialUUID) != MaterialLibrary::s_EnvMapMaterials.end())
                    {
                        envMapMaterial = MaterialLibrary::s_EnvMapMaterials.at(materialUUID);
                        UpdateShaderPBRUniforms(EnvMapSharedData::s_ShaderHazelPBR, envMapMaterial);
                    }

                    bool wireframeEnabledScene = EnvMapSharedData::s_Scene->IsWireframeEnabled();
                    bool wireframeEnabledModel = EnvMapSharedData::s_DisplayWireframe;

                    // Log::GetLogger()->debug("wireframeEnabledScene: {0}, wireframeEnabledModel: {1}", wireframeEnabledScene, wireframeEnabledModel);

                    submesh.Render(meshComponent.Mesh, EnvMapSharedData::s_ShaderHazelPBR, entityTransform, samplerSlot,
                        MaterialLibrary::s_EnvMapMaterials, entity, wireframeEnabledScene, wireframeEnabledModel);
                }
            }
        }
    }

    s_Renderer2D->BeginScene(viewProj, glm::mat4(1.0f), true);
    {
        // RendererBasic::SetLineThickness(2.0f);

        if (EnvMapSharedData::s_DisplayRay)
        {
            glm::vec3 camPosition = EnvMapSharedData::s_ActiveCamera->GetPosition();
            s_Renderer2D->DrawLine(EnvMapSharedData::s_NewRay, EnvMapSharedData::s_NewRay + glm::vec3(1.0f, 0.0f, 0.0f) * 100.0f, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
        }

        if (EntitySelection::s_SelectionContext.size()) {
            for (auto selection : EntitySelection::s_SelectionContext)
            {
                if (selection.Mesh) {
                    Hazel::Entity meshEntity = selection.Entity;
                    glm::mat4 transform = glm::mat4(1.0f);
                    if (meshEntity.HasComponent<Hazel::TransformComponent>()) {
                        transform = meshEntity.GetComponent<Hazel::TransformComponent>().GetTransform();
                    }
                    glm::vec4 color = EnvMapEditorLayer::s_SelectionMode == SelectionMode::Entity ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) : glm::vec4(0.2f, 0.9f, 0.2f, 1.0f);
                    Hazel::HazelRenderer::DrawAABB(selection.Mesh->BoundingBox, transform * selection.Mesh->Transform, color);
                }
            }
        }
    }
    s_Renderer2D->EndScene();

    GetGeoPass()->GetSpecification().TargetFramebuffer->Bind();
}

void EnvMapSceneRenderer::CompositePass()
{
    // Hazel::HazelRenderer::BeginRenderPass(s_Data.CompositePass, false); // should we clear the framebuffer at this stage?

    s_Data.CompositeShader->Bind();

    auto targetFramebuffer = static_cast<Hazel::Ref<MoravaFramebuffer>>(s_Data.GeoPass->GetSpecification().TargetFramebuffer);

    targetFramebuffer->GetTextureAttachmentColor()->Bind(EnvMapSharedData::s_SamplerSlots.at("u_Texture"));
    s_Data.CompositeShader->SetInt("u_Texture", EnvMapSharedData::s_SamplerSlots.at("u_Texture"));

    // ERROR: OpenGLMoravaShader::SetFloat() failed [name='u_Exposure', location='-1']
    // s_Data.CompositeShader->SetFloat("u_Exposure", EnvMapEditorLayer::GetMainCameraComponent().Camera.GetExposure()); // s_Data.SceneData.SceneCamera.Camera

    s_Data.CompositeShader->SetInt("u_TextureSamples", s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetSpecification().Samples);
    s_Data.CompositeShader->Validate();

    // Hazel::HazelRenderer::SubmitFullscreenQuad(Hazel::Ref<Hazel::HazelMaterial>());
    // Hazel::HazelRenderer::EndRenderPass();
}

void EnvMapSceneRenderer::BloomBlurPass()
{
}

void EnvMapSceneRenderer::ShadowMapPass()
{
    /****
    auto& directionalLights = s_Data.SceneData.SceneLightEnvironment.DirectionalLights;
    if (directionalLights[0].Multiplier == 0.0f || !directionalLights[0].CastShadows)
    {
        for (int i = 0; i < 4; i++)
        {
            HazelRenderer::BeginRenderPass(s_Data.ShadowMapRenderPass[i]);
            HazelRenderer::EndRenderPass();
        }
        return;
    }

    CascadeData cascades[4];
    CalculateCascades(cascades, directionalLights[0].Direction);
    s_Data.LightViewMatrix = cascades[0].View;

    // HazelRenderer::Submit([](){});
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }

    for (int i = 0; i < 4; i++)
    {
        s_Data.CascadeSplits[i] = cascades[i].SplitDepth;

        HazelRenderer::BeginRenderPass(s_Data.ShadowMapRenderPass[i]);

        glm::mat4 shadowMapVP = cascades[i].ViewProj;

        static glm::mat4 scaleBiasMatrix = glm::scale(glm::mat4(1.0f), { 0.5f, 0.5f, 0.5f }) * glm::translate(glm::mat4(1.0f), { 1, 1, 1 });
        s_Data.LightMatrices[i] = scaleBiasMatrix * cascades[i].ViewProj;


        // Render entities
        for (auto& dc : s_Data.ShadowPassDrawList)
        {
            Ref<HazelShader> shader = dc.Mesh->IsAnimated() ? s_Data.ShadowMapAnimShader : s_Data.ShadowMapShader;
            shader->SetMat4("u_ViewProjection", shadowMapVP);
            HazelRenderer::SubmitMeshWithShader(dc.Mesh, dc.Transform, shader);
        }

        HazelRenderer::EndRenderPass();
    }
    ****/
}

void EnvMapSceneRenderer::SubmitEntityEnvMap(Hazel::Entity entity)
{
    auto mesh = entity.GetComponent<Hazel::MeshComponent>().Mesh;
    if (!mesh) {
        return;
    }

    auto transform = entity.GetComponent<Hazel::TransformComponent>().GetTransform();

    auto name = entity.GetComponent<Hazel::TagComponent>().Tag;
    AddToDrawList(name, mesh, entity, transform);
}

void EnvMapSceneRenderer::FlushDrawList()
{
    // HZ_CORE_ASSERT(!s_Data.ActiveScene, "");

    //  if (!s_Data.ActiveScene) {
    //      Log::GetLogger()->error("Active scene is not specified!");
    //  }

    GeometryPass();
    // Log::GetLogger()->debug("EnvironmentMap::FlushDrawList GeometryPass executed...");
    // CompositePass(framebuffer?);
    // Log::GetLogger()->debug("EnvironmentMap::FlushDrawList CompositePass executed...");

    // m_Data.DrawList.clear(); // TODO: make DrawList update every tick
    //  m_Data.SceneData = {};   // TODO: make SceneData update every tick
}

uint32_t EnvMapSceneRenderer::GetFinalColorBufferRendererID()
{
    auto targetFramebuffer = static_cast<Hazel::Ref<MoravaFramebuffer>>(s_Data.CompositePass->GetSpecification().TargetFramebuffer);

    return (uint32_t)targetFramebuffer->GetTextureAttachmentColor()->GetID();
}

Hazel::SceneRendererOptions& EnvMapSceneRenderer::GetOptions()
{
    return s_Data.Options;
}

Hazel::Ref<Hazel::HazelTextureCube> EnvMapSceneRenderer::GetRadianceMap()
{
    return s_Data.SceneData.SceneEnvironment.RadianceMap;
}

Hazel::Ref<Hazel::HazelTextureCube> EnvMapSceneRenderer::GetIrradianceMap()
{
    return s_Data.SceneData.SceneEnvironment.IrradianceMap;
}

Hazel::Ref<Hazel::HazelTexture2D> EnvMapSceneRenderer::GetBRDFLUT()
{
    return s_Data.BRDFLUT;
}

Hazel::Ref<MoravaShader> EnvMapSceneRenderer::GetShaderComposite()
{
    return s_Data.CompositeShader;
}

Hazel::Ref<Hazel::RenderPass> EnvMapSceneRenderer::GetGeoPass()
{
    return s_Data.GeoPass;
}

Hazel::Ref<Hazel::RenderPass> EnvMapSceneRenderer::GetCompositePass()
{
    return s_Data.CompositePass;
}

void EnvMapSceneRenderer::CreateDrawCommand(std::string fileNameNoExt, Hazel::Ref<Hazel::HazelMesh> mesh)
{
    // s_Data.DrawList.clear(); // doesn't work for multiple meshes on the scene
    EnvMapSceneRendererData::DrawCommand drawCommand;

    drawCommand.Name = fileNameNoExt;
    drawCommand.MeshPtr = mesh;
    drawCommand.Transform = glm::mat4(1.0f);

    s_Data.DrawList.push_back(drawCommand);
}

Hazel::HazelDirLight& EnvMapSceneRenderer::GetActiveLight()
{
    return s_Data.SceneData.ActiveLight;
}

void EnvMapSceneRenderer::SetActiveLight(Hazel::HazelDirLight& light)
{
    s_Data.SceneData.ActiveLight = light;
}

void EnvMapSceneRenderer::AddToDrawList(std::string name, Hazel::Ref<Hazel::HazelMesh> mesh, Hazel::Entity entity, glm::mat4 transform)
{
    s_Data.DrawList.push_back({ name, mesh.Raw(), entity.GetMaterial(), transform });
}

Hazel::Ref<Hazel::RenderPass> EnvMapSceneRenderer::GetFinalRenderPass()
{
    return s_Data.CompositePass;
}

FramebufferTexture* EnvMapSceneRenderer::GetFinalColorBuffer()
{
    auto targetFramebuffer = static_cast<Hazel::Ref<MoravaFramebuffer>>(s_Data.CompositePass->GetSpecification().TargetFramebuffer);

    return targetFramebuffer->GetTextureAttachmentColor();
}
