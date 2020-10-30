#include "EnvironmentMap.h"

#include "Hazel/Renderer/RenderPass.h"
#include "Hazel/Renderer/HazelMaterial.h"
#include "Hazel/Renderer/VertexArray.h"
#include "Hazel/Renderer/Buffer.h"
#include "Hazel/Renderer/HazelRenderer.h"
#include "Hazel/Renderer/Renderer2D.h"

#include "Framebuffer.h"
#include "RendererBasic.h"
#include "Log.h"
#include "ImGuiWrapper.h"
#include "Application.h"
#include "Util.h"
#include "Input.h"


EnvironmentMap::EnvironmentMap(const std::string& filepath, Scene* scene)
{
    m_SamplerSlots = new std::map<std::string, unsigned int>();

    //  // PBR texture inputs
    m_SamplerSlots->insert(std::make_pair("albedo",     1)); // uniform sampler2D u_AlbedoTexture
    m_SamplerSlots->insert(std::make_pair("normal",     2)); // uniform sampler2D u_NormalTexture
    m_SamplerSlots->insert(std::make_pair("metalness",  3)); // uniform sampler2D u_MetalnessTexture
    m_SamplerSlots->insert(std::make_pair("roughness",  4)); // uniform sampler2D u_RoughnessTexture
    m_SamplerSlots->insert(std::make_pair("ao",         5)); // uniform sampler2D u_AOTexture
    // Environment maps
    m_SamplerSlots->insert(std::make_pair("radiance",   6)); // uniform samplerCube u_EnvRadianceTex
    m_SamplerSlots->insert(std::make_pair("irradiance", 7)); // uniform samplerCube u_EnvIrradianceTex
    // BRDF LUT
    m_SamplerSlots->insert(std::make_pair("BRDF_LUT",   8)); // uniform sampler2D u_BRDFLUTTexture

    // Skybox.fs         - uniform samplerCube u_Texture;
    // SceneComposite.fs - uniform sampler2DMS u_Texture;
    m_SamplerSlots->insert(std::make_pair("u_Texture",  1));

    m_SceneRenderer = new Hazel::SceneRenderer(filepath, scene);
    SetSkybox(m_SceneRenderer->s_Data.SceneData.SceneEnvironment.RadianceMap);

    Init();

    m_CheckerboardTexture = Hazel::HazelTexture2D::Create("Textures/Hazel/Checkerboard.tga");

    m_DisplayHazelGrid = true;
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
    // Hazel::Renderer2D::Init();

    bool depthTest = true;
    Hazel::Renderer2D::Init();
    Hazel::Renderer2D::BeginScene(RendererBasic::GetProjectionMatrix(), depthTest);
}

void EnvironmentMap::SetupContextData()
{
    // Setup default texture info
    m_TextureInfoDefault = {};
    m_TextureInfoDefault.albedo    = "Textures/PBR/non_reflective/albedo.png";
    m_TextureInfoDefault.normal    = "Textures/PBR/non_reflective/normal.png";
    m_TextureInfoDefault.metallic  = "Textures/PBR/non_reflective/metallic.png";
    m_TextureInfoDefault.roughness = "Textures/PBR/non_reflective/roughness.png";
    m_TextureInfoDefault.ao        = "Textures/PBR/non_reflective/ao.png";

    Log::GetLogger()->info("-- BEGIN Setup PBR Materials --");
    {
        // PBR EnvMapMaterial Weapon (Index = 0)
        TextureInfo textureInfoWeapon = {};
        textureInfoWeapon.albedo    = "Models/Gladiator/Gladiator_weapon_BaseColor.jpg";
        textureInfoWeapon.normal    = "Models/Gladiator/Gladiator_weapon_Normal.jpg";
        textureInfoWeapon.metallic  = "Models/Gladiator/Gladiator_weapon_Metallic.jpg";
        textureInfoWeapon.roughness = "Models/Gladiator/Gladiator_weapon_Roughness.jpg";
        textureInfoWeapon.ao        = "Textures/plain.png";

        m_TextureInfo.insert(std::make_pair("Gladiator_weapon", textureInfoWeapon));

        // PBR EnvMapMaterial Gladiator (Index = 1)
        TextureInfo textureInfoGladiator = {};
        textureInfoGladiator.albedo    = "Models/Gladiator/Gladiator_BaseColor.jpg";
        textureInfoGladiator.normal    = "Models/Gladiator/Gladiator_Normal.jpg";
        textureInfoGladiator.metallic  = "Models/Gladiator/Gladiator_Metallic.jpg";
        textureInfoGladiator.roughness = "Models/Gladiator/Gladiator_Roughness.jpg";
        textureInfoGladiator.ao        = "Models/Gladiator/Gladiator_AO.jpg";

        m_TextureInfo.insert(std::make_pair("Gladiator", textureInfoGladiator));

        // PBR EnvMapMaterial Cerberus (Index = 0)
        TextureInfo textureInfoCerberus = {};
        textureInfoCerberus.albedo    = "Models/Cerberus/Textures/Cerberus_A.tga";
        textureInfoCerberus.normal    = "Models/Cerberus/Textures/Cerberus_N.tga";
        textureInfoCerberus.metallic  = "Models/Cerberus/Textures/Cerberus_M.tga";
        textureInfoCerberus.roughness = "Models/Cerberus/Textures/Cerberus_R.tga";
        textureInfoCerberus.ao        = "Models/Cerberus/Textures/Cerberus_AO.tga";

        m_TextureInfo.insert(std::make_pair("Cerberus00_Fixed", textureInfoCerberus));

        // PBR EnvMapMaterial M1911 (Index = 0)
        TextureInfo textureInfoM1911 = {};
        textureInfoM1911.albedo    = "Models/M1911/m1911_color.png";
        textureInfoM1911.normal    = "Models/M1911/m1911_normal.png";
        textureInfoM1911.metallic  = "Models/M1911/m1911_metalness.png";
        textureInfoM1911.roughness = "Models/M1911/m1911_roughness.png";
        textureInfoM1911.ao        = "Textures/plain.png";

        m_TextureInfo.insert(std::make_pair("pCylinder5", textureInfoM1911));

        // Material* material = new Material(textureInfoM1911, m_MaterialSpecular, m_MaterialShininess);
        // mesh = new Hazel::HazelMesh("Models/Hazel/Sphere1m.fbx", m_ShaderHazelPBR_Anim, drawCommand.Material, false);
        // mesh = new Hazel::HazelMesh("Models/M1911/M1911.fbx", m_ShaderHazelPBR_Anim, nullptr, true);
        // mesh = new Hazel::HazelMesh("Models/Cerberus/Cerberus_LP.FBX", m_ShaderHazelPBR_Static, nullptr, false);

        LoadMesh("Models/Cerberus/Cerberus_LP.FBX");
    }
    Log::GetLogger()->info("-- END Setup PBR Materials --");
}

