#include "Renderer/RendererEditor.h"

#include "Core/Application.h"
#include "Core/Profiler.h"
#include "Core/Timer.h"
#include "Framebuffer/Framebuffer.h"
#include "Mesh/GeometryFactory.h"
#include "Scene/SceneEditor.h"

#include <stdexcept>


RendererEditor::RendererEditor()
{
}

RendererEditor::~RendererEditor()
{
}

void RendererEditor::Init(Scene* scene)
{
    m_IsViewportEnabled = ((SceneEditor*)scene)->m_IsViewportEnabled;

	SetUniforms();
	SetShaders();
}

void RendererEditor::SetUniforms()
{
}

void RendererEditor::SetShaders()
{
    Shader* shaderEditor = new Shader("Shaders/editor_object.vs", "Shaders/editor_object.fs");
    RendererBasic::GetShaders().insert(std::make_pair("editor_object", shaderEditor));
    Log::GetLogger()->info("RendererEditor: shaderEditor compiled [programID={0}]", shaderEditor->GetProgramID());

    Shader* shaderEditorPBR = new Shader("Shaders/editor_object.vs", "Shaders/PBR/editor_object_pbr.fs");
    RendererBasic::GetShaders().insert(std::make_pair("editor_object_pbr", shaderEditorPBR));
    Log::GetLogger()->info("RendererEditor: shaderEditorPBR compiled [programID={0}]", shaderEditorPBR->GetProgramID());

    Shader* shaderSkinning = new Shader("Shaders/OGLdev/skinning.vs", "Shaders/OGLdev/skinning.fs");
    RendererBasic::GetShaders().insert(std::make_pair("skinning", shaderSkinning));
    Log::GetLogger()->info("RendererEditor: shaderSkinning compiled [programID={0}]", shaderSkinning->GetProgramID());

    Shader* shaderHybridAnimPBR = new Shader("Shaders/HybridAnimPBR.vs", "Shaders/HybridAnimPBR.fs");
    RendererBasic::GetShaders().insert(std::make_pair("hybrid_anim_pbr", shaderHybridAnimPBR));
    Log::GetLogger()->info("RendererEditor: shaderHybridAnimPBR compiled [programID={0}]", shaderHybridAnimPBR->GetProgramID());

    Shader* shaderShadowMap = new Shader("Shaders/directional_shadow_map.vert", "Shaders/directional_shadow_map.frag");
    RendererBasic::GetShaders().insert(std::make_pair("shadow_map", shaderShadowMap));
    Log::GetLogger()->info("RendererEditor: shaderShadowMap compiled [programID={0}]", shaderShadowMap->GetProgramID());

    Shader* shaderOmniShadowMap = new Shader("Shaders/omni_shadow_map.vert", "Shaders/omni_shadow_map.geom", "Shaders/omni_shadow_map.frag");
    RendererBasic::GetShaders().insert(std::make_pair("omni_shadow_map", shaderOmniShadowMap));
    Log::GetLogger()->info("RendererEditor: shaderOmniShadowMap compiled [programID={0}]", shaderOmniShadowMap->GetProgramID());

    Shader* shaderWater = new Shader("Shaders/water.vert", "Shaders/water.frag");
    RendererBasic::GetShaders().insert(std::make_pair("water", shaderWater));
    Log::GetLogger()->info("RendererEditor: shaderWater compiled [programID={0}]", shaderWater->GetProgramID());

    Shader* shaderBackground = new Shader("Shaders/LearnOpenGL/2.2.2.background.vs", "Shaders/LearnOpenGL/2.2.2.background.fs");
    RendererBasic::GetShaders().insert(std::make_pair("background", shaderBackground));
    Log::GetLogger()->info("RendererEditor: shaderBackground compiled [programID={0}]", shaderBackground->GetProgramID());

    Shader* shaderBasic = new Shader("Shaders/basic.vs", "Shaders/basic.fs");
    RendererBasic::GetShaders().insert(std::make_pair("basic", shaderBasic));
    Log::GetLogger()->info("RendererEditor: shaderBasic compiled [programID={0}]", shaderBasic->GetProgramID());

    Shader* shaderGizmo = new Shader("Shaders/gizmo.vs", "Shaders/gizmo.fs");
    RendererBasic::GetShaders().insert(std::make_pair("gizmo", shaderGizmo));
    Log::GetLogger()->info("RendererEditor: shaderGizmo compiled [programID={0}]", shaderGizmo->GetProgramID());

    Shader* shaderGlass = new Shader("Shaders/glass.vs", "Shaders/glass.fs");
    RendererBasic::GetShaders().insert(std::make_pair("glass", shaderGlass));
    Log::GetLogger()->info("RendererEditor: shaderGlass compiled [programID={0}]", shaderGlass->GetProgramID());

    shaderEditor->Bind();
    shaderEditor->setInt("albedoMap", 0);
    shaderEditor->setInt("cubeMap",   1);
    shaderEditor->setInt("shadowMap", 2);
    m_OmniShadowTxSlots.insert(std::make_pair("editor_object", 3)); // omniShadowMaps[i].shadowMap = 3

    shaderEditorPBR->Bind();
    shaderEditorPBR->setInt("irradianceMap", 0);
    shaderEditorPBR->setInt("prefilterMap",  1);
    shaderEditorPBR->setInt("brdfLUT",       2);
    shaderEditorPBR->setInt("albedoMap",     3);
    shaderEditorPBR->setInt("normalMap",     4);
    shaderEditorPBR->setInt("metallicMap",   5);
    shaderEditorPBR->setInt("roughnessMap",  6);
    shaderEditorPBR->setInt("aoMap",         7);
    shaderEditorPBR->setInt("shadowMap",     8);
    m_OmniShadowTxSlots.insert(std::make_pair("editor_object_pbr", 9)); // omniShadowMaps[i].shadowMap = 9

    shaderHybridAnimPBR->Bind();
    shaderHybridAnimPBR->setInt("u_AlbedoTexture",    1);
    shaderHybridAnimPBR->setInt("u_NormalTexture",    2);
    shaderHybridAnimPBR->setInt("u_MetalnessTexture", 3);
    shaderHybridAnimPBR->setInt("u_RoughnessTexture", 4);
    shaderHybridAnimPBR->setInt("u_EnvRadianceTex",   5);
    shaderHybridAnimPBR->setInt("u_PrefilterMap",     6);
    shaderHybridAnimPBR->setInt("u_BRDFLUT",          7);
}

