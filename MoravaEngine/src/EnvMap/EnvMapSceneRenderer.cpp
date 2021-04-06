#include "EnvMap/EnvMapSceneRenderer.h"

#include "Hazel/Core/Assert.h"
#include "Hazel/Platform/OpenGL/OpenGLRenderPass.h"
#include "Hazel/Renderer/HazelRenderer.h"
#include "Hazel/Renderer/HazelShader.h"
#include "Hazel/Scene/HazelScene.h"

#include "Core/Application.h"
#include "Core/Log.h"
#include "Core/ResourceManager.h"
#include "Renderer/RendererBasic.h"


std::map<std::string, unsigned int>* EnvMapSceneRenderer::m_SamplerSlots;
Hazel::Ref<Shader> EnvMapSceneRenderer::s_ShaderEquirectangularConversion;
Hazel::Ref<Shader> EnvMapSceneRenderer::s_ShaderEnvFiltering;
Hazel::Ref<Shader> EnvMapSceneRenderer::s_ShaderEnvIrradiance;
Hazel::Ref<Shader> EnvMapSceneRenderer::s_ShaderGrid;
Hazel::Ref<Shader> EnvMapSceneRenderer::s_ShaderSkybox;
Hazel::Ref<Hazel::HazelTextureCube> EnvMapSceneRenderer::s_EnvUnfiltered;
Hazel::Ref<Hazel::HazelTexture2D> EnvMapSceneRenderer::s_EnvEquirect;
Hazel::Ref<Hazel::HazelTextureCube> EnvMapSceneRenderer::s_EnvFiltered;
Hazel::Ref<Hazel::HazelTextureCube> EnvMapSceneRenderer::s_IrradianceMap;
float EnvMapSceneRenderer::s_GridScale = 16.025f;
float EnvMapSceneRenderer::s_GridSize = 0.025f;
uint32_t EnvMapSceneRenderer::s_FramebufferWidth = 1280;
uint32_t EnvMapSceneRenderer::s_FramebufferHeight = 720;


struct EnvMapSceneRendererData
{
    const Hazel::HazelScene* ActiveScene = nullptr;
    struct SceneInfo
    {
        SceneRendererCamera SceneCamera;

        // Resources
        Ref<Hazel::HazelMaterial> HazelSkyboxMaterial;
        Material* SkyboxMaterial;
        Hazel::Environment SceneEnvironment;
        Hazel::HazelLight ActiveLight;
    } SceneData;

    Hazel::Ref<Hazel::HazelTexture2D> BRDFLUT;

    Hazel::Ref<Shader> CompositeShader;

    Hazel::Ref<EnvMapRenderPass> GeoPass;
    Hazel::Ref<EnvMapRenderPass> CompositePass;
    Hazel::Ref<EnvMapRenderPass> ActiveRenderPass;

    struct DrawCommand
    {
        std::string Name;
        Mesh* Mesh;
        Material* Material;
        glm::mat4 Transform;
    };
    std::vector<DrawCommand> DrawList;
    std::vector<DrawCommand> SelectedMeshDrawList;

    // Grid
    Material* GridMaterial;
    // Ref<HazelShader> HazelGridShader;
    // Ref<Shader> GridShader;
    Ref<Hazel::HazelMaterial> OutlineMaterial;

    SceneRendererOptions Options;

    // Renderer data
    Hazel::RenderCommandQueue* m_CommandQueue;
};

static EnvMapSceneRendererData s_Data;

