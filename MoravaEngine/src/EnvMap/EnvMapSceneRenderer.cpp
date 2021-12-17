#include "EnvMapSceneRenderer.h"

#include "H2M/Core/AssertH2M.h"
#include "H2M/ImGui/ImGuiH2M.h"
#include "H2M/Platform/OpenGL/OpenGLRenderPassH2M.h"
#include "H2M/Platform/OpenGL/OpenGLShaderH2M.h"
#include "H2M/Platform/OpenGL/OpenGLTextureH2M.h"
#include "H2M/Platform/Vulkan/VulkanRendererH2M.h"
#include "H2M/Renderer/ShaderH2M.h"
#include "H2M/Renderer/Renderer2D_H2M.h"
#include "H2M/Renderer/Renderer2D_LinesH2M.h"
#include "H2M/Renderer/MeshH2M.h"
#include "H2M/Renderer/RendererH2M.h"
#include "H2M/Scene/ComponentsH2M.h"
#include "H2M/Scene/SceneH2M.h"

#include "Core/Application.h"
#include "Core/Log.h"
#include "Core/ResourceManager.h"
#include "EnvMap/EnvMapEditorLayer.h"
#include "ImGui/ImGuiWrapper.h"
#include "Material/MaterialLibrary.h"
#include "Renderer/RendererBasic.h"
#include "Shader/MoravaShaderLibrary.h"


EnvMapEditorLayer* EnvMapSceneRenderer::s_EditorLayer;

H2M::RefH2M<MoravaShader> EnvMapSceneRenderer::s_ShaderEquirectangularConversion;
H2M::RefH2M<MoravaShader> EnvMapSceneRenderer::s_ShaderEnvFiltering;
H2M::RefH2M<MoravaShader> EnvMapSceneRenderer::s_ShaderEnvIrradiance;
H2M::RefH2M<MoravaShader> EnvMapSceneRenderer::s_ShaderGrid;
H2M::RefH2M<MoravaShader> EnvMapSceneRenderer::s_ShaderSkybox;
// H2M::RefH2M<H2M::TextureCubeH2M> EnvMapSceneRenderer::s_EnvUnfiltered;
H2M::RefH2M<H2M::Texture2D_H2M> EnvMapSceneRenderer::s_EnvEquirect;
// H2M::RefH2M<H2M::TextureCubeH2M> EnvMapSceneRenderer::s_EnvFiltered;
// H2M::RefH2M<H2M::TextureCubeH2M> EnvMapSceneRenderer::s_IrradianceMap;
float EnvMapSceneRenderer::s_GridScale = 16.025f;
float EnvMapSceneRenderer::s_GridSize = 0.025f;
uint32_t EnvMapSceneRenderer::s_FramebufferWidth = 1280;
uint32_t EnvMapSceneRenderer::s_FramebufferHeight = 720;

// variables from SceneRenderer
H2M::RefH2M<H2M::RenderCommandBufferH2M> EnvMapSceneRenderer::s_CommandBuffer;

EnvMapSceneRenderer::UBRendererData EnvMapSceneRenderer::s_RendererDataUB;

float EnvMapSceneRenderer::s_CascadeSplitLambda = 0.92f;
float EnvMapSceneRenderer::s_CascadeFarPlaneOffset = 50.0f;
float EnvMapSceneRenderer::s_CascadeNearPlaneOffset = -50.0f;

H2M::RefH2M<H2M::PipelineH2M> EnvMapSceneRenderer::s_GeometryPipeline;
H2M::RefH2M<H2M::PipelineH2M> EnvMapSceneRenderer::s_SelectedGeometryPipeline;
H2M::RefH2M<H2M::PipelineH2M> EnvMapSceneRenderer::s_GeometryWireframePipeline;
H2M::RefH2M<H2M::PipelineH2M> EnvMapSceneRenderer::s_GeometryWireframeOnTopPipeline;
H2M::RefH2M<H2M::PipelineH2M> EnvMapSceneRenderer::s_PreDepthPipeline;
H2M::RefH2M<H2M::PipelineH2M> EnvMapSceneRenderer::s_CompositePipeline;
H2M::RefH2M<H2M::PipelineH2M> EnvMapSceneRenderer::s_ShadowPassPipelines[4];
H2M::RefH2M<H2M::MaterialH2M> EnvMapSceneRenderer::s_ShadowPassMaterial;
H2M::RefH2M<H2M::MaterialH2M> EnvMapSceneRenderer::s_PreDepthMaterial;
H2M::RefH2M<H2M::PipelineH2M> EnvMapSceneRenderer::s_SkyboxPipeline;
H2M::RefH2M<H2M::MaterialH2M> EnvMapSceneRenderer::s_SkyboxMaterial;
H2M::RefH2M<H2M::PipelineH2M> EnvMapSceneRenderer::s_DOFPipeline;
H2M::RefH2M<H2M::MaterialH2M> EnvMapSceneRenderer::s_DOFMaterial;

SceneRendererOptions EnvMapSceneRenderer::s_Options;

H2M::RefH2M<H2M::Texture2D_H2M> EnvMapSceneRenderer::s_BloomComputeTextures[3];

bool EnvMapSceneRenderer::s_ResourcesCreated = false;

BloomSettings EnvMapSceneRenderer::s_BloomSettings;
H2M::RefH2M<H2M::Texture2D_H2M> EnvMapSceneRenderer::s_BloomDirtTexture;

EnvMapSceneRenderer::GPUTimeQueries EnvMapSceneRenderer::s_GPUTimeQueries;