void RendererEditor::RenderPassShadow(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
    if (!scene->GetSettings().enableShadows) return;
    if (!LightManager::directionalLight.GetEnabled()) return;
    if (LightManager::directionalLight.GetShadowMap() == nullptr) return;

    Shader* shaderShadowMap = RendererBasic::GetShaders()["shadow_map"];
    shaderShadowMap->Bind();

    DirectionalLight* light = &LightManager::directionalLight;
    glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

    light->GetShadowMap()->BindForWriting();
    // printf("RenderPassShadow write to FBO = %i Texture ID = %i\n", light->GetShadowMap()->GetFBO(), light->GetShadowMap()->GetTextureID());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);

    shaderShadowMap->setMat4("u_DirLightTransform", light->CalculateLightTransform());
    shaderShadowMap->setBool("u_Animated", false);
    shaderShadowMap->Validate();

    DisableCulling();
    std::string passType = "shadow_dir";
    scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RendererEditor::RenderPassOmniShadow(PointLight* light, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
    if (!scene->GetSettings().enableOmniShadows) return;

    Shader* shaderOmniShadow = RendererBasic::GetShaders()["omni_shadow_map"];
    shaderOmniShadow->Bind();

    glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

    light->GetShadowMap()->BindForWriting();
    // printf("RenderPassOmniShadow write to FBO = %i Texture ID = %i\n", light->GetShadowMap()->GetFBO(), light->GetShadowMap()->GetTextureID());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);

    shaderOmniShadow->setVec3("lightPosition", light->GetPosition());
    shaderOmniShadow->setFloat("farPlane", light->GetFarPlane());
    shaderOmniShadow->setLightMat4(light->CalculateLightTransform());
    shaderOmniShadow->Validate();

    EnableCulling();
    std::string passType = "shadow_omni";
    scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RendererEditor::RenderPassWaterReflection(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
    if (!scene->GetSettings().enableWaterEffects) return;

    glViewport(0, 0, scene->GetWaterManager()->GetFramebufferWidth(), scene->GetWaterManager()->GetFramebufferHeight());

    scene->GetWaterManager()->GetReflectionFramebuffer()->Bind();

    // Clear the window
    RendererBasic::Clear();

    Shader* shaderEditor = RendererBasic::GetShaders()["editor_object"];
    shaderEditor->Bind();
    shaderEditor->setMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderEditor->setMat4("projection", projectionMatrix);
    shaderEditor->setVec3("eyePosition", scene->GetCamera()->GetPosition());
    shaderEditor->setMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());
    shaderEditor->setVec4("clipPlane", glm::vec4(0.0f, 1.0f, 0.0f, -scene->GetWaterManager()->GetWaterHeight())); // reflection clip plane
    
    Shader* shaderEditorPBR = RendererBasic::GetShaders()["editor_object_pbr"];
    shaderEditorPBR->Bind();
    shaderEditorPBR->setMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderEditorPBR->setMat4("projection", projectionMatrix);
    shaderEditorPBR->setVec3("eyePosition", scene->GetCamera()->GetPosition());
    shaderEditorPBR->setMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());
    shaderEditorPBR->setVec4("clipPlane", glm::vec4(0.0f, 1.0f, 0.0f, -scene->GetWaterManager()->GetWaterHeight())); // reflection clip plane

    Shader* shaderSkinning = RendererBasic::GetShaders()["skinning"];
    shaderSkinning->Bind();
    shaderSkinning->setMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderSkinning->setMat4("projection", projectionMatrix);
    shaderSkinning->setVec4("clipPlane", glm::vec4(0.0f, 1.0f, 0.0f, -scene->GetWaterManager()->GetWaterHeight())); // reflection clip plane

    Shader* shaderHybridAnimPBR = RendererBasic::GetShaders()["hybrid_anim_pbr"];
    shaderHybridAnimPBR->Bind();
    shaderHybridAnimPBR->setMat4("u_ViewProjectionMatrix", projectionMatrix * scene->GetCamera()->GetViewMatrix());

    DisableCulling();
    std::string passType = "water_reflect";
    scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RendererEditor::RenderPassWaterRefraction(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
    if (!scene->GetSettings().enableWaterEffects) return;

    glViewport(0, 0, scene->GetWaterManager()->GetFramebufferWidth(), scene->GetWaterManager()->GetFramebufferHeight());

    scene->GetWaterManager()->GetRefractionFramebuffer()->Bind();
    scene->GetWaterManager()->GetRefractionFramebuffer()->GetColorAttachment()->Bind(scene->GetTextureSlots()["refraction"]);
    scene->GetWaterManager()->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(scene->GetTextureSlots()["depth"]);

    // Clear the window
    RendererBasic::Clear();

    Shader* shaderEditor = RendererBasic::GetShaders()["editor_object"];
    shaderEditor->Bind();
    shaderEditor->setMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderEditor->setMat4("projection", projectionMatrix);
    shaderEditor->setVec3("eyePosition", scene->GetCamera()->GetPosition());
    shaderEditor->setMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());
    shaderEditor->setVec4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, scene->GetWaterManager()->GetWaterHeight())); // refraction clip plane

    Shader* shaderEditorPBR = RendererBasic::GetShaders()["editor_object_pbr"];
    shaderEditorPBR->Bind();
    shaderEditorPBR->setMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderEditorPBR->setMat4("projection", projectionMatrix);
    shaderEditorPBR->setVec3("eyePosition", scene->GetCamera()->GetPosition());
    shaderEditorPBR->setMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());
    shaderEditorPBR->setVec4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, scene->GetWaterManager()->GetWaterHeight())); // refraction clip plane

    Shader* shaderSkinning = RendererBasic::GetShaders()["skinning"];
    shaderSkinning->Bind();
    shaderSkinning->setMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderSkinning->setMat4("projection", projectionMatrix);
    shaderSkinning->setVec4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, scene->GetWaterManager()->GetWaterHeight())); // refraction clip plane

    Shader* shaderHybridAnimPBR = RendererBasic::GetShaders()["hybrid_anim_pbr"];
    shaderHybridAnimPBR->Bind();
    shaderHybridAnimPBR->setMat4("u_ViewProjectionMatrix", projectionMatrix * scene->GetCamera()->GetViewMatrix());

    DisableCulling();
    std::string passType = "water_refract";
    scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RendererEditor::RenderOmniShadows(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
    if (!scene->GetSettings().enableOmniShadows) return;

    for (size_t i = 0; i < LightManager::pointLightCount; i++) {
        if (LightManager::pointLights[i].GetEnabled()) {
            std::string profilerTitle = "RE::RenderPassOmniShadow[PL_" + std::to_string(i) + ']';
            Profiler profiler(profilerTitle);
            RenderPassOmniShadow(&LightManager::pointLights[i], mainWindow, scene, projectionMatrix);
            scene->GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
        }
    }

    for (size_t i = 0; i < LightManager::spotLightCount; i++) {
        if (LightManager::spotLights[i].GetBasePL()->GetEnabled()) {
            std::string profilerTitle = "RE::RenderPassOmniShadow[SL_" + std::to_string(i) + ']';
            Profiler profiler(profilerTitle);
            RenderPassOmniShadow((PointLight*)&LightManager::spotLights[i], mainWindow, scene, projectionMatrix);
            scene->GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
        }
    }
}