void EnvMapSceneRenderer::Init(std::string filepath, Hazel::HazelScene* scene)
{
    m_SamplerSlots = new std::map<std::string, unsigned int>();
    m_SamplerSlots->insert(std::make_pair("u_Texture", 1));

    SetupShaders();

    s_Data.SceneData.SceneEnvironment = Load(filepath);
    SetEnvironment(s_Data.SceneData.SceneEnvironment);

    // Set lights
    s_Data.SceneData.ActiveLight.Direction = { 0.0f, -1.0f, 0.0f };
    s_Data.SceneData.ActiveLight.Radiance = { 1.0f, 1.0f, 1.0f };
    s_Data.SceneData.ActiveLight.Multiplier = 0.5f;

    // Grid
    // s_Data.OutlineMaterial = Hazel::Ref<HazelMaterial>::Create(s_ShaderGrid);
    s_Data.GridMaterial = new Material(s_ShaderGrid);
    s_ShaderGrid->Bind();
    s_ShaderGrid->setFloat("u_Scale", s_GridScale);
    s_ShaderGrid->setFloat("u_Res", s_GridSize);

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

    EnvMapRenderPassSpecification geoRenderPassSpec;
    geoRenderPassSpec.TargetFramebuffer = Framebuffer::Create(geoFramebufferSpec);
    geoRenderPassSpec.TargetFramebuffer->AddColorAttachment(geoFramebufferSpec);
    geoRenderPassSpec.TargetFramebuffer->AddDepthAttachment(geoFramebufferDepthSpec);
    Log::GetLogger()->debug("Generating the GEO RenderPass framebuffer with AttachmentFormat::RGBA16F");

    geoRenderPassSpec.TargetFramebuffer->Generate(geoFramebufferSpec.Width, geoFramebufferSpec.Height);

    s_Data.GeoPass = Hazel::Ref<EnvMapRenderPass>::Create(geoRenderPassSpec);

    FramebufferSpecification compFramebufferSpec;
    compFramebufferSpec.Width = s_FramebufferWidth;
    compFramebufferSpec.Height = s_FramebufferHeight;
    compFramebufferSpec.attachmentType = AttachmentType::Texture;
    compFramebufferSpec.attachmentFormat = AttachmentFormat::RGBA;
    compFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };

    isMultisample = compFramebufferSpec.Samples > 1;

    EnvMapRenderPassSpecification compRenderPassSpec;
    compRenderPassSpec.TargetFramebuffer = Framebuffer::Create(compFramebufferSpec);
    compRenderPassSpec.TargetFramebuffer->AddColorAttachment(compFramebufferSpec);

    FramebufferSpecification compFramebufferDepthSpec;
    compFramebufferDepthSpec = compFramebufferSpec;
    compFramebufferDepthSpec.attachmentType = AttachmentType::Renderbuffer;
    compFramebufferDepthSpec.attachmentFormat = AttachmentFormat::Depth;
    compRenderPassSpec.TargetFramebuffer->AddDepthAttachment(compFramebufferDepthSpec);

    Log::GetLogger()->debug("Generating the COMPOSITE RenderPass framebuffer with AttachmentFormat::RGBA");
    compRenderPassSpec.TargetFramebuffer->Generate(compFramebufferSpec.Width, compFramebufferSpec.Height);
    s_Data.CompositePass = Hazel::Ref<EnvMapRenderPass>::Create(compRenderPassSpec);

    s_Data.BRDFLUT = Hazel::HazelTexture2D::Create("Textures/Hazel/BRDF_LUT.tga");
}

void EnvMapSceneRenderer::SetupShaders()
{
    s_Data.CompositeShader = Shader::Create("Shaders/Hazel/SceneComposite.vs", "Shaders/Hazel/SceneComposite.fs");
    Log::GetLogger()->info("EnvMapSceneRenderer: s_ShaderComposite compiled [programID={0}]", s_Data.CompositeShader->GetProgramID());

    s_ShaderEquirectangularConversion = Shader::Create("Shaders/Hazel/EquirectangularToCubeMap.cs");
    Log::GetLogger()->info("EnvMapSceneRenderer: s_ShaderEquirectangularConversion compiled [programID={0}]", s_ShaderEquirectangularConversion->GetProgramID());

    s_ShaderEnvFiltering = Shader::Create("Shaders/Hazel/EnvironmentMipFilter.cs");
    Log::GetLogger()->info("EnvMapSceneRenderer: s_ShaderEnvFiltering compiled [programID={0}]", s_ShaderEnvFiltering->GetProgramID());

    s_ShaderEnvIrradiance = Shader::Create("Shaders/Hazel/EnvironmentIrradiance.cs");
    Log::GetLogger()->info("EnvMapSceneRenderer: s_ShaderEnvIrradiance compiled [programID={0}]", s_ShaderEnvIrradiance->GetProgramID());

    s_ShaderSkybox = Shader::Create("Shaders/Hazel/Skybox.vs", "Shaders/Hazel/Skybox.fs");
    Log::GetLogger()->info("EnvMapSceneRenderer: s_ShaderSkybox compiled [programID={0}]", s_ShaderSkybox->GetProgramID());

    s_ShaderGrid = Shader::Create("Shaders/Hazel/Grid.vs", "Shaders/Hazel/Grid.fs");
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
    return { "", radiance, irradiance };
}

