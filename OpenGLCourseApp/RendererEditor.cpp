#include "RendererEditor.h"

#include "SceneParticles.h"
#include "MousePicker.h"
#include "GeometryFactory.h"
#include "Timer.h"

#include <stdexcept>


RendererEditor::RendererEditor()
{
}

void RendererEditor::Init(Scene* scene)
{
	SetUniforms();
	SetShaders();
}

void RendererEditor::SetUniforms()
{
}

void RendererEditor::SetShaders()
{
    Shader* shaderEditor = new Shader("Shaders/editor_object.vs", "Shaders/editor_object.fs");
    shaders.insert(std::make_pair("editor_object", shaderEditor));
    printf("RendererEditor: shaderEditorObject compiled [programID=%d]\n", shaderEditor->GetProgramID());

    Shader* shaderEditorPBR = new Shader("Shaders/editor_object.vs", "Shaders/PBR/editor_object_pbr.fs");
    shaders.insert(std::make_pair("editor_object_pbr", shaderEditorPBR));
    printf("RendererEditor: shaderEditorObjectPBR compiled [programID=%d]\n", shaderEditorPBR->GetProgramID());

    Shader* shaderGizmo = new Shader("Shaders/gizmo.vs", "Shaders/gizmo.fs");
    shaders.insert(std::make_pair("gizmo", shaderGizmo));
    printf("RendererEditor: shaderGizmo compiled [programID=%d]\n", shaderGizmo->GetProgramID());

    Shader* shaderBasic = new Shader("Shaders/basic.vs", "Shaders/basic.fs");
    shaders.insert(std::make_pair("basic", shaderBasic));
    printf("RendererEditor: shaderBasic compiled [programID=%d]\n", shaderBasic->GetProgramID());

    Shader* shaderBackground = new Shader("Shaders/learnopengl/2.2.2.background.vs", "Shaders/learnopengl/2.2.2.background.fs");
    shaders.insert(std::make_pair("background", shaderBackground));
    printf("RendererEditor: shaderBackground compiled [programID=%d]\n", shaderBackground->GetProgramID());

    Shader* shaderShadowMap = new Shader("Shaders/directional_shadow_map.vert", "Shaders/directional_shadow_map.frag");
    shaders.insert(std::make_pair("shadow_map", shaderShadowMap));
    printf("RendererEditor: shaderShadowMap compiled [programID=%d]\n", shaderShadowMap->GetProgramID());

    Shader* shaderOmniShadowMap = new Shader("Shaders/omni_shadow_map.vert", "Shaders/omni_shadow_map.geom", "Shaders/omni_shadow_map.frag");
    shaders.insert(std::make_pair("omni_shadow_map", shaderOmniShadowMap));
    printf("RendererEditor: shaderOmniShadowMap compiled [programID=%d]\n", shaderOmniShadowMap->GetProgramID());

    Shader* shaderSkinning = new Shader("Shaders/OGLdev/skinning.vs", "Shaders/OGLdev/skinning.fs");
    shaders.insert(std::make_pair("skinning", shaderSkinning));
    printf("RendererEditor: shaderSkinning compiled [programID=%d]\n", shaderSkinning->GetProgramID());

    Shader* shaderGlass = new Shader("Shaders/glass.vs", "Shaders/glass.fs");
    shaders.insert(std::make_pair("glass", shaderGlass));
    printf("RendererEditor: shaderGlass compiled [programID=%d]\n", shaderGlass->GetProgramID());

    Shader* shaderWater = new Shader("Shaders/water.vert", "Shaders/water.frag");
    shaders.insert(std::make_pair("water", shaderWater));
    printf("RendererEditor: shaderWater compiled [programID=%d]\n", shaderWater->GetProgramID());

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

    shaderWater->Bind();
    shaderWater->setInt("reflectionTexture", 0);
    shaderWater->setInt("refractionTexture", 1);
    shaderWater->setInt("normalMap",         2);
    shaderWater->setInt("dudvMap",           3);
    shaderWater->setInt("depthMap",          4);
}

