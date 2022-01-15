#include "VulkanSceneRenderer.h"

#include "H2M/Core/AssertH2M.h"
#include "H2M/Platform/OpenGL/OpenGLRenderPassH2M.h"
#include "H2M/Renderer/RendererH2M.h"
#include "H2M/Renderer/Renderer2D_H2M.h"
#include "H2M/Renderer/ShaderH2M.h"
#include "H2M/Scene/SceneH2M.h"

#include "Core/Application.h"
#include "Core/Log.h"
#include "Core/ResourceManager.h"
#include "EnvMap/EnvMapEditorLayer.h"
#include "Material/MaterialLibrary.h"
#include "Renderer/RendererBasic.h"
#include "Shader/MoravaShaderLibrary.h"


H2M::RefH2M<H2M::ShaderH2M> VulkanSceneRenderer::s_ShaderEquirectangularConversion;
H2M::RefH2M<H2M::ShaderH2M> VulkanSceneRenderer::s_ShaderEnvFiltering;
H2M::RefH2M<H2M::ShaderH2M> VulkanSceneRenderer::s_ShaderEnvIrradiance;
H2M::RefH2M<H2M::ShaderH2M> VulkanSceneRenderer::s_ShaderGrid;
H2M::RefH2M<H2M::ShaderH2M> VulkanSceneRenderer::s_ShaderSkybox;
H2M::RefH2M<H2M::TextureCubeH2M> VulkanSceneRenderer::s_EnvUnfiltered;
H2M::RefH2M<H2M::Texture2D_H2M> VulkanSceneRenderer::s_EnvEquirect;
H2M::RefH2M<H2M::TextureCubeH2M> VulkanSceneRenderer::s_EnvFiltered;
H2M::RefH2M<H2M::TextureCubeH2M> VulkanSceneRenderer::s_IrradianceMap;
float VulkanSceneRenderer::s_GridScale = 16.025f;
float VulkanSceneRenderer::s_GridSize = 0.025f;
uint32_t VulkanSceneRenderer::s_FramebufferWidth = 1280;
uint32_t VulkanSceneRenderer::s_FramebufferHeight = 720;


struct VulkanSceneRendererData
{
    const ::H2M::SceneH2M* ActiveScene = nullptr;
    struct SceneInfo
    {
        H2M::SceneRendererCameraH2M SceneCamera;

        // Resources
        H2M::RefH2M<H2M::MaterialH2M> HazelSkyboxMaterial;
        Material* SkyboxMaterial;
        H2M::EnvironmentH2M SceneEnvironment;
        H2M::LightH2M ActiveLight;
    } SceneData;

    H2M::RefH2M<H2M::Texture2D_H2M> BRDFLUT;

    H2M::RefH2M<H2M::ShaderH2M> CompositeShader;

    H2M::RefH2M<H2M::RenderPassH2M> GeoPass;
    H2M::RefH2M<H2M::RenderPassH2M> CompositePass;
    H2M::RefH2M<H2M::RenderPassH2M> ActiveRenderPass;

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
    H2M::RefH2M<H2M::MaterialH2M> OutlineMaterial;

    H2M::SceneRendererOptionsH2M Options;

    // Renderer data
    H2M::RenderCommandQueueH2M* m_CommandQueue;
};

static VulkanSceneRendererData s_Data;

