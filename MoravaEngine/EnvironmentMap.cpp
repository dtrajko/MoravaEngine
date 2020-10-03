#include "EnvironmentMap.h"
#include "Hazel/Renderer/RenderPass.h"
#include "Framebuffer.h"
#include "RendererBasic.h"
#include "Log.h"


EnvironmentMap::Data EnvironmentMap::s_Data;

EnvironmentMap::EnvironmentMap(const std::string& filepath)
{
    m_SamplerSlots = new std::map<std::string, unsigned int>();

    //  // PBR texture inputs
    m_SamplerSlots->insert(std::make_pair("albedo",     1)); // uniform sampler2D u_AlbedoTexture
    m_SamplerSlots->insert(std::make_pair("normal",     2)); // uniform sampler2D u_NormalTexture
    m_SamplerSlots->insert(std::make_pair("metalness",  3)); // uniform sampler2D u_MetalnessTexture
    m_SamplerSlots->insert(std::make_pair("roughness",  4)); // uniform sampler2D u_RoughnessTexture
    // m_SamplerSlots.insert(std::make_pair("ao", 5)); // uniform sampler2D u_AOTexture
    // Environment maps
    m_SamplerSlots->insert(std::make_pair("radiance",   5)); // uniform samplerCube u_EnvRadianceTex
    m_SamplerSlots->insert(std::make_pair("irradiance", 6)); // uniform samplerCube u_EnvIrradianceTex
    // BRDF LUT
    m_SamplerSlots->insert(std::make_pair("BRDF_LUT",   7)); // uniform sampler2D u_BRDFLUTTexture

    Init();
    SetupShaders();

    s_Data.SceneData.SceneEnvironment = Load(filepath);

    m_CheckerboardTexture = Hazel::HazelTexture2D::Create("Textures/Hazel/Checkerboard.tga");

    // Set lights
    s_Data.SceneData.ActiveLight.Direction = { -0.5f, -0.5f, 1.0f };
    s_Data.SceneData.ActiveLight.Radiance = { 1.0f, 1.0f, 1.0f };
}

void EnvironmentMap::SetupShaders()
{
    m_ShaderEquirectangularConversion = new Shader("Shaders/Hazel/EquirectangularToCubeMap.cs");
    Log::GetLogger()->info("EnvironmentMap: m_ShaderEquirectangularConversion compiled [programID={0}]", m_ShaderEquirectangularConversion->GetProgramID());

    m_ShaderEnvFiltering = new Shader("Shaders/Hazel/EnvironmentMipFilter.cs");
    Log::GetLogger()->info("EnvironmentMap: m_ShaderEnvFiltering compiled [programID={0}]", m_ShaderEnvFiltering->GetProgramID());

    m_ShaderEnvIrradiance = new Shader("Shaders/Hazel/EnvironmentIrradiance.cs");
    Log::GetLogger()->info("EnvironmentMap: m_ShaderEnvIrradiance compiled [programID={0}]", m_ShaderEnvIrradiance->GetProgramID());

    m_ShaderSkybox = new Shader("Shaders/Hazel/Skybox.vs", "Shaders/Hazel/Skybox.fs");
    Log::GetLogger()->info("EnvironmentMap: m_ShaderSkybox compiled [programID={0}]", m_ShaderSkybox->GetProgramID());

    m_ShaderHazelAnimPBR = new Shader("Shaders/Hazel/HazelPBR_Anim.vs", "Shaders/Hazel/HazelPBR_Anim.fs");
    Log::GetLogger()->info("EnvironmentMap: m_ShaderHazelAnimPBR compiled [programID={0}]", m_ShaderHazelAnimPBR->GetProgramID());

    m_ShaderComposite = new Shader("Shaders/Hazel/SceneComposite.vs", "Shaders/Hazel/SceneComposite.fs");
    Log::GetLogger()->info("EnvironmentMap: m_ShaderComposite compiled [programID={0}]", m_ShaderComposite->GetProgramID());
}

