#include "SceneRenderer.h"

#include "HazelMaterial.h"
#include "HazelShader.h"
#include "HazelRenderer.h"
#include "../Core/Assert.h"
#include "../Scene/HazelScene.h"
#include "../Platform/OpenGL/OpenGLRenderPass.h"

#include "../../Scene.h"
#include "../../RendererBasic.h"
#include "../../ResourceManager.h"
#include "../../Log.h"


namespace Hazel {


    SceneRendererData SceneRenderer::s_Data;
    std::map<std::string, unsigned int>* SceneRenderer::m_SamplerSlots;

    SceneRenderer::SceneRenderer(std::string filepath, Scene* scene)
    {
        Init(filepath, scene);
    }

    SceneRenderer::~SceneRenderer()
    {
    }

    void SceneRenderer::Init(std::string filepath, Scene* scene)
    {
        m_SamplerSlots = new std::map<std::string, unsigned int>();
        m_SamplerSlots->insert(std::make_pair("u_Texture", 1));

        SetupShaders();

        BeginScene(scene);

        s_Data.SceneData.SceneEnvironment = Load(filepath);
        SetEnvironment(s_Data.SceneData.SceneEnvironment);

        // Set lights
        s_Data.SceneData.ActiveLight.Direction = { 0.0f, -1.0f, 0.0f };
        s_Data.SceneData.ActiveLight.Radiance = { 1.0f, 1.0f, 1.0f };
        s_Data.SceneData.ActiveLight.Multiplier = 0.5f;

        // Grid
        s_Data.GridMaterial = new Material(m_ShaderGrid);
        m_ShaderGrid->Bind();
        m_ShaderGrid->setFloat("u_Scale", m_GridScale);
        m_ShaderGrid->setFloat("u_Res", m_GridSize);

        s_Data.SceneData.SkyboxMaterial = new Material(m_ShaderSkybox);
        s_Data.SceneData.SkyboxMaterial->SetFlag(MaterialFlag::DepthTest, true); // false
    }

    void SceneRenderer::SetupShaders()
    {
        s_Data.CompositeShader = new Shader("Shaders/Hazel/SceneComposite.vs", "Shaders/Hazel/SceneComposite.fs");
        Log::GetLogger()->info("SceneRenderer: m_ShaderComposite compiled [programID={0}]", s_Data.CompositeShader->GetProgramID());

        m_ShaderEquirectangularConversion = new Shader("Shaders/Hazel/EquirectangularToCubeMap.cs");
        Log::GetLogger()->info("SceneRenderer: m_ShaderEquirectangularConversion compiled [programID={0}]", m_ShaderEquirectangularConversion->GetProgramID());

        m_ShaderEnvFiltering = new Shader("Shaders/Hazel/EnvironmentMipFilter.cs");
        Log::GetLogger()->info("SceneRenderer: m_ShaderEnvFiltering compiled [programID={0}]", m_ShaderEnvFiltering->GetProgramID());

        m_ShaderEnvIrradiance = new Shader("Shaders/Hazel/EnvironmentIrradiance.cs");
        Log::GetLogger()->info("SceneRenderer: m_ShaderEnvIrradiance compiled [programID={0}]", m_ShaderEnvIrradiance->GetProgramID());

        m_ShaderSkybox = new Shader("Shaders/Hazel/Skybox.vs", "Shaders/Hazel/Skybox.fs");
        Log::GetLogger()->info("SceneRenderer: m_ShaderSkybox compiled [programID={0}]", m_ShaderSkybox->GetProgramID());

        m_ShaderGrid = new Shader("Shaders/Hazel/Grid.vs", "Shaders/Hazel/Grid.fs");
        Log::GetLogger()->info("SceneRenderer: m_ShaderGrid compiled [programID={0}]", m_ShaderGrid->GetProgramID());

        ResourceManager::AddShader("Hazel/SceneComposite", s_Data.CompositeShader);
        ResourceManager::AddShader("Hazel/EquirectangularToCubeMap", m_ShaderEquirectangularConversion);
        ResourceManager::AddShader("Hazel/EnvironmentMipFilter", m_ShaderEnvFiltering);
        ResourceManager::AddShader("Hazel/EnvironmentIrradiance", m_ShaderEnvIrradiance);
        ResourceManager::AddShader("Hazel/Skybox", m_ShaderSkybox);
        ResourceManager::AddShader("Hazel/Grid", m_ShaderGrid);
    }