void VulkanSceneRenderer::Init(std::string filepath, H2M::SceneH2M* scene)
{
    SetupShaders();

    s_Data.SceneData.SceneEnvironment = Load(filepath);
    SetEnvironment(s_Data.SceneData.SceneEnvironment);

    // Set lights
    s_Data.SceneData.ActiveLight.Direction = { 0.0f, -1.0f, 0.0f };
    s_Data.SceneData.ActiveLight.Radiance = { 1.0f, 1.0f, 1.0f };
    s_Data.SceneData.ActiveLight.Multiplier = 0.5f;

    // Grid
    // s_Data.OutlineMaterial = H2M::RefH2M<HazelMaterial>::Create(s_ShaderGrid);
    s_Data.GridMaterial = new Material(s_ShaderGrid);
    s_ShaderGrid->Bind();
    s_ShaderGrid->SetFloat("u_Scale", s_GridScale);
    s_ShaderGrid->SetFloat("u_Res", s_GridSize);

    s_Data.SceneData.SkyboxMaterial = new Material(s_ShaderSkybox);
    s_Data.SceneData.SkyboxMaterial->SetFlag(MaterialFlag::DepthTest, true); // false

    // s_Data.ActiveScene->m_ViewportWidth = Application::Get()->GetWindow()->GetWidth();
    // s_Data.ActiveScene->m_ViewportHeight = Application::Get()->GetWindow()->GetHeight();

    bool isMultisample = false;

    H2M::FramebufferSpecificationH2M geoFramebufferSpec;
    geoFramebufferSpec.Width = s_FramebufferWidth;
    geoFramebufferSpec.Height = s_FramebufferHeight;
    // geoFramebufferSpec.attachmentType = AttachmentType::Texture;
    // geoFramebufferSpec.attachmentFormat = AttachmentFormat::RGBA16F;
    geoFramebufferSpec.Samples = 8;
    geoFramebufferSpec.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

    isMultisample = geoFramebufferSpec.Samples > 1;

    H2M::FramebufferSpecificationH2M geoFramebufferDepthSpec;
    geoFramebufferDepthSpec = geoFramebufferSpec;
    // geoFramebufferDepthSpec.attachmentType = AttachmentType::Texture;
    // geoFramebufferDepthSpec.attachmentFormat = AttachmentFormat::Depth_24_Stencil_8;

    H2M::RenderPassSpecificationH2M geoRenderPassSpec;
    geoRenderPassSpec.TargetFramebuffer = H2M::FramebufferH2M::Create(geoFramebufferSpec);

    auto targetFramebufferGeo = static_cast<H2M::RefH2M<H2M::FramebufferH2M>>(geoRenderPassSpec.TargetFramebuffer);

    // targetFramebufferGeo->AddColorAttachment(geoFramebufferSpec);
    // targetFramebufferGeo->AddDepthAttachment(geoFramebufferDepthSpec);
    Log::GetLogger()->debug("Generating the GEO RenderPass framebuffer with AttachmentFormat::RGBA16F");

    // targetFramebufferGeo->Generate(geoFramebufferSpec.Width, geoFramebufferSpec.Height);

    s_Data.GeoPass = H2M::RenderPassH2M::Create(geoRenderPassSpec);

    H2M::FramebufferSpecificationH2M compFramebufferSpec;
    compFramebufferSpec.Width = s_FramebufferWidth;
    compFramebufferSpec.Height = s_FramebufferHeight;
    // compFramebufferSpec.attachmentType = AttachmentType::Texture;
    // compFramebufferSpec.attachmentFormat = AttachmentFormat::RGBA;
    compFramebufferSpec.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };

    isMultisample = compFramebufferSpec.Samples > 1;

    H2M::RenderPassSpecificationH2M compRenderPassSpec;
    compRenderPassSpec.TargetFramebuffer = H2M::FramebufferH2M::Create(compFramebufferSpec);

    auto targetFramebufferComp = static_cast<H2M::RefH2M<H2M::FramebufferH2M>>(compRenderPassSpec.TargetFramebuffer);

    // targetFramebufferComp->AddColorAttachment(compFramebufferSpec);

    H2M::FramebufferSpecificationH2M compFramebufferDepthSpec;
    compFramebufferDepthSpec = compFramebufferSpec;
    // compFramebufferDepthSpec.attachmentType = AttachmentType::Renderbuffer;
    // compFramebufferDepthSpec.attachmentFormat = AttachmentFormat::Depth;
    // targetFramebufferComp->AddDepthAttachment(compFramebufferDepthSpec);

    Log::GetLogger()->debug("Generating the COMPOSITE RenderPass framebuffer with AttachmentFormat::RGBA");
    // targetFramebufferComp->Generate(compFramebufferSpec.Width, compFramebufferSpec.Height);
    s_Data.CompositePass = H2M::RenderPassH2M::Create(compRenderPassSpec);

    s_Data.BRDFLUT = H2M::Texture2D_H2M::Create("Textures/Hazel/BRDF_LUT.tga", false);
}

