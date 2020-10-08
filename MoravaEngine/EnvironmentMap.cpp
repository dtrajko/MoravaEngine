#include "EnvironmentMap.h"

#include "Hazel/Renderer/RenderPass.h"
#include "Hazel/Renderer/HazelMaterial.h"
#include "Hazel/Renderer/VertexArray.h"
#include "Hazel/Renderer/Buffer.h"
#include "Framebuffer.h"
#include "RendererBasic.h"
#include "Log.h"


static const std::string DefaultEntityName = "Entity";

EnvironmentMap::EnvironmentMap(const std::string& filepath)
{
    m_Data = {};

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

    m_Data.SceneData.SceneEnvironment = Load(filepath);
    SetEnvironment(m_Data.SceneData.SceneEnvironment);

    // Skybox temporary version
    m_SkyboxCube = new CubeSkybox();

    m_CheckerboardTexture = Hazel::HazelTexture2D::Create("Textures/Hazel/Checkerboard.tga");

    // Set lights
    m_Data.SceneData.ActiveLight.Direction = { 0.0f, 0.0f, -1.0f };
    m_Data.SceneData.ActiveLight.Radiance = { 1.0f, 1.0f, 1.0f };
}

EnvironmentMap::Environment EnvironmentMap::Load(const std::string& filepath)
{
    auto [radiance, irradiance] = CreateEnvironmentMap(filepath);
    return { radiance, irradiance };
}