void RendererEditor::RenderPassShadow(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
    if (!scene->GetSettings().enableShadows) return;
    if (!LightManager::directionalLight.GetEnabled()) return;

    Shader* shaderShadowMap = shaders["shadow_map"];
    shaderShadowMap->Bind();

    DirectionalLight* light = &LightManager::directionalLight;
    glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

    light->GetShadowMap()->Write();
    // printf("RenderPassShadow write to FBO = %i Texture ID = %i\n", light->GetShadowMap()->GetFBO(), light->GetShadowMap()->GetTextureID());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);

    shaderShadowMap->setMat4("dirLightTransform", light->CalculateLightTransform());
    shaderShadowMap->Validate();

    DisableCulling();
    std::string passType = "shadow_dir";
    scene->Render(mainWindow, projectionMatrix, passType, shaders, uniforms);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RendererEditor::RenderPassOmniShadow(PointLight* light, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
    if (!scene->GetSettings().enableOmniShadows) return;

    Shader* shaderOmniShadow = shaders["omni_shadow_map"];
    shaderOmniShadow->Bind();

    glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

    light->GetShadowMap()->Write();
    // printf("RenderPassOmniShadow write to FBO = %i Texture ID = %i\n", light->GetShadowMap()->GetFBO(), light->GetShadowMap()->GetTextureID());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);

    shaderOmniShadow->setVec3("omniLightPos", light->GetPosition());
    shaderOmniShadow->setFloat("farPlane", light->GetFarPlane());
    shaderOmniShadow->SetLightMatrices(light->CalculateLightTransform());
    shaderOmniShadow->Validate();

    EnableCulling();
    std::string passType = "shadow_omni";
    scene->Render(mainWindow, projectionMatrix, passType, shaders, uniforms);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RendererEditor::RenderPassWaterReflection(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
    if (!scene->GetSettings().enableWaterEffects) return;

    glViewport(0, 0, scene->GetWaterManager()->GetFramebufferWidth(), scene->GetWaterManager()->GetFramebufferHeight());

    scene->GetWaterManager()->GetReflectionFramebuffer()->Bind();

    // Clear the window
    glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Shader* shaderEditor = shaders["editor_object"];
    shaderEditor->Bind();
    shaderEditor->setVec3("eyePosition", scene->GetCamera()->GetPosition().x, scene->GetCamera()->GetPosition().y, scene->GetCamera()->GetPosition().z);
    shaderEditor->setMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());
    shaderEditor->setVec4("clipPlane", glm::vec4(0.0f, 1.0f, 0.0f, -scene->GetWaterManager()->GetWaterHeight())); // reflection clip plane

    Shader* shaderEditorPBR = shaders["editor_object_pbr"];
    shaderEditorPBR->Bind();
    shaderEditorPBR->setVec3("eyePosition", scene->GetCamera()->GetPosition().x, scene->GetCamera()->GetPosition().y, scene->GetCamera()->GetPosition().z);
    shaderEditorPBR->setMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());
    shaderEditorPBR->setVec4("clipPlane", glm::vec4(0.0f, 1.0f, 0.0f, -scene->GetWaterManager()->GetWaterHeight())); // reflection clip plane

    EnableCulling();
    std::string passType = "water_reflect";
    scene->Render(mainWindow, projectionMatrix, passType, shaders, uniforms);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RendererEditor::RenderPassWaterRefraction(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
    if (!scene->GetSettings().enableWaterEffects) return;

    glViewport(0, 0, scene->GetWaterManager()->GetFramebufferWidth(), scene->GetWaterManager()->GetFramebufferHeight());

    scene->GetWaterManager()->GetRefractionFramebuffer()->Bind();
    scene->GetWaterManager()->GetRefractionFramebuffer()->GetColorAttachment()->Bind(scene->GetTextureSlots()["refraction"]);
    scene->GetWaterManager()->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(scene->GetTextureSlots()["depth"]);

    // Clear the window
    glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Shader* shaderEditor = shaders["editor_object"];
    shaderEditor->Bind();

    shaderEditor->setVec3("eyePosition", scene->GetCamera()->GetPosition().x, scene->GetCamera()->GetPosition().y, scene->GetCamera()->GetPosition().z);
    shaderEditor->setMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());

    std::string passType = "water_refract";
    scene->Render(mainWindow, projectionMatrix, passType, shaders, uniforms);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RendererEditor::RenderOmniShadows(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
    if (!scene->GetSettings().enableOmniShadows) return;

    for (size_t i = 0; i < LightManager::pointLightCount; i++)
        if (LightManager::pointLights[i].GetEnabled())
            RenderPassOmniShadow(&LightManager::pointLights[i], mainWindow, scene, projectionMatrix);

    for (size_t i = 0; i < LightManager::spotLightCount; i++)
        if (LightManager::spotLights[i].GetBasePL()->GetEnabled())
            RenderPassOmniShadow((PointLight*)&LightManager::spotLights[i], mainWindow, scene, projectionMatrix);
}

void RendererEditor::RenderWaterEffects(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
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
    scene->GetCamera()->InvertPitch();

    RenderPassWaterReflection(mainWindow, scene, projectionMatrix);

    scene->GetCamera()->SetPosition(glm::vec3(scene->GetCamera()->GetPosition().x, scene->GetCamera()->GetPosition().y + distance, scene->GetCamera()->GetPosition().z));
    scene->GetCamera()->InvertPitch();

    RenderPassWaterRefraction(mainWindow, scene, projectionMatrix);

    glDisable(GL_CLIP_DISTANCE0);
}

void RendererEditor::RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());

	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);
    // set depth function to less than AND equal for skybox depth trick.
    glDepthFunc(GL_LEQUAL);
    // enable seamless cubemap sampling for lower mip levels in the pre-filter map.
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // then before rendering, configure the viewport to the original framebuffer's screen dimensions
    SetDefaultFramebuffer((unsigned int)mainWindow.GetBufferWidth(), (unsigned int)mainWindow.GetBufferHeight());

    EnableTransparency();
    EnableCulling();

    /**** Begin editor_object ****/
    Shader* shaderEditor = shaders["editor_object"];

    shaderEditor->Bind();

    // Directional Light
    shaderEditor->setBool( "directionalLight.base.enabled",          LightManager::directionalLight.GetEnabled());
    shaderEditor->setVec3( "directionalLight.base.color",            LightManager::directionalLight.GetColor());
    shaderEditor->setFloat("directionalLight.base.ambientIntensity", LightManager::directionalLight.GetAmbientIntensity());
    shaderEditor->setFloat("directionalLight.base.diffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
    shaderEditor->setVec3( "directionalLight.direction",             LightManager::directionalLight.GetDirection());

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
        if (scene->GetSettings().enableOmniShadows && LightManager::pointLights[i].GetEnabled())
        {
            LightManager::pointLights[i].GetShadowMap()->Read(m_OmniShadowTxSlots["editor_object"] + textureSlotOffset + i);

            // printf("RendererEditor::RenderPass shaderEditor READ from FBO = %i Texture ID = %i\n",
            //     LightManager::pointLights[i].GetShadowMap()->GetFBO(), LightManager::pointLights[i].GetShadowMap()->GetTextureID());
        }

        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].shadowMap", textureSlotOffset + i);
        shaderEditor->setInt(locBuff, m_OmniShadowTxSlots["editor_object"] + textureSlotOffset + i);
        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].farPlane", textureSlotOffset + i);
        shaderEditor->setFloat(locBuff, LightManager::pointLights[i].GetFarPlane());
        // printf("editor_object pointLights omniShadowMaps[%d].shadowMap = %d\n", textureSlotOffset + i, m_OmniShadowTxSlots["editor_object"] + textureSlotOffset + i);
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
        if (scene->GetSettings().enableOmniShadows && LightManager::spotLights[i].GetBasePL()->GetEnabled())
        {
            LightManager::spotLights[i].GetShadowMap()->Read(m_OmniShadowTxSlots["editor_object"] + textureSlotOffset + i);
            // printf("RendererEditor::RenderPass shaderEditor READ from FBO = %i Texture ID = %i\n",
            //     LightManager::spotLights[i].GetShadowMap()->GetFBO(), LightManager::spotLights[i].GetShadowMap()->GetTextureID());
        }

        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].shadowMap", textureSlotOffset + i);
        shaderEditor->setInt(locBuff, m_OmniShadowTxSlots["editor_object"] + textureSlotOffset + i);
        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].farPlane", textureSlotOffset + i);
        shaderEditor->setFloat(locBuff, LightManager::spotLights[i].GetFarPlane());
        // printf("editor_object spotLights omniShadowMaps[%d].shadowMap = %d\n", textureSlotOffset + i, m_OmniShadowTxSlots["editor_object"] + textureSlotOffset + i);
    }

    shaderEditor->setMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());
    shaderEditor->setVec4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, -10000));
    shaderEditor->setInt("pointLightCount", LightManager::pointLightCount);
    shaderEditor->setInt("spotLightCount", LightManager::spotLightCount);
    // Eye position / camera direction
    shaderEditor->setVec3("eyePosition", scene->GetCamera()->GetPosition());
    /**** End editor_object ****/

    /**** Begin editor_object_pbr ****/
    Shader* shaderEditorPBR = shaders["editor_object_pbr"];

    // Init shaderEditorPBR
    // initialize static shader uniforms before rendering
    shaderEditorPBR->Bind();
    shaderEditorPBR->setVec3("eyePosition", scene->GetCamera()->GetPosition().x, scene->GetCamera()->GetPosition().y, scene->GetCamera()->GetPosition().z);
    shaderEditorPBR->setMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());
    shaderEditorPBR->setVec4("clipPlane", glm::vec4(0.0f, 1.0f, 0.0f, -scene->GetWaterManager()->GetWaterHeight())); // reflection clip plane

    shaderEditorPBR->setInt("pointSpotLightCount", LightManager::pointLightCount + LightManager::spotLightCount);

    // directional light
    shaderEditorPBR->setBool( "directionalLight.base.enabled",          LightManager::directionalLight.GetEnabled());
    shaderEditorPBR->setVec3( "directionalLight.base.color",            LightManager::directionalLight.GetColor());
    shaderEditorPBR->setFloat("directionalLight.base.ambientIntensity", LightManager::directionalLight.GetAmbientIntensity());
    shaderEditorPBR->setFloat("directionalLight.base.diffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
    shaderEditorPBR->setVec3( "directionalLight.direction",             LightManager::directionalLight.GetDirection());
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
        // texture slot for 'omniShadowMaps[i].shadowMap' samplerCube in editor_object_pbr.fs
        int textureSlotOffset = 0;
        if (scene->GetSettings().enableOmniShadows && LightManager::pointLights[i].GetEnabled())
        {
            LightManager::pointLights[i].GetShadowMap()->Read(m_OmniShadowTxSlots["editor_object_pbr"] + textureSlotOffset + i);

            // printf("RendererEditor::RenderPass shaderEditorPBR READ from FBO = %i Texture ID = %i\n",
            //     LightManager::pointLights[i].GetShadowMap()->GetFBO(), LightManager::pointLights[i].GetShadowMap()->GetTextureID());
        }

        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].shadowMap", textureSlotOffset + i);
        shaderEditorPBR->setInt(locBuff, m_OmniShadowTxSlots["editor_object_pbr"] + textureSlotOffset + i);
        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].farPlane", textureSlotOffset + i);
        shaderEditorPBR->setFloat(locBuff, LightManager::pointLights[i].GetFarPlane());
        // printf("editor_object_pbr pointLights omniShadowMaps[%d].shadowMap = %d\n", textureSlotOffset + i, m_OmniShadowTxSlots["editor_object_pbr"] + textureSlotOffset + i);
    }

    for (unsigned int i = 0; i < LightManager::spotLightCount; ++i)
    {
        lightIndex = LightManager::pointLightCount + i; // offset for point lights

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
        // texture slot for 'omniShadowMaps[i].shadowMap' samplerCube in editor_object_pbr.fs
        int textureSlotOffset = LightManager::pointLightCount;
        if (scene->GetSettings().enableOmniShadows && LightManager::spotLights[i].GetBasePL()->GetEnabled())
        {
            LightManager::spotLights[i].GetShadowMap()->Read(m_OmniShadowTxSlots["editor_object_pbr"] + textureSlotOffset + i);

            // printf("RendererEditor::RenderPass shaderEditorPBR READ from FBO = %i Texture ID = %i\n",
            //     LightManager::spotLights[i].GetShadowMap()->GetFBO(), LightManager::spotLights[i].GetShadowMap()->GetTextureID());
        }

        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].shadowMap", textureSlotOffset + i);
        shaderEditorPBR->setInt(locBuff, m_OmniShadowTxSlots["editor_object_pbr"] + textureSlotOffset + i);
        snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].farPlane", textureSlotOffset + i);
        shaderEditorPBR->setFloat(locBuff, LightManager::spotLights[i].GetFarPlane());
        // printf("editor_object_pbr spotLights omniShadowMaps[%d].shadowMap = %d\n", textureSlotOffset + i, m_OmniShadowTxSlots["editor_object_pbr"] + textureSlotOffset + i);
    }
    /**** End editor_object_pbr ****/

    /**** Begin skinning ****/
    Shader* shaderSkinning = shaders["skinning"];
    shaderSkinning->Bind();
    shaderSkinning->setVec3("gEyeWorldPos", scene->GetCamera()->GetPosition());

    // Directional Light
    shaderSkinning->setVec3( "gDirectionalLight.Base.Color",            LightManager::directionalLight.GetColor());
    shaderSkinning->setFloat("gDirectionalLight.Base.AmbientIntensity", LightManager::directionalLight.GetAmbientIntensity());
    shaderSkinning->setFloat("gDirectionalLight.Base.DiffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
    shaderSkinning->setVec3( "gDirectionalLight.Direction",             LightManager::directionalLight.GetDirection());

    // TODO: point lights
    shaderSkinning->setInt("gNumPointLights", 0);

    // TODO: spot lights
    shaderSkinning->setInt("gNumSpotLights", 0);

    /**** End skinning ****/

    /**** Begin glass ****/
    Shader* shaderGlass = shaders["glass"];
    shaderGlass->Bind();
    shaderGlass->setVec3("cameraPosition", scene->GetCamera()->GetPosition());
    /**** End glass ****/

    /**** Begin Background shader ****/
    Shader* shaderBackground = shaders["background"];
    shaderBackground->Bind();
    shaderBackground->setMat4("projection", projectionMatrix);
    shaderBackground->setMat4("view", scene->GetCamera()->CalculateViewMatrix());
    /**** End Background shader ****/

    /**** Beging gizmo shader ****/
    Shader* shaderGizmo = shaders["gizmo"];
    shaderGizmo->Bind();
    // Directional Light
    shaderGizmo->setBool( "directionalLight.base.enabled",          LightManager::directionalLight.GetEnabled());
    shaderGizmo->setVec3( "directionalLight.base.color",            LightManager::directionalLight.GetColor());
    shaderGizmo->setFloat("directionalLight.base.ambientIntensity", LightManager::directionalLight.GetAmbientIntensity());
    shaderGizmo->setFloat("directionalLight.base.diffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
    shaderGizmo->setVec3( "directionalLight.direction",             LightManager::directionalLight.GetDirection());
    /**** End gizmo shader ****/

    /**** Begin of shaderBasic ****/
    Shader* shaderBasic = shaders["basic"];
    shaderBasic->Bind();
    shaderBasic->setMat4("projection", projectionMatrix);
    shaderBasic->setMat4("view", scene->GetCamera()->CalculateViewMatrix());
    /**** End of shaderBasic ****/

    /**** Begin shaderWater ****/
    Shader* shaderWater = shaders["water"];
    shaderWater->Bind();
    shaderWater->setMat4("projection", projectionMatrix);
    shaderWater->setMat4("view", scene->GetCamera()->CalculateViewMatrix());
    shaderWater->setFloat("moveFactor", scene->GetWaterManager()->GetWaterMoveFactor());
    shaderWater->setFloat("nearPlane", scene->GetSettings().nearPlane);
    shaderWater->setFloat("farPlane", scene->GetSettings().farPlane);
    /**** End shaderWater ****/

    if (scene->GetSettings().enableSkybox)
    {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        float angleRadians = glm::radians((GLfloat)glfwGetTime());
        modelMatrix = glm::rotate(modelMatrix, angleRadians, glm::vec3(0.0f, 1.0f, 0.0f));
        scene->GetSkybox()->Draw(modelMatrix, scene->GetCamera()->CalculateViewMatrix(), projectionMatrix);
    }

    std::string passType = "main";
    scene->Render(mainWindow, projectionMatrix, passType, shaders, uniforms);
}

void RendererEditor::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
    // printf("RendererEditor::Render\n");

    // Override the Projection matrix (update FOV)
    if (mainWindow.GetBufferWidth() > 0 && mainWindow.GetBufferHeight() > 0)
    {
        projectionMatrix = glm::perspective(glm::radians(scene->GetFOV()),
            (float)mainWindow.GetBufferWidth() / (float)mainWindow.GetBufferHeight(),
            scene->GetSettings().nearPlane, scene->GetSettings().farPlane);
    }

    Shader* shaderEditor = shaders["editor_object"];
    shaderEditor->Bind();
    shaderEditor->setMat4("projection", projectionMatrix);
    shaderEditor->setMat4("view", scene->GetCamera()->CalculateViewMatrix());

    Shader* shaderEditorPBR = shaders["editor_object_pbr"];
    shaderEditorPBR->Bind();
    shaderEditorPBR->setMat4("projection", projectionMatrix);
    shaderEditorPBR->setMat4("view", scene->GetCamera()->CalculateViewMatrix());
    shaderEditorPBR->setVec3("eyePosition", scene->GetCamera()->GetPosition());

    Shader* shaderGizmo = shaders["gizmo"];
    shaderGizmo->Bind();
    shaderGizmo->setMat4("projection", projectionMatrix);
    shaderGizmo->setMat4("view", scene->GetCamera()->CalculateViewMatrix());

    Shader* shaderSkinning = shaders["skinning"];
    shaderSkinning->Bind();
    shaderSkinning->setMat4("projection", projectionMatrix);
    shaderSkinning->setMat4("view", scene->GetCamera()->CalculateViewMatrix());

    Shader* shaderGlass = shaders["glass"];
    shaderGlass->Bind();
    shaderGlass->setMat4("projection", projectionMatrix);
    shaderGlass->setMat4("view", scene->GetCamera()->CalculateViewMatrix());

    RenderPassShadow(mainWindow, scene, projectionMatrix);
    RenderOmniShadows(mainWindow, scene, projectionMatrix);
    RenderWaterEffects(deltaTime, mainWindow, scene, projectionMatrix);
    RenderPass(mainWindow, scene, projectionMatrix);
}

RendererEditor::~RendererEditor()
{
}