void VulkanSceneRenderer::SetupShaders()
{
    s_Data.CompositeShader = H2M::ShaderH2M::Create("Shaders/Hazel/SceneComposite.vs", "Shaders/Hazel/SceneComposite.fs");
    Log::GetLogger()->info("VulkanSceneRenderer: s_ShaderComposite compiled [programID={0}]", s_Data.CompositeShader->GetRendererID());

    s_ShaderEquirectangularConversion = H2M::ShaderH2M::Create("Shaders/Hazel/EquirectangularToCubeMap.cs");
    Log::GetLogger()->info("VulkanSceneRenderer: s_ShaderEquirectangularConversion compiled [programID={0}]", s_ShaderEquirectangularConversion->GetRendererID());

    s_ShaderEnvFiltering = H2M::ShaderH2M::Create("Shaders/Hazel/EnvironmentMipFilter.cs");
    Log::GetLogger()->info("VulkanSceneRenderer: s_ShaderEnvFiltering compiled [programID={0}]", s_ShaderEnvFiltering->GetRendererID());

    s_ShaderEnvIrradiance = H2M::ShaderH2M::Create("Shaders/Hazel/EnvironmentIrradiance.cs");
    Log::GetLogger()->info("VulkanSceneRenderer: s_ShaderEnvIrradiance compiled [programID={0}]", s_ShaderEnvIrradiance->GetRendererID());

    s_ShaderSkybox = H2M::ShaderH2M::Create("Shaders/Hazel/Skybox.vs", "Shaders/Hazel/Skybox.fs");
    Log::GetLogger()->info("VulkanSceneRenderer: s_ShaderSkybox compiled [programID={0}]", s_ShaderSkybox->GetRendererID());

    s_ShaderGrid = H2M::ShaderH2M::Create("Shaders/Hazel/Grid.vs", "Shaders/Hazel/Grid.fs");
    Log::GetLogger()->info("VulkanSceneRenderer: s_ShaderGrid compiled [programID={0}]", s_ShaderGrid->GetRendererID());

    ResourceManager::AddShader("Hazel/SceneComposite", s_Data.CompositeShader);
    ResourceManager::AddShader("Hazel/EquirectangularToCubeMap", s_ShaderEquirectangularConversion.Raw());
    ResourceManager::AddShader("Hazel/EnvironmentMipFilter", s_ShaderEnvFiltering.Raw());
    ResourceManager::AddShader("Hazel/EnvironmentIrradiance", s_ShaderEnvIrradiance.Raw());
    ResourceManager::AddShader("Hazel/Skybox", s_ShaderSkybox.Raw());
    ResourceManager::AddShader("Hazel/Grid", s_ShaderGrid.Raw());
}

// Moved from EnvironmentMap
H2M::EnvironmentH2M VulkanSceneRenderer::Load(const std::string& filepath)
{
    auto [radiance, irradiance] = CreateEnvironmentMap(filepath);
    return { radiance, irradiance };
}

void VulkanSceneRenderer::SetViewportSize(uint32_t width, uint32_t height)
{
    s_Data.GeoPass->GetSpecification().TargetFramebuffer->Resize(width, height, true);
    s_Data.CompositePass->GetSpecification().TargetFramebuffer->Resize(width, height, true);
}

void VulkanSceneRenderer::BeginScene(H2M::SceneH2M* scene, const H2M::SceneRendererCameraH2M& camera)
{
    // HZ_CORE_ASSERT(!s_Data.ActiveScene, "");

    s_Data.ActiveScene = scene;

    s_Data.SceneData.SceneCamera = camera;
}

void VulkanSceneRenderer::EndScene()
{
    H2M_CORE_ASSERT(s_Data.ActiveScene, "");

    s_Data.ActiveScene = nullptr;

    FlushDrawList();
}

void VulkanSceneRenderer::SubmitMesh(H2M::RefH2M<H2M::MeshH2M> mesh, const glm::mat4& transform, H2M::RefH2M<H2M::MaterialH2M> overrideMaterial)
{
}

void VulkanSceneRenderer::SubmitSelectedMesh(H2M::RefH2M<H2M::MeshH2M> mesh, const glm::mat4& transform)
{
}

void VulkanSceneRenderer::SubmitEntity(H2M::EntityH2M entity)
{
    // TODO: Culling, sorting, etc.

    auto& mesh = entity.GetComponent<H2M::MeshComponentH2M>().Mesh;
    if (!mesh) {
        return;
    }

    // TODO: s_Data.DrawList.push_back({ mesh, entity->GetMaterial(), entity->GetTransform() });
}

H2M::SceneRendererCameraH2M& VulkanSceneRenderer::GetCamera()
{
    return s_Data.SceneData.SceneCamera;
}

static H2M::RefH2M<H2M::ShaderH2M> equirectangularConversionShader, envFilteringShader, envIrradianceShader;

// Moved from EnvironmentMap
void VulkanSceneRenderer::SetEnvironment(H2M::EnvironmentH2M environment)
{
    s_Data.SceneData.SceneEnvironment = environment;
}