void EnvMapSceneRenderer::SetViewportSize(uint32_t width, uint32_t height)
{
    s_Data.GeoPass->GetSpecification().TargetFramebuffer->Resize(width, height, true);
    s_Data.CompositePass->GetSpecification().TargetFramebuffer->Resize(width, height, true);
}

void EnvMapSceneRenderer::BeginScene(Hazel::HazelScene* scene, const SceneRendererCamera& camera)
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

static Ref<Hazel::HazelShader> equirectangularConversionShader, envFilteringShader, envIrradianceShader;

// Moved from EnvironmentMap
void EnvMapSceneRenderer::SetEnvironment(Hazel::Environment environment)
{
    s_Data.SceneData.SceneEnvironment = environment;
}

// Moved from EnvironmentMap
std::pair<Hazel::Ref<Hazel::HazelTextureCube>, Hazel::Ref<Hazel::HazelTextureCube>> EnvMapSceneRenderer::CreateEnvironmentMap(const std::string& filepath)
{
    const uint32_t cubemapSize = 512;
    const uint32_t irradianceMapSize = 32;

    s_EnvUnfiltered = Hazel::HazelTextureCube::Create(Hazel::HazelImageFormat::RGBA16F, cubemapSize, cubemapSize);
    s_EnvEquirect = Hazel::HazelTexture2D::Create(filepath);

    if (s_EnvEquirect->GetFormat() != Hazel::HazelImageFormat::RGBA16F) {
        Log::GetLogger()->error("Texture is not HDR!");
    }

    s_ShaderEquirectangularConversion->Bind();
    s_EnvEquirect->Bind();

    glBindImageTexture(0, s_EnvUnfiltered->GetID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glDispatchCompute(cubemapSize / 32, cubemapSize / 32, 6);
    glGenerateTextureMipmap(s_EnvUnfiltered->GetID());

    s_EnvFiltered = Hazel::HazelTextureCube::Create(Hazel::HazelImageFormat::RGBA16F, cubemapSize, cubemapSize);
    glCopyImageSubData(s_EnvUnfiltered->GetID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
        s_EnvFiltered->GetID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
        s_EnvFiltered->GetWidth(), s_EnvFiltered->GetHeight(), 6);

    s_ShaderEnvFiltering->Bind();
    s_EnvFiltered->Bind();

    const float deltaRoughness = 1.0f / glm::max((float)(s_EnvFiltered->GetMipLevelCount() - 1.0f), 1.0f);
    for (int level = 1, size = cubemapSize / 2; level < (int)s_EnvFiltered->GetMipLevelCount(); level++, size /= 2) // <= ?
    {
        Log::GetLogger()->debug("BEGIN EnvFiltering size {0} level {1}/{2}", size, level, s_EnvFiltered->GetMipLevelCount());
        const GLuint numGroups = glm::max(1, size / 32);
        glBindImageTexture(0, s_EnvFiltered->GetID(), level, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        glProgramUniform1f(s_ShaderEnvFiltering->GetProgramID(), 0, level * deltaRoughness);
        glDispatchCompute(numGroups, numGroups, 6);
        Log::GetLogger()->debug("END EnvFiltering size {0} numGroups {1} level {2}/{3}", size, numGroups, level, s_EnvFiltered->GetMipLevelCount());
    }

    s_IrradianceMap = Hazel::HazelTextureCube::Create(Hazel::HazelImageFormat::RGBA16F, irradianceMapSize, irradianceMapSize);
    s_ShaderEnvIrradiance->Bind();
    s_EnvFiltered->Bind();
    glBindImageTexture(0, s_IrradianceMap->GetID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glDispatchCompute(s_IrradianceMap->GetWidth() / 32, s_IrradianceMap->GetHeight() / 32, 6);
    glGenerateTextureMipmap(s_IrradianceMap->GetID());

    return { s_EnvFiltered, s_IrradianceMap };
}

void EnvMapSceneRenderer::CompositePass()
{
    Hazel::HazelRenderer::BeginRenderPass(s_Data.CompositePass, false); // should we clear the framebuffer at this stage?

    s_Data.CompositeShader->Bind();

    s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->Bind(m_SamplerSlots->at("u_Texture"));
    s_Data.CompositeShader->setInt("u_Texture", m_SamplerSlots->at("u_Texture"));
    s_Data.CompositeShader->setFloat("u_Exposure", s_Data.SceneData.SceneCamera.Camera.GetExposure());
    s_Data.CompositeShader->setInt("u_TextureSamples", s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetSpecification().Samples);

    Hazel::HazelRenderer::SubmitFullscreenQuad(nullptr);

    Hazel::HazelRenderer::EndRenderPass();
}

void EnvMapSceneRenderer::FlushDrawList()
{
    // HZ_CORE_ASSERT(!s_Data.ActiveScene, "");

    //  if (!s_Data.ActiveScene) {
    //      Log::GetLogger()->error("Active scene is not specified!");
    //  }

    GeometryPass();
    // Log::GetLogger()->debug("EnvironmentMap::FlushDrawList GeometryPass executed...");
    CompositePass();
    // Log::GetLogger()->debug("EnvironmentMap::FlushDrawList CompositePass executed...");

    // m_Data.DrawList.clear(); // TODO: make DrawList update every tick
    //  m_Data.SceneData = {};   // TODO: make SceneData update every tick
}

void EnvMapSceneRenderer::GeometryPass()
{
    bool outline = s_Data.SelectedMeshDrawList.size() > 0;

    if (outline)
    {
        Hazel::HazelRenderer::Submit([]() {
        });

        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    }

    Hazel::HazelRenderer::BeginRenderPass(s_Data.GeoPass);

    if (outline)
    {
        Hazel::HazelRenderer::Submit([]() {
        });

        glStencilMask(0);
    }

    auto viewProjection = s_Data.SceneData.SceneCamera.Camera.GetProjectionMatrix() * s_Data.SceneData.SceneCamera.Camera.GetViewMatrix();
    glm::vec3 cameraPosition = glm::inverse(s_Data.SceneData.SceneCamera.Camera.GetViewMatrix())[3];

    // Skybox
    auto skyboxShader = s_Data.SceneData.HazelSkyboxMaterial->GetShader();

}

uint32_t EnvMapSceneRenderer::GetFinalColorBufferRendererID()
{
    return (uint32_t)s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetID();
}

SceneRendererOptions& EnvMapSceneRenderer::GetOptions()
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

Hazel::Ref<Shader> EnvMapSceneRenderer::GetShaderComposite()
{
    return s_Data.CompositeShader;
}

Hazel::Ref<EnvMapRenderPass> EnvMapSceneRenderer::GetGeoPass()
{
    return s_Data.GeoPass;
}

Hazel::Ref<EnvMapRenderPass> EnvMapSceneRenderer::GetCompositePass()
{
    return s_Data.CompositePass;
}

void EnvMapSceneRenderer::CreateDrawCommand(std::string fileNameNoExt, Hazel::HazelMesh* mesh)
{
    // s_Data.DrawList.clear(); // doesn't work for multiple meshes on the scene
    EnvMapSceneRendererData::DrawCommand drawCommand;

    drawCommand.Name = fileNameNoExt;
    drawCommand.Mesh = mesh;
    drawCommand.Transform = glm::mat4(1.0f);

    s_Data.DrawList.push_back(drawCommand);
}

Hazel::HazelLight& EnvMapSceneRenderer::GetActiveLight()
{
    return s_Data.SceneData.ActiveLight;
}

void EnvMapSceneRenderer::SetActiveLight(Hazel::HazelLight& light)
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
    return s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor();
}