void EnvironmentMap::UpdateUniforms()
{
    /**** BEGIN HazelPBR_Anim ***/
    m_ShaderHazelAnimPBR->Bind();

    m_ShaderHazelAnimPBR->setVec3("lights.Direction", s_Data.SceneData.ActiveLight.Direction);
    m_ShaderHazelAnimPBR->setVec3("lights.Radiance", s_Data.SceneData.ActiveLight.Radiance);

    m_ShaderHazelAnimPBR->setInt("u_AlbedoTexture", m_SamplerSlots->at("albedo"));
    m_ShaderHazelAnimPBR->setInt("u_NormalTexture", m_SamplerSlots->at("normal"));
    m_ShaderHazelAnimPBR->setInt("u_MetalnessTexture", m_SamplerSlots->at("metalness"));
    m_ShaderHazelAnimPBR->setInt("u_RoughnessTexture", m_SamplerSlots->at("roughness"));
    // m_ShaderHazelAnimPBR->setInt("u_AOTexture", m_SamplerSlots->at("ao"));

    m_ShaderHazelAnimPBR->setInt("u_EnvRadianceTex", m_SamplerSlots->at("radiance"));
    m_ShaderHazelAnimPBR->setInt("u_EnvIrradianceTex", m_SamplerSlots->at("irradiance"));

    m_ShaderHazelAnimPBR->setInt("u_BRDFLUTTexture", m_SamplerSlots->at("BRDF_LUT"));

    m_ShaderHazelAnimPBR->setVec3("u_AlbedoColor", m_AlbedoInput.Color);
    m_ShaderHazelAnimPBR->setFloat("u_Metalness", m_MetalnessInput.Value);
    m_ShaderHazelAnimPBR->setFloat("u_Roughness", m_RoughnessInput.Value);

    m_ShaderHazelAnimPBR->setFloat("u_EnvMapRotation", m_EnvMapRotation);

    m_ShaderHazelAnimPBR->setFloat("u_RadiancePrefilter", m_RadiancePrefilter ? 1.0f : 0.0f);
    m_ShaderHazelAnimPBR->setFloat("u_AlbedoTexToggle", m_AlbedoInput.UseTexture ? 1.0f : 0.0f);
    m_ShaderHazelAnimPBR->setFloat("u_NormalTexToggle", m_NormalInput.UseTexture ? 1.0f : 0.0f);
    m_ShaderHazelAnimPBR->setFloat("u_MetalnessTexToggle", m_MetalnessInput.UseTexture ? 1.0f : 0.0f);
    m_ShaderHazelAnimPBR->setFloat("u_RoughnessTexToggle", m_RoughnessInput.UseTexture ? 1.0f : 0.0f);
    /**** END HazelPBR_Anim ***/
}

EnvironmentMap::Environment EnvironmentMap::Load(const std::string& filepath)
{
    auto [radiance, irradiance] = CreateEnvironmentMap(filepath);
    return { radiance, irradiance };
}

void EnvironmentMap::SetEnvironment(Environment environment)
{
    s_Data.SceneData.SceneEnvironment = environment;
    SetSkybox(s_Data.SceneData.SceneEnvironment.RadianceMap);
}

void EnvironmentMap::Init()
{
    FramebufferSpecification geoFramebufferSpec;
    geoFramebufferSpec.Width = 1280;
    geoFramebufferSpec.Height = 720;
    geoFramebufferSpec.attachmentFormat = AttachmentFormat::RGBA16F;
    geoFramebufferSpec.Samples = 8;
    geoFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

    Hazel::RenderPassSpecification geoRenderPassSpec;
    geoRenderPassSpec.TargetFramebuffer = new Framebuffer(geoFramebufferSpec);
    s_Data.GeoPass = Hazel::RenderPass::Create(geoRenderPassSpec);

    FramebufferSpecification compFramebufferSpec;
    compFramebufferSpec.Width = 1280;
    compFramebufferSpec.Height = 720;
    compFramebufferSpec.attachmentFormat = AttachmentFormat::RGBA8;
    compFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };

    Hazel::RenderPassSpecification compRenderPassSpec;
    compRenderPassSpec.TargetFramebuffer = new Framebuffer(compFramebufferSpec);
    s_Data.CompositePass = Hazel::RenderPass::Create(compRenderPassSpec);

    s_Data.BRDFLUT = Hazel::HazelTexture2D::Create("Textures/Hazel/BRDF_LUT.tga");
}

void EnvironmentMap::SetSkybox(Hazel::HazelTextureCube* skybox)
{
    m_SkyboxTexture = skybox;
    m_ShaderSkybox->setInt("u_Texture", skybox->GetID());
}

EnvironmentMap::~EnvironmentMap()
{
}

void EnvironmentMap::SetViewportSize(uint32_t width, uint32_t height)
{
    s_Data.GeoPass->GetSpecification().TargetFramebuffer->Resize(width, height);
    s_Data.CompositePass->GetSpecification().TargetFramebuffer->Resize(width, height);
}