void RendererEditor::RenderWaterEffects(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
    if (!scene->GetSettings().enableWaterEffects) return;
    if (!scene->IsWaterOnScene()) return;

    glEnable(GL_CLIP_DISTANCE0);

    float waterMoveFactor = scene->GetWaterManager()->GetWaterMoveFactor();
    waterMoveFactor += WaterManager::m_WaveSpeed * deltaTime;
    if (waterMoveFactor >= 1.0f)
        waterMoveFactor = waterMoveFactor - 1.0f;
    scene->GetWaterManager()->SetWaterMoveFactor(waterMoveFactor);

    float distance = 2.0f * (scene->GetCamera()->GetPosition().y - scene->GetWaterManager()->GetWaterHeight());

    scene->GetCamera()->SetPosition(glm::vec3(scene->GetCamera()->GetPosition().x, scene->GetCamera()->GetPosition().y - distance, scene->GetCamera()->GetPosition().z));
    scene->GetCameraController()->InvertPitch();

    {
        Profiler profiler("RE::RenderPassWaterReflection");
        RenderPassWaterReflection(mainWindow, scene, projectionMatrix);
        scene->GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
    }

    scene->GetCamera()->SetPosition(glm::vec3(scene->GetCamera()->GetPosition().x, scene->GetCamera()->GetPosition().y + distance, scene->GetCamera()->GetPosition().z));
    scene->GetCameraController()->InvertPitch();

    {
        Profiler profiler("RE::RenderPassWaterRefraction");
        RenderPassWaterRefraction(mainWindow, scene, projectionMatrix);
        scene->GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
    }

    glDisable(GL_CLIP_DISTANCE0);
}