void EnvironmentMap::LoadMesh(std::string fullPath)
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

    Hazel::HazelMesh* mesh = new Hazel::HazelMesh(fullPath, m_ShaderHazelPBR, nullptr, isAnimated);

    ((Hazel::HazelMesh*)mesh)->SetTimeMultiplier(1.0f);

    m_SceneRenderer->s_Data.DrawList.clear(); // doesn't work for multiple meshes on the scene
    Hazel::SceneRendererData::DrawCommand drawCommand;

    drawCommand.Name = fileNameNoExt;
    drawCommand.Mesh = mesh;
    drawCommand.Transform = glm::mat4(1.0f);

    m_SceneRenderer->s_Data.DrawList.push_back(drawCommand);

    // m_MeshEntity: NoECS version
    m_MeshEntity = CreateEntity(drawCommand.Name);
    m_MeshEntity->Transform() = glm::translate(glm::mat4(1.0f), { 0.0f, 6.0f, 0.0f }) * glm::scale(glm::mat4(1.0f), { 0.1f, 0.1f, 0.1f });
    m_MeshEntity->SetMesh(drawCommand.Mesh);

    SubmitEntity(m_MeshEntity);

    LoadEnvMapMaterials(mesh);
}

void EnvironmentMap::LoadEnvMapMaterials(Mesh* mesh)
{
    for (auto material : m_EnvMapMaterials) {
        delete material.second;
    }
    m_EnvMapMaterials.clear();

    for (Hazel::Submesh* submesh : ((Hazel::HazelMesh*)mesh)->GetSubmeshes())
    {
        if (m_EnvMapMaterials.contains(submesh->NodeName)) {
            continue;
        }

        TextureInfo textureInfo;
        if (m_TextureInfo.contains(submesh->NodeName)) {
            textureInfo = m_TextureInfo.at(submesh->NodeName);
        }
        else {
            textureInfo = m_TextureInfoDefault;
        }

        EnvMapMaterial* envMapMaterial = new EnvMapMaterial();

        // Load Hazel/Renderer/HazelTexture
        envMapMaterial->GetAlbedoInput().TextureMap = Hazel::HazelTexture2D::Create(textureInfo.albedo);
        envMapMaterial->GetAlbedoInput().UseTexture = true;
        envMapMaterial->GetNormalInput().TextureMap = Hazel::HazelTexture2D::Create(textureInfo.normal);
        envMapMaterial->GetNormalInput().UseTexture = true;
        envMapMaterial->GetMetalnessInput().TextureMap = Hazel::HazelTexture2D::Create(textureInfo.metallic);
        envMapMaterial->GetMetalnessInput().UseTexture = true;
        envMapMaterial->GetRoughnessInput().TextureMap = Hazel::HazelTexture2D::Create(textureInfo.roughness);
        envMapMaterial->GetRoughnessInput().UseTexture = true;
        envMapMaterial->GetAOInput().TextureMap = Hazel::HazelTexture2D::Create(textureInfo.ao);
        envMapMaterial->GetAOInput().UseTexture = true;

        m_EnvMapMaterials.insert(std::make_pair(submesh->NodeName, envMapMaterial));
    }
}