void EnvironmentMap::BeginScene(const Scene* scene)
{
    s_Data.ActiveScene = scene;
    s_Data.SceneData.SceneCamera = scene->GetCamera();
}

void EnvironmentMap::EndScene()
{
    FlushDrawList();
}

void EnvironmentMap::SubmitEntity(Hazel::Entity* entity)
{
    auto mesh = entity->GetMesh();
    if (!mesh)
        return;

    s_Data.DrawList.push_back({ mesh, entity->GetMaterial(), entity->GetTransform() });
}

Hazel::RenderPass* EnvironmentMap::GetFinalRenderPass()
{
    return s_Data.CompositePass;
}

FramebufferTexture* EnvironmentMap::GetFinalColorBuffer()
{
    return s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor();
}

uint32_t EnvironmentMap::GetFinalColorBufferID()
{
    return (uint32_t)s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetID();
}

EnvironmentMap::Options& EnvironmentMap::GetOptions()
{
    return s_Data.Options;
}

void EnvironmentMap::FlushDrawList()
{
    if (!s_Data.ActiveScene) {
        Log::GetLogger()->error("Active scene is not specified!");
    }

    GeometryPass();
    CompositePass();

    s_Data.DrawList.clear();
    s_Data.SceneData = {};
}

std::pair<Hazel::HazelTextureCube*, Hazel::HazelTextureCube*> EnvironmentMap::CreateEnvironmentMap(const std::string& filepath)
{
    const uint32_t cubemapSize = 2048;
    const uint32_t irradianceMapSize = 32;

    //  Ref<TextureCube> envUnfiltered = TextureCube::Create(TextureFormat::Float16, cubemapSize, cubemapSize);
    //  if (!equirectangularConversionShader)
    //      equirectangularConversionShader = Shader::Create("assets/shaders/EquirectangularToCubeMap.glsl");
    //  Ref<Texture2D> envEquirect = Texture2D::Create(filepath);
    //  HZ_CORE_ASSERT(envEquirect->GetFormat() == TextureFormat::Float16, "Texture is not HDR!");
    //  
    //  equirectangularConversionShader->Bind();
    //  envEquirect->Bind();
    //  Renderer::Submit([envUnfiltered, cubemapSize, envEquirect]()
    //      {
    //          glBindImageTexture(0, envUnfiltered->GetRendererID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    //          glDispatchCompute(cubemapSize / 32, cubemapSize / 32, 6);
    //          glGenerateTextureMipmap(envUnfiltered->GetRendererID());
    //      });
    //  
    //  
    //  if (!envFilteringShader)
    //      envFilteringShader = Shader::Create("assets/shaders/EnvironmentMipFilter.glsl");
    //  
    //  Ref<TextureCube> envFiltered = TextureCube::Create(TextureFormat::Float16, cubemapSize, cubemapSize);
    //  
    //  Renderer::Submit([envUnfiltered, envFiltered]()
    //      {
    //          glCopyImageSubData(envUnfiltered->GetRendererID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
    //              envFiltered->GetRendererID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
    //              envFiltered->GetWidth(), envFiltered->GetHeight(), 6);
    //      });
    //  
    //  envFilteringShader->Bind();
    //  envUnfiltered->Bind();
    //  
    //  Renderer::Submit([envUnfiltered, envFiltered, cubemapSize]() {
    //      const float deltaRoughness = 1.0f / glm::max((float)(envFiltered->GetMipLevelCount() - 1.0f), 1.0f);
    //      for (int level = 1, size = cubemapSize / 2; level < envFiltered->GetMipLevelCount(); level++, size /= 2) // <= ?
    //      {
    //          const GLuint numGroups = glm::max(1, size / 32);
    //          glBindImageTexture(0, envFiltered->GetRendererID(), level, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    //          glProgramUniform1f(envFilteringShader->GetRendererID(), 0, level * deltaRoughness);
    //          glDispatchCompute(numGroups, numGroups, 6);
    //      }
    //      });
    //  
    //  if (!envIrradianceShader)
    //      envIrradianceShader = Shader::Create("assets/shaders/EnvironmentIrradiance.glsl");
    //  
    //  Ref<TextureCube> irradianceMap = TextureCube::Create(TextureFormat::Float16, irradianceMapSize, irradianceMapSize);
    //  envIrradianceShader->Bind();
    //  envFiltered->Bind();
    //  Renderer::Submit([irradianceMap]()
    //      {
    //          glBindImageTexture(0, irradianceMap->GetRendererID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    //          glDispatchCompute(irradianceMap->GetWidth() / 32, irradianceMap->GetHeight() / 32, 6);
    //          glGenerateTextureMipmap(irradianceMap->GetRendererID());
    //      });
    //  
    //  return { envFiltered, irradianceMap };

    return std::pair<Hazel::HazelTextureCube*, Hazel::HazelTextureCube*>();
}