H2M::RefH2M<H2M::Renderer2D_H2M> EnvMapSceneRenderer::s_Renderer2D;


struct EnvMapSceneRendererData
{
    const H2M::SceneH2M* ActiveScene = nullptr;
    struct SceneInfo
    {
        H2M::SceneRendererCameraH2M SceneCamera;

        // Resources
        RefH2M<H2M::MaterialH2M> HazelSkyboxMaterial;
        Material* SkyboxMaterial;
        H2M::EnvironmentH2M SceneEnvironment;
        H2M::LightH2M ActiveLight;
    } SceneData;

    H2M::RefH2M<H2M::Texture2D_H2M> BRDFLUT;

    H2M::RefH2M<MoravaShader> CompositeShader;

    H2M::RefH2M<H2M::RenderPassH2M> GeoPass;
    H2M::RefH2M<H2M::RenderPassH2M> CompositePass;
    H2M::RefH2M<H2M::RenderPassH2M> ActiveRenderPass;

    struct DrawCommand
    {
        std::string Name;
        H2M::RefH2M<H2M::MeshH2M> MeshPtr;
        Material* MaterialPtr;
        glm::mat4 Transform;
    };
    std::vector<DrawCommand> DrawList;
    std::vector<DrawCommand> SelectedMeshDrawList;

    // Grid
    Material* GridMaterial;
    // RefH2M<ShaderH2M> HazelGridShader;
    // RefH2M<MoravaShader> GridShader;
    RefH2M<H2M::MaterialH2M> OutlineMaterial;

    H2M::SceneRendererOptionsH2M Options;

    // Renderer data
    H2M::RenderCommandQueueH2M* m_CommandQueue;
};

static EnvMapSceneRendererData s_Data;

void EnvMapSceneRenderer::Init(std::string filepath, H2M::SceneH2M* scene, EnvMapEditorLayer* editorLayer)
{
    s_EditorLayer = editorLayer;

    s_Data.ActiveScene = scene;

    SetupShaders();

    s_Data.SceneData.SceneEnvironment = Load(filepath);
    SetEnvironment(s_Data.SceneData.SceneEnvironment);

    // Set lights
    s_Data.SceneData.ActiveLight.Direction = { 0.0f, -1.0f, 0.0f };
    s_Data.SceneData.ActiveLight.Radiance = { 1.0f, 1.0f, 1.0f };
    s_Data.SceneData.ActiveLight.Multiplier = 1.0f;

    // Grid
    // s_Data.OutlineMaterial = H2M::RefH2M<MaterialH2M>::Create(s_ShaderGrid);
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

    H2M::RenderPassSpecificationH2M geoRenderPassSpec;
    geoRenderPassSpec.TargetFramebuffer = MoravaFramebuffer::Create(geoFramebufferSpec);

    auto targetFramebufferGeo = static_cast<H2M::RefH2M<MoravaFramebuffer>>(geoRenderPassSpec.TargetFramebuffer);

    targetFramebufferGeo->AddColorAttachment(geoFramebufferSpec);
    targetFramebufferGeo->AddDepthAttachment(geoFramebufferDepthSpec);
    Log::GetLogger()->debug("Generating the GEO RenderPass framebuffer with AttachmentFormat::RGBA16F");

    targetFramebufferGeo->Generate(geoFramebufferSpec.Width, geoFramebufferSpec.Height);

    s_Data.GeoPass = H2M::RenderPassH2M::Create(geoRenderPassSpec);

    FramebufferSpecification compFramebufferSpec;
    compFramebufferSpec.Width = s_FramebufferWidth;
    compFramebufferSpec.Height = s_FramebufferHeight;
    compFramebufferSpec.attachmentType = AttachmentType::Texture;
    compFramebufferSpec.attachmentFormat = AttachmentFormat::RGBA;
    compFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };

    isMultisample = compFramebufferSpec.Samples > 1;

    H2M::RenderPassSpecificationH2M compRenderPassSpec;
    compRenderPassSpec.TargetFramebuffer = MoravaFramebuffer::Create(compFramebufferSpec);

    auto targetFramebufferComp = static_cast<H2M::RefH2M<MoravaFramebuffer>>(compRenderPassSpec.TargetFramebuffer);

    targetFramebufferComp->AddColorAttachment(compFramebufferSpec);

    FramebufferSpecification compFramebufferDepthSpec;
    compFramebufferDepthSpec = compFramebufferSpec;
    compFramebufferDepthSpec.attachmentType = AttachmentType::Renderbuffer;
    compFramebufferDepthSpec.attachmentFormat = AttachmentFormat::Depth;
    targetFramebufferComp->AddDepthAttachment(compFramebufferDepthSpec);

    Log::GetLogger()->debug("Generating the COMPOSITE RenderPass framebuffer with AttachmentFormat::RGBA");
    targetFramebufferComp->Generate(compFramebufferSpec.Width, compFramebufferSpec.Height);
    s_Data.CompositePass = H2M::RenderPassH2M::Create(compRenderPassSpec);

    s_Data.BRDFLUT = H2M::Texture2D_H2M::Create("Textures/Hazel/BRDF_LUT.tga");

    s_Renderer2D = H2M::RefH2M<H2M::Renderer2D_H2M>::Create();

    H2M::Renderer2D_LinesH2M::Init();
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
H2M::EnvironmentH2M EnvMapSceneRenderer::Load(const std::string& filepath)
{
    auto [radiance, irradiance] = CreateEnvironmentMap(filepath);
    // auto [radiance, irradiance] = H2M::RendererH2M::CreateEnvironmentMap(filepath);
    return { radiance, irradiance };
}