void EnvironmentMap::SetupShaders()
{
    m_ShaderHazelPBR_Static = new Shader("Shaders/Hazel/HazelPBR_Static.vs", "Shaders/Hazel/HazelPBR.fs");
    Log::GetLogger()->info("EnvironmentMap: m_ShaderHazelPBR_Static compiled [programID={0}]", m_ShaderHazelPBR_Static->GetProgramID());

    m_ShaderHazelPBR_Anim = new Shader("Shaders/Hazel/HazelPBR_Anim.vs", "Shaders/Hazel/HazelPBR.fs");
    Log::GetLogger()->info("EnvironmentMap: m_ShaderHazelPBR_Anim compiled [programID={0}]", m_ShaderHazelPBR_Anim->GetProgramID());

    m_ShaderRenderer2D_Line = new Shader("Shaders/Hazel/Renderer2D_Line.vs", "Shaders/Hazel/Renderer2D_Line.fs");
    Log::GetLogger()->info("EnvironmentMap: m_ShaderRenderer2D_Line compiled [programID={0}]", m_ShaderRenderer2D_Line->GetProgramID());
}

void EnvironmentMap::UpdateUniforms()
{
    //  for (auto& material : m_EnvMapMaterials) {
    //      UpdateShaderPBRUniforms(m_ShaderHazelPBR_Anim, material.second);
    //      UpdateShaderPBRUniforms(m_ShaderHazelPBR_Static, material.second);
    //  }

    /**** BEGIN Shaders/Hazel/SceneComposite ****/
    m_SceneRenderer->GetShaderComposite()->Bind();
    m_SceneRenderer->GetShaderComposite()->setInt("u_Texture", m_SamplerSlots->at("u_Texture"));
    m_SceneRenderer->GetShaderComposite()->setFloat("u_Exposure", m_SceneRenderer->s_Data.SceneData.SceneCamera->GetExposure());
    /**** END Shaders/Hazel/SceneComposite ****/

    /**** BEGIN Shaders/Hazel/Skybox ****/
    m_SceneRenderer->GetShaderSkybox()->Bind();
    m_SceneRenderer->GetShaderSkybox()->setInt("u_Texture", m_SamplerSlots->at("u_Texture"));
    m_SceneRenderer->GetShaderSkybox()->setFloat("u_TextureLod", ((Hazel::HazelScene*)m_SceneRenderer->s_Data.ActiveScene)->GetSkyboxLOD());
    // apply exposure to Shaders/Hazel/Skybox, considering that Shaders/Hazel/SceneComposite is not yet enabled
    m_SceneRenderer->GetShaderSkybox()->setFloat("u_Exposure", m_SceneRenderer->s_Data.SceneData.SceneCamera->GetExposure() * m_SkyboxExposureFactor); // originally used in Shaders/Hazel/SceneComposite
    /**** END Shaders/Hazel/Skybox ****/
}

void EnvironmentMap::UpdateShaderPBRUniforms(Shader* shaderHazelPBR, EnvMapMaterial* envMapMaterial)
{
    /**** BEGIN Shaders/Hazel/HazelPBR_Anim / Shaders/Hazel/HazelPBR_Static ***/

    shaderHazelPBR->Bind();

    shaderHazelPBR->setInt("u_AlbedoTexture",    m_SamplerSlots->at("albedo"));
    shaderHazelPBR->setInt("u_NormalTexture",    m_SamplerSlots->at("normal"));
    shaderHazelPBR->setInt("u_MetalnessTexture", m_SamplerSlots->at("metalness"));
    shaderHazelPBR->setInt("u_RoughnessTexture", m_SamplerSlots->at("roughness"));
    shaderHazelPBR->setInt("u_AOTexture",        m_SamplerSlots->at("ao"));

    shaderHazelPBR->setVec3("u_AlbedoColor", envMapMaterial->GetAlbedoInput().Color);
    shaderHazelPBR->setFloat("u_Metalness",  envMapMaterial->GetMetalnessInput().Value);
    shaderHazelPBR->setFloat("u_Roughness",  envMapMaterial->GetRoughnessInput().Value);
    shaderHazelPBR->setFloat("u_AO",         envMapMaterial->GetAOInput().Value);

    shaderHazelPBR->setFloat("u_EnvMapRotation", m_EnvMapRotation);

    shaderHazelPBR->setFloat("u_RadiancePrefilter",  m_RadiancePrefilter ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_AlbedoTexToggle",    envMapMaterial->GetAlbedoInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_NormalTexToggle",    envMapMaterial->GetNormalInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_MetalnessTexToggle", envMapMaterial->GetMetalnessInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_RoughnessTexToggle", envMapMaterial->GetRoughnessInput().UseTexture ? 1.0f : 0.0f);
    shaderHazelPBR->setFloat("u_AOTexToggle",        envMapMaterial->GetAOInput().UseTexture ? 1.0f : 0.0f);
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
    m_SkyboxTexture = skybox;
    m_SkyboxTexture->Bind(m_SamplerSlots->at("u_Texture"));
}

EnvironmentMap::~EnvironmentMap()
{
    for (auto const& material : m_EnvMapMaterials) {
        delete &material;
    }

    delete m_SceneRenderer;
}

Hazel::Entity* EnvironmentMap::CreateEntity(const std::string& name)
{
    // Both NoECS and ECS
    Hazel::Entity* entity = ((Hazel::HazelScene*)m_SceneRenderer->s_Data.ActiveScene)->CreateEntity(name);

    return entity;
}