// Moved from EnvironmentMap
std::pair<H2M::RefH2M<H2M::TextureCubeH2M>, H2M::RefH2M<H2M::TextureCubeH2M>> VulkanSceneRenderer::CreateEnvironmentMap(const std::string& filepath)
{
    const uint32_t cubemapSize = 512;
    const uint32_t irradianceMapSize = 32;

    s_EnvUnfiltered = H2M::TextureCubeH2M::Create(H2M::ImageFormatH2M::RGBA16F, cubemapSize, cubemapSize, true);
    s_EnvEquirect = H2M::Texture2D_H2M::Create(filepath, false);

    if (s_EnvEquirect->GetFormat() != H2M::ImageFormatH2M::RGBA16F) {
        Log::GetLogger()->error("Texture is not HDR!");
    }

    s_ShaderEquirectangularConversion->Bind();
    s_EnvEquirect->Bind();

    glBindImageTexture(0, s_EnvUnfiltered->GetID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glDispatchCompute(cubemapSize / 32, cubemapSize / 32, 6);
    glGenerateTextureMipmap(s_EnvUnfiltered->GetID());

    s_EnvFiltered = H2M::TextureCubeH2M::Create(H2M::ImageFormatH2M::RGBA16F, cubemapSize, cubemapSize, true);
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
        glProgramUniform1f(s_ShaderEnvFiltering->GetRendererID(), 0, level * deltaRoughness);
        glDispatchCompute(numGroups, numGroups, 6);
        Log::GetLogger()->debug("END EnvFiltering size {0} numGroups {1} level {2}/{3}", size, numGroups, level, s_EnvFiltered->GetMipLevelCount());
    }

    s_IrradianceMap = H2M::TextureCubeH2M::Create(H2M::ImageFormatH2M::RGBA16F, irradianceMapSize, irradianceMapSize, true);
    s_ShaderEnvIrradiance->Bind();
    s_EnvFiltered->Bind();
    glBindImageTexture(0, s_IrradianceMap->GetID(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glDispatchCompute(s_IrradianceMap->GetWidth() / 32, s_IrradianceMap->GetHeight() / 32, 6);
    glGenerateTextureMipmap(s_IrradianceMap->GetID());

    return { s_EnvFiltered, s_IrradianceMap };
}

void VulkanSceneRenderer::RenderSkybox()
{
    RendererBasic::DisableCulling();
    RendererBasic::DisableDepthTest();

    // render skybox (render as last to prevent overdraw)
    VulkanSceneRenderer::s_ShaderSkybox->Bind();

    VulkanSceneRenderer::GetRadianceMap()->Bind(EnvMapSharedData::s_SamplerSlots.at("u_Texture"));

    glm::mat4 viewProj = GetViewProjection();
    VulkanSceneRenderer::s_ShaderSkybox->SetMat4("u_InverseVP", glm::inverse(viewProj));

    s_ShaderSkybox->SetInt("u_Texture", EnvMapSharedData::s_SamplerSlots.at("u_Texture"));
    // s_ShaderSkybox->SetFloat("u_TextureLod", EnvMapSharedData::s_EditorScene->GetSkyboxLod());
    // s_ShaderSkybox->SetFloat("u_Exposure", EnvMapEditorLayer::GetMainCameraComponent().Camera.GetExposure() * EnvMapSharedData::s_SkyboxExposureFactor); // originally used in Shaders/Hazel/SceneComposite

    EnvMapSharedData::s_SkyboxCube->Render();

    // VulkanSceneRenderer::s_ShaderSkybox->Unbind();
}

void VulkanSceneRenderer::RenderHazelGrid()
{
    // Grid
    // -- Shaders/Hazel/Grid.vs
    // ---- uniform mat4 u_ViewProjection;
    // ---- uniform mat4 u_Transform;
    // -- Shaders/Hazel/Grid.fs
    // ---- uniform float u_Scale;
    // ---- uniform float u_Res;

    s_ShaderGrid->Bind();
    s_ShaderGrid->SetFloat("u_Scale", VulkanSceneRenderer::s_GridScale);
    s_ShaderGrid->SetFloat("u_Res", VulkanSceneRenderer::s_GridSize);

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

void VulkanSceneRenderer::RenderOutline(H2M::RefH2M<H2M::ShaderH2M> shader, H2M::EntityH2M entity, const glm::mat4& entityTransform, H2M::SubmeshH2M& submesh)
{
    if (!EnvMapSharedData::s_DisplayOutline) return;

    auto& meshComponent = entity.GetComponent<H2M::MeshComponentH2M>();

    // Render outline
    if (EntitySelection::s_SelectionContext.size()) {
        for (auto selection : EntitySelection::s_SelectionContext)
        {
            if (selection.Mesh /*&& &submesh == selection.Mesh*/) {
                submesh.RenderOutline(meshComponent.Mesh, shader, entityTransform, entity);
            }
        }
    }
}

void VulkanSceneRenderer::UpdateShaderPBRUniforms(H2M::RefH2M<H2M::ShaderH2M> shaderHazelPBR, H2M::RefH2M<EnvMapMaterial> envMapMaterial)
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

    shaderHazelPBR->SetFloat("u_MaterialUniforms.RadiancePrefilter", EnvMapSharedData::s_RadiancePrefilter ? 1.0f : 0.0f);
    shaderHazelPBR->SetFloat("u_MaterialUniforms.AlbedoTexToggle", envMapMaterial->GetAlbedoInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->SetFloat("u_MaterialUniforms.NormalTexToggle", envMapMaterial->GetNormalInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->SetFloat("u_MaterialUniforms.MetalnessTexToggle", envMapMaterial->GetMetalnessInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->SetFloat("u_MaterialUniforms.RoughnessTexToggle", envMapMaterial->GetRoughnessInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->SetFloat("u_MaterialUniforms.EmissiveTexToggle", envMapMaterial->GetEmissiveInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->SetFloat("u_MaterialUniforms.AOTexToggle", envMapMaterial->GetAOInput().UseTexture ? 1.0f : 0.0f);

    // apply exposure to Shaders/Hazel/HazelPBR_Anim, considering that Shaders/Hazel/SceneComposite is not yet enabled
    // shaderHazelPBR->SetFloat("u_Exposure", EnvMapEditorLayer::GetMainCameraComponent().Camera.GetExposure()); // originally used in Shaders/Hazel/SceneComposite

    shaderHazelPBR->SetFloat("u_TilingFactor", envMapMaterial->GetTilingFactor());

    glm::mat4 viewProj = GetViewProjection();
    shaderHazelPBR->SetMat4("u_ViewProjectionMatrix", viewProj);
    // shaderHazelPBR->SetFloat3("u_CameraPosition", EnvMapSharedData::s_ActiveCamera->GetPosition());
    shaderHazelPBR->SetMat4("u_DirLightTransform", EnvMapSharedData::s_DirLightTransform);

    // Environment (TODO: don't do this per mesh)
    shaderHazelPBR->SetInt("u_EnvRadianceTex", EnvMapSharedData::s_SamplerSlots.at("radiance"));
    shaderHazelPBR->SetInt("u_EnvIrradianceTex", EnvMapSharedData::s_SamplerSlots.at("irradiance"));
    shaderHazelPBR->SetInt("u_BRDFLUTTexture", EnvMapSharedData::s_SamplerSlots.at("BRDF_LUT"));

    // Set lights (TODO: move to light environment and don't do per mesh)
    shaderHazelPBR->SetFloat3("lights.Direction", VulkanSceneRenderer::GetActiveLight().Direction);
    shaderHazelPBR->SetFloat3("lights.Radiance", VulkanSceneRenderer::GetActiveLight().Radiance);
    shaderHazelPBR->SetFloat("lights.Multiplier", VulkanSceneRenderer::GetActiveLight().Multiplier);

    shaderHazelPBR->SetInt("pointLightCount", 1);
    shaderHazelPBR->SetInt("spotLightCount", 1);

    // Point lights / Omni directional shadows
    //  if (EnvMapSharedData::s_PointLightEntity.HasComponent<H2M::PointLightComponentH2M>/   ())
    //  {
    //      auto& plc = //    EnvMapSharedData::s_PointLightEntity.GetComponent<H2M::PointLightComponentH2M>/();
    //      auto& tc = // EnvMapSharedData::s_PointLightEntity.GetComponent<H2M::TransformComponentH2M>/();
    //      shaderHazelPBR->SetBool("pointLights[0].base.enabled", plc.Enabled);
    //      shaderHazelPBR->SetFloat3("pointLights[0].base.color", plc.Color);
    //      shaderHazelPBR->SetFloat("pointLights[0].base.ambientIntensity", //   plc.AmbientIntensity);
    //      shaderHazelPBR->SetFloat("pointLights[0].base.diffuseIntensity", //   plc.DiffuseIntensity);
    //      shaderHazelPBR->SetFloat3("pointLights[0].position", tc.Translation);
    //      shaderHazelPBR->SetFloat("pointLights[0].constant", plc.Constant);
    //      shaderHazelPBR->SetFloat("pointLights[0].linear", plc.Linear);
    //      shaderHazelPBR->SetFloat("pointLights[0].exponent", plc.Exponent);
    //  }

    // Spot lights / Omni directional shadows
    //  if (EnvMapSharedData::s_SpotLightEntity.HasComponent<H2M::SpotLightComponentH2M>())
    //  {
    //      // auto& slc = // EnvMapSharedData::s_SpotLightEntity.GetComponent<H2M::SpotLightComponentH2M>();
    //      // auto& tc = //  EnvMapSharedData::s_SpotLightEntity.GetComponent<H2M::TransformComponentH2M>();
    //      // shaderHazelPBR->SetBool("spotLights[0].base.base.enabled", slc.Enabled);
    //      shaderHazelPBR->SetFloat3("spotLights[0].base.base.color", slc.Color);
    //      shaderHazelPBR->SetFloat("spotLights[0].base.base.ambientIntensity", //   slc.AmbientIntensity);
    //      shaderHazelPBR->SetFloat("spotLights[0].base.base.diffuseIntensity", //   slc.DiffuseIntensity);
    //      shaderHazelPBR->SetFloat3("spotLights[0].base.position", tc.Translation);
    //      shaderHazelPBR->SetFloat("spotLights[0].base.constant", slc.Constant);
    //      shaderHazelPBR->SetFloat("spotLights[0].base.linear", slc.Linear);
    //      shaderHazelPBR->SetFloat("spotLights[0].base.exponent", slc.Exponent);
    //      shaderHazelPBR->SetFloat3("spotLights[0].direction", tc.Rotation);
    //      shaderHazelPBR->SetFloat("spotLights[0].edge", slc.Edge);
    //  }

    // shaderHazelPBR->Validate();
    // shaderHazelPBR->Unbind();

    /**** END Shaders/Hazel/HazelPBR_Anim / Shaders/Hazel/HazelPBR_Static ***/
}

glm::mat4 VulkanSceneRenderer::GetViewProjection()
{
    // glm::mat4 viewProjECS = EnvMapEditorLayer::GetMainCameraComponent().Camera.GetViewProjection();
    // glm::mat4 viewProj = EnvMapSharedData::s_ActiveCamera->GetViewProjection();
    // return viewProj;
    return glm::mat4(1.0f);
}

void VulkanSceneRenderer::GeometryPass()
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

    // auto meshEntities = EnvMapSharedData::s_EditorScene->GetAllEntitiesWith<H2M::MeshComponentH2M>();
    // auto meshEntities = m_SceneHierarchyPanel->GetContext()->GetAllEntitiesWith<H2M::MeshComponent>();

    // Render all entities with mesh component
    //  if (meshEntities.size())
    //  {
    //      for (auto entt : meshEntities)
    //      {
    //          H2M::EntityH2M entity = { entt, EnvMapSharedData::s_EditorScene.Raw() };
    //          auto& meshComponent = entity.GetComponent<H2M::MeshComponent>();
    //  
    //          if (meshComponent.Mesh)
    //          {
    //              glm::mat4 entityTransform = glm::mat4(1.0f);
    //              if (entity && entity.HasComponent<H2M::TransformComponent>()) {
    //                  entityTransform = entity.GetComponent<H2M::TransformComponent>//  ().GetTransform();
    //              }
    //  
    //              EnvMapSharedData::s_ShaderHazelPBR = meshComponent.Mesh->IsAnimated/  () ? /ShaderLibrary::Get("HazelPBR_Anim") : ShaderLibrary::Get/("HazelPBR_Static");
    //  
    //              EnvMapSharedData::s_ShaderHazelPBR->Bind();
    //  
    //              EnvMapSharedData::s_ShadowMapDirLight->ReadTexture//  (EnvMapSharedData::s_SamplerSlots.at("shadow"));
    //              EnvMapSharedData::s_ShaderHazelPBR->SetInt("u_ShadowMap", //  EnvMapSharedData::s_SamplerSlots.at("shadow"));
    //  
    //              {
    //                  EnvMapSharedData::s_OmniShadowMapPointLight->ReadTexture//    (EnvMapSharedData::s_SamplerSlots.at("shadow_omni"));
    //                  EnvMapSharedData::s_ShaderHazelPBR->SetInt("u_OmniShadowMaps//    [0].shadowMap", EnvMapSharedData::s_SamplerSlots.at/("shadow_omni"));
    //  
    //                  float farPlane = 1000.0f;
    //                  if // (EnvMapSharedData::s_PointLightEntity.HasComponent<H2M::PointLightC/omponent>()) {
    //                      farPlane = // EnvMapSharedData::s_PointLightEntity.GetComponent<H2M::PointLightCo/mponent>/    ().FarPlane;
    //                  }
    //                  EnvMapSharedData::s_ShaderHazelPBR->SetFloat("u_OmniShadowMaps//  [0].farPlane", farPlane);
    //              }
    //  
    //              {
    //                  EnvMapSharedData::s_OmniShadowMapSpotLight->ReadTexture// (EnvMapSharedData::s_SamplerSlots.at("shadow_omni") + 1);
    //                  EnvMapSharedData::s_ShaderHazelPBR->SetInt("u_OmniShadowMaps//    [1].shadowMap", EnvMapSharedData::s_SamplerSlots.at("shadow_omni") /+ 1);
    //  
    //                  float farPlane = 1000.0f;
    //                  if // (EnvMapSharedData::s_SpotLightEntity.HasComponent<H2M::SpotLightCom/ponent>()) {
    //                      farPlane = // EnvMapSharedData::s_SpotLightEntity.GetComponent<H2M::SpotLightComp/onent>/  ().FarPlane;
    //                  }
    //                  EnvMapSharedData::s_ShaderHazelPBR->SetFloat("u_OmniShadowMaps//  [1].farPlane", farPlane);
    //              }
    //  
    //              H2M::RefH2M<EnvMapMaterial> envMapMaterial = /    H2M::RefH2M<EnvMapMaterial>/();
    //              std::string materialUUID;
    //  
    //              for (H2M::Submesh& submesh : meshComponent.Mesh->GetSubmeshes())
    //              {
    //                  materialUUID = MaterialLibrary::GetSubmeshMaterialUUID//  (meshComponent.Mesh.Raw(), submesh, &entity);
    //  
    //                  RenderOutline(EnvMapSharedData::s_ShaderOutline, entity, //   entityTransform, submesh);
    //  
    //                  // Render Submesh
    //                  // load submesh materials for each specific submesh from the //   s_EnvMapMaterials list
    //                  if (MaterialLibrary::s_EnvMapMaterials.find(materialUUID) != //   MaterialLibrary::s_EnvMapMaterials.end()) {
    //                      envMapMaterial = MaterialLibrary::s_EnvMapMaterials.at//  (materialUUID);
    //                      UpdateShaderPBRUniforms(EnvMapSharedData::s_ShaderHazelPBR, //    envMapMaterial);
    //                  }
    //  
    //                  submesh.Render(meshComponent.Mesh, // EnvMapSharedData::s_ShaderHazelPBR, entityTransform, samplerSlot, // MaterialLibrary::s_EnvMapMaterials, entity);
    //              }
    //          }
    //      }
    //  }

    H2M::Renderer2D_H2M::BeginScene(viewProj, true);
    {
        // RendererBasic::SetLineThickness(2.0f);

        if (EnvMapSharedData::s_DisplayRay)
        {
            // glm::vec3 camPosition = EnvMapSharedData::s_ActiveCamera->GetPosition();
            H2M::Renderer2D_H2M::DrawLine(EnvMapSharedData::s_NewRay, EnvMapSharedData::s_NewRay + glm::vec3(1.0f, 0.0f, 0.0f) * 100.0f, glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));
        }

        if (EntitySelection::s_SelectionContext.size()) {
            for (auto selection : EntitySelection::s_SelectionContext)
            {
                if (selection.Mesh) {
                    H2M::EntityH2M meshEntity = selection.Entity;
                    glm::mat4 transform = glm::mat4(1.0f);
                    if (meshEntity.HasComponent<H2M::TransformComponentH2M>()) {
                        transform = meshEntity.GetComponent<H2M::TransformComponentH2M>().GetTransform();
                    }
                    glm::vec4 color = EnvMapEditorLayer::s_SelectionMode == SelectionMode::Entity ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) : glm::vec4(0.2f, 0.9f, 0.2f, 1.0f);
                    H2M::RendererH2M::DrawAABB(selection.Mesh->BoundingBox, transform * selection.Mesh->Transform, color);
                }
            }
        }
    }
    H2M::Renderer2D_H2M::EndScene();

    GetGeoPass()->GetSpecification().TargetFramebuffer->Bind();
}

void VulkanSceneRenderer::CompositePass()
{
    // H2M::HazelRenderer::BeginRenderPass(s_Data.CompositePass, false); // should we clear the framebuffer at this stage?

    s_Data.CompositeShader->Bind();

    auto targetFramebuffer = static_cast<H2M::RefH2M<H2M::FramebufferH2M>>(s_Data.GeoPass->GetSpecification().TargetFramebuffer);

    // targetFramebuffer->GetTextureAttachmentColor()->Bind(EnvMapSharedData::s_SamplerSlots.at("u_Texture"));
    s_Data.CompositeShader->SetInt("u_Texture", EnvMapSharedData::s_SamplerSlots.at("u_Texture"));
    // s_Data.CompositeShader->SetFloat("u_Exposure", EnvMapEditorLayer::GetMainCameraComponent().Camera.GetExposure()); // s_Data.SceneData.SceneCamera.Camera
    s_Data.CompositeShader->SetInt("u_TextureSamples", s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetSpecification().Samples);

    // H2M::HazelRenderer::SubmitFullscreenQuad(H2M::RefH2M<H2M::HazelMaterial>());

    // H2M::HazelRenderer::EndRenderPass();
}

void VulkanSceneRenderer::SubmitEntityEnvMap(H2M::EntityH2M entity)
{
    auto mesh = entity.GetComponent<H2M::MeshComponentH2M>().Mesh;
    if (!mesh) {
        return;
    }

    auto transform = entity.GetComponent<H2M::TransformComponentH2M>().GetTransform();

    auto name = entity.GetComponent<H2M::TagComponentH2M>().Tag;
    AddToDrawList(name, mesh, entity, transform);
}

void VulkanSceneRenderer::FlushDrawList()
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

uint32_t VulkanSceneRenderer::GetFinalColorBufferRendererID()
{
    auto targetFramebuffer = static_cast<H2M::RefH2M<H2M::FramebufferH2M>>(s_Data.CompositePass->GetSpecification().TargetFramebuffer);

    // return targetFramebuffer->GetTextureAttachmentColor()->GetID();
    return uint32_t();
}

H2M::SceneRendererOptionsH2M& VulkanSceneRenderer::GetOptions()
{
    return s_Data.Options;
}

H2M::RefH2M<H2M::TextureCubeH2M> VulkanSceneRenderer::GetRadianceMap()
{
    return s_Data.SceneData.SceneEnvironment.RadianceMap;
}

H2M::RefH2M<H2M::TextureCubeH2M> VulkanSceneRenderer::GetIrradianceMap()
{
    return s_Data.SceneData.SceneEnvironment.IrradianceMap;
}

H2M::RefH2M<H2M::Texture2D_H2M> VulkanSceneRenderer::GetBRDFLUT()
{
    return s_Data.BRDFLUT;
}

H2M::RefH2M<H2M::ShaderH2M> VulkanSceneRenderer::GetShaderComposite()
{
    return s_Data.CompositeShader;
}

H2M::RefH2M<H2M::RenderPassH2M> VulkanSceneRenderer::GetGeoPass()
{
    return s_Data.GeoPass;
}

H2M::RefH2M<H2M::RenderPassH2M> VulkanSceneRenderer::GetCompositePass()
{
    return s_Data.CompositePass;
}

void VulkanSceneRenderer::CreateDrawCommand(std::string fileNameNoExt, H2M::MeshH2M* mesh)
{
    // s_Data.DrawList.clear(); // doesn't work for multiple meshes on the scene
    VulkanSceneRendererData::DrawCommand drawCommand;

    drawCommand.Name = fileNameNoExt;
    drawCommand.MeshPtr = mesh;
    drawCommand.Transform = glm::mat4(1.0f);

    s_Data.DrawList.push_back(drawCommand);
}

H2M::LightH2M& VulkanSceneRenderer::GetActiveLight()
{
    return s_Data.SceneData.ActiveLight;
}

void VulkanSceneRenderer::SetActiveLight(H2M::LightH2M& light)
{
    s_Data.SceneData.ActiveLight = light;
}

void VulkanSceneRenderer::AddToDrawList(std::string name, H2M::RefH2M<H2M::MeshH2M> mesh, H2M::EntityH2M entity, glm::mat4 transform)
{
    s_Data.DrawList.push_back({ name, mesh.Raw(), entity.GetMaterial(), transform });
}

H2M::RefH2M<H2M::RenderPassH2M> VulkanSceneRenderer::GetFinalRenderPass()
{
    return s_Data.CompositePass;
}

//  H2M::FramebufferTextureH2M* VulkanSceneRenderer::GetFinalColorBuffer()
//  {
//      auto targetFramebuffer = static_cast<H2M::RefH2M<H2M::FramebufferH2M>>//  (s_Data.CompositePass->GetSpecification().TargetFramebuffer);
//  
//      return targetFramebuffer->GetColorAttachmentRendererID();
//  }