void EnvMapSceneRenderer::SetViewportSize(uint32_t width, uint32_t height)
{
    s_Data.GeoPass->GetSpecification().TargetFramebuffer->Resize(width, height, true);
    s_Data.CompositePass->GetSpecification().TargetFramebuffer->Resize(width, height, true);
}

void EnvMapSceneRenderer::BeginScene(H2M::SceneH2M* scene, const H2M::SceneRendererCameraH2M& camera)
{
    // H2M_CORE_ASSERT(!s_Data.ActiveScene, "");

    s_Data.ActiveScene = scene;

    s_Data.SceneData.SceneCamera = camera;
}

void EnvMapSceneRenderer::EndScene()
{
    H2M_CORE_ASSERT(s_Data.ActiveScene, "");

    s_Data.ActiveScene = nullptr;

    FlushDrawList();
}

void EnvMapSceneRenderer::SubmitMesh(RefH2M<Mesh> mesh, const glm::mat4& transform, RefH2M<H2M::MaterialH2M> overrideMaterial)
{
}

void EnvMapSceneRenderer::SubmitSelectedMesh(RefH2M<Mesh> mesh, const glm::mat4& transform)
{
}

void EnvMapSceneRenderer::SubmitEntity(H2M::EntityH2M entity)
{
    // TODO: Culling, sorting, etc.

    auto& mesh = entity.GetComponent<H2M::MeshComponentH2M>().Mesh;
    if (!mesh) {
        return;
    }

    // TODO: s_Data.DrawList.push_back({ mesh, entity->GetMaterial(), entity->GetTransform() });
}

H2M::SceneRendererCameraH2M& EnvMapSceneRenderer::GetCamera()
{
    return s_Data.SceneData.SceneCamera;
}

static RefH2M<H2M::ShaderH2M> equirectangularConversionShader, envFilteringShader, envIrradianceShader;

// Moved from EnvironmentMap
void EnvMapSceneRenderer::SetEnvironment(H2M::EnvironmentH2M environment)
{
    s_Data.SceneData.SceneEnvironment = environment;
}

/****
 * Moved from EnvironmentMap
 * This version doesn't give satisfying results, OpenGLRenderer::CreateEnvironmentMap() is probably better
 ****/
