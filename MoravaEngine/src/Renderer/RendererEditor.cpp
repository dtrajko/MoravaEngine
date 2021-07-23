#include "Renderer/RendererEditor.h"

#include "Core/Application.h"
#include "Core/Profiler.h"
#include "Core/Timer.h"
#include "Framebuffer/MoravaFramebuffer.h"
#include "Mesh/GeometryFactory.h"
#include "Scene/SceneEditor.h"
#include "Platform/OpenGL/OpenGLMoravaShader.h"

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
    Hazel::Ref<OpenGLMoravaShader> shaderEditor = MoravaShader::Create("Shaders/editor_object.vs", "Shaders/editor_object.fs");
    RendererBasic::GetShaders().insert(std::make_pair("editor_object", shaderEditor));
    Log::GetLogger()->info("RendererEditor: shaderEditor compiled [programID={0}]", shaderEditor->GetProgramID());

    Hazel::Ref<OpenGLMoravaShader> shaderEditorPBR = MoravaShader::Create("Shaders/editor_object.vs", "Shaders/PBR/editor_object_pbr.fs");
    RendererBasic::GetShaders().insert(std::make_pair("editor_object_pbr", shaderEditorPBR));
    Log::GetLogger()->info("RendererEditor: shaderEditorPBR compiled [programID={0}]", shaderEditorPBR->GetProgramID());

    Hazel::Ref<OpenGLMoravaShader> shaderSkinning = MoravaShader::Create("Shaders/OGLdev/skinning.vs", "Shaders/OGLdev/skinning.fs");
    RendererBasic::GetShaders().insert(std::make_pair("skinning", shaderSkinning));
    Log::GetLogger()->info("RendererEditor: shaderSkinning compiled [programID={0}]", shaderSkinning->GetProgramID());

    Hazel::Ref<OpenGLMoravaShader> shaderHybridAnimPBR = MoravaShader::Create("Shaders/HybridAnimPBR.vs", "Shaders/HybridAnimPBR.fs");
    RendererBasic::GetShaders().insert(std::make_pair("hybrid_anim_pbr", shaderHybridAnimPBR));
    Log::GetLogger()->info("RendererEditor: shaderHybridAnimPBR compiled [programID={0}]", shaderHybridAnimPBR->GetProgramID());

    Hazel::Ref<OpenGLMoravaShader> shaderShadowMap = MoravaShader::Create("Shaders/directional_shadow_map.vert", "Shaders/directional_shadow_map.frag");
    RendererBasic::GetShaders().insert(std::make_pair("shadow_map", shaderShadowMap));
    Log::GetLogger()->info("RendererEditor: shaderShadowMap compiled [programID={0}]", shaderShadowMap->GetProgramID());

    Hazel::Ref<OpenGLMoravaShader> shaderOmniShadowMap = MoravaShader::Create("Shaders/omni_shadow_map.vert", "Shaders/omni_shadow_map.geom", "Shaders/omni_shadow_map.frag");
    RendererBasic::GetShaders().insert(std::make_pair("omni_shadow_map", shaderOmniShadowMap));
    Log::GetLogger()->info("RendererEditor: shaderOmniShadowMap compiled [programID={0}]", shaderOmniShadowMap->GetProgramID());

    Hazel::Ref<OpenGLMoravaShader> shaderWater = MoravaShader::Create("Shaders/water.vert", "Shaders/water.frag");
    RendererBasic::GetShaders().insert(std::make_pair("water", shaderWater));
    Log::GetLogger()->info("RendererEditor: shaderWater compiled [programID={0}]", shaderWater->GetProgramID());

    Hazel::Ref<OpenGLMoravaShader> shaderBackground = MoravaShader::Create("Shaders/LearnOpenGL/2.2.2.background.vs", "Shaders/LearnOpenGL/2.2.2.background.fs");
    RendererBasic::GetShaders().insert(std::make_pair("background", shaderBackground));
    Log::GetLogger()->info("RendererEditor: shaderBackground compiled [programID={0}]", shaderBackground->GetProgramID());

    Hazel::Ref<OpenGLMoravaShader> shaderBasic = MoravaShader::Create("Shaders/basic.vs", "Shaders/basic.fs");
    RendererBasic::GetShaders().insert(std::make_pair("basic", shaderBasic));
    Log::GetLogger()->info("RendererEditor: shaderBasic compiled [programID={0}]", shaderBasic->GetProgramID());

    Hazel::Ref<OpenGLMoravaShader> shaderGizmo = MoravaShader::Create("Shaders/gizmo.vs", "Shaders/gizmo.fs");
    RendererBasic::GetShaders().insert(std::make_pair("gizmo", shaderGizmo));
    Log::GetLogger()->info("RendererEditor: shaderGizmo compiled [programID={0}]", shaderGizmo->GetProgramID());

    Hazel::Ref<OpenGLMoravaShader> shaderGlass = MoravaShader::Create("Shaders/glass.vs", "Shaders/glass.fs");
    RendererBasic::GetShaders().insert(std::make_pair("glass", shaderGlass));
    Log::GetLogger()->info("RendererEditor: shaderGlass compiled [programID={0}]", shaderGlass->GetProgramID());

    shaderEditor->Bind();
    shaderEditor->SetInt("albedoMap", 0);
    shaderEditor->SetInt("cubeMap",   1);
    shaderEditor->SetInt("shadowMap", 2);
    m_OmniShadowTxSlots.insert(std::make_pair("editor_object", 3)); // omniShadowMaps[i].shadowMap = 3

    shaderEditorPBR->Bind();
    shaderEditorPBR->SetInt("irradianceMap", 0);
    shaderEditorPBR->SetInt("prefilterMap",  1);
    shaderEditorPBR->SetInt("brdfLUT",       2);
    shaderEditorPBR->SetInt("albedoMap",     3);
    shaderEditorPBR->SetInt("normalMap",     4);
    shaderEditorPBR->SetInt("metallicMap",   5);
    shaderEditorPBR->SetInt("roughnessMap",  6);
    shaderEditorPBR->SetInt("aoMap",         7);
    shaderEditorPBR->SetInt("shadowMap",     8);
    m_OmniShadowTxSlots.insert(std::make_pair("editor_object_pbr", 9)); // omniShadowMaps[i].shadowMap = 9

    shaderHybridAnimPBR->Bind();
    shaderHybridAnimPBR->SetInt("u_AlbedoTexture",    1);
    shaderHybridAnimPBR->SetInt("u_NormalTexture",    2);
    shaderHybridAnimPBR->SetInt("u_MetalnessTexture", 3);
    shaderHybridAnimPBR->SetInt("u_RoughnessTexture", 4);
    shaderHybridAnimPBR->SetInt("u_EnvRadianceTex",   5);
    shaderHybridAnimPBR->SetInt("u_PrefilterMap",     6);
    shaderHybridAnimPBR->SetInt("u_BRDFLUT",          7);
}

