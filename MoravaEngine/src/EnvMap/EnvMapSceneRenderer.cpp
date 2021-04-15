#include "EnvMapSceneRenderer.h"

#include "Hazel/Core/Assert.h"
#include "Hazel/Platform/OpenGL/OpenGLRenderPass.h"
#include "Hazel/Renderer/HazelRenderer.h"
#include "Hazel/Renderer/HazelShader.h"
#include "Hazel/Scene/HazelScene.h"

#include "Core/Application.h"
#include "Core/Log.h"
#include "Core/ResourceManager.h"
#include "EnvMap/EnvMapEditorLayer.h"
#include "Material/MaterialLibrary.h"
#include "Renderer/RendererBasic.h"
#include "Shader/ShaderLibrary.h"


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
        Mesh* MeshPtr;
        Material* MaterialPtr;
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

SceneRendererCamera& EnvMapSceneRenderer::GetCamera()
{
    return s_Data.SceneData.SceneCamera;
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

void EnvMapSceneRenderer::RenderSkybox()
{
    RendererBasic::DisableCulling();
    RendererBasic::DisableDepthTest();

    // render skybox (render as last to prevent overdraw)
    EnvMapSceneRenderer::s_ShaderSkybox->Bind();

    EnvMapSceneRenderer::GetRadianceMap()->Bind(EnvMapSharedData::s_SamplerSlots.at("u_Texture"));

    glm::mat4 viewProj = GetViewProjection();
    EnvMapSceneRenderer::s_ShaderSkybox->setMat4("u_InverseVP", glm::inverse(viewProj));

    s_ShaderSkybox->setInt("u_Texture", EnvMapSharedData::s_SamplerSlots.at("u_Texture"));
    s_ShaderSkybox->setFloat("u_TextureLod", EnvMapSharedData::s_EditorScene->GetSkyboxLod());
    s_ShaderSkybox->setFloat("u_Exposure", EnvMapEditorLayer::GetMainCameraComponent().Camera.GetExposure() * EnvMapSharedData::s_SkyboxExposureFactor); // originally used in Shaders/Hazel/SceneComposite

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
    s_ShaderGrid->setFloat("u_Scale", EnvMapSceneRenderer::s_GridScale);
    s_ShaderGrid->setFloat("u_Res", EnvMapSceneRenderer::s_GridSize);

    glm::mat4 viewProj = GetViewProjection();
    s_ShaderGrid->setMat4("u_ViewProjection", viewProj);

    bool depthTest = true;

    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::scale(transform, glm::vec3(16.0f, 1.0f, 16.0f));
    transform = glm::rotate(transform, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    s_ShaderGrid->setMat4("u_Transform", transform);

    EnvMapSharedData::s_Quad->Render();

    RendererBasic::EnableTransparency();
    RendererBasic::EnableMSAA();
}

void EnvMapSceneRenderer::RenderOutline(Hazel::Ref<Shader> shader, Hazel::Entity entity, const glm::mat4& entityTransform, Hazel::Submesh& submesh)
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

void EnvMapSceneRenderer::UpdateShaderPBRUniforms(Hazel::Ref<Shader> shaderHazelPBR, Hazel::Ref<EnvMapMaterial> envMapMaterial)
{
    /**** BEGIN Shaders/Hazel/HazelPBR_Anim / Shaders/Hazel/HazelPBR_Static ***/

    shaderHazelPBR->Bind();

    shaderHazelPBR->setInt("u_AlbedoTexture", EnvMapSharedData::s_SamplerSlots.at("albedo"));
    shaderHazelPBR->setInt("u_NormalTexture", EnvMapSharedData::s_SamplerSlots.at("normal"));
    shaderHazelPBR->setInt("u_MetalnessTexture", EnvMapSharedData::s_SamplerSlots.at("metalness"));
    shaderHazelPBR->setInt("u_RoughnessTexture", EnvMapSharedData::s_SamplerSlots.at("roughness"));
    shaderHazelPBR->setInt("u_EmissiveTexture", EnvMapSharedData::s_SamplerSlots.at("emissive"));
    shaderHazelPBR->setInt("u_AOTexture", EnvMapSharedData::s_SamplerSlots.at("ao"));

    shaderHazelPBR->setVec3("u_MaterialUniforms.AlbedoColor", envMapMaterial->GetAlbedoInput().Color);
    shaderHazelPBR->setFloat("u_MaterialUniforms.Metalness", envMapMaterial->GetMetalnessInput().Value);
    shaderHazelPBR->setFloat("u_MaterialUniforms.Roughness", envMapMaterial->GetRoughnessInput().Value);
    shaderHazelPBR->setFloat("u_MaterialUniforms.Emissive", envMapMaterial->GetEmissiveInput().Value);
    shaderHazelPBR->setFloat("u_MaterialUniforms.AO", envMapMaterial->GetAOInput().Value);

    shaderHazelPBR->setFloat("u_MaterialUniforms.EnvMapRotation", EnvMapSharedData::s_EnvMapRotation);

    shaderHazelPBR->setFloat("u_MaterialUniforms.RadiancePrefilter", EnvMapSharedData::s_RadiancePrefilter ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_MaterialUniforms.AlbedoTexToggle", envMapMaterial->GetAlbedoInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_MaterialUniforms.NormalTexToggle", envMapMaterial->GetNormalInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_MaterialUniforms.MetalnessTexToggle", envMapMaterial->GetMetalnessInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_MaterialUniforms.RoughnessTexToggle", envMapMaterial->GetRoughnessInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_MaterialUniforms.EmissiveTexToggle", envMapMaterial->GetEmissiveInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_MaterialUniforms.AOTexToggle", envMapMaterial->GetAOInput().UseTexture ? 1.0f : 0.0f);

    // apply exposure to Shaders/Hazel/HazelPBR_Anim, considering that Shaders/Hazel/SceneComposite is not yet enabled
    shaderHazelPBR->setFloat("u_Exposure", EnvMapEditorLayer::GetMainCameraComponent().Camera.GetExposure()); // originally used in Shaders/Hazel/SceneComposite

    shaderHazelPBR->setFloat("u_TilingFactor", envMapMaterial->GetTilingFactor());

    glm::mat4 viewProj = GetViewProjection();
    shaderHazelPBR->setMat4("u_ViewProjectionMatrix", viewProj);
    shaderHazelPBR->setVec3("u_CameraPosition", EnvMapSharedData::s_ActiveCamera->GetPosition());
    shaderHazelPBR->setMat4("u_DirLightTransform", EnvMapSharedData::s_DirLightTransform);

    // Environment (TODO: don't do this per mesh)
    shaderHazelPBR->setInt("u_EnvRadianceTex", EnvMapSharedData::s_SamplerSlots.at("radiance"));
    shaderHazelPBR->setInt("u_EnvIrradianceTex", EnvMapSharedData::s_SamplerSlots.at("irradiance"));
    shaderHazelPBR->setInt("u_BRDFLUTTexture", EnvMapSharedData::s_SamplerSlots.at("BRDF_LUT"));

    // Set lights (TODO: move to light environment and don't do per mesh)
    shaderHazelPBR->setVec3("lights.Direction", EnvMapSceneRenderer::GetActiveLight().Direction);
    shaderHazelPBR->setVec3("lights.Radiance", EnvMapSceneRenderer::GetActiveLight().Radiance);
    shaderHazelPBR->setFloat("lights.Multiplier", EnvMapSceneRenderer::GetActiveLight().Multiplier);

    shaderHazelPBR->setInt("pointLightCount", 1);
    shaderHazelPBR->setInt("spotLightCount", 1);

    // Point lights / Omni directional shadows
    if (EnvMapSharedData::s_PointLightEntity.HasComponent<Hazel::PointLightComponent>())
    {
        auto& plc = EnvMapSharedData::s_PointLightEntity.GetComponent<Hazel::PointLightComponent>();
        auto& tc = EnvMapSharedData::s_PointLightEntity.GetComponent<Hazel::TransformComponent>();
        shaderHazelPBR->setBool("pointLights[0].base.enabled", plc.Enabled);
        shaderHazelPBR->setVec3("pointLights[0].base.color", plc.Color);
        shaderHazelPBR->setFloat("pointLights[0].base.ambientIntensity", plc.AmbientIntensity);
        shaderHazelPBR->setFloat("pointLights[0].base.diffuseIntensity", plc.DiffuseIntensity);
        shaderHazelPBR->setVec3("pointLights[0].position", tc.Translation);
        shaderHazelPBR->setFloat("pointLights[0].constant", plc.Constant);
        shaderHazelPBR->setFloat("pointLights[0].linear", plc.Linear);
        shaderHazelPBR->setFloat("pointLights[0].exponent", plc.Exponent);
    }

    // Spot lights / Omni directional shadows
    if (EnvMapSharedData::s_SpotLightEntity.HasComponent<Hazel::SpotLightComponent>())
    {
        auto& slc = EnvMapSharedData::s_SpotLightEntity.GetComponent<Hazel::SpotLightComponent>();
        auto& tc = EnvMapSharedData::s_SpotLightEntity.GetComponent<Hazel::TransformComponent>();
        shaderHazelPBR->setBool("spotLights[0].base.base.enabled", slc.Enabled);
        shaderHazelPBR->setVec3("spotLights[0].base.base.color", slc.Color);
        shaderHazelPBR->setFloat("spotLights[0].base.base.ambientIntensity", slc.AmbientIntensity);
        shaderHazelPBR->setFloat("spotLights[0].base.base.diffuseIntensity", slc.DiffuseIntensity);
        shaderHazelPBR->setVec3("spotLights[0].base.position", tc.Translation);
        shaderHazelPBR->setFloat("spotLights[0].base.constant", slc.Constant);
        shaderHazelPBR->setFloat("spotLights[0].base.linear", slc.Linear);
        shaderHazelPBR->setFloat("spotLights[0].base.exponent", slc.Exponent);
        shaderHazelPBR->setVec3("spotLights[0].direction", tc.Rotation);
        shaderHazelPBR->setFloat("spotLights[0].edge", slc.Edge);
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

                EnvMapSharedData::s_ShaderHazelPBR = meshComponent.Mesh->IsAnimated() ? ShaderLibrary::Get("HazelPBR_Anim") : ShaderLibrary::Get("HazelPBR_Static");

                EnvMapSharedData::s_ShaderHazelPBR->Bind();

                EnvMapSharedData::s_ShadowMapDirLight->ReadTexture(EnvMapSharedData::s_SamplerSlots.at("shadow"));
                EnvMapSharedData::s_ShaderHazelPBR->setInt("u_ShadowMap", EnvMapSharedData::s_SamplerSlots.at("shadow"));

                {
                    EnvMapSharedData::s_OmniShadowMapPointLight->ReadTexture(EnvMapSharedData::s_SamplerSlots.at("shadow_omni"));
                    EnvMapSharedData::s_ShaderHazelPBR->setInt("u_OmniShadowMaps[0].shadowMap", EnvMapSharedData::s_SamplerSlots.at("shadow_omni"));

                    float farPlane = 1000.0f;
                    if (EnvMapSharedData::s_PointLightEntity.HasComponent<Hazel::PointLightComponent>()) {
                        farPlane = EnvMapSharedData::s_PointLightEntity.GetComponent<Hazel::PointLightComponent>().FarPlane;
                    }
                    EnvMapSharedData::s_ShaderHazelPBR->setFloat("u_OmniShadowMaps[0].farPlane", farPlane);
                }

                {
                    EnvMapSharedData::s_OmniShadowMapSpotLight->ReadTexture(EnvMapSharedData::s_SamplerSlots.at("shadow_omni") + 1);
                    EnvMapSharedData::s_ShaderHazelPBR->setInt("u_OmniShadowMaps[1].shadowMap", EnvMapSharedData::s_SamplerSlots.at("shadow_omni") + 1);

                    float farPlane = 1000.0f;
                    if (EnvMapSharedData::s_SpotLightEntity.HasComponent<Hazel::SpotLightComponent>()) {
                        farPlane = EnvMapSharedData::s_SpotLightEntity.GetComponent<Hazel::SpotLightComponent>().FarPlane;
                    }
                    EnvMapSharedData::s_ShaderHazelPBR->setFloat("u_OmniShadowMaps[1].farPlane", farPlane);
                }

                Hazel::Ref<EnvMapMaterial> envMapMaterial = Hazel::Ref<EnvMapMaterial>();
                std::string materialUUID;

                for (Hazel::Submesh& submesh : meshComponent.Mesh->GetSubmeshes())
                {
                    materialUUID = MaterialLibrary::GetSubmeshMaterialUUID(meshComponent.Mesh.Raw(), submesh, &entity);

                    RenderOutline(EnvMapSharedData::s_ShaderOutline, entity, entityTransform, submesh);

                    // Render Submesh
                    // load submesh materials for each specific submesh from the s_EnvMapMaterials list
                    if (MaterialLibrary::s_EnvMapMaterials.find(materialUUID) != MaterialLibrary::s_EnvMapMaterials.end()) {
                        envMapMaterial = MaterialLibrary::s_EnvMapMaterials.at(materialUUID);
                        UpdateShaderPBRUniforms(EnvMapSharedData::s_ShaderHazelPBR, envMapMaterial);
                    }

                    submesh.Render(meshComponent.Mesh, EnvMapSharedData::s_ShaderHazelPBR, entityTransform, samplerSlot, MaterialLibrary::s_EnvMapMaterials, entity);
                }
            }
        }
    }

    Hazel::Renderer2D::BeginScene(viewProj, true);
    {
        // RendererBasic::SetLineThickness(2.0f);

        if (EnvMapSharedData::s_DisplayRay)
        {
            glm::vec3 camPosition = EnvMapSharedData::s_ActiveCamera->GetPosition();
            Hazel::Renderer2D::DrawLine(EnvMapSharedData::s_NewRay, EnvMapSharedData::s_NewRay + glm::vec3(1.0f, 0.0f, 0.0f) * 100.0f, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
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
    Hazel::Renderer2D::EndScene();

    GetGeoPass()->GetSpecification().TargetFramebuffer->Bind();
}

void EnvMapSceneRenderer::CompositePass()
{
    // Hazel::HazelRenderer::BeginRenderPass(s_Data.CompositePass, false); // should we clear the framebuffer at this stage?

    s_Data.CompositeShader->Bind();

    s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->Bind(EnvMapSharedData::s_SamplerSlots.at("u_Texture"));
    s_Data.CompositeShader->setInt("u_Texture", EnvMapSharedData::s_SamplerSlots.at("u_Texture"));
    s_Data.CompositeShader->setFloat("u_Exposure", EnvMapEditorLayer::GetMainCameraComponent().Camera.GetExposure()); // s_Data.SceneData.SceneCamera.Camera
    s_Data.CompositeShader->setInt("u_TextureSamples", s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetSpecification().Samples);

    // Hazel::HazelRenderer::SubmitFullscreenQuad(Hazel::Ref<Hazel::HazelMaterial>());

    // Hazel::HazelRenderer::EndRenderPass();
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
    drawCommand.MeshPtr = mesh;
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