    // Moved from EnvironmentMap
    Environment SceneRenderer::Load(const std::string& filepath)
    {
        auto [radiance, irradiance] = CreateEnvironmentMap(filepath);
        return { radiance, irradiance };
    }

    void SceneRenderer::SetViewportSize(uint32_t width, uint32_t height)
    {
        s_Data.GeoPass->GetSpecification().TargetFramebuffer->Resize(width, height);
        s_Data.CompositePass->GetSpecification().TargetFramebuffer->Resize(width, height);
    }

    void SceneRenderer::BeginScene(HazelScene* scene)
    {
        HZ_CORE_ASSERT(!s_Data.ActiveScene, "");

        s_Data.ActiveScene = scene;

        s_Data.SceneData.SceneCamera = ((Scene*)scene)->GetCamera();
        // s_Data.SceneData.SkyboxMaterial = ((Scene*)scene)->GetSkyboxMaterial();
        // s_Data.SceneData.SceneEnvironment = ((Scene*)scene)->GetEnvironment();
        // s_Data.SceneData.ActiveLight = ((Scene*)scene)->GetLight();
    }

    void SceneRenderer::EndScene()
    {
        HZ_CORE_ASSERT(s_Data.ActiveScene, "");

        // s_Data.ActiveScene = nullptr;

        FlushDrawList();
    }

    void SceneRenderer::SubmitEntity(Entity entity)
    {
        // TODO: Culling, sorting, etc.

        auto mesh = entity.GetMesh();
        if (!mesh) {
            return;
        }

        // TODO: s_Data.DrawList.push_back({ mesh, entity->GetMaterial(), entity->GetTransform() });
    }

    static Ref<HazelShader> equirectangularConversionShader, envFilteringShader, envIrradianceShader;

    // Moved from EnvironmentMap
    void SceneRenderer::SetEnvironment(Hazel::Environment environment)
    {
        s_Data.SceneData.SceneEnvironment = environment;
    }

    // Moved from EnvironmentMap
    std::pair<Hazel::HazelTextureCube*, Hazel::HazelTextureCube*> SceneRenderer::CreateEnvironmentMap(const std::string& filepath)
    {
        const uint32_t cubemapSize = 512;
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

    void SceneRenderer::CompositePass()
    {
        HazelRenderer::BeginRenderPass(s_Data.CompositePass, false); // should we clear the framebuffer at this stage?

        s_Data.CompositeShader->Bind();

        s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->Bind(m_SamplerSlots->at("u_Texture"));
        s_Data.CompositeShader->setInt("u_Texture", m_SamplerSlots->at("u_Texture"));
        s_Data.CompositeShader->setFloat("u_Exposure", s_Data.SceneData.SceneCamera->GetExposure());
        s_Data.CompositeShader->setInt("u_TextureSamples", s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetSpecification().Samples);

        HazelRenderer::SubmitFullscreenQuad(nullptr);

        HazelRenderer::EndRenderPass();
    }

    void SceneRenderer::FlushDrawList()
    {
        HZ_CORE_ASSERT(!s_Data.ActiveScene, "");

        if (!s_Data.ActiveScene) {
            Log::GetLogger()->error("Active scene is not specified!");
        }

        GeometryPass();
        // Log::GetLogger()->debug("EnvironmentMap::FlushDrawList GeometryPass executed...");
        CompositePass();
        // Log::GetLogger()->debug("EnvironmentMap::FlushDrawList CompositePass executed...");

        // m_Data.DrawList.clear(); // TODO: make DrawList update every tick
        //  m_Data.SceneData = {};   // TODO: make SceneData update every tick
    }

    void SceneRenderer::GeometryPass()
    {
        HZ_ASSERT(false, "Method not yet implemented!");
    }

    SceneRendererOptions& SceneRenderer::GetOptions()
    {
        return s_Data.Options;
    }

    Ref<RenderPass> SceneRenderer::GetFinalRenderPass()
    {
        return s_Data.CompositePass;
    }

    FramebufferTexture* SceneRenderer::GetFinalColorBuffer()
    {
        return s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor();
    }

    uint32_t SceneRenderer::GetFinalColorBufferID()
    {
        return (uint32_t)s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetID();
    }

}