std::pair<H2M::RefH2M<H2M::TextureCubeH2M>, H2M::RefH2M<H2M::TextureCubeH2M>> EnvMapSceneRenderer::CreateEnvironmentMap(const std::string& filepath)
{
    Log::GetLogger()->debug("ComputeEnvironmentMaps: {0}", H2M::RendererH2M::GetConfig().ComputeEnvironmentMaps);

    if (!H2M::RendererH2M::GetConfig().ComputeEnvironmentMaps)
    {
        return { H2M::RendererH2M::GetBlackCubeTexture(), H2M::RendererH2M::GetBlackCubeTexture() };
    }

    const uint32_t cubemapSize = H2M::RendererH2M::GetConfig().EnvironmentMapResolution;
    const uint32_t irradianceMapSize = 32;

    H2M::RefH2M<H2M::OpenGLTextureCubeH2M> envUnfiltered = H2M::TextureCubeH2M::Create(H2M::ImageFormatH2M::RGBA16F, cubemapSize, cubemapSize).As<H2M::OpenGLTextureCubeH2M>();
    // RefH2M<OpenGLShaderH2M> equirectangularConversionShader = RendererH2M::GetShaderLibrary()->Get("EquirectangularToCubeMap").As<OpenGLShaderH2M>();
    H2M::RefH2M<H2M::OpenGLShaderH2M> equirectangularConversionShader = ResourceManager::GetShader("Hazel/EquirectangularToCubeMap").As<H2M::OpenGLShaderH2M>();
    s_EnvEquirect = H2M::Texture2D_H2M::Create(filepath);

    // H2M_CORE_ASSERT(envEquirect->GetFormat() == ImageFormat::RGBA32F, "Texture is not HDR!");
    if (s_EnvEquirect->GetFormat() != H2M::ImageFormatH2M::RGBA32F)
    {
        Log::GetLogger()->error("Texture is not HDR!");
    }

    equirectangularConversionShader->Bind();
    s_EnvEquirect->Bind(1);
    // RendererH2M::Submit([envUnfiltered, cubemapSize, envEquirect]() {});
    {
        glBindImageTexture(0, envUnfiltered->GetRendererID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        glDispatchCompute(cubemapSize / 32, cubemapSize / 32, 6);
        glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glGenerateTextureMipmap(envUnfiltered->GetRendererID());
    }

    // RefH2M<OpenGLShaderH2M> envFilteringShader = RendererH2M::GetShaderLibrary()->Get("EnvironmentMipFilter").As<OpenGLShaderH2M>();
    H2M::RefH2M<H2M::OpenGLShaderH2M> envFilteringShader = ResourceManager::GetShader("Hazel/EnvironmentMipFilter").As<H2M::OpenGLShaderH2M>();

    // s_EnvFiltered = H2M::TextureCubeH2M::Create(H2M::ImageFormatH2M::RGBA16F, cubemapSize, cubemapSize, true);
    H2M::RefH2M<H2M::OpenGLTextureCubeH2M> envFiltered = H2M::TextureCubeH2M::Create(H2M::ImageFormatH2M::RGBA16F, cubemapSize, cubemapSize).As<H2M::OpenGLTextureCubeH2M>();

    // RendererH2M::Submit([envUnfiltered, envFiltered]() {});
    {
        glCopyImageSubData(envUnfiltered->GetRendererID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
            envFiltered->GetRendererID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
            envFiltered->GetWidth(), envFiltered->GetHeight(), 6);
    }

    envFilteringShader->Bind();
    envFiltered->Bind(1);

    // RendererH2M::Submit([envFilteringShader, envUnfiltered, envFiltered, cubemapSize]() {});
    {
        const float deltaRoughness = 1.0f / glm::max((float)(envFiltered->GetMipLevelCount() - 1.0f), 1.0f);
        for (uint32_t level = 1, size = cubemapSize / 2; level < envFiltered->GetMipLevelCount(); level++, size /= 2) // <= ?
        {
            Log::GetLogger()->debug("BEGIN EnvFiltering size {0} level {1}/{2}", size, level, envFiltered->GetMipLevelCount());

            glBindImageTexture(0, envFiltered->GetRendererID(), level, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);

            const GLint roughnessUniformLocation = glGetUniformLocation(envFilteringShader->GetRendererID(), "u_Uniforms.Roughness");
            // H2M_CORE_ASSERT(roughnessUniformLocation != -1);
            glUniform1f(roughnessUniformLocation, (float)level * deltaRoughness);

            // glProgramUniform1f(envFilteringShader->GetRendererID(), roughnessUniformLocation, (float)(level * deltaRoughness));

            const GLuint numGroups = glm::max(1u, size / 32);
            glDispatchCompute(numGroups, numGroups, 6);
            glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            Log::GetLogger()->debug("END EnvFiltering size {0} numGroups {1} level {2}/{3}", size, numGroups, level, envFiltered->GetMipLevelCount());
        }
    }

    // RefH2M<OpenGLShaderH2M> envIrradianceShader = RendererH2M::GetShaderLibrary()->Get("EnvironmentIrradiance").As<OpenGLShaderH2M>();
    H2M::RefH2M<H2M::OpenGLShaderH2M> envIrradianceShader = ResourceManager::GetShader("Hazel/EnvironmentIrradiance").As<H2M::OpenGLShaderH2M>();

    // s_IrradianceMap = H2M::TextureCubeH2M::Create(H2M::ImageFormatH2M::RGBA16F, irradianceMapSize, irradianceMapSize, true);
    H2M::RefH2M<H2M::OpenGLTextureCubeH2M> irradianceMap = H2M::TextureCubeH2M::Create(H2M::ImageFormatH2M::RGBA16F, irradianceMapSize, irradianceMapSize).As<H2M::OpenGLTextureCubeH2M>();

    envIrradianceShader->Bind();
    envFiltered->Bind(1);

    // RendererH2M::Submit([irradianceMap, envIrradianceShader]() {});
    {
        glBindImageTexture(0, irradianceMap->GetID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);

        const GLint samplesUniformLocation = glGetUniformLocation(envIrradianceShader->GetRendererID(), "u_Uniforms.Samples");
        // H2M_CORE_ASSERT(samplesUniformLocation != -1);
        const uint32_t samples = H2M::RendererH2M::GetConfig().IrradianceMapComputeSamples;
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
    // Rotate skybox by 180 degrees
    viewProj = glm::rotate(viewProj, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    EnvMapSceneRenderer::s_ShaderSkybox->SetMat4("u_InverseVP", glm::inverse(viewProj));

    s_ShaderSkybox->SetInt("u_Texture", EnvMapSharedData::s_SamplerSlots.at("u_Texture"));
    s_ShaderSkybox->SetFloat("u_TextureLod", s_EditorLayer->GetActiveScene()->GetSkyboxLod());
    s_ShaderSkybox->SetFloat("u_Exposure", s_EditorLayer->GetMainCameraComponent().Camera.GetExposure() * EnvMapSharedData::s_SkyboxExposureFactor); // originally used in Shaders/Hazel/SceneComposite

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

void EnvMapSceneRenderer::RenderOutline(H2M::RefH2M<MoravaShader> shader, H2M::EntityH2M entity, const glm::mat4& entityTransform, H2M::RefH2M<H2M::SubmeshH2M> submesh)
{
    if (!EnvMapSharedData::s_DisplayOutline) return;

    auto& meshComponent = entity.GetComponent<H2M::MeshComponentH2M>();

    // Render outline
    if (EntitySelection::s_SelectionContext.size()) {
        for (auto selection : EntitySelection::s_SelectionContext)
        {
            if (selection.Mesh && submesh == selection.Mesh) {
                submesh->RenderOutline(meshComponent.Mesh, shader, entityTransform, entity);
            }
        }
    }
}

void EnvMapSceneRenderer::UpdateShaderPBRUniforms(H2M::RefH2M<MoravaShader> shaderHazelPBR, H2M::RefH2M<EnvMapMaterial> envMapMaterial)
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
    shaderHazelPBR->SetFloat("u_Exposure", s_EditorLayer->GetMainCameraComponent().Camera.GetExposure()); // originally used in Shaders/Hazel/SceneComposite

    shaderHazelPBR->SetFloat("u_TilingFactor", envMapMaterial->GetTilingFactor());

    glm::mat4 viewProj = GetViewProjection();
    shaderHazelPBR->SetMat4("u_ViewProjectionMatrix", viewProj);
    shaderHazelPBR->SetFloat3("u_CameraPosition", s_EditorLayer->GetActiveCamera()->GetPosition());
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
    if (EnvMapSharedData::s_PointLightEntity.HasComponent<H2M::PointLightComponentH2M>())
    {
        auto& plc = EnvMapSharedData::s_PointLightEntity.GetComponent<H2M::PointLightComponentH2M>();
        auto& tc = EnvMapSharedData::s_PointLightEntity.GetComponent<H2M::TransformComponentH2M>();
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
    if (EnvMapSharedData::s_SpotLightEntity.HasComponent<H2M::SpotLightComponentH2M>())
    {
        auto& slc = EnvMapSharedData::s_SpotLightEntity.GetComponent<H2M::SpotLightComponentH2M>();
        auto& tc = EnvMapSharedData::s_SpotLightEntity.GetComponent<H2M::TransformComponentH2M>();
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
    glm::mat4 viewProjECS = s_EditorLayer->GetActiveCamera()->GetViewProjection();
    glm::mat4 viewProj = s_EditorLayer->GetActiveCamera()->GetViewProjection();
    return viewProj;
}

void EnvMapSceneRenderer::OnImGuiRender()
{
    // HZ_PROFILE_FUNC();

    ImGui::Begin("Scene Renderer");

    if (ImGui::TreeNode("Shaders"))
    {
        auto& shaders = H2M::ShaderH2M::s_AllShaders;
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

    if (H2M::UI::BeginTreeNode("Visualization"))
    {
        H2M::UI::BeginPropertyGrid();
        H2M::UI::Property("Show Light Complexity", s_RendererDataUB.ShowLightComplexity);
        H2M::UI::Property("Show Shadow Cascades", s_RendererDataUB.ShowCascades);
        static int maxDrawCall = 1000;
        H2M::UI::PropertySlider("Selected Draw", H2M::VulkanRendererH2M::GetSelectedDrawCall(), -1, maxDrawCall);
        H2M::UI::Property("Max Draw Call", maxDrawCall);
        H2M::UI::EndPropertyGrid();
        H2M::UI::EndTreeNode();
    }

    if (H2M::UI::BeginTreeNode("Render Statistics"))
    {
        // uint32_t frameIndex = H2M::RendererH2M::GetCurrentFrameIndex();
        // ImGui::Text("GPU time: %.3fms", s_CommandBuffer->GetExecutionGPUTime(frameIndex));

        // ImGui::Text("Shadow Map Pass: %.3fms", s_CommandBuffer->GetExecutionGPUTime(frameIndex, s_GPUTimeQueries.ShadowMapPassQuery));
        // ImGui::Text("Depth Pre-Pass: %.3fms", s_CommandBuffer->GetExecutionGPUTime(frameIndex, s_GPUTimeQueries.DepthPrePassQuery));
        // ImGui::Text("Light Culling Pass: %.3fms", s_CommandBuffer->GetExecutionGPUTime(frameIndex, s_GPUTimeQueries.LightCullingPassQuery));
        // ImGui::Text("Geometry Pass: %.3fms", s_CommandBuffer->GetExecutionGPUTime(frameIndex, s_GPUTimeQueries.GeometryPassQuery));
        // ImGui::Text("HBAO Pass: %.3fms", s_CommandBuffer->GetExecutionGPUTime(frameIndex, s_GPUTimeQueries.HBAOPassQuery));
        // ImGui::Text("Bloom Pass: %.3fms", s_CommandBuffer->GetExecutionGPUTime(frameIndex, s_GPUTimeQueries.BloomComputePassQuery));
        // ImGui::Text("Jump Flood Pass: %.3fms", s_CommandBuffer->GetExecutionGPUTime(frameIndex, s_GPUTimeQueries.JumpFloodPassQuery));
        // ImGui::Text("Composite Pass: %.3fms", s_CommandBuffer->GetExecutionGPUTime(frameIndex, s_GPUTimeQueries.CompositePassQuery));

        if (H2M::UI::BeginTreeNode("Pipeline Statistics"))
        {
            // const H2M::PipelineStatistics& pipelineStats = s_CommandBuffer->GetPipelineStatistics(frameIndex);
            // ImGui::Text("Input Assembly Vertices: %llu", pipelineStats.InputAssemblyVertices);
            // ImGui::Text("Input Assembly Primitives: %llu", pipelineStats.InputAssemblyPrimitives);
            // ImGui::Text("Vertex Shader Invocations: %llu", pipelineStats.VertexShaderInvocations);
            // ImGui::Text("Clipping Invocations: %llu", pipelineStats.ClippingInvocations);
            // ImGui::Text("Clipping Primitives: %llu", pipelineStats.ClippingPrimitives);
            // ImGui::Text("Fragment Shader Invocations: %llu", pipelineStats.FragmentShaderInvocations);
            // ImGui::Text("Compute Shader Invocations: %llu", pipelineStats.ComputeShaderInvocations);
            H2M::UI::EndTreeNode();
        }

        H2M::UI::EndTreeNode();
    }

    if (H2M::UI::BeginTreeNode("Bloom Settings"))
    {
        H2M::UI::BeginPropertyGrid();
        H2M::UI::Property("Bloom Enabled", s_BloomSettings.Enabled);
        H2M::UI::Property("Threshold", s_BloomSettings.Threshold);
        H2M::UI::Property("Knee", s_BloomSettings.Knee);
        H2M::UI::Property("Upsample Scale", s_BloomSettings.UpsampleScale);
        H2M::UI::Property("Intensity", s_BloomSettings.Intensity, 0.05f, 0.0f, 20.0f);
        H2M::UI::Property("Dirt Intensity", s_BloomSettings.DirtIntensity, 0.05f, 0.0f, 20.0f);

        // TODO(Yan): move this to somewhere else
        // H2M::UI::Image(s_BloomDirtTexture, ImVec2(64, 64));
        if (ImGui::IsItemHovered())
        {
            if (ImGui::IsItemClicked())
            {
                std::string filename = Application::Get()->OpenFile("");
                if (!filename.empty())
                {
                    s_BloomDirtTexture = H2M::Texture2D_H2M::Create(filename);
                }
            }
        }

        H2M::UI::EndPropertyGrid();
        H2M::UI::EndTreeNode();
    }

    if (H2M::UI::BeginTreeNode("Horizon-Based Ambient Occlusion"))
    {
        H2M::UI::BeginPropertyGrid();
        H2M::UI::Property("Enable", s_Options.EnableHBAO);
        H2M::UI::Property("Intensity", s_Options.HBAOIntensity, 0.05f, 0.1f, 4.0f);
        H2M::UI::Property("Radius", s_Options.HBAORadius, 0.05f, 0.0f, 4.0f);
        H2M::UI::Property("Bias", s_Options.HBAOBias, 0.02f, 0.0f, 0.95f);
        H2M::UI::Property("Blur Sharpness", s_Options.HBAOBlurSharpness, 0.5f, 0.0f, 100.f);
        H2M::UI::EndPropertyGrid();

        float size = ImGui::GetContentRegionAvailWidth();
        if (s_ResourcesCreated)
        {
            float size = ImGui::GetContentRegionAvailWidth();
            auto image = s_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetImage(1);
            // H2M::UI::Image(image, { size, size * (1.0f / image->GetAspectRatio()) }, { 0, 1 }, { 1, 0 });
        }
        H2M::UI::EndTreeNode();
    }

    if (H2M::UI::BeginTreeNode("Shadows"))
    {
        H2M::UI::BeginPropertyGrid();
        H2M::UI::Property("Soft Shadows", s_RendererDataUB.SoftShadows);
        H2M::UI::Property("DirLight Size", s_RendererDataUB.LightSize, 0.01f);
        H2M::UI::Property("Max Shadow Distance", s_RendererDataUB.MaxShadowDistance, 1.0f);
        H2M::UI::Property("Shadow Fade", s_RendererDataUB.ShadowFade, 5.0f);
        H2M::UI::EndPropertyGrid();
        if (H2M::UI::BeginTreeNode("Cascade Settings"))
        {
            H2M::UI::BeginPropertyGrid();
            H2M::UI::Property("Cascade Fading", s_RendererDataUB.CascadeFading);
            H2M::UI::Property("Cascade Transition Fade", s_RendererDataUB.CascadeTransitionFade, 0.05f, 0.0f, FLT_MAX);
            H2M::UI::Property("Cascade Split", s_CascadeSplitLambda, 0.01f);
            H2M::UI::Property("CascadeNearPlaneOffset", s_CascadeNearPlaneOffset, 0.1f, -FLT_MAX, 0.0f);
            H2M::UI::Property("CascadeFarPlaneOffset", s_CascadeFarPlaneOffset, 0.1f, 0.0f, FLT_MAX);
            H2M::UI::EndPropertyGrid();
            H2M::UI::EndTreeNode();
        }

        if (H2M::UI::BeginTreeNode("Shadow Map", false))
        {
            static int cascadeIndex = 0;
            // auto fb = s_ShadowPassPipelines[cascadeIndex]->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer;
            // auto image = fb->GetDepthImage();

            float size = ImGui::GetContentRegionAvailWidth(); // (float)fb->GetWidth() * 0.5f, (float)fb->GetHeight() * 0.5f
            H2M::UI::BeginPropertyGrid();
            H2M::UI::PropertySlider("Cascade Index", cascadeIndex, 0, 3);
            H2M::UI::EndPropertyGrid();
            if (s_ResourcesCreated)
            {
                // H2M::UI::Image(image, (uint32_t)cascadeIndex, { size, size }, { 0, 1 }, { 1, 0 });
            }
            H2M::UI::EndTreeNode();
        }

        H2M::UI::EndTreeNode();
    }

    if (H2M::UI::BeginTreeNode("Compute Bloom"))
    {
        float size = ImGui::GetContentRegionAvailWidth();
        if (s_ResourcesCreated)
        {
            static int tex = 0;
            H2M::UI::PropertySlider("Texture", tex, 0, 2);
            static int mip = 0;
            auto [mipWidth, mipHeight] = s_BloomComputeTextures[tex]->GetMipSize(mip);
            std::string label = fmt::format("Mip ({0}x{1})", mipWidth, mipHeight);
            H2M::UI::PropertySlider(label.c_str(), mip, 0, s_BloomComputeTextures[tex]->GetMipLevelCount() - 1);
            // H2M::UI::ImageMip(s_BloomComputeTextures[tex]->GetImage(), mip, { size, size * (1.0f / s_BloomComputeTextures[tex]->GetImage()->GetAspectRatio()) }, { 0, 1 }, { 1, 0 });
        }
        H2M::UI::EndTreeNode();
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

    auto meshEntities = s_EditorLayer->GetActiveScene()->GetAllEntitiesWith<H2M::MeshComponentH2M>();
    // auto meshEntities = m_SceneHierarchyPanel->GetContext()->GetAllEntitiesWith<H2M::MeshComponentH2M>();

    // Render all entities with mesh component
    if (meshEntities.size())
    {
        for (auto entt : meshEntities)
        {
            H2M::EntityH2M entity = { entt, s_EditorLayer->GetActiveScene().Raw() };
            auto& meshComponent = entity.GetComponent<H2M::MeshComponentH2M>();

            if (meshComponent.Mesh)
            {
                glm::mat4 entityTransform = glm::mat4(1.0f);
                if (entity && entity.HasComponent<H2M::TransformComponentH2M>()) {
                    entityTransform = entity.GetComponent<H2M::TransformComponentH2M>().GetTransform();
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
                    if (EnvMapSharedData::s_PointLightEntity.HasComponent<H2M::PointLightComponentH2M>()) {
                        farPlane = EnvMapSharedData::s_PointLightEntity.GetComponent<H2M::PointLightComponentH2M>().FarPlane;
                    }
                    EnvMapSharedData::s_ShaderHazelPBR->SetFloat("u_OmniShadowMaps[0].farPlane", farPlane);
                }

                {
                    EnvMapSharedData::s_OmniShadowMapSpotLight->ReadTexture(EnvMapSharedData::s_SamplerSlots.at("shadow_omni") + 1);
                    EnvMapSharedData::s_ShaderHazelPBR->SetInt("u_OmniShadowMaps[1].shadowMap", EnvMapSharedData::s_SamplerSlots.at("shadow_omni") + 1);

                    float farPlane = 1000.0f;
                    if (EnvMapSharedData::s_SpotLightEntity.HasComponent<H2M::SpotLightComponentH2M>()) {
                        farPlane = EnvMapSharedData::s_SpotLightEntity.GetComponent<H2M::SpotLightComponentH2M>().FarPlane;
                    }
                    EnvMapSharedData::s_ShaderHazelPBR->SetFloat("u_OmniShadowMaps[1].farPlane", farPlane);
                }

                H2M::RefH2M<EnvMapMaterial> envMapMaterial = H2M::RefH2M<EnvMapMaterial>();
                std::string materialUUID;

                for (H2M::RefH2M<H2M::SubmeshH2M> submesh : meshComponent.Mesh->GetSubmeshes())
                {
                    materialUUID = MaterialLibrary::GetSubmeshMaterialUUID(meshComponent.Mesh.Raw(), submesh, entity);

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

                    submesh->Render(meshComponent.Mesh, EnvMapSharedData::s_ShaderHazelPBR, entityTransform, samplerSlot,
                        MaterialLibrary::s_EnvMapMaterials, entity, wireframeEnabledScene, wireframeEnabledModel);
                }
            }
        }
    }

    // BEGIN Renderer2D_H2M
    H2M::Renderer2D_H2M::BeginScene(viewProj, true);
    {
        // RendererBasic::SetLineThickness(2.0f);

        // BEGIN Draw Circles
        {
            auto view = s_EditorLayer->GetActiveScene()->GetRegistry().view<H2M::TransformComponentH2M, H2M::CircleRendererComponentH2M>();
            for (auto entity : view)
            {
                auto [transform, circle] = view.get<H2M::TransformComponentH2M, H2M::CircleRendererComponentH2M>(entity);

                H2M::Renderer2D_H2M::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness, circle.Fade, (int)entity);
            }
        }
        // END Draw Circles

    }
    H2M::Renderer2D_H2M::EndScene();
    // END Renderer2D_H2M

    // BEGIN Renderer2D_LinesH2M
    H2M::Renderer2D_LinesH2M::BeginScene(viewProj, true);
    {
        // BEGIN Draw Lines
        if (EnvMapSharedData::s_DisplayRay)
        {
            glm::vec3 camPosition = s_EditorLayer->GetActiveCamera()->GetPosition();
            H2M::Renderer2D_LinesH2M::DrawLine(EnvMapSharedData::s_NewRay, EnvMapSharedData::s_NewRay + glm::vec3(1.0f, 0.0f, 0.0f) * 100.0f, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
        }
        // END Draw Lines
    
        // BEGIN Draw AABB Bounding Boxes
        if (EntitySelection::s_SelectionContext.size())
        {
            for (auto selection : EntitySelection::s_SelectionContext)
            {
                if (selection.Mesh) {
                    H2M::EntityH2M meshEntity = selection.Entity;
                    glm::mat4 transform = glm::mat4(1.0f);
                    if (meshEntity.HasComponent<H2M::TransformComponentH2M>())
                    {
                        transform = meshEntity.GetComponent<H2M::TransformComponentH2M>().GetTransform();
                    }
                    glm::vec4 color = EnvMapEditorLayer::s_SelectionMode == SelectionMode::Entity ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) : glm::vec4(0.2f, 0.9f, 0.2f, 1.0f);
                    H2M::RendererH2M::DrawAABB(selection.Mesh->BoundingBox, transform * selection.Mesh->Transform, color);
                }
            }
        }
        // END Draw AABB Bounding Boxes
    }
    H2M::Renderer2D_LinesH2M::EndScene();
    // END Renderer2D_LinesH2M

    GetGeoPass()->GetSpecification().TargetFramebuffer->Bind();
}

void EnvMapSceneRenderer::CompositePass()
{
    // H2M::RendererH2M::BeginRenderPass(s_Data.CompositePass, false); // should we clear the framebuffer at this stage?

    s_Data.CompositeShader->Bind();

    auto targetFramebuffer = static_cast<H2M::RefH2M<MoravaFramebuffer>>(s_Data.GeoPass->GetSpecification().TargetFramebuffer);

    targetFramebuffer->GetTextureAttachmentColor()->Bind(EnvMapSharedData::s_SamplerSlots.at("u_Texture"));
    s_Data.CompositeShader->SetInt("u_Texture", EnvMapSharedData::s_SamplerSlots.at("u_Texture"));

    // ERROR: OpenGLMoravaShader::SetFloat() failed [name='u_Exposure', location='-1']
    // s_Data.CompositeShader->SetFloat("u_Exposure", EnvMapEditorLayer::GetMainCameraComponent().Camera.GetExposure()); // s_Data.SceneData.SceneCamera.Camera

    s_Data.CompositeShader->SetInt("u_TextureSamples", s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetSpecification().Samples);
    s_Data.CompositeShader->Validate();

    // H2M::RendererH2M::SubmitFullscreenQuad(H2M::RefH2M<H2M::HazelMaterial>());
    // H2M::RendererH2M::EndRenderPass();
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
            RendererH2M::BeginRenderPass(s_Data.ShadowMapRenderPass[i]);
            RendererH2M::EndRenderPass();
        }
        return;
    }

    CascadeData cascades[4];
    CalculateCascades(cascades, directionalLights[0].Direction);
    s_Data.LightViewMatrix = cascades[0].View;

    // RendererH2M::Submit([](){});
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }

    for (int i = 0; i < 4; i++)
    {
        s_Data.CascadeSplits[i] = cascades[i].SplitDepth;

        RendererH2M::BeginRenderPass(s_Data.ShadowMapRenderPass[i]);

        glm::mat4 shadowMapVP = cascades[i].ViewProj;

        static glm::mat4 scaleBiasMatrix = glm::scale(glm::mat4(1.0f), { 0.5f, 0.5f, 0.5f }) * glm::translate(glm::mat4(1.0f), { 1, 1, 1 });
        s_Data.LightMatrices[i] = scaleBiasMatrix * cascades[i].ViewProj;


        // Render entities
        for (auto& dc : s_Data.ShadowPassDrawList)
        {
            RefH2M<ShaderH2M> shader = dc.Mesh->IsAnimated() ? s_Data.ShadowMapAnimShader : s_Data.ShadowMapShader;
            shader->SetMat4("u_ViewProjection", shadowMapVP);
            RendererH2M::SubmitMeshWithShader(dc.Mesh, dc.Transform, shader);
        }

        RendererH2M::EndRenderPass();
    }
    ****/
}

void EnvMapSceneRenderer::SubmitEntityEnvMap(H2M::EntityH2M entity)
{
    auto mesh = entity.GetComponent<H2M::MeshComponentH2M>().Mesh;
    if (!mesh) {
        return;
    }

    auto transform = entity.GetComponent<H2M::TransformComponentH2M>().GetTransform();

    auto name = entity.GetComponent<H2M::TagComponentH2M>().Tag;
    AddToDrawList(name, mesh, entity, transform);
}

void EnvMapSceneRenderer::FlushDrawList()
{
    // H2M_CORE_ASSERT(!s_Data.ActiveScene, "");

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
    auto targetFramebuffer = static_cast<H2M::RefH2M<MoravaFramebuffer>>(s_Data.CompositePass->GetSpecification().TargetFramebuffer);

    return (uint32_t)targetFramebuffer->GetTextureAttachmentColor()->GetID();
}

H2M::SceneRendererOptionsH2M& EnvMapSceneRenderer::GetOptions()
{
    return s_Data.Options;
}

H2M::RefH2M<H2M::TextureCubeH2M> EnvMapSceneRenderer::GetRadianceMap()
{
    return s_Data.SceneData.SceneEnvironment.RadianceMap;
}

H2M::RefH2M<H2M::TextureCubeH2M> EnvMapSceneRenderer::GetIrradianceMap()
{
    return s_Data.SceneData.SceneEnvironment.IrradianceMap;
}

H2M::RefH2M<H2M::Texture2D_H2M> EnvMapSceneRenderer::GetBRDFLUT()
{
    return s_Data.BRDFLUT;
}

H2M::RefH2M<MoravaShader> EnvMapSceneRenderer::GetShaderComposite()
{
    return s_Data.CompositeShader;
}

H2M::RefH2M<H2M::RenderPassH2M> EnvMapSceneRenderer::GetGeoPass()
{
    return s_Data.GeoPass;
}

H2M::RefH2M<H2M::RenderPassH2M> EnvMapSceneRenderer::GetCompositePass()
{
    return s_Data.CompositePass;
}

void EnvMapSceneRenderer::CreateDrawCommand(std::string fileNameNoExt, H2M::RefH2M<H2M::MeshH2M> mesh)
{
    // s_Data.DrawList.clear(); // doesn't work for multiple meshes on the scene
    EnvMapSceneRendererData::DrawCommand drawCommand;

    drawCommand.Name = fileNameNoExt;
    drawCommand.MeshPtr = mesh;
    drawCommand.Transform = glm::mat4(1.0f);

    s_Data.DrawList.push_back(drawCommand);
}

H2M::LightH2M& EnvMapSceneRenderer::GetActiveLight()
{
    return s_Data.SceneData.ActiveLight;
}

void EnvMapSceneRenderer::SetActiveLight(H2M::LightH2M& light)
{
    s_Data.SceneData.ActiveLight = light;
}

void EnvMapSceneRenderer::AddToDrawList(std::string name, H2M::RefH2M<H2M::MeshH2M> mesh, H2M::EntityH2M entity, glm::mat4 transform)
{
    s_Data.DrawList.push_back({ name, mesh.Raw(), entity.GetMaterial(), transform });
}

H2M::RefH2M<H2M::RenderPassH2M> EnvMapSceneRenderer::GetFinalRenderPass()
{
    return s_Data.CompositePass;
}

H2M::RefH2M<FramebufferTexture> EnvMapSceneRenderer::GetFinalColorBuffer()
{
    auto targetFramebuffer = static_cast<H2M::RefH2M<MoravaFramebuffer>>(s_Data.CompositePass->GetSpecification().TargetFramebuffer);

    return targetFramebuffer->GetTextureAttachmentColor();
}