void RendererEditor::RenderPassMain(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
    SceneEditor* sceneEditor = (SceneEditor*)scene;
    Framebuffer* renderFramebuffer = nullptr;

    if (m_IsViewportEnabled)
    {
        renderFramebuffer = sceneEditor->GetRenderFramebuffer();
        renderFramebuffer->Bind();
        renderFramebuffer->Clear(); // Clear the window
    }
    else
    {
        glViewport(0, 0, (GLsizei)mainWindow->GetWidth(), (GLsizei)mainWindow->GetHeight());
    }

    // Clear the window
    RendererBasic::Clear();

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);
    // set depth function to less than AND equal for skybox depth trick.
    glDepthFunc(GL_LEQUAL);
    // enable seamless cubemap sampling for lower mip levels in the pre-filter map.
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // then before rendering, configure the viewport to the original framebuffer's screen dimensions
    if (!m_IsViewportEnabled)
        SetDefaultFramebuffer((unsigned int)mainWindow->GetWidth(), (unsigned int)mainWindow->GetHeight());

    EnableTransparency();
    EnableCulling();

    if (scene->GetSettings().enableSkybox)
    {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        float angleRadians = glm::radians((GLfloat)glfwGetTime());
        modelMatrix = glm::rotate(modelMatrix, angleRadians, glm::vec3(0.0f, 1.0f, 0.0f));
        scene->GetSkybox()->Draw(modelMatrix, scene->GetCamera()->GetViewMatrix(), projectionMatrix);
    }

    scene->GetSettings().enableCulling ? EnableCulling() : DisableCulling();
    std::string passType = "main";
    scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());

    if (m_IsViewportEnabled)
    {
        renderFramebuffer->Unbind();
    }
}

