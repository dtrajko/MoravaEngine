#include "EnvironmentMap.h"

#include "Hazel/Renderer/RenderPass.h"
#include "Hazel/Renderer/HazelMaterial.h"
#include "Hazel/Renderer/VertexArray.h"
#include "Hazel/Renderer/Buffer.h"
#include "Framebuffer.h"
#include "RendererBasic.h"
#include "Log.h"


EnvironmentMap::Data EnvironmentMap::s_Data = {};

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

    s_Data.SceneData.SceneEnvironment = Load(filepath);

    m_CheckerboardTexture = Hazel::HazelTexture2D::Create("Textures/Hazel/Checkerboard.tga");

    // Set lights
    s_Data.SceneData.ActiveLight.Direction = { -0.5f, -0.5f, 1.0f };
    s_Data.SceneData.ActiveLight.Radiance = { 1.0f, 1.0f, 1.0f };
}

void EnvironmentMap::SetupContextData()
{
    Log::GetLogger()->info("-- BEGIN EnvironmentMap loading MeshAnimPBR M1911 --");
    {
        // M1911
        TextureInfo textureInfoM1911 = {};
        textureInfoM1911.albedo = "Models/m1911/m1911_color.png";
        textureInfoM1911.normal = "Models/m1911/m1911_normal.png";
        textureInfoM1911.metallic = "Models/m1911/m1911_metalness.png";
        textureInfoM1911.roughness = "Models/m1911/m1911_roughness.png";
        textureInfoM1911.ao = "Textures/PBR/silver/ao.png";

        Data::DrawCommand drawCommand;
        drawCommand.Material = new Material(textureInfoM1911, m_MaterialSpecular, m_MaterialShininess);
        drawCommand.Mesh = new Hazel::MeshAnimPBR("Models/m1911/m1911.fbx", m_ShaderHazelAnimPBR, drawCommand.Material);
        drawCommand.Mesh->SetTimeMultiplier(1.0f);
        drawCommand.Transform = glm::mat4(1.0f);

        s_Data.DrawList.push_back(drawCommand);
    }
    Log::GetLogger()->info("-- END EnvironmentMap loading MeshAnimPBR M1911 --");

    SetupFullscreenQuad();
}

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

    glCreateVertexArrays(1, &s_Data.FullscreenQuadVAO);

    // setup plane VAO
    glGenBuffers(1, &s_Data.FullscreenQuadIBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_Data.FullscreenQuadIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices[0]) * 6, &indices[0], GL_STATIC_DRAW);

    glGenVertexArrays(1, &s_Data.FullscreenQuadVAO);
    glGenBuffers(1, &s_Data.FullscreenQuadVBO);

    // fill buffer
    glBindBuffer(GL_ARRAY_BUFFER, s_Data.FullscreenQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertex) * 4, &data[0], GL_STATIC_DRAW);

    // link vertex attributes
    glBindVertexArray(s_Data.FullscreenQuadVAO);

    // layout (location = 0) in vec3 aPos;
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, Position));

    // layout(location = 1) in vec2 aTexCoords;
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (void*)offsetof(QuadVertex, TexCoord));

    glBindBuffer(GL_ARRAY_BUFFER, 0);         // Unbind VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind IBO/EBO
    glBindVertexArray(0);                     // Unbind VAO
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
    geoRenderPassSpec.TargetFramebuffer->Generate(geoFramebufferSpec.Width, geoFramebufferSpec.Height);
    s_Data.GeoPass = Hazel::RenderPass::Create(geoRenderPassSpec);

    FramebufferSpecification compFramebufferSpec;
    compFramebufferSpec.Width = 1280;
    compFramebufferSpec.Height = 720;
    compFramebufferSpec.attachmentType = AttachmentType::Texture;
    compFramebufferSpec.attachmentFormat = AttachmentFormat::RGBA8;
    compFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };

    Hazel::RenderPassSpecification compRenderPassSpec;
    compRenderPassSpec.TargetFramebuffer = new Framebuffer(compFramebufferSpec);
    compRenderPassSpec.TargetFramebuffer->CreateAttachment(compFramebufferSpec);
    compRenderPassSpec.TargetFramebuffer->Generate(compFramebufferSpec.Width, compFramebufferSpec.Height);
    s_Data.CompositePass = Hazel::RenderPass::Create(compRenderPassSpec);

    s_Data.BRDFLUT = Hazel::HazelTexture2D::Create("Textures/Hazel/BRDF_LUT.tga");

    SetupContextData();
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

    s_Data.DrawList.push_back({ (Hazel::MeshAnimPBR*)mesh, entity->GetMaterial(), entity->GetTransform() });
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
    const uint32_t cubemapSize = 1024;
    const uint32_t irradianceMapSize = 32;

    Hazel::HazelTextureCube* envUnfiltered = Hazel::HazelTextureCube::Create(Hazel::HazelTextureFormat::Float16, cubemapSize, cubemapSize);
    Hazel::HazelTexture2D* envEquirect = Hazel::HazelTexture2D::Create(filepath);

    if (envEquirect->GetFormat() != Hazel::HazelTextureFormat::Float16) {
        Log::GetLogger()->error("Texture is not HDR!");
    }

    m_ShaderEquirectangularConversion->Bind();
    envEquirect->Bind();

    glBindImageTexture(0, envUnfiltered->GetID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glDispatchCompute(cubemapSize / 32, cubemapSize / 32, 6);
    glGenerateTextureMipmap(envUnfiltered->GetID());

    Hazel::HazelTextureCube* envFiltered = Hazel::HazelTextureCube::Create(Hazel::HazelTextureFormat::Float16, cubemapSize, cubemapSize);
    glCopyImageSubData(envUnfiltered->GetID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
        envFiltered->GetID(), GL_TEXTURE_CUBE_MAP, 0, 0, 0, 0,
        envFiltered->GetWidth(), envFiltered->GetHeight(), 6);

    m_ShaderEnvFiltering->Bind();
    envFiltered->Bind();

    const float deltaRoughness = 1.0f / glm::max((float)(envFiltered->GetMipLevelCount() - 1.0f), 1.0f);
    for (int level = 1, size = cubemapSize / 2; level < (int)envFiltered->GetMipLevelCount(); level++, size /= 2) // <= ?
    {
        Log::GetLogger()->debug("BEGIN EnvFiltering size {0} level {1}/{2}", size, level, envFiltered->GetMipLevelCount());
        const GLuint numGroups = glm::max(1, size / 32);
        glBindImageTexture(0, envFiltered->GetID(), level, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        glProgramUniform1f(m_ShaderEnvFiltering->GetProgramID(), 0, level * deltaRoughness);
        glDispatchCompute(numGroups, numGroups, 6);
        Log::GetLogger()->debug("END EnvFiltering size {0} numGroups {1} level {2}/{3}", size, numGroups, level, envFiltered->GetMipLevelCount());
    }

    Hazel::HazelTextureCube* irradianceMap = Hazel::HazelTextureCube::Create(Hazel::HazelTextureFormat::Float16, irradianceMapSize, irradianceMapSize);
    m_ShaderEnvIrradiance->Bind();
    envFiltered->Bind();
    glBindImageTexture(0, irradianceMap->GetID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glDispatchCompute(irradianceMap->GetWidth() / 32, irradianceMap->GetHeight() / 32, 6);
    glGenerateTextureMipmap(irradianceMap->GetID());

    return { envFiltered, irradianceMap };
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
        auto baseMaterial = dc.Mesh->GetBaseMaterial();
        m_ShaderHazelAnimPBR->setMat4("u_ViewProjectionMatrix", viewProjection);
        m_ShaderHazelAnimPBR->setVec3("u_CameraPosition", s_Data.SceneData.SceneCamera->GetPosition());

        // Environment (TODO: don't do this per mesh)
        m_ShaderHazelAnimPBR->setInt("u_EnvRadianceTex", s_Data.SceneData.SceneEnvironment.RadianceMap->GetID());
        m_ShaderHazelAnimPBR->setInt("u_EnvIrradianceTex", s_Data.SceneData.SceneEnvironment.IrradianceMap->GetID());
        m_ShaderHazelAnimPBR->setInt("u_BRDFLUTTexture", s_Data.BRDFLUT->GetID());

        // Set lights (TODO: move to light environment and don't do per mesh)
        m_ShaderHazelAnimPBR->setVec3("lights.Direction", s_Data.SceneData.ActiveLight.Direction);
        m_ShaderHazelAnimPBR->setVec3("lights.Radiance", s_Data.SceneData.ActiveLight.Radiance);
        m_ShaderHazelAnimPBR->setFloat("lights.Multiplier", s_Data.SceneData.ActiveLight.Multiplier);

        auto overrideMaterial = nullptr; // dc.Material;
        SubmitMesh(dc.Mesh, dc.Transform, overrideMaterial);
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
    BeginRenderPass(s_Data.CompositePass, true); // should we clear the framebuffer at this stage?
    m_ShaderComposite->Bind();
    m_ShaderComposite->setFloat("u_Exposure", s_Data.SceneData.SceneCamera->GetExposure());
    m_ShaderComposite->setInt("u_TextureSamples", s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetSpecification().Samples);
    s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->Bind();
    SubmitFullscreenQuad(nullptr);
    EndRenderPass();
}

void EnvironmentMap::Update(Scene* scene, float timestep)
{
    s_Data.ActiveScene = scene;

    UpdateUniforms();

    m_ShaderSkybox->setFloat("u_TextureLod", m_SkyboxLOD);

    // Update MeshAnimPBR List
    for (auto& dc : s_Data.DrawList)
    {
        dc.Mesh->OnUpdate(timestep, false);
    }

    BeginScene(s_Data.ActiveScene);

    // Render MeshAnimPBR meshes (later entt entities)
    uint32_t samplerSlot = 0;
    glm::mat4 entityTransform = glm::mat4(1.0f);
    for (auto& dc : s_Data.DrawList)
    {
        dc.Mesh->Render(samplerSlot, entityTransform);
    }

    EndScene();
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
        const glm::vec4& clearColor = renderPass->GetSpecification().TargetFramebuffer->GetSpecification().ClearColor;
        RendererBasic::Clear(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    }
}

void EnvironmentMap::SubmitFullscreenQuad(Material* material)
{
    bool depthTest = true;
    if (material)
    {
        m_ShaderHazelAnimPBR->Bind();
        depthTest = material->GetFlag(MaterialFlag::DepthTest);
    }

    glBindVertexArray(s_Data.FullscreenQuadVAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_Data.FullscreenQuadIBO);
    DrawIndexed(6, PrimitiveType::Triangles, depthTest);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind IBO/EBO
    glBindVertexArray(0);                     // Unbind VAO

    Log::GetLogger()->debug("END EnvironmentMap::SubmitFullscreenQuad");
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

        if (material->GetFlag(MaterialFlag::DepthTest)) {
            glEnable(GL_DEPTH_TEST);
        }
        else {
            glDisable(GL_DEPTH_TEST);
        }

        glDrawElementsBaseVertex(GL_TRIANGLES, submesh->GetIndexCount(), GL_UNSIGNED_INT, (void*)(sizeof(uint32_t)* submesh->BaseIndex), submesh->BaseVertex);
    }
}