void EnvironmentMap::GeometryPass()
{
    BeginRenderPass(s_Data.GeoPass, true); // should we clear the buffer?

    auto viewProjection = s_Data.ActiveScene->GetCameraController()->CalculateViewMatrix();

    // Skybox
    m_ShaderSkybox->Bind();
    m_ShaderSkybox->setMat4("u_InverseVP", glm::inverse(viewProjection));
    SubmitFullscreenQuad(s_Data.SceneData.SkyboxMaterial);

    // Render entities
    for (auto& dc : s_Data.DrawList)
    {
        //  auto baseMaterial = dc.Mesh->GetMaterial();
        //  baseMaterial->Set("u_ViewProjectionMatrix", viewProjection);
        //  baseMaterial->Set("u_CameraPosition", s_Data.SceneData.SceneCamera.GetPosition());
        //  
        //  // Environment (TODO: don't do this per mesh)
        //  baseMaterial->Set("u_EnvRadianceTex", s_Data.SceneData.SceneEnvironment.RadianceMap);
        //  baseMaterial->Set("u_EnvIrradianceTex", s_Data.SceneData.SceneEnvironment.IrradianceMap);
        //  baseMaterial->Set("u_BRDFLUTTexture", s_Data.BRDFLUT);
        //  
        //  // Set lights (TODO: move to light environment and don't do per mesh)
        //  baseMaterial->Set("lights", s_Data.SceneData.ActiveLight);
        //  
        //  auto overrideMaterial = nullptr; // dc.Material;
        //  Renderer::SubmitMesh(dc.Mesh, dc.Transform, overrideMaterial);
    }

    // Grid
    if (GetOptions().ShowGrid)
    {
        //  s_Data.GridMaterial->Set("u_ViewProjection", viewProjection);
        //  SubmitQuad(s_Data.GridMaterial, glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(16.0f)));
    }

    if (GetOptions().ShowBoundingBoxes)
    {
        //  BeginScene(viewProjection);
        //  for (auto& dc : s_Data.DrawList)
        //      DrawAABB(dc.Mesh, dc.Transform);
        //  EndScene();
    }

    EndRenderPass();
}

void EnvironmentMap::CompositePass()
{
}

void EnvironmentMap::Update()
{
    UpdateUniforms();
}

void EnvironmentMap::BeginRenderPass(Hazel::RenderPass* renderPass, bool clear)
{
    if (!renderPass) {
        Log::GetLogger()->error("Render pass cannot be null!");
    }

    // TODO: Convert all of this into a render command buffer
    s_Data.ActiveRenderPass = renderPass;

    renderPass->GetSpecification().TargetFramebuffer->Bind();

    if (clear)
    {
        const glm::vec4& clearColor = renderPass->GetSpecification().TargetFramebuffer->GetSpecification()[0].ClearColor;
        RendererBasic::Clear(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    }
}

void EnvironmentMap::SubmitFullscreenQuad(Material* material)
{
    bool depthTest = true;
    if (material)
    {
        //  material->Bind();
        //  depthTest = material->GetFlag(MaterialFlag::DepthTest);
    }

    s_Data.m_FullscreenQuadVertexArray->Bind();
    DrawIndexed(6, PrimitiveType::Triangles, depthTest);
}

void EnvironmentMap::DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest)
{
    if (!depthTest)
        glDisable(GL_DEPTH_TEST);

    GLenum glPrimitiveType = 0;
    switch (type)
    {
    case PrimitiveType::Triangles:
        glPrimitiveType = GL_TRIANGLES;
        break;
    case PrimitiveType::Lines:
        glPrimitiveType = GL_LINES;
        break;
    }

    glDrawElements(glPrimitiveType, count, GL_UNSIGNED_INT, nullptr);

    if (!depthTest)
        glEnable(GL_DEPTH_TEST);
}

void EnvironmentMap::EndRenderPass()
{
    if (!s_Data.ActiveRenderPass) {
        Log::GetLogger()->error("No active render pass! Have you called Renderer::EndRenderPass twice?");
    }

    s_Data.ActiveRenderPass->GetSpecification().TargetFramebuffer->Unbind();
    s_Data.ActiveRenderPass = nullptr;
}