std::pair<Hazel::HazelTextureCube*, Hazel::HazelTextureCube*> EnvironmentMap::CreateEnvironmentMap(const std::string& filepath)
{
    const uint32_t cubemapSize = 1024;
    const uint32_t irradianceMapSize = 32;

    m_EnvUnfiltered = Hazel::HazelTextureCube::Create(Hazel::HazelTextureFormat::Float16, cubemapSize, cubemapSize);
    m_EnvEquirect = Hazel::HazelTexture2D::Create(filepath);

    if (m_EnvEquirect->GetFormat() != Hazel::HazelTextureFormat::Float16) {
        Log::GetLogger()->error("Texture is not HDR!");
    }

    m_ShaderEquirectangularConversion->Bind();
    m_EnvEquirect->Bind();

    glBindImageTexture(0, m_EnvUnfiltered->GetID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glDispatchCompute(cubemapSize / 32, cubemapSize / 32, 6);
    glGenerateTextureMipmap(m_EnvUnfiltered->GetID());

    m_EnvFiltered = Hazel::HazelTextureCube::Create(Hazel::HazelTextureFormat::Float16, cubemapSize, cubemapSize);
    glCopyImageSubData(m_EnvUnfiltered->GetID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
        m_EnvFiltered->GetID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
        m_EnvFiltered->GetWidth(), m_EnvFiltered->GetHeight(), 6);

    m_ShaderEnvFiltering->Bind();
    m_EnvFiltered->Bind();

    const float deltaRoughness = 1.0f / glm::max((float)(m_EnvFiltered->GetMipLevelCount() - 1.0f), 1.0f);
    for (int level = 1, size = cubemapSize / 2; level < (int)m_EnvFiltered->GetMipLevelCount(); level++, size /= 2) // <= ?
    {
        Log::GetLogger()->debug("BEGIN EnvFiltering size {0} level {1}/{2}", size, level, m_EnvFiltered->GetMipLevelCount());
        const GLuint numGroups = glm::max(1, size / 32);
        glBindImageTexture(0, m_EnvFiltered->GetID(), level, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        glProgramUniform1f(m_ShaderEnvFiltering->GetProgramID(), 0, level * deltaRoughness);
        glDispatchCompute(numGroups, numGroups, 6);
        Log::GetLogger()->debug("END EnvFiltering size {0} numGroups {1} level {2}/{3}", size, numGroups, level, m_EnvFiltered->GetMipLevelCount());
    }

    m_IrradianceMap = Hazel::HazelTextureCube::Create(Hazel::HazelTextureFormat::Float16, irradianceMapSize, irradianceMapSize);
    m_ShaderEnvIrradiance->Bind();
    m_EnvFiltered->Bind();
    glBindImageTexture(0, m_IrradianceMap->GetID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glDispatchCompute(m_IrradianceMap->GetWidth() / 32, m_IrradianceMap->GetHeight() / 32, 6);
    glGenerateTextureMipmap(m_IrradianceMap->GetID());

    return { m_EnvFiltered, m_IrradianceMap };
}

void EnvironmentMap::SetupContextData()
{
    Log::GetLogger()->info("-- BEGIN EnvironmentMap loading MeshAnimPBR M1911 --");
    {
        // M1911
        TextureInfo textureInfoM1911 = {};
        textureInfoM1911.albedo    = "Models/m1911/m1911_color.png";
        textureInfoM1911.normal    = "Models/m1911/m1911_normal.png";
        textureInfoM1911.metallic  = "Models/m1911/m1911_metalness.png";
        textureInfoM1911.roughness = "Models/m1911/m1911_roughness.png";
        textureInfoM1911.ao        = "Textures/PBR/silver/ao.png";

        Data::DrawCommand drawCommand;
        drawCommand.Name = "M1911";
        drawCommand.Material = new Material(textureInfoM1911, m_MaterialSpecular, m_MaterialShininess);
        drawCommand.Mesh = new Hazel::MeshAnimPBR("Models/m1911/m1911.fbx", m_ShaderHazelAnimPBR, drawCommand.Material);
        drawCommand.Mesh->SetTimeMultiplier(1.0f);
        drawCommand.Transform = glm::mat4(1.0f);

        m_Data.DrawList.push_back(drawCommand);

        m_MeshEntity = CreateEntity("drawCommand.Name");
        m_MeshEntity->SetMesh(drawCommand.Mesh);
    }
    Log::GetLogger()->info("-- END EnvironmentMap loading MeshAnimPBR M1911 --");

    m_Data.SceneData.SkyboxMaterial = new Material(m_ShaderSkybox);
    m_Data.SceneData.SkyboxMaterial->SetFlag(MaterialFlag::DepthTest, true); // false

    // SetupFullscreenQuad();
    // Framebuffer quad temporary version, in place of the broken SetupFullscreenQuad()
    m_HazelFullscreenQuad = new HazelFullscreenQuad();
}

/****
void EnvironmentMap::SetupFullscreenQuad()
{
    // Create fullscreen quad
    float x = -1;
    float y = -1;
    float width = 2;
    float height = 2;

    struct QuadVertex
    {
        glm::vec3 Position;
        glm::vec2 TexCoord;
    };

    QuadVertex* data = new QuadVertex[4];

    data[0].Position = glm::vec3(x, y, 0.1f);
    data[0].TexCoord = glm::vec2(0, 0);

    data[1].Position = glm::vec3(x + width, y, 0.1f);
    data[1].TexCoord = glm::vec2(1, 0);

    data[2].Position = glm::vec3(x + width, y + height, 0.1f);
    data[2].TexCoord = glm::vec2(1, 1);

    data[3].Position = glm::vec3(x, y + height, 0.1f);
    data[3].TexCoord = glm::vec2(0, 1);

    uint32_t indices[6] = { 0, 1, 2, 2, 3, 0, };

    glGenVertexArrays(1, &m_Data.FullscreenQuadVAO);
    // link vertex attributes
    glBindVertexArray(m_Data.FullscreenQuadVAO);

    glGenBuffers(1, &m_Data.FullscreenQuadVBO);

    // fill buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_Data.FullscreenQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertex) * 4, &data[0], GL_STATIC_DRAW);

    glGenBuffers(1, &m_Data.FullscreenQuadIBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Data.FullscreenQuadIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * 6, &indices[0], GL_STATIC_DRAW);

    // position layout (location = 0) in vec3 aPos;
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (const void*)offsetof(QuadVertex, Position));

    // tex coord layout(location = 1) in vec2 aTexCoords;
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (const void*)offsetof(QuadVertex, TexCoord));

    glBindBuffer(GL_ARRAY_BUFFER, 0);         // Unbind VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind IBO/EBO
    glBindVertexArray(0);                     // Unbind VAO
}
****/

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

    m_ShaderHazelAnimPBR->setVec3("lights.Direction", m_Data.SceneData.ActiveLight.Direction);
    m_ShaderHazelAnimPBR->setVec3("lights.Radiance", m_Data.SceneData.ActiveLight.Radiance);

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

    /**** BEGIN Shaders/Hazel/SceneComposite ****/
    m_ShaderComposite->Bind();
    m_ShaderComposite->setInt("u_Texture", 0);
    /**** END Shaders/Hazel/SceneComposite ****/

}

void EnvironmentMap::SetEnvironment(Environment environment)
{
    m_Data.SceneData.SceneEnvironment = environment;
    SetSkybox(m_Data.SceneData.SceneEnvironment.RadianceMap);
}

void EnvironmentMap::Init()
{
    SetupShaders();

    FramebufferSpecification geoFramebufferSpec;
    geoFramebufferSpec.Width = 1280;
    geoFramebufferSpec.Height = 720;
    geoFramebufferSpec.attachmentType = AttachmentType::Texture;
    geoFramebufferSpec.attachmentFormat = AttachmentFormat::RGBA16F;
    geoFramebufferSpec.Samples = 8;
    geoFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

    Hazel::RenderPassSpecification geoRenderPassSpec;
    geoRenderPassSpec.TargetFramebuffer = new Framebuffer(geoFramebufferSpec);
    geoRenderPassSpec.TargetFramebuffer->CreateAttachment(geoFramebufferSpec);
    geoRenderPassSpec.TargetFramebuffer->CreateAttachmentDepth(geoFramebufferSpec.Width, geoFramebufferSpec.Height, 
        AttachmentType::Renderbuffer, AttachmentFormat::Depth);
    Log::GetLogger()->debug("Generating the GEO RenderPass framebuffer with AttachmentFormat::RGBA16F");
    geoRenderPassSpec.TargetFramebuffer->Generate(geoFramebufferSpec.Width, geoFramebufferSpec.Height);
    m_Data.GeoPass = Hazel::RenderPass::Create(geoRenderPassSpec);

    FramebufferSpecification compFramebufferSpec;
    compFramebufferSpec.Width = 1280;
    compFramebufferSpec.Height = 720;
    compFramebufferSpec.attachmentType = AttachmentType::Texture;
    compFramebufferSpec.attachmentFormat = AttachmentFormat::RGBA8;
    compFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };

    Hazel::RenderPassSpecification compRenderPassSpec;
    compRenderPassSpec.TargetFramebuffer = new Framebuffer(compFramebufferSpec);
    compRenderPassSpec.TargetFramebuffer->CreateAttachment(compFramebufferSpec);
    compRenderPassSpec.TargetFramebuffer->CreateAttachmentDepth(compFramebufferSpec.Width, compFramebufferSpec.Height,
        AttachmentType::Renderbuffer, AttachmentFormat::Depth);
    Log::GetLogger()->debug("Generating the COMPOSITE RenderPass framebuffer with AttachmentFormat::RGBA8");
    compRenderPassSpec.TargetFramebuffer->Generate(compFramebufferSpec.Width, compFramebufferSpec.Height);
    m_Data.CompositePass = Hazel::RenderPass::Create(compRenderPassSpec);

    m_Data.BRDFLUT = Hazel::HazelTexture2D::Create("Textures/Hazel/BRDF_LUT.tga");

    SetupContextData();
}

void EnvironmentMap::SetSkybox(Hazel::HazelTextureCube* skybox)
{
    m_SkyboxTexture = skybox;
    m_ShaderSkybox->setInt("u_Texture", skybox->GetID());
}

EnvironmentMap::~EnvironmentMap()
{
    for (Hazel::Entity* entity : m_Entities)
        delete entity;
}

void EnvironmentMap::AddEntity(Hazel::Entity* entity)
{
    m_Entities.push_back(entity);
}

Hazel::Entity* EnvironmentMap::CreateEntity(const std::string& name)
{
    const std::string& entityName = name.empty() ? DefaultEntityName : name;
    Hazel::Entity* entity = new Hazel::Entity(entityName);
    AddEntity(entity);
    return entity;
}

void EnvironmentMap::Update(Scene* scene, float timestep)
{
    m_Data.ActiveScene = scene;

    UpdateUniforms();

    // Update all entities
    //  for (auto entity : m_Entities)
    //  {
    //      Hazel::MeshAnimPBR* mesh = (Hazel::MeshAnimPBR*)entity->GetMesh();
    //      if (mesh) {
    //          mesh->Update(glm::vec3(1.0f));
    //      }
    //  }

    // Update MeshAnimPBR List
    for (auto& dc : m_Data.DrawList)
    {
        dc.Mesh->OnUpdate(timestep, false);
    }

    m_ShaderSkybox->setFloat("u_TextureLod", m_SkyboxLOD);

    //  BeginScene(m_Data.ActiveScene);
    //  // Render entities
    //  for (auto entity : m_Entities)
    //  {
    //      // TODO: Should we render (logically)
    //      SubmitEntity(entity);
    //  }
    //  EndScene();
}

void EnvironmentMap::SetViewportSize(uint32_t width, uint32_t height)
{
    m_Data.GeoPass->GetSpecification().TargetFramebuffer->Resize(width, height);
    m_Data.CompositePass->GetSpecification().TargetFramebuffer->Resize(width, height);
}

void EnvironmentMap::BeginScene(const Scene* scene)
{
    m_Data.ActiveScene = scene;
    m_Data.SceneData.SceneCamera = scene->GetCamera();
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

    m_Data.DrawList.push_back({ entity->GetName(), (Hazel::MeshAnimPBR*)mesh, entity->GetMaterial(), entity->GetTransform() });
}

Hazel::RenderPass* EnvironmentMap::GetFinalRenderPass()
{
    return m_Data.CompositePass;
}

FramebufferTexture* EnvironmentMap::GetFinalColorBuffer()
{
    return m_Data.CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor();
}

uint32_t EnvironmentMap::GetFinalColorBufferID()
{
    return (uint32_t)m_Data.CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetID();
}

EnvironmentMap::Options& EnvironmentMap::GetOptions()
{
    return m_Data.Options;
}

void EnvironmentMap::FlushDrawList()
{
    if (!m_Data.ActiveScene) {
        Log::GetLogger()->error("Active scene is not specified!");
    }

    GeometryPass();
    // Log::GetLogger()->debug("EnvironmentMap::FlushDrawList GeometryPass executed...");
    CompositePass();
    // Log::GetLogger()->debug("EnvironmentMap::FlushDrawList CompositePass executed...");

    // m_Data.DrawList.clear(); // TODO: make DrawList update every tick
    //  m_Data.SceneData = {};   // TODO: make SceneData update every tick
}

void EnvironmentMap::GeometryPass()
{
    BeginRenderPass(m_Data.GeoPass, false); // should we clear the buffer?

    glm::mat4 viewProjection = RendererBasic::GetProjectionMatrix() * m_Data.ActiveScene->GetCameraController()->CalculateViewMatrix();

    // Skybox
    m_ShaderSkybox->Bind();
    m_ShaderSkybox->setFloat("u_TextureLod", m_SkyboxLOD);
    m_ShaderSkybox->setMat4("u_InverseVP", glm::inverse(viewProjection));
    SubmitFullscreenQuad(m_Data.SceneData.SkyboxMaterial);

    // Render entities
    for (auto& dc : m_Data.DrawList)
    {
        //  auto baseMaterial = dc.Mesh->GetBaseMaterial();
        auto baseMaterial = dc.Material;
        m_ShaderHazelAnimPBR->setMat4("u_ViewProjectionMatrix", viewProjection);
        m_ShaderHazelAnimPBR->setVec3("u_CameraPosition", m_Data.SceneData.SceneCamera->GetPosition());

        // Environment (TODO: don't do this per mesh)
        m_ShaderHazelAnimPBR->setInt("u_EnvRadianceTex", m_SamplerSlots->at("radiance"));
        m_ShaderHazelAnimPBR->setInt("u_EnvIrradianceTex", m_SamplerSlots->at("irradiance"));
        m_ShaderHazelAnimPBR->setInt("u_BRDFLUTTexture", m_SamplerSlots->at("BRDF_LUT"));

        // Set lights (TODO: move to light environment and don't do per mesh)
        m_ShaderHazelAnimPBR->setVec3("lights.Direction", m_Data.SceneData.ActiveLight.Direction);
        m_ShaderHazelAnimPBR->setVec3("lights.Radiance", m_Data.SceneData.ActiveLight.Radiance);
        m_ShaderHazelAnimPBR->setFloat("lights.Multiplier", m_Data.SceneData.ActiveLight.Multiplier);

        auto overrideMaterial = nullptr; // dc.Material;
        SubmitMesh(dc.Mesh, m_MeshEntity->Transform(), overrideMaterial);
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
    BeginRenderPass(m_Data.CompositePass, false); // should we clear the framebuffer at this stage?

    m_ShaderComposite->Bind();

    m_Data.GeoPass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->Bind();
    m_ShaderComposite->setInt("u_Texture", 0);

    m_ShaderComposite->setFloat("u_Exposure", m_Data.SceneData.SceneCamera->GetExposure());
    m_ShaderComposite->setInt("u_TextureSamples", m_Data.GeoPass->GetSpecification().TargetFramebuffer->GetSpecification().Samples);

    SubmitFullscreenQuad(nullptr);

    EndRenderPass();
}

void EnvironmentMap::SubmitFullscreenQuad(Material* material)
{
    bool depthTest = true;
    if (material)
    {
        m_ShaderHazelAnimPBR->Bind();
        depthTest = material->GetFlag(MaterialFlag::DepthTest);
    }

    //  glBindVertexArray(m_Data.FullscreenQuadVAO);
    //  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Data.FullscreenQuadIBO);
    //  DrawIndexed(6, PrimitiveType::Triangles, depthTest);
    //  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind IBO/EBO
    //  glBindVertexArray(0);                     // Unbind VAO

    m_HazelFullscreenQuad->Render();

    // Log::GetLogger()->debug("END EnvironmentMap::SubmitFullscreenQuad");
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

void EnvironmentMap::Render()
{
    BeginScene(m_Data.ActiveScene);

    m_Data.SceneData.SceneEnvironment.RadianceMap->Bind(m_SamplerSlots->at("radiance"));
    m_Data.SceneData.SceneEnvironment.IrradianceMap->Bind(m_SamplerSlots->at("irradiance"));
    m_Data.BRDFLUT->Bind(m_SamplerSlots->at("BRDF_LUT"));

    // Render MeshAnimPBR meshes (later entt entities)
    uint32_t samplerSlot = m_SamplerSlots->at("albedo");
    for (auto& dc : m_Data.DrawList)
    {
        // Log::GetLogger()->debug("EM::Render dc.Transform {0} {1} {2}", dc.Transform[3][0], dc.Transform[3][1], dc.Transform[3][2]);
        // dc.Mesh->Render(samplerSlot, m_MeshEntity->Transform());
    }

    ((Hazel::MeshAnimPBR*)m_MeshEntity->GetMesh())->Render(samplerSlot, m_MeshEntity->Transform());

    EndScene();
}

void EnvironmentMap::BeginRenderPass(Hazel::RenderPass* renderPass, bool clear)
{
    if (!renderPass) {
        Log::GetLogger()->error("Render pass cannot be null!");
    }

    // TODO: Convert all of this into a render command buffer
    m_Data.ActiveRenderPass = renderPass;

    renderPass->GetSpecification().TargetFramebuffer->Bind();

    //  Log::GetLogger()->debug("EnvironmentMap::BeginRenderPass Bind TargetFramebuffer [ID={0}, {1}x{2}]",
    //      renderPass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetID(),
    //      renderPass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetWidth(),
    //      renderPass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetHeight());

    if (clear)
    {
        const glm::vec4& clearColor = renderPass->GetSpecification().TargetFramebuffer->GetSpecification().ClearColor;
        RendererBasic::Clear(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    }
}

void EnvironmentMap::EndRenderPass()
{
    if (!m_Data.ActiveRenderPass) {
        Log::GetLogger()->error("No active render pass! Have you called Renderer::EndRenderPass twice?");
    }

    m_Data.ActiveRenderPass->GetSpecification().TargetFramebuffer->Unbind();
    m_Data.ActiveRenderPass = nullptr;
}

void EnvironmentMap::SubmitMesh(Hazel::MeshAnimPBR* mesh, const glm::mat4& transform, Material* overrideMaterial)
{
    // auto material = overrideMaterial ? overrideMaterial : mesh->GetMaterialInstance();
    // auto shader = material->GetShader();
    // TODO: Sort this out
    mesh->BindVertexArray();

    auto& materials = mesh->GetMaterials();
    for (Hazel::Submesh* submesh : mesh->GetSubmeshes())
    {
        // Material
        auto material = materials[submesh->MaterialIndex];

        for (size_t i = 0; i < mesh->m_BoneTransforms.size(); i++)
        {
            std::string uniformName = std::string("u_BoneTransforms[") + std::to_string(i) + std::string("]");
            m_ShaderHazelAnimPBR->setMat4(uniformName, mesh->m_BoneTransforms[i]);
        }

        m_ShaderHazelAnimPBR->setMat4("u_Transform", transform * submesh->Transform);

        if (material->GetFlag(MaterialFlag::DepthTest)) { // TODO: Fix Material flags
            glEnable(GL_DEPTH_TEST);
        }
        else {
            glDisable(GL_DEPTH_TEST);
        }

        glDisable(GL_DEPTH_TEST);

        glDrawElementsBaseVertex(GL_TRIANGLES, submesh->GetIndexCount(), GL_UNSIGNED_INT, (void*)(sizeof(uint32_t)* submesh->BaseIndex), submesh->BaseVertex);
    }
}

/**
 * Skybox temporary version Shaders/Hazel/Skybox
 */
void EnvironmentMap::RenderTemporarySkybox()
{
    RendererBasic::DisableCulling();

    m_ShaderSkybox->Bind();
    glm::mat4 viewProjection = RendererBasic::GetProjectionMatrix() * m_Data.ActiveScene->GetCameraController()->CalculateViewMatrix();
    m_ShaderSkybox->setMat4("u_InverseVP", glm::inverse(viewProjection));
    m_Data.SceneData.SceneEnvironment.RadianceMap->Bind(0);
    m_ShaderSkybox->setInt("u_Texture", 0);
    m_ShaderSkybox->setFloat("u_TextureLod", m_SkyboxLOD);

    m_SkyboxCube->Render();
}