void EnvironmentMap::Update(Scene* scene, float timestep)
{
    m_SceneRenderer->s_Data.ActiveScene = scene;

    m_SceneRenderer->BeginScene((Scene*)m_SceneRenderer->s_Data.ActiveScene);

    UpdateUniforms();

    // Update HazelMesh List
    for (auto& dc : m_SceneRenderer->s_Data.DrawList)
    {
        ((Hazel::HazelMesh*)dc.Mesh)->OnUpdate(timestep, false);
    }
}

void EnvironmentMap::SubmitEntity(Hazel::Entity* entity)
{
    auto mesh = entity->GetMesh();
    if (!mesh)
        return;

    m_SceneRenderer->s_Data.DrawList.push_back({ entity->GetName(), (Hazel::HazelMesh*)mesh, entity->GetMaterial(), entity->GetTransform() });
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

void EnvironmentMap::DrawIndexed(uint32_t count, Hazel::PrimitiveType type, bool depthTest)
{
    if (!depthTest)
        glDisable(GL_DEPTH_TEST);

    GLenum glPrimitiveType = 0;
    switch (type)
    {
    case Hazel::PrimitiveType::Triangles:
        glPrimitiveType = GL_TRIANGLES;
        break;
    case Hazel::PrimitiveType::Lines:
        glPrimitiveType = GL_LINES;
        break;
    }

    glDrawElements(glPrimitiveType, count, GL_UNSIGNED_INT, nullptr);

    if (!depthTest)
        glEnable(GL_DEPTH_TEST);
}

void EnvironmentMap::OnImGuiRender()
{
    ((Hazel::HazelMesh*)m_MeshEntity->GetMesh())->OnImGuiRender();

    ImGui::Begin("EnvMap Materials");
    {
        for (auto& material : m_EnvMapMaterials)
        {
            std::string materialName = material.first;
            std::string materialLabel = "Material " + materialName;

            // Material section
            if (ImGui::CollapsingHeader(materialLabel.c_str(), nullptr, ImGuiTreeNodeFlags_DefaultOpen))
            {
                // BEGIN PBR Textures
                ImGui::Indent();
                {
                    {
                        // Albedo
                        std::string textureLabel = materialName + " Albedo";
                        if (ImGui::CollapsingHeader(textureLabel.c_str(), nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                            ImGui::Image(material.second->GetAlbedoInput().TextureMap ?
                                (void*)(intptr_t)material.second->GetAlbedoInput().TextureMap->GetID() :
                                (void*)(intptr_t)m_CheckerboardTexture->GetID(), ImVec2(64, 64));
                            ImGui::PopStyleVar();
                            if (ImGui::IsItemHovered())
                            {
                                if (material.second->GetAlbedoInput().TextureMap)
                                {
                                    ImGui::BeginTooltip();
                                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                    ImGui::TextUnformatted(material.second->GetAlbedoInput().TextureMap->GetPath().c_str());
                                    ImGui::PopTextWrapPos();
                                    ImGui::Image((void*)(intptr_t)material.second->GetAlbedoInput().TextureMap->GetID(), ImVec2(384, 384));
                                    ImGui::EndTooltip();
                                }
                                if (ImGui::IsItemClicked())
                                {
                                    std::string filename = Application::Get()->OpenFile("");
                                    if (filename != "")
                                        material.second->GetAlbedoInput().TextureMap = Hazel::HazelTexture2D::Create(filename, material.second->GetAlbedoInput().SRGB);
                                }
                            }
                            ImGui::SameLine();
                            ImGui::BeginGroup();

                            std::string checkboxLabel = "Use##" + materialName + "AlbedoMap";
                            ImGui::Checkbox(checkboxLabel.c_str(), &material.second->GetAlbedoInput().UseTexture);

                            std::string checkboxLabelSRGB = "sRGB##" + materialName + "AlbedoMap";
                            if (ImGui::Checkbox(checkboxLabelSRGB.c_str(), &material.second->GetAlbedoInput().SRGB))
                            {
                                if (material.second->GetAlbedoInput().TextureMap)
                                    material.second->GetAlbedoInput().TextureMap = Hazel::HazelTexture2D::Create(
                                        material.second->GetAlbedoInput().TextureMap->GetPath(),
                                        material.second->GetAlbedoInput().SRGB);
                            }
                            ImGui::EndGroup();
                            ImGui::SameLine();
                            std::string colorLabel = "Color##" + materialName + "Albedo";
                            ImGui::ColorEdit3(colorLabel.c_str(), glm::value_ptr(material.second->GetAlbedoInput().Color), ImGuiColorEditFlags_NoInputs);
                        }
                    }
                    {
                        // Normals
                        std::string textureLabel = materialName + " Normals";
                        if (ImGui::CollapsingHeader(textureLabel.c_str(), nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                            ImGui::Image(material.second->GetNormalInput().TextureMap ?
                                (void*)(intptr_t)material.second->GetNormalInput().TextureMap->GetID() :
                                (void*)(intptr_t)m_CheckerboardTexture->GetID(), ImVec2(64, 64));
                            ImGui::PopStyleVar();
                            if (ImGui::IsItemHovered())
                            {
                                if (material.second->GetNormalInput().TextureMap)
                                {
                                    ImGui::BeginTooltip();
                                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                    ImGui::TextUnformatted(material.second->GetNormalInput().TextureMap->GetPath().c_str());
                                    ImGui::PopTextWrapPos();
                                    ImGui::Image((void*)(intptr_t)material.second->GetNormalInput().TextureMap->GetID(), ImVec2(384, 384));
                                    ImGui::EndTooltip();
                                }
                                if (ImGui::IsItemClicked())
                                {
                                    std::string filename = Application::Get()->OpenFile("");
                                    if (filename != "")
                                        material.second->GetNormalInput().TextureMap = Hazel::HazelTexture2D::Create(filename);
                                }
                            }
                            ImGui::SameLine();
                            std::string checkboxLabel = "Use##" + materialName + "NormalMap";
                            ImGui::Checkbox(checkboxLabel.c_str(), &material.second->GetNormalInput().UseTexture);
                        }
                    }
                    {
                        // Metalness
                        std::string textureLabel = materialName + " Metalness";
                        if (ImGui::CollapsingHeader(textureLabel.c_str(), nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                            ImGui::Image(material.second->GetMetalnessInput().TextureMap ?
                                (void*)(intptr_t)material.second->GetMetalnessInput().TextureMap->GetID() :
                                (void*)(intptr_t)m_CheckerboardTexture->GetID(), ImVec2(64, 64));
                            ImGui::PopStyleVar();
                            if (ImGui::IsItemHovered())
                            {
                                if (material.second->GetMetalnessInput().TextureMap)
                                {
                                    ImGui::BeginTooltip();
                                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                    ImGui::TextUnformatted(material.second->GetMetalnessInput().TextureMap->GetPath().c_str());
                                    ImGui::PopTextWrapPos();
                                    ImGui::Image((void*)(intptr_t)material.second->GetMetalnessInput().TextureMap->GetID(), ImVec2(384, 384));
                                    ImGui::EndTooltip();
                                }
                                if (ImGui::IsItemClicked())
                                {
                                    std::string filename = Application::Get()->OpenFile("");
                                    if (filename != "")
                                        material.second->GetMetalnessInput().TextureMap = Hazel::HazelTexture2D::Create(filename);
                                }
                            }
                            ImGui::SameLine();
                            std::string checkboxLabel = "Use##" + materialName + "MetalnessMap";
                            ImGui::Checkbox(checkboxLabel.c_str(), &material.second->GetMetalnessInput().UseTexture);
                            ImGui::SameLine();
                            std::string sliderLabel = "Value##" + materialName + "MetalnessInput";
                            ImGui::SliderFloat(sliderLabel.c_str(), &material.second->GetMetalnessInput().Value, 0.0f, 1.0f);
                        }
                    }
                    {
                        // Roughness
                        std::string textureLabel = materialName + " Roughness";
                        if (ImGui::CollapsingHeader(textureLabel.c_str(), nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                            ImGui::Image(material.second->GetRoughnessInput().TextureMap ?
                                (void*)(intptr_t)material.second->GetRoughnessInput().TextureMap->GetID() :
                                (void*)(intptr_t)m_CheckerboardTexture->GetID(), ImVec2(64, 64));
                            ImGui::PopStyleVar();
                            if (ImGui::IsItemHovered())
                            {
                                if (material.second->GetRoughnessInput().TextureMap)
                                {
                                    ImGui::BeginTooltip();
                                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                    ImGui::TextUnformatted(material.second->GetRoughnessInput().TextureMap->GetPath().c_str());
                                    ImGui::PopTextWrapPos();
                                    ImGui::Image((void*)(intptr_t)material.second->GetRoughnessInput().TextureMap->GetID(), ImVec2(384, 384));
                                    ImGui::EndTooltip();
                                }
                                if (ImGui::IsItemClicked())
                                {
                                    std::string filename = Application::Get()->OpenFile("");
                                    if (filename != "")
                                        material.second->GetRoughnessInput().TextureMap = Hazel::HazelTexture2D::Create(filename);
                                }
                            }
                            ImGui::SameLine();
                            std::string checkboxLabel = "Use##" + materialName + "RoughnessMap";
                            ImGui::Checkbox(checkboxLabel.c_str(), &material.second->GetRoughnessInput().UseTexture);
                            ImGui::SameLine();
                            std::string sliderLabel = "Value##" + materialName + "RoughnessInput";
                            ImGui::SliderFloat(sliderLabel.c_str(), &material.second->GetRoughnessInput().Value, 0.0f, 1.0f);
                        }
                    }
                    {
                        // AO (Ambient Occlusion
                        std::string textureLabel = materialName + " AO";
                        if (ImGui::CollapsingHeader(textureLabel.c_str(), nullptr, ImGuiTreeNodeFlags_DefaultOpen))
                        {
                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                            ImGui::Image(material.second->GetAOInput().TextureMap ?
                                (void*)(intptr_t)material.second->GetAOInput().TextureMap->GetID() :
                                (void*)(intptr_t)m_CheckerboardTexture->GetID(), ImVec2(64, 64));
                            ImGui::PopStyleVar();
                            if (ImGui::IsItemHovered())
                            {
                                if (material.second->GetAOInput().TextureMap)
                                {
                                    ImGui::BeginTooltip();
                                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                                    ImGui::TextUnformatted(material.second->GetAOInput().TextureMap->GetPath().c_str());
                                    ImGui::PopTextWrapPos();
                                    ImGui::Image((void*)(intptr_t)material.second->GetAOInput().TextureMap->GetID(), ImVec2(384, 384));
                                    ImGui::EndTooltip();
                                }
                                if (ImGui::IsItemClicked())
                                {
                                    std::string filename = Application::Get()->OpenFile("");
                                    if (filename != "")
                                        material.second->GetAOInput().TextureMap = Hazel::HazelTexture2D::Create(filename);
                                }
                            }
                            ImGui::SameLine();
                            std::string checkboxLabel = "Use##" + materialName + "AOMap";
                            ImGui::Checkbox(checkboxLabel.c_str(), &material.second->GetAOInput().UseTexture);
                            ImGui::SameLine();
                            std::string sliderLabel = "Value##" + materialName + "AOInput";
                            ImGui::SliderFloat(sliderLabel.c_str(), &material.second->GetAOInput().Value, 0.0f, 1.0f);
                        }
                    }
                }
                ImGui::Unindent();
                // END PBR Textures
            }
        }
    }
    ImGui::End();
}

void EnvironmentMap::SubmitMesh(Hazel::HazelMesh* mesh, const glm::mat4& transform, Material* overrideMaterial)
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
            m_ShaderHazelPBR->setMat4(uniformName, mesh->m_BoneTransforms[i]);
        }

        m_ShaderHazelPBR->setMat4("u_Transform", transform * submesh->Transform);

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

void EnvironmentMap::RenderHazelSkybox()
{
    RendererBasic::DisableCulling();

    // Hazel Skybox
    m_SceneRenderer->GetShaderSkybox()->Bind();
    glm::mat4 viewProjection = RendererBasic::GetProjectionMatrix() * ((Scene*)m_SceneRenderer->s_Data.ActiveScene)->GetCameraController()->CalculateViewMatrix();
    m_SceneRenderer->GetShaderSkybox()->setMat4("u_InverseVP", glm::inverse(viewProjection));
    // m_SkyboxTexture->Bind(m_SamplerSlots->at("u_Texture"));
    m_SceneRenderer->s_Data.SceneData.SceneEnvironment.RadianceMap->Bind(m_SamplerSlots->at("u_Texture"));
    // SubmitFullscreenQuad(m_Data.SceneData.SkyboxMaterial);
    m_SceneRenderer->Renderer_SubmitFullscreenQuad(nullptr); // m_Data.SceneData.SkyboxMaterial
}

void EnvironmentMap::RenderHazelGrid()
{
    // Grid
    // -- Shaders/Hazel/Grid.vs
    // ---- uniform mat4 u_ViewProjection;
    // ---- uniform mat4 u_Transform;
    // -- Shaders/Hazel/Grid.fs
    // ---- uniform float u_Scale;
    // ---- uniform float u_Res;

    m_SceneRenderer->GetShaderGrid()->Bind();
    glm::mat4 viewProjection = RendererBasic::GetProjectionMatrix() * ((Scene*)m_SceneRenderer->s_Data.ActiveScene)->GetCameraController()->CalculateViewMatrix();
    m_SceneRenderer->GetShaderGrid()->setMat4("u_ViewProjection", viewProjection);

    bool depthTest = true;

    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::scale(transform, glm::vec3(16.0f, 1.0f, 16.0f));
    transform = glm::rotate(transform, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    m_SceneRenderer->GetShaderGrid()->setMat4("u_Transform", transform);

    RendererBasic::EnableTransparency();
    RendererBasic::EnableMSAA();

    m_SceneRenderer->GetFullscreenQuad()->Render();
}

void EnvironmentMap::OnEvent(Event& e)
{
    if (m_AllowViewportCameraEvents) {
        ((Scene*)m_SceneRenderer->s_Data.ActiveScene)->GetCamera()->OnEvent(e);
    }

    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<KeyPressedEvent>(HZ_BIND_EVENT_FN(EnvironmentMap::OnKeyPressedEvent));
    dispatcher.Dispatch<MouseButtonPressedEvent>(HZ_BIND_EVENT_FN(EnvironmentMap::OnMouseButtonPressed));
}

bool EnvironmentMap::OnKeyPressedEvent(KeyPressedEvent& e)
{
    return false;
}

bool EnvironmentMap::OnMouseButtonPressed(MouseButtonPressedEvent& e)
{
    auto [mx, my] = Input::GetMousePosition();
    if (e.GetMouseButton() == (int)Mouse::ButtonLeft && !Input::IsKeyPressed(Key::LeftAlt))
    {
        m_SelectedSubmeshes.clear();

        auto [mouseX, mouseY] = GetMouseViewportSpace();

        Log::GetLogger()->debug("EnvironmentMap::OnMouseButtonPressed GetMouseViewportSpace [mouseX = {0}, mouseY = {1}] ", mouseX, mouseY);

        if (mouseX > -1.0f && mouseX < 1.0f && mouseY > -1.0f && mouseY < 1.0f)
        {
            auto [origin, direction] = CastRay(mouseX, mouseY);

            auto mesh = m_MeshEntity->GetMesh();
            const auto& submeshes = ((Hazel::HazelMesh*)mesh)->GetSubmeshes();
            float lastT = 200.0f; // Distance between camera and intersection in CastRay

            for (const auto& submesh : submeshes)
            {
                auto newRay = glm::inverse(submesh->Transform) * glm::vec4(origin, 1.0f);
                m_NewRay = submesh->Transform * newRay;
                auto newDir = glm::inverse(glm::mat3(submesh->Transform)) * direction;

                float t = 0.0f;
                bool intersects = submesh->BoundingBox.Intersect(newRay, newDir, t);
                if (intersects) {
                    // if (m_SelectedSubmeshes.size())
                    {
                        // if (t < lastT)
                        {
                            m_SelectedSubmeshes.push_back(*submesh);
                            lastT = t;
                        }
                    }
                }

                Log::GetLogger()->warn("origin [ {0} {1} {2} ] direction [ {3} {4} {5} ]", origin.x, origin.y, origin.z, direction.x, direction.y, direction.z);
                Log::GetLogger()->warn("Intersects = {0}, t = {1}", intersects, t);
            }
        }
    }

    return false;
}

std::pair<float, float> EnvironmentMap::GetMouseViewportSpace()
{
    auto [mx, my] = ImGui::GetMousePos(); // Input::GetMousePosition();
    mx -= m_ViewportBounds[0].x;
    my -= m_ViewportBounds[0].y;
    auto viewportWidth = m_ViewportBounds[1].x - m_ViewportBounds[0].x;
    auto viewportHeight = m_ViewportBounds[1].y - m_ViewportBounds[0].y;

    Log::GetLogger()->debug("EnvironmentMap::GetMouseViewportSpace | viewportWidth {0} viewportHeight {1}", viewportWidth, viewportHeight);

    // return { (mx / viewportWidth) * 2.0f - 1.0f, ((my / viewportHeight) * 2.0f + 1.0f) * -1.0f };
    return { (mx / viewportWidth) * 2.0f - 1.0f, ((my / viewportHeight) * 2.0f - 1.0f) * -1.0f };
}

std::pair<glm::vec3, glm::vec3> EnvironmentMap::CastRay(float mx, float my)
{
    glm::vec4 mouseClipPos = { mx, my, -1.0f, 1.0f };

    auto inverseProj = glm::inverse(RendererBasic::GetProjectionMatrix());
    auto inverseView = glm::inverse(((Scene*)m_SceneRenderer->s_Data.ActiveScene)->GetCameraController()->CalculateViewMatrix());

    glm::vec4 ray = inverseProj * mouseClipPos;
    glm::vec3 rayPos = ((Scene*)m_SceneRenderer->s_Data.ActiveScene)->GetCamera()->GetPosition();
    glm::vec3 rayDir = inverseView * ray; // inverseView * glm::vec3(ray)

    Log::GetLogger()->debug("EnvironmentMap::CastRay | MousePosition [ {0} {1} ]", mx, my);
    Log::GetLogger()->debug("EnvironmentMap::CastRay | m_ViewportBounds[0] [ {0} {1} ]", m_ViewportBounds[0].x, m_ViewportBounds[0].y);
    Log::GetLogger()->debug("EnvironmentMap::CastRay | m_ViewportBounds[1] [ {0} {1} ]", m_ViewportBounds[1].x, m_ViewportBounds[1].y);
    Log::GetLogger()->debug("EnvironmentMap::CastRay | mouseClipPos [ {0} {1} ]", mouseClipPos.x, mouseClipPos.y);

    return { rayPos, rayDir };
}

void EnvironmentMap::GeometryPassTemporary()
{
    Hazel::HazelMesh* hazelMesh = ((Hazel::HazelMesh*)m_MeshEntity->GetMesh());
    m_ShaderHazelPBR = hazelMesh->IsAnimated() ? m_ShaderHazelPBR_Anim : m_ShaderHazelPBR_Static;

    m_SceneRenderer->s_Data.SceneData.SceneEnvironment.RadianceMap->Bind(m_SamplerSlots->at("radiance"));
    m_SceneRenderer->s_Data.SceneData.SceneEnvironment.IrradianceMap->Bind(m_SamplerSlots->at("irradiance"));
    m_SceneRenderer->s_Data.BRDFLUT->Bind(m_SamplerSlots->at("BRDF_LUT"));

    RenderHazelSkybox();

    if (m_DisplayHazelGrid) {
        RenderHazelGrid();
    }

    // Render HazelMesh meshes (later entt entities)
    uint32_t samplerSlot = m_SamplerSlots->at("albedo");
    for (auto& dc : m_SceneRenderer->s_Data.DrawList)
    {
        // Log::GetLogger()->debug("EM::Render dc.Transform {0} {1} {2}", dc.Transform[3][0], dc.Transform[3][1], dc.Transform[3][2]);
        // dc.Mesh->Render(samplerSlot, m_MeshEntity->Transform());
    }

    // m_ShaderHazelPBR_Anim->Bind();
    // m_ShaderHazelPBR_Anim->setFloat("u_Exposure", m_Data.SceneData.SceneCamera->GetExposure());

    // in conflict with HazelMesh::m_BaseMaterial
    // TODO: Convert m_BaseMaterial type to Hazel/Renderer/HazelMaterial

    m_ShaderHazelPBR->Bind();

    for (Hazel::Submesh* submesh : hazelMesh->GetSubmeshes())
    {
        if (m_EnvMapMaterials.contains(submesh->NodeName)) {
            UpdateShaderPBRUniforms(m_ShaderHazelPBR, m_EnvMapMaterials.at(submesh->NodeName));
        }
        submesh->Render(hazelMesh, m_ShaderHazelPBR, m_MeshEntity->Transform(), samplerSlot, m_EnvMapMaterials);
    }

    // hazelMesh->RenderSubmeshes(samplerSlot, m_MeshEntity->Transform(), m_EnvMapMaterials);
    // hazelMesh->Render(samplerSlot, m_MeshEntity->Transform(), m_EnvMapMaterials);

    m_SceneRenderer->Renderer_BeginRenderPass(m_SceneRenderer->s_Data.GeoPass, false); // should we clear the buffer?

    // Render entities
    for (auto& dc : m_SceneRenderer->s_Data.DrawList)
    {
        //  auto baseMaterial = dc.Mesh->GetBaseMaterial();
        auto baseMaterial = dc.Material;

        auto overrideMaterial = nullptr; // dc.Material;
        SubmitMesh(((Hazel::HazelMesh*)dc.Mesh), m_MeshEntity->Transform(), overrideMaterial);
    }

    m_SceneRenderer->Renderer_EndRenderPass();
}

void EnvironmentMap::CompositePassTemporary(Framebuffer* framebuffer)
{
    m_SceneRenderer->GetShaderComposite()->Bind();
    framebuffer->GetTextureAttachmentColor()->Bind(m_SamplerSlots->at("u_Texture"));
    m_SceneRenderer->GetShaderComposite()->setInt("u_Texture", m_SamplerSlots->at("u_Texture"));
    m_SceneRenderer->GetShaderComposite()->setFloat("u_Exposure", m_SceneRenderer->s_Data.SceneData.SceneCamera->GetExposure());
    // m_ShaderComposite->setInt("u_TextureSamples", framebuffer->GetSpecification().Samples);
    m_SceneRenderer->GetShaderComposite()->setInt("u_TextureSamples", m_SceneRenderer->s_Data.GeoPass->GetSpecification().TargetFramebuffer->GetSpecification().Samples);

    m_SceneRenderer->Renderer_SubmitFullscreenQuad(nullptr);
}

void EnvironmentMap::Render(Framebuffer* framebuffer)
{
    RendererBasic::EnableTransparency();
    RendererBasic::EnableMSAA();

    m_SceneRenderer->BeginScene((Scene*)m_SceneRenderer->s_Data.ActiveScene);

    glm::mat4 viewProjection = RendererBasic::GetProjectionMatrix() * ((Scene*)m_SceneRenderer->s_Data.ActiveScene)->GetCameraController()->CalculateViewMatrix();

    // Temporary Hazel LIVE #006
    m_ShaderRenderer2D_Line->Bind();
    m_ShaderRenderer2D_Line->setMat4("u_ViewProjection", viewProjection);
    RendererBasic::SetLineThickness(2.0f);

    // Hazel::Renderer2D::DrawLine(m_NewRay, m_NewRay + glm::vec3(1, 0, 0) * 100.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    auto mesh = m_MeshEntity->GetMesh();
    const auto& submeshes = ((Hazel::HazelMesh*)mesh)->GetSubmeshes();

    bool depthTest = true;
    Hazel::Renderer2D::BeginScene(viewProjection, depthTest);
    for (const auto& submesh : submeshes)
    {
        Hazel::HazelRenderer::DrawAABB(submesh->BoundingBox, submesh->Transform, glm::vec4(1.0f));
    }
    Hazel::Renderer2D::EndScene();
    // END dtrajko test code

    GeometryPassTemporary();

    /**
    if (m_SelectedSubmeshes.size()) {
        for (auto& submesh : m_SelectedSubmeshes) {
            Hazel::HazelRenderer::DrawAABB(submesh.BoundingBox, submesh.Transform, glm::vec4(1.0f));
        }
    }

    if (m_SceneRenderer->GetOptions().ShowBoundingBoxes)
    {
        bool depthTest = true;
        Hazel::Renderer2D::BeginScene(viewProjection, depthTest);
        for (auto& dc : m_SceneRenderer->s_Data.DrawList)
            Hazel::HazelRenderer::DrawAABB(dc.Mesh, dc.Transform);
        Hazel::Renderer2D::EndScene();
    }
    **/

    CompositePassTemporary(framebuffer);

    // Currently not in use
    m_SceneRenderer->EndScene();
}