void RendererEditor::RenderPassShadow(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
    if (!scene->GetSettings().enableShadows) return;
    if (!LightManager::directionalLight.GetEnabled()) return;
    if (LightManager::directionalLight.GetShadowMap() == nullptr) return;

    Hazel::Ref<OpenGLMoravaShader> shaderShadowMap = RendererBasic::GetShaders()["shadow_map"];
    shaderShadowMap->Bind();

    DirectionalLight* light = &LightManager::directionalLight;
    glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

    light->GetShadowMap()->BindForWriting();
    // printf("RenderPassShadow write to FBO = %i Texture ID = %i\n", light->GetShadowMap()->GetFBO(), light->GetShadowMap()->GetTextureID());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);

    shaderShadowMap->SetMat4("u_DirLightTransform", light->CalculateLightTransform());
    shaderShadowMap->SetBool("u_Animated", false);
    shaderShadowMap->Validate();

    DisableCulling();
    std::string passType = "shadow_dir";
    scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RendererEditor::RenderPassOmniShadow(PointLight* light, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
    if (!scene->GetSettings().enableOmniShadows) return;

    Hazel::Ref<MoravaShader> shaderOmniShadow = RendererBasic::GetShaders()["omni_shadow_map"];
    shaderOmniShadow->Bind();

    glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

    light->GetShadowMap()->BindForWriting();
    // printf("RenderPassOmniShadow write to FBO = %i Texture ID = %i\n", light->GetShadowMap()->GetFBO(), light->GetShadowMap()->GetTextureID());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);

    shaderOmniShadow->SetFloat3("lightPosition", light->GetPosition());
    shaderOmniShadow->SetFloat("farPlane", light->GetFarPlane());
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

    Hazel::Ref<OpenGLMoravaShader> shaderEditor = RendererBasic::GetShaders()["editor_object"];
    shaderEditor->Bind();
    shaderEditor->SetMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderEditor->SetMat4("projection", projectionMatrix);
    shaderEditor->SetFloat3("eyePosition", scene->GetCamera()->GetPosition());
    shaderEditor->SetMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());
    shaderEditor->SetFloat4("clipPlane", glm::vec4(0.0f, 1.0f, 0.0f, -scene->GetWaterManager()->GetWaterHeight())); // reflection clip plane
    
    Hazel::Ref<MoravaShader> shaderEditorPBR = RendererBasic::GetShaders()["editor_object_pbr"];
    shaderEditorPBR->Bind();
    shaderEditorPBR->SetMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderEditorPBR->SetMat4("projection", projectionMatrix);
    shaderEditorPBR->SetFloat3("eyePosition", scene->GetCamera()->GetPosition());
    shaderEditorPBR->SetMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());
    shaderEditorPBR->SetFloat4("clipPlane", glm::vec4(0.0f, 1.0f, 0.0f, -scene->GetWaterManager()->GetWaterHeight())); // reflection clip plane

    Hazel::Ref<MoravaShader> shaderSkinning = RendererBasic::GetShaders()["skinning"];
    shaderSkinning->Bind();
    shaderSkinning->SetMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderSkinning->SetMat4("projection", projectionMatrix);
    shaderSkinning->SetFloat4("clipPlane", glm::vec4(0.0f, 1.0f, 0.0f, -scene->GetWaterManager()->GetWaterHeight())); // reflection clip plane

    Hazel::Ref<OpenGLMoravaShader> shaderHybridAnimPBR = RendererBasic::GetShaders()["hybrid_anim_pbr"];
    shaderHybridAnimPBR->Bind();
    shaderHybridAnimPBR->SetMat4("u_ViewProjectionMatrix", projectionMatrix * scene->GetCamera()->GetViewMatrix());

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

    Hazel::Ref<OpenGLMoravaShader> shaderEditor = RendererBasic::GetShaders()["editor_object"];
    shaderEditor->Bind();
    shaderEditor->SetMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderEditor->SetMat4("projection", projectionMatrix);
    shaderEditor->SetFloat3("eyePosition", scene->GetCamera()->GetPosition());
    shaderEditor->SetMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());
    shaderEditor->SetFloat4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, scene->GetWaterManager()->GetWaterHeight())); // refraction clip plane

    Hazel::Ref<OpenGLMoravaShader> shaderEditorPBR = RendererBasic::GetShaders()["editor_object_pbr"];
    shaderEditorPBR->Bind();
    shaderEditorPBR->SetMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderEditorPBR->SetMat4("projection", projectionMatrix);
    shaderEditorPBR->SetFloat3("eyePosition", scene->GetCamera()->GetPosition());
    shaderEditorPBR->SetMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());
    shaderEditorPBR->SetFloat4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, scene->GetWaterManager()->GetWaterHeight())); // refraction clip plane

    Hazel::Ref<OpenGLMoravaShader> shaderSkinning = RendererBasic::GetShaders()["skinning"];
    shaderSkinning->Bind();
    shaderSkinning->SetMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderSkinning->SetMat4("projection", projectionMatrix);
    shaderSkinning->SetFloat4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, scene->GetWaterManager()->GetWaterHeight())); // refraction clip plane

    Hazel::Ref<OpenGLMoravaShader> shaderHybridAnimPBR = RendererBasic::GetShaders()["hybrid_anim_pbr"];
    shaderHybridAnimPBR->Bind();
    shaderHybridAnimPBR->SetMat4("u_ViewProjectionMatrix", projectionMatrix * scene->GetCamera()->GetViewMatrix());

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
    Hazel::Ref<MoravaFramebuffer> renderFramebuffer;

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
    Hazel::Ref<OpenGLMoravaShader> shaderEditor = RendererBasic::GetShaders()["editor_object"];
    shaderEditor->Bind();

    shaderEditor->SetMat4("model", glm::mat4(1.0f));
    shaderEditor->SetMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderEditor->SetMat4("projection", *projectionMatrix);
    shaderEditor->SetMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());
    shaderEditor->SetFloat4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, -10000));
    shaderEditor->SetInt("pointLightCount", LightManager::pointLightCount);
    shaderEditor->SetInt("spotLightCount", LightManager::spotLightCount);
    // Eye position / camera direction
    shaderEditor->SetFloat3("eyePosition", scene->GetCamera()->GetPosition());
    shaderEditor->SetFloat("waterLevel", scene->GetWaterManager()->GetWaterHeight());
    shaderEditor->SetFloat4("waterColor", scene->GetWaterManager()->GetWaterColor());

    // Directional Light
    shaderEditor->SetBool("directionalLight.base.enabled", LightManager::directionalLight.GetEnabled());
    shaderEditor->SetFloat3("directionalLight.base.color", LightManager::directionalLight.GetColor());
    shaderEditor->SetFloat("directionalLight.base.ambientIntensity", LightManager::directionalLight.GetAmbientIntensity());
    shaderEditor->SetFloat("directionalLight.base.diffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
    shaderEditor->SetFloat3("directionalLight.direction", LightManager::directionalLight.GetDirection());

    char locBuff[100] = { '\0' };

    // Point Lights
    for (unsigned int i = 0; i < LightManager::pointLightCount; i++)
    {
        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.enabled", i);
        // printf("PointLight[%d] enabled: %d\n", i, LightManager::pointLights[i].GetEnabled());
        shaderEditor->SetBool(locBuff, LightManager::pointLights[i].GetEnabled());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.color", i);
        shaderEditor->SetFloat3(locBuff, LightManager::pointLights[i].GetColor());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.ambientIntensity", i);
        shaderEditor->SetFloat(locBuff, LightManager::pointLights[i].GetAmbientIntensity());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.diffuseIntensity", i);
        shaderEditor->SetFloat(locBuff, LightManager::pointLights[i].GetDiffuseIntensity());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].position", i);
        shaderEditor->SetFloat3(locBuff, LightManager::pointLights[i].GetPosition());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].constant", i);
        shaderEditor->SetFloat(locBuff, LightManager::pointLights[i].GetConstant());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].linear", i);
        shaderEditor->SetFloat(locBuff, LightManager::pointLights[i].GetLinear());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].exponent", i);
        shaderEditor->SetFloat(locBuff, LightManager::pointLights[i].GetExponent());

        // set uniforms for omni shadow maps
        // texture slot for 'omniShadowMaps[i].shadowMap' samplerCube in editor_object.fs is 3
        int textureSlotOffset = 0;
        LightManager::pointLights[i].GetShadowMap()->ReadTexture(m_OmniShadowTxSlots["editor_object"] + textureSlotOffset + i);
        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].shadowMap", textureSlotOffset + i);
        shaderEditor->SetInt(locBuff, m_OmniShadowTxSlots["editor_object"] + textureSlotOffset + i);
        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].farPlane", textureSlotOffset + i);
        shaderEditor->SetFloat(locBuff, LightManager::pointLights[i].GetFarPlane());
    }

    // Spot Lights
    for (unsigned int i = 0; i < LightManager::spotLightCount; i++)
    {
        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.base.enabled", i);
        shaderEditor->SetBool(locBuff, LightManager::spotLights[i].GetBasePL()->GetEnabled());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.base.color", i);
        shaderEditor->SetFloat3(locBuff, LightManager::spotLights[i].GetBasePL()->GetColor());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.base.ambientIntensity", i);
        shaderEditor->SetFloat(locBuff, LightManager::spotLights[i].GetBasePL()->GetAmbientIntensity());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.base.diffuseIntensity", i);
        shaderEditor->SetFloat(locBuff, LightManager::spotLights[i].GetBasePL()->GetDiffuseIntensity());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.position", i);
        shaderEditor->SetFloat3(locBuff, LightManager::spotLights[i].GetBasePL()->GetPosition());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.constant", i);
        shaderEditor->SetFloat(locBuff, LightManager::spotLights[i].GetBasePL()->GetConstant());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.linear", i);
        shaderEditor->SetFloat(locBuff, LightManager::spotLights[i].GetBasePL()->GetLinear());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.exponent", i);
        shaderEditor->SetFloat(locBuff, LightManager::spotLights[i].GetBasePL()->GetExponent());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].direction", i);
        shaderEditor->SetFloat3(locBuff, LightManager::spotLights[i].GetDirection());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].edge", i);
        shaderEditor->SetFloat(locBuff, LightManager::spotLights[i].GetEdge());

        // set uniforms for omni shadow maps
        // texture slot for 'omniShadowMaps[i].shadowMap' samplerCube in editor_object.fs is 3
        int textureSlotOffset = LightManager::pointLightCount;
        LightManager::spotLights[i].GetShadowMap()->ReadTexture(m_OmniShadowTxSlots["editor_object"] + textureSlotOffset + i);
        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].shadowMap", textureSlotOffset + i);
        shaderEditor->SetInt(locBuff, m_OmniShadowTxSlots["editor_object"] + textureSlotOffset + i);
        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].farPlane", textureSlotOffset + i);
        shaderEditor->SetFloat(locBuff, LightManager::spotLights[i].GetFarPlane());
    }
    /**** End editor_object ****/

    /**** Begin editor_object_pbr ****/
    // Init shaderEditorPBR
    Hazel::Ref<OpenGLMoravaShader> shaderEditorPBR = RendererBasic::GetShaders()["editor_object_pbr"];
    shaderEditorPBR->Bind();

    // initialize static shader uniforms before rendering
    shaderEditorPBR->SetMat4("model", glm::mat4(1.0f));
    shaderEditorPBR->SetMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderEditorPBR->SetMat4("projection", *projectionMatrix);
    shaderEditorPBR->SetFloat3("eyePosition", scene->GetCamera()->GetPosition());
    shaderEditorPBR->SetMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());
    shaderEditorPBR->SetFloat4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, -10000));
    shaderEditorPBR->SetFloat("waterLevel", scene->GetWaterManager()->GetWaterHeight());
    shaderEditorPBR->SetFloat4("waterColor", scene->GetWaterManager()->GetWaterColor());

    shaderEditorPBR->SetInt("pointSpotLightCount", LightManager::pointLightCount + LightManager::spotLightCount);

    // directional light
    shaderEditorPBR->SetBool("directionalLight.base.enabled", LightManager::directionalLight.GetEnabled());
    shaderEditorPBR->SetFloat3("directionalLight.base.color", LightManager::directionalLight.GetColor());
    shaderEditorPBR->SetFloat("directionalLight.base.ambientIntensity", LightManager::directionalLight.GetAmbientIntensity());
    shaderEditorPBR->SetFloat("directionalLight.base.diffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
    shaderEditorPBR->SetFloat3("directionalLight.direction", LightManager::directionalLight.GetDirection());
    // printf("Exponent = %.2ff Linear = %.2ff Constant = %.2ff\n", *m_PointLightExponent, *m_PointLightLinear, *m_PointLightConstant);

    // point lights
    unsigned int lightIndex = 0;
    for (unsigned int i = 0; i < LightManager::pointLightCount; ++i)
    {
        lightIndex = 0 + i; // offset for point lights
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].base.enabled", lightIndex);
        shaderEditorPBR->SetBool(locBuff, LightManager::pointLights[i].GetEnabled());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].base.color", lightIndex);
        shaderEditorPBR->SetFloat3(locBuff, LightManager::pointLights[i].GetColor());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].base.ambientIntensity", lightIndex);
        shaderEditorPBR->SetFloat(locBuff, LightManager::pointLights[i].GetAmbientIntensity());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].base.diffuseIntensity", lightIndex);
        shaderEditorPBR->SetFloat(locBuff, LightManager::pointLights[i].GetDiffuseIntensity());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].position", lightIndex);
        shaderEditorPBR->SetFloat3(locBuff, LightManager::pointLights[i].GetPosition());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].exponent", lightIndex);
        shaderEditorPBR->SetFloat(locBuff, LightManager::pointLights[i].GetExponent());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].linear", lightIndex);
        shaderEditorPBR->SetFloat(locBuff, LightManager::pointLights[i].GetLinear());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].constant", lightIndex);
        shaderEditorPBR->SetFloat(locBuff, LightManager::pointLights[i].GetConstant());

        // set uniforms for omni shadow maps
        // texture slot for 'omniShadowMaps[i].shadowMap' samplerCube in editor_object_pbr.fs is 9
        int textureSlotOffset = 0;
        LightManager::pointLights[i].GetShadowMap()->ReadTexture(m_OmniShadowTxSlots["editor_object_pbr"] + textureSlotOffset + i);
        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].shadowMap", textureSlotOffset + i);
        shaderEditorPBR->SetInt(locBuff, m_OmniShadowTxSlots["editor_object_pbr"] + textureSlotOffset + i);
        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].farPlane", textureSlotOffset + i);
        shaderEditorPBR->SetFloat(locBuff, LightManager::pointLights[i].GetFarPlane());
    }

    for (unsigned int i = 0; i < LightManager::spotLightCount; ++i)
    {
        lightIndex = LightManager::pointLightCount + i; // offset for spot lights

        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].base.enabled", lightIndex);
        shaderEditorPBR->SetBool(locBuff, LightManager::spotLights[i].GetBasePL()->GetEnabled());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].base.color", lightIndex);
        shaderEditorPBR->SetFloat3(locBuff, LightManager::spotLights[i].GetBasePL()->GetColor());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].base.ambientIntensity", lightIndex);
        shaderEditorPBR->SetFloat(locBuff, LightManager::spotLights[i].GetBasePL()->GetAmbientIntensity());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].base.diffuseIntensity", lightIndex);
        shaderEditorPBR->SetFloat(locBuff, LightManager::spotLights[i].GetBasePL()->GetDiffuseIntensity());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].position", lightIndex);
        shaderEditorPBR->SetFloat3(locBuff, LightManager::spotLights[i].GetBasePL()->GetPosition());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].exponent", lightIndex);
        shaderEditorPBR->SetFloat(locBuff, LightManager::spotLights[i].GetBasePL()->GetExponent());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].linear", lightIndex);
        shaderEditorPBR->SetFloat(locBuff, LightManager::spotLights[i].GetBasePL()->GetLinear());
        snprintf(locBuff, sizeof(locBuff), "pointSpotLights[%d].constant", lightIndex);
        shaderEditorPBR->SetFloat(locBuff, LightManager::spotLights[i].GetBasePL()->GetConstant());

        // set uniforms for omni shadow maps
        // texture slot for 'omniShadowMaps[i].shadowMap' samplerCube in editor_object_pbr.fs is 9
        int textureSlotOffset = LightManager::pointLightCount;
        LightManager::spotLights[i].GetShadowMap()->ReadTexture(m_OmniShadowTxSlots["editor_object_pbr"] + textureSlotOffset + i);
        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].shadowMap", textureSlotOffset + i);
        shaderEditorPBR->SetInt(locBuff, m_OmniShadowTxSlots["editor_object_pbr"] + textureSlotOffset + i);
        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].farPlane", textureSlotOffset + i);
        shaderEditorPBR->SetFloat(locBuff, LightManager::spotLights[i].GetFarPlane());
    }
    /**** End editor_object_pbr ****/

    /**** Begin skinning ****/
    Hazel::Ref<OpenGLMoravaShader> shaderSkinning = RendererBasic::GetShaders()["skinning"];
    shaderSkinning->Bind();

    shaderSkinning->SetMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderSkinning->SetMat4("projection", *projectionMatrix);
    shaderSkinning->SetFloat4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, -10000));
    shaderSkinning->SetFloat3("gEyeWorldPos", scene->GetCamera()->GetPosition());
    shaderSkinning->SetFloat("waterLevel", scene->GetWaterManager()->GetWaterHeight());
    shaderSkinning->SetFloat4("waterColor", scene->GetWaterManager()->GetWaterColor());

    // Directional Light
    shaderSkinning->SetFloat3("gDirectionalLight.Base.Color", LightManager::directionalLight.GetColor());
    shaderSkinning->SetFloat("gDirectionalLight.Base.AmbientIntensity", LightManager::directionalLight.GetAmbientIntensity());
    shaderSkinning->SetFloat("gDirectionalLight.Base.DiffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
    shaderSkinning->SetFloat3("gDirectionalLight.Direction", LightManager::directionalLight.GetDirection());

    // TODO: point lights
    shaderSkinning->SetInt("gNumPointLights", 0);

    // TODO: spot lights
    shaderSkinning->SetInt("gNumSpotLights", 0);
    /**** End skinning ****/

    /**** Begin Hybrid Anim PBR ****/
    Hazel::Ref<OpenGLMoravaShader> shaderHybridAnimPBR = RendererBasic::GetShaders()["hybrid_anim_pbr"];
    shaderHybridAnimPBR->Bind();
    shaderHybridAnimPBR->SetMat4("u_ViewProjectionMatrix", *projectionMatrix * scene->GetCamera()->GetViewMatrix());
    shaderHybridAnimPBR->SetFloat3("u_CameraPosition", scene->GetCamera()->GetPosition());

    // point lights
    lightIndex = 0;
    for (unsigned int i = 0; i < LightManager::pointLightCount; ++i)
    {
        lightIndex = 0 + i; // offset for point lights

        std::string uniformName = std::string("lightPositions[") + std::to_string(lightIndex) + std::string("]");
        shaderHybridAnimPBR->SetFloat3(uniformName, LightManager::pointLights[i].GetPosition());
        uniformName = std::string("lightColors[") + std::to_string(lightIndex) + std::string("]");
        shaderHybridAnimPBR->SetFloat3(uniformName, LightManager::pointLights[i].GetColor());
    }

    for (unsigned int i = 0; i < LightManager::spotLightCount; ++i)
    {
        lightIndex = LightManager::pointLightCount + i; // offset for spot lights

        std::string uniformName = std::string("lightPositions[") + std::to_string(lightIndex) + std::string("]");
        shaderHybridAnimPBR->SetFloat3(uniformName, LightManager::spotLights[i].GetBasePL()->GetPosition());
        uniformName = std::string("lightColors[") + std::to_string(lightIndex) + std::string("]");
        shaderHybridAnimPBR->SetFloat3(uniformName, LightManager::spotLights[i].GetBasePL()->GetColor());
    }
    /**** End Hybrid Anim PBR ****/

    /**** Begin shadow_map ****/
    Hazel::Ref<OpenGLMoravaShader> shaderShadowMap = RendererBasic::GetShaders()["shadow_map"];
    shaderShadowMap->Bind();
    shaderShadowMap->SetMat4("u_DirLightTransform", LightManager::directionalLight.CalculateLightTransform());
    shaderShadowMap->SetBool("u_Animated", false);
    /**** End shadow_map ****/

    /**** Begin omni_shadow_map ****/
    Hazel::Ref<OpenGLMoravaShader> shaderOmniShadowMap = RendererBasic::GetShaders()["omni_shadow_map"];
    shaderOmniShadowMap->Bind();
    shaderOmniShadowMap->SetFloat3("lightPosition", LightManager::directionalLight.GetPosition());
    shaderOmniShadowMap->SetFloat("farPlane", scene->GetSettings().farPlane);
    /**** End omni_shadow_map ****/

    /**** Begin shaderWater ****/
    Hazel::Ref<OpenGLMoravaShader> shaderWater = RendererBasic::GetShaders()["water"];
    shaderWater->Bind();

    shaderWater->SetMat4("projection", *projectionMatrix);
    shaderWater->SetMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderWater->SetFloat3("lightPosition", LightManager::directionalLight.GetPosition());
    shaderWater->SetFloat3("cameraPosition", scene->GetCamera()->GetPosition());
    shaderWater->SetFloat3("lightColor", LightManager::directionalLight.GetColor());
    shaderWater->SetFloat("moveFactor", scene->GetWaterManager()->GetWaterMoveFactor());
    shaderWater->SetFloat("nearPlane", scene->GetSettings().nearPlane);
    shaderWater->SetFloat("farPlane", scene->GetSettings().farPlane);
    shaderWater->SetFloat3("eyePosition", scene->GetCamera()->GetPosition());
    shaderWater->SetFloat("waterLevel", scene->GetWaterManager()->GetWaterHeight());
    shaderWater->SetFloat4("waterColor", scene->GetWaterManager()->GetWaterColor());
    /**** End shaderWater ****/

    /**** Begin Background shader ****/
    Hazel::Ref<OpenGLMoravaShader> shaderBackground = RendererBasic::GetShaders()["background"];
    shaderBackground->Bind();
    shaderBackground->SetMat4("projection", *projectionMatrix);
    shaderBackground->SetMat4("view", scene->GetCamera()->GetViewMatrix());
    /**** End Background shader ****/

    /**** Begin of shaderBasic ****/
    Hazel::Ref<OpenGLMoravaShader> shaderBasic = RendererBasic::GetShaders()["basic"];
    shaderBasic->Bind();
    shaderBasic->SetMat4("projection", *projectionMatrix);
    shaderBasic->SetMat4("view", scene->GetCamera()->GetViewMatrix());
    /**** End of shaderBasic ****/

    /**** Begin gizmo shader ****/
    Hazel::Ref<OpenGLMoravaShader> shaderGizmo = RendererBasic::GetShaders()["gizmo"];
    shaderGizmo->Bind();
    // shaderGizmo->SetMat4("projection", *projectionMatrix);

    // experimental
    if (((SceneEditor*)scene)->m_GizmoOrthoProjection) {
        float aspectRatio = scene->GetCameraController()->GetAspectRatio();
        float sizeCoef = 5.0f;
        glm::mat4 orthoMatrix = glm::ortho(-aspectRatio * sizeCoef, aspectRatio * sizeCoef, -1.0f * sizeCoef, 1.0f * sizeCoef,
            scene->GetSettings().nearPlane, scene->GetSettings().farPlane);
        shaderGizmo->SetMat4("projection", orthoMatrix);
    }
    else {
        shaderGizmo->SetMat4("projection", *projectionMatrix);
    }

    shaderGizmo->SetMat4("view", scene->GetCamera()->GetViewMatrix());
    // Directional Light
    shaderGizmo->SetBool("directionalLight.base.enabled", LightManager::directionalLight.GetEnabled());
    shaderGizmo->SetFloat3("directionalLight.base.color", LightManager::directionalLight.GetColor());
    shaderGizmo->SetFloat("directionalLight.base.ambientIntensity", LightManager::directionalLight.GetAmbientIntensity());
    shaderGizmo->SetFloat("directionalLight.base.diffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
    shaderGizmo->SetFloat3("directionalLight.direction", LightManager::directionalLight.GetDirection());
    /**** End gizmo shader ****/

    /**** Begin glass ****/
    Hazel::Ref<OpenGLMoravaShader> shaderGlass = RendererBasic::GetShaders()["glass"];
    shaderGlass->Bind();
    shaderGlass->SetMat4("view", scene->GetCamera()->GetViewMatrix());
    shaderGlass->SetMat4("projection", *projectionMatrix);
    shaderGlass->SetFloat3("cameraPosition", scene->GetCamera()->GetPosition());
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