void RendererEditor::RenderStageSetUniforms(Scene* scene, glm::mat4* projectionMatrix)
{
    /**** Begin editor_object ****/
    Shader* shaderEditor = RendererBasic::GetShaders()["editor_object"];
    shaderEditor->Bind();

    shaderEditor->setMat4("model", glm::mat4(1.0f));
    shaderEditor->setMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderEditor->setMat4("projection", *projectionMatrix);
    shaderEditor->setMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());
    shaderEditor->setVec4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, -10000));
    shaderEditor->setInt("pointLightCount", LightManager::pointLightCount);
    shaderEditor->setInt("spotLightCount", LightManager::spotLightCount);
    // Eye position / camera direction
    shaderEditor->setVec3("eyePosition", scene->GetCamera()->GetPosition());
    shaderEditor->setFloat("waterLevel", scene->GetWaterManager()->GetWaterHeight());
    shaderEditor->setVec4("waterColor", scene->GetWaterManager()->GetWaterColor());

    // Directional Light
    shaderEditor->setBool("directionalLight.base.enabled", LightManager::directionalLight.GetEnabled());
    shaderEditor->setVec3("directionalLight.base.color", LightManager::directionalLight.GetColor());
    shaderEditor->setFloat("directionalLight.base.ambientIntensity", LightManager::directionalLight.GetAmbientIntensity());
    shaderEditor->setFloat("directionalLight.base.diffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
    shaderEditor->setVec3("directionalLight.direction", LightManager::directionalLight.GetDirection());

    char locBuff[100] = { '\0' };

    // Point Lights
    for (unsigned int i = 0; i < LightManager::pointLightCount; i++)
    {
        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.enabled", i);
        // printf("PointLight[%d] enabled: %d\n", i, LightManager::pointLights[i].GetEnabled());
        shaderEditor->setBool(locBuff, LightManager::pointLights[i].GetEnabled());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.color", i);
        shaderEditor->setVec3(locBuff, LightManager::pointLights[i].GetColor());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.ambientIntensity", i);
        shaderEditor->setFloat(locBuff, LightManager::pointLights[i].GetAmbientIntensity());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.diffuseIntensity", i);
        shaderEditor->setFloat(locBuff, LightManager::pointLights[i].GetDiffuseIntensity());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].position", i);
        shaderEditor->setVec3(locBuff, LightManager::pointLights[i].GetPosition());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].constant", i);
        shaderEditor->setFloat(locBuff, LightManager::pointLights[i].GetConstant());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].linear", i);
        shaderEditor->setFloat(locBuff, LightManager::pointLights[i].GetLinear());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].exponent", i);
        shaderEditor->setFloat(locBuff, LightManager::pointLights[i].GetExponent());

        // set uniforms for omni shadow maps
        // texture slot for 'omniShadowMaps[i].shadowMap' samplerCube in editor_object.fs is 3
        int textureSlotOffset = 0;
        LightManager::pointLights[i].GetShadowMap()->ReadTexture(m_OmniShadowTxSlots["editor_object"] + textureSlotOffset + i);
        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].shadowMap", textureSlotOffset + i);
        shaderEditor->setInt(locBuff, m_OmniShadowTxSlots["editor_object"] + textureSlotOffset + i);
        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].farPlane", textureSlotOffset + i);
        shaderEditor->setFloat(locBuff, LightManager::pointLights[i].GetFarPlane());
    }

    // Spot Lights
    for (unsigned int i = 0; i < LightManager::spotLightCount; i++)
    {
        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.base.enabled", i);
        shaderEditor->setBool(locBuff, LightManager::spotLights[i].GetBasePL()->GetEnabled());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.base.color", i);
        shaderEditor->setVec3(locBuff, LightManager::spotLights[i].GetBasePL()->GetColor());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.base.ambientIntensity", i);
        shaderEditor->setFloat(locBuff, LightManager::spotLights[i].GetBasePL()->GetAmbientIntensity());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.base.diffuseIntensity", i);
        shaderEditor->setFloat(locBuff, LightManager::spotLights[i].GetBasePL()->GetDiffuseIntensity());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.position", i);
        shaderEditor->setVec3(locBuff, LightManager::spotLights[i].GetBasePL()->GetPosition());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.constant", i);
        shaderEditor->setFloat(locBuff, LightManager::spotLights[i].GetBasePL()->GetConstant());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.linear", i);
        shaderEditor->setFloat(locBuff, LightManager::spotLights[i].GetBasePL()->GetLinear());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.exponent", i);
        shaderEditor->setFloat(locBuff, LightManager::spotLights[i].GetBasePL()->GetExponent());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].direction", i);
        shaderEditor->setVec3(locBuff, LightManager::spotLights[i].GetDirection());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].edge", i);
        shaderEditor->setFloat(locBuff, LightManager::spotLights[i].GetEdge());

        // set uniforms for omni shadow maps
        // texture slot for 'omniShadowMaps[i].shadowMap' samplerCube in editor_object.fs is 3
        int textureSlotOffset = LightManager::pointLightCount;
        LightManager::spotLights[i].GetShadowMap()->ReadTexture(m_OmniShadowTxSlots["editor_object"] + textureSlotOffset + i);
        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].shadowMap", textureSlotOffset + i);
        shaderEditor->setInt(locBuff, m_OmniShadowTxSlots["editor_object"] + textureSlotOffset + i);
        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].farPlane", textureSlotOffset + i);
        shaderEditor->setFloat(locBuff, LightManager::spotLights[i].GetFarPlane());
    }
    /**** End editor_object ****/

    /**** Begin editor_object_pbr ****/
    // Init shaderEditorPBR
    Shader* shaderEditorPBR = RendererBasic::GetShaders()["editor_object_pbr"];
    shaderEditorPBR->Bind();

    // initialize static shader uniforms before rendering
    shaderEditorPBR->setMat4("model", glm::mat4(1.0f));
    shaderEditorPBR->setMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderEditorPBR->setMat4("projection", *projectionMatrix);
    shaderEditorPBR->setVec3("eyePosition", scene->GetCamera()->GetPosition());
    shaderEditorPBR->setMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());
    shaderEditorPBR->setVec4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, -10000));
    shaderEditorPBR->setFloat("waterLevel", scene->GetWaterManager()->GetWaterHeight());
    shaderEditorPBR->setVec4("waterColor", scene->GetWaterManager()->GetWaterColor());

    shaderEditorPBR->setInt("pointSpotLightCount", LightManager::pointLightCount + LightManager::spotLightCount);

    // directional light
    shaderEditorPBR->setBool("directionalLight.base.enabled", LightManager::directionalLight.GetEnabled());
    shaderEditorPBR->setVec3("directionalLight.base.color", LightManager::directionalLight.GetColor());
    shaderEditorPBR->setFloat("directionalLight.base.ambientIntensity", LightManager::directionalLight.GetAmbientIntensity());
    shaderEditorPBR->setFloat("directionalLight.base.diffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
    shaderEditorPBR->setVec3("directionalLight.direction", LightManager::directionalLight.GetDirection());
    // printf("Exponent = %.2ff Linear = %.2ff Constant = %.2ff\n", *m_PointLightExponent, *m_PointLightLinear, *m_PointLightConstant);

    // point lights
    unsigned int lightIndex = 0;
    for (unsigned int i = 0; i < LightManager::pointLightCount; ++i)
    {
        lightIndex = 0 + i; // offset for point lights
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].base.enabled", lightIndex);
        shaderEditorPBR->setBool(locBuff, LightManager::pointLights[i].GetEnabled());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].base.color", lightIndex);
        shaderEditorPBR->setVec3(locBuff, LightManager::pointLights[i].GetColor());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].base.ambientIntensity", lightIndex);
        shaderEditorPBR->setFloat(locBuff, LightManager::pointLights[i].GetAmbientIntensity());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].base.diffuseIntensity", lightIndex);
        shaderEditorPBR->setFloat(locBuff, LightManager::pointLights[i].GetDiffuseIntensity());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].position", lightIndex);
        shaderEditorPBR->setVec3(locBuff, LightManager::pointLights[i].GetPosition());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].exponent", lightIndex);
        shaderEditorPBR->setFloat(locBuff, LightManager::pointLights[i].GetExponent());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].linear", lightIndex);
        shaderEditorPBR->setFloat(locBuff, LightManager::pointLights[i].GetLinear());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].constant", lightIndex);
        shaderEditorPBR->setFloat(locBuff, LightManager::pointLights[i].GetConstant());

        // set uniforms for omni shadow maps
        // texture slot for 'omniShadowMaps[i].shadowMap' samplerCube in editor_object_pbr.fs is 9
        int textureSlotOffset = 0;
        LightManager::pointLights[i].GetShadowMap()->ReadTexture(m_OmniShadowTxSlots["editor_object_pbr"] + textureSlotOffset + i);
        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].shadowMap", textureSlotOffset + i);
        shaderEditorPBR->setInt(locBuff, m_OmniShadowTxSlots["editor_object_pbr"] + textureSlotOffset + i);
        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].farPlane", textureSlotOffset + i);
        shaderEditorPBR->setFloat(locBuff, LightManager::pointLights[i].GetFarPlane());
    }

    for (unsigned int i = 0; i < LightManager::spotLightCount; ++i)
    {
        lightIndex = LightManager::pointLightCount + i; // offset for spot lights

        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].base.enabled", lightIndex);
        shaderEditorPBR->setBool(locBuff, LightManager::spotLights[i].GetBasePL()->GetEnabled());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].base.color", lightIndex);
        shaderEditorPBR->setVec3(locBuff, LightManager::spotLights[i].GetBasePL()->GetColor());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].base.ambientIntensity", lightIndex);
        shaderEditorPBR->setFloat(locBuff, LightManager::spotLights[i].GetBasePL()->GetAmbientIntensity());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].base.diffuseIntensity", lightIndex);
        shaderEditorPBR->setFloat(locBuff, LightManager::spotLights[i].GetBasePL()->GetDiffuseIntensity());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].position", lightIndex);
        shaderEditorPBR->setVec3(locBuff, LightManager::spotLights[i].GetBasePL()->GetPosition());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].exponent", lightIndex);
        shaderEditorPBR->setFloat(locBuff, LightManager::spotLights[i].GetBasePL()->GetExponent());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].linear", lightIndex);
        shaderEditorPBR->setFloat(locBuff, LightManager::spotLights[i].GetBasePL()->GetLinear());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].constant", lightIndex);
        shaderEditorPBR->setFloat(locBuff, LightManager::spotLights[i].GetBasePL()->GetConstant());

        // set uniforms for omni shadow maps
        // texture slot for 'omniShadowMaps[i].shadowMap' samplerCube in editor_object_pbr.fs is 9
        int textureSlotOffset = LightManager::pointLightCount;
        LightManager::spotLights[i].GetShadowMap()->ReadTexture(m_OmniShadowTxSlots["editor_object_pbr"] + textureSlotOffset + i);
        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].shadowMap", textureSlotOffset + i);
        shaderEditorPBR->setInt(locBuff, m_OmniShadowTxSlots["editor_object_pbr"] + textureSlotOffset + i);
        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].farPlane", textureSlotOffset + i);
        shaderEditorPBR->setFloat(locBuff, LightManager::spotLights[i].GetFarPlane());
    }
    /**** End editor_object_pbr ****/

    /**** Begin skinning ****/
    Shader* shaderSkinning = RendererBasic::GetShaders()["skinning"];
    shaderSkinning->Bind();

    shaderSkinning->setMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderSkinning->setMat4("projection", *projectionMatrix);
    shaderSkinning->setVec4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, -10000));
    shaderSkinning->setVec3("gEyeWorldPos", scene->GetCamera()->GetPosition());
    shaderSkinning->setFloat("waterLevel", scene->GetWaterManager()->GetWaterHeight());
    shaderSkinning->setVec4("waterColor", scene->GetWaterManager()->GetWaterColor());

    // Directional Light
    shaderSkinning->setVec3("gDirectionalLight.Base.Color", LightManager::directionalLight.GetColor());
    shaderSkinning->setFloat("gDirectionalLight.Base.AmbientIntensity", LightManager::directionalLight.GetAmbientIntensity());
    shaderSkinning->setFloat("gDirectionalLight.Base.DiffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
    shaderSkinning->setVec3("gDirectionalLight.Direction", LightManager::directionalLight.GetDirection());

    // TODO: point lights
    shaderSkinning->setInt("gNumPointLights", 0);

    // TODO: spot lights
    shaderSkinning->setInt("gNumSpotLights", 0);
    /**** End skinning ****/

    /**** Begin Hybrid Anim PBR ****/
    Shader* shaderHybridAnimPBR = RendererBasic::GetShaders()["hybrid_anim_pbr"];
    shaderHybridAnimPBR->Bind();
    shaderHybridAnimPBR->setMat4("u_ViewProjectionMatrix", *projectionMatrix * scene->GetCamera()->GetViewMatrix());
    shaderHybridAnimPBR->setVec3("u_CameraPosition", scene->GetCamera()->GetPosition());

    // point lights
    lightIndex = 0;
    for (unsigned int i = 0; i < LightManager::pointLightCount; ++i)
    {
        lightIndex = 0 + i; // offset for point lights

        std::string uniformName = std::string("lightPositions[") + std::to_string(lightIndex) + std::string("]");
        shaderHybridAnimPBR->setVec3(uniformName, LightManager::pointLights[i].GetPosition());
        uniformName = std::string("lightColors[") + std::to_string(lightIndex) + std::string("]");
        shaderHybridAnimPBR->setVec3(uniformName, LightManager::pointLights[i].GetColor());
    }

    for (unsigned int i = 0; i < LightManager::spotLightCount; ++i)
    {
        lightIndex = LightManager::pointLightCount + i; // offset for spot lights

        std::string uniformName = std::string("lightPositions[") + std::to_string(lightIndex) + std::string("]");
        shaderHybridAnimPBR->setVec3(uniformName, LightManager::spotLights[i].GetBasePL()->GetPosition());
        uniformName = std::string("lightColors[") + std::to_string(lightIndex) + std::string("]");
        shaderHybridAnimPBR->setVec3(uniformName, LightManager::spotLights[i].GetBasePL()->GetColor());
    }
    /**** End Hybrid Anim PBR ****/

    /**** Begin shadow_map ****/
    Shader* shaderShadowMap = RendererBasic::GetShaders()["shadow_map"];
    shaderShadowMap->Bind();
    shaderShadowMap->setMat4("u_DirLightTransform", LightManager::directionalLight.CalculateLightTransform());
    shaderShadowMap->setBool("u_Animated", false);
    /**** End shadow_map ****/

    /**** Begin omni_shadow_map ****/
    Shader* shaderOmniShadowMap = RendererBasic::GetShaders()["omni_shadow_map"];
    shaderOmniShadowMap->Bind();
    shaderOmniShadowMap->setVec3("lightPosition", LightManager::directionalLight.GetPosition());
    shaderOmniShadowMap->setFloat("farPlane", scene->GetSettings().farPlane);
    /**** End omni_shadow_map ****/

    /**** Begin shaderWater ****/
    Shader* shaderWater = RendererBasic::GetShaders()["water"];
    shaderWater->Bind();

    shaderWater->setMat4("projection", *projectionMatrix);
    shaderWater->setMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderWater->setVec3("lightPosition", LightManager::directionalLight.GetPosition());
    shaderWater->setVec3("cameraPosition", scene->GetCamera()->GetPosition());
    shaderWater->setVec3("lightColor", LightManager::directionalLight.GetColor());
    shaderWater->setFloat("moveFactor", scene->GetWaterManager()->GetWaterMoveFactor());
    shaderWater->setFloat("nearPlane", scene->GetSettings().nearPlane);
    shaderWater->setFloat("farPlane", scene->GetSettings().farPlane);
    shaderWater->setVec3("eyePosition", scene->GetCamera()->GetPosition());
    shaderWater->setFloat("waterLevel", scene->GetWaterManager()->GetWaterHeight());
    shaderWater->setVec4("waterColor", scene->GetWaterManager()->GetWaterColor());
    /**** End shaderWater ****/

    /**** Begin Background shader ****/
    Shader* shaderBackground = RendererBasic::GetShaders()["background"];
    shaderBackground->Bind();
    shaderBackground->setMat4("projection", *projectionMatrix);
    shaderBackground->setMat4("view", scene->GetCamera()->GetViewMatrix());
    /**** End Background shader ****/

    /**** Begin of shaderBasic ****/
    Shader* shaderBasic = RendererBasic::GetShaders()["basic"];
    shaderBasic->Bind();
    shaderBasic->setMat4("projection", *projectionMatrix);
    shaderBasic->setMat4("view", scene->GetCamera()->GetViewMatrix());
    /**** End of shaderBasic ****/

    /**** Begin gizmo shader ****/
    Shader* shaderGizmo = RendererBasic::GetShaders()["gizmo"];
    shaderGizmo->Bind();
    // shaderGizmo->setMat4("projection", *projectionMatrix);

    // experimental
    if (((SceneEditor*)scene)->m_GizmoOrthoProjection) {
        float aspectRatio = scene->GetCameraController()->GetAspectRatio();
        float sizeCoef = 5.0f;
        glm::mat4 orthoMatrix = glm::ortho(-aspectRatio * sizeCoef, aspectRatio * sizeCoef, -1.0f * sizeCoef, 1.0f * sizeCoef,
            scene->GetSettings().nearPlane, scene->GetSettings().farPlane);
        shaderGizmo->setMat4("projection", orthoMatrix);
    }
    else {
        shaderGizmo->setMat4("projection", *projectionMatrix);
    }

    shaderGizmo->setMat4("view", scene->GetCamera()->GetViewMatrix());
    // Directional Light
    shaderGizmo->setBool("directionalLight.base.enabled", LightManager::directionalLight.GetEnabled());
    shaderGizmo->setVec3("directionalLight.base.color", LightManager::directionalLight.GetColor());
    shaderGizmo->setFloat("directionalLight.base.ambientIntensity", LightManager::directionalLight.GetAmbientIntensity());
    shaderGizmo->setFloat("directionalLight.base.diffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
    shaderGizmo->setVec3("directionalLight.direction", LightManager::directionalLight.GetDirection());
    /**** End gizmo shader ****/

    /**** Begin glass ****/
    Shader* shaderGlass = RendererBasic::GetShaders()["glass"];
    shaderGlass->Bind();
    shaderGlass->setMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderGlass->setMat4("projection", *projectionMatrix);
    shaderGlass->setVec3("cameraPosition", scene->GetCamera()->GetPosition());
    /**** End glass ****/
}

void RendererEditor::BeginFrame()
{
}

void RendererEditor::WaitAndRender(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
    float aspectRatio = scene->GetCameraController()->GetAspectRatio();
    projectionMatrix = glm::perspective(glm::radians(scene->GetFOV()), aspectRatio,
        scene->GetSettings().nearPlane, scene->GetSettings().farPlane);
    RendererBasic::SetProjectionMatrix(projectionMatrix);

    RenderStageSetUniforms(scene, &projectionMatrix);

    {
        Profiler profiler("RE::RenderPassShadow");
        RenderPassShadow(mainWindow, scene, projectionMatrix);
        scene->GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
    }
    {
        Profiler profiler("RE::RenderOmniShadows");
        RenderOmniShadows(mainWindow, scene, projectionMatrix);
        scene->GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
    }
    {
        Profiler profiler("RE::RenderWaterEffects");
        RenderWaterEffects(deltaTime, mainWindow, scene, projectionMatrix);
        scene->GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
    }
    {
        Profiler profiler("RE::RenderPass");
        RenderPassMain(mainWindow, scene, projectionMatrix);
        scene->GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
    }
}
