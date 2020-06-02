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

    shaderEditor->Bind();
    shaderEditor->setInt("albedoMap", 0);
    shaderEditor->setInt("cubeMap",   1);
    shaderEditor->setInt("shadowMap", 2);

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
}

void RendererEditor::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
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
	RenderPass(mainWindow, scene, projectionMatrix);
}

void RendererEditor::RenderPassShadow(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
    if (!scene->GetSettings().enableShadows) return;

    Shader* shaderShadowMap = shaders["shadow_map"];
    shaderShadowMap->Bind();

    DirectionalLight* light = &scene->GetLightManager()->directionalLight;
    glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

    light->GetShadowMap()->Write();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);

    shaderShadowMap->setMat4("dirLightTransform", light->CalculateLightTransform());
    shaderShadowMap->Validate();

    DisableCulling();
    std::string passType = "shadow";
    scene->Render(mainWindow, projectionMatrix, passType, shaders, uniforms);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

    shaderEditor->setMat4("dirLightTransform", scene->GetLightManager()->directionalLight.CalculateLightTransform());

    // Directional Light
    shaderEditor->setBool( "directionalLight.base.enabled", scene->GetLightManager()->directionalLight.GetEnabled());
    shaderEditor->setVec3( "directionalLight.base.color", scene->GetLightManager()->directionalLight.GetColor());
    shaderEditor->setFloat("directionalLight.base.ambientIntensity", scene->GetLightManager()->directionalLight.GetAmbientIntensity());
    shaderEditor->setFloat("directionalLight.base.diffuseIntensity", scene->GetLightManager()->directionalLight.GetDiffuseIntensity());
    shaderEditor->setVec3( "directionalLight.direction", scene->GetLightManager()->directionalLight.GetDirection());

    // Point Lights
    for (unsigned int i = 0; i < scene->GetLightManager()->pointLightCount; i++)
    {
        char locBuff[100] = { '\0' };

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.enabled", i);
        // printf("PointLight[%d] enabled: %d\n", i, scene->GetLightManager()->pointLights[i].GetEnabled());
        shaderEditor->setBool(locBuff, scene->GetLightManager()->pointLights[i].GetEnabled());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.color", i);
        shaderEditor->setVec3(locBuff, scene->GetLightManager()->pointLights[i].GetColor());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.ambientIntensity", i);
        shaderEditor->setFloat(locBuff, scene->GetLightManager()->pointLights[i].GetAmbientIntensity());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.diffuseIntensity", i);
        shaderEditor->setFloat(locBuff, scene->GetLightManager()->pointLights[i].GetDiffuseIntensity());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].position", i);
        shaderEditor->setVec3(locBuff, scene->GetLightManager()->pointLights[i].GetPosition());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].constant", i);
        shaderEditor->setFloat(locBuff, scene->GetLightManager()->pointLights[i].GetConstant());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].linear", i);
        shaderEditor->setFloat(locBuff, scene->GetLightManager()->pointLights[i].GetLinear());

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].exponent", i);
        shaderEditor->setFloat(locBuff, scene->GetLightManager()->pointLights[i].GetExponent());
    }

    shaderEditor->setInt("pointLightCount", scene->GetLightManager()->pointLightCount);

    // Spot Lights
    for (unsigned int i = 0; i < scene->GetLightManager()->spotLightCount; i++)
    {
        char locBuff[100] = { '\0' };

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.base.enabled", i);
        shaderEditor->setBool(locBuff, scene->GetLightManager()->spotLights[i].GetBasePL()->GetEnabled());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.base.color", i);
        shaderEditor->setVec3(locBuff, scene->GetLightManager()->spotLights[i].GetBasePL()->GetColor());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.base.ambientIntensity", i);
        shaderEditor->setFloat(locBuff, scene->GetLightManager()->spotLights[i].GetBasePL()->GetAmbientIntensity());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.base.diffuseIntensity", i);
        shaderEditor->setFloat(locBuff, scene->GetLightManager()->spotLights[i].GetBasePL()->GetDiffuseIntensity());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.position", i);
        shaderEditor->setVec3(locBuff, scene->GetLightManager()->spotLights[i].GetBasePL()->GetPosition());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.constant", i);
        shaderEditor->setFloat(locBuff, scene->GetLightManager()->spotLights[i].GetBasePL()->GetConstant());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.linear", i);
        shaderEditor->setFloat(locBuff, scene->GetLightManager()->spotLights[i].GetBasePL()->GetLinear());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.exponent", i);
        shaderEditor->setFloat(locBuff, scene->GetLightManager()->spotLights[i].GetBasePL()->GetExponent());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].direction", i);
        shaderEditor->setVec3(locBuff, scene->GetLightManager()->spotLights[i].GetDirection());

        snprintf(locBuff, sizeof(locBuff), "spotLights[%d].edge", i);
        shaderEditor->setFloat(locBuff, scene->GetLightManager()->spotLights[i].GetEdge());
    }

    shaderEditor->setInt("spotLightCount", scene->GetLightManager()->spotLightCount);

    // Eye position / camera direction
    shaderEditor->setVec3("eyePosition", scene->GetCamera()->GetPosition());
    /**** End editor_object ****/

    /**** Begin editor_object_pbr ****/
    Shader* shaderEditorPBR = shaders["editor_object_pbr"];

    // Init shaderEditorPBR
    // initialize static shader uniforms before rendering
    shaderEditorPBR->Bind();

    shaderEditorPBR->setMat4("dirLightTransform", scene->GetLightManager()->directionalLight.CalculateLightTransform());

    // directional light
    shaderEditorPBR->setBool( "directionalLight.base.enabled", scene->GetLightManager()->directionalLight.GetEnabled());
    shaderEditorPBR->setVec3( "directionalLight.base.color", scene->GetLightManager()->directionalLight.GetColor());
    shaderEditorPBR->setFloat("directionalLight.base.ambientIntensity", scene->GetLightManager()->directionalLight.GetAmbientIntensity());
    shaderEditorPBR->setFloat("directionalLight.base.diffuseIntensity", scene->GetLightManager()->directionalLight.GetDiffuseIntensity());
    shaderEditorPBR->setVec3( "directionalLight.direction", scene->GetLightManager()->directionalLight.GetDirection());

    // printf("Exponent = %.2ff Linear = %.2ff Constant = %.2ff\n", *m_PointLightExponent, *m_PointLightLinear, *m_PointLightConstant);

    // point lights
    unsigned int lightIndex = 0;
    for (unsigned int i = 0; i < scene->GetLightManager()->pointLightCount; ++i)
    {
        lightIndex = 0 + i; // offset for point lights
        shaderEditorPBR->setBool( "pointSpotLights[" + std::to_string(lightIndex) + "].enabled", scene->GetLightManager()->pointLights[i].GetEnabled());
        shaderEditorPBR->setVec3( "pointSpotLights[" + std::to_string(lightIndex) + "].position", scene->GetLightManager()->pointLights[i].GetPosition());
        shaderEditorPBR->setVec3( "pointSpotLights[" + std::to_string(lightIndex) + "].color", scene->GetLightManager()->pointLights[i].GetColor());
        shaderEditorPBR->setFloat("pointSpotLights[" + std::to_string(lightIndex) + "].exponent", scene->GetLightManager()->pointLights[i].GetExponent());
        shaderEditorPBR->setFloat("pointSpotLights[" + std::to_string(lightIndex) + "].linear", scene->GetLightManager()->pointLights[i].GetLinear());
        shaderEditorPBR->setFloat("pointSpotLights[" + std::to_string(lightIndex) + "].constant", scene->GetLightManager()->pointLights[i].GetConstant());
    }

    for (unsigned int i = 0; i < scene->GetLightManager()->spotLightCount; ++i)
    {
        lightIndex = 4 + i; // offset for point lights
        shaderEditorPBR->setBool( "pointSpotLights[" + std::to_string(lightIndex) + "].enabled", scene->GetLightManager()->spotLights[i].GetBasePL()->GetEnabled());
        shaderEditorPBR->setVec3( "pointSpotLights[" + std::to_string(lightIndex) + "].position", scene->GetLightManager()->spotLights[i].GetBasePL()->GetPosition());
        shaderEditorPBR->setVec3( "pointSpotLights[" + std::to_string(lightIndex) + "].color", scene->GetLightManager()->spotLights[i].GetBasePL()->GetColor());
        shaderEditorPBR->setFloat("pointSpotLights[" + std::to_string(lightIndex) + "].exponent", scene->GetLightManager()->spotLights[i].GetBasePL()->GetExponent());
        shaderEditorPBR->setFloat("pointSpotLights[" + std::to_string(lightIndex) + "].linear", scene->GetLightManager()->spotLights[i].GetBasePL()->GetLinear());
        shaderEditorPBR->setFloat("pointSpotLights[" + std::to_string(lightIndex) + "].constant", scene->GetLightManager()->spotLights[i].GetBasePL()->GetConstant());
    }
    /**** End editor_object_pbr ****/

    /**** Begin skinning ****/
    Shader* shaderSkinning = shaders["skinning"];
    shaderSkinning->Bind();
    shaderSkinning->setVec3("gEyeWorldPos", scene->GetCamera()->GetPosition());

    // Directional Light
    shaderSkinning->setVec3( "gDirectionalLight.Base.Color",            scene->GetLightManager()->directionalLight.GetColor());
    shaderSkinning->setFloat("gDirectionalLight.Base.AmbientIntensity", scene->GetLightManager()->directionalLight.GetAmbientIntensity());
    shaderSkinning->setFloat("gDirectionalLight.Base.DiffuseIntensity", scene->GetLightManager()->directionalLight.GetDiffuseIntensity());
    shaderSkinning->setVec3( "gDirectionalLight.Direction",             scene->GetLightManager()->directionalLight.GetDirection());

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
    shaderGizmo->setBool( "directionalLight.base.enabled", scene->GetLightManager()->directionalLight.GetEnabled());
    shaderGizmo->setVec3( "directionalLight.base.color", scene->GetLightManager()->directionalLight.GetColor());
    shaderGizmo->setFloat("directionalLight.base.ambientIntensity", scene->GetLightManager()->directionalLight.GetAmbientIntensity());
    shaderGizmo->setFloat("directionalLight.base.diffuseIntensity", scene->GetLightManager()->directionalLight.GetDiffuseIntensity());
    shaderGizmo->setVec3( "directionalLight.direction", scene->GetLightManager()->directionalLight.GetDirection());
    /**** End gizmo shader ****/

    /**** Begin of shaderBasic ****/
    Shader* shaderBasic = shaders["basic"];
    shaderBasic->Bind();
    shaderBasic->setMat4("projection", projectionMatrix);
    shaderBasic->setMat4("view", scene->GetCamera()->CalculateViewMatrix());
    /**** End of shaderBasic ****/

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

RendererEditor::~RendererEditor()
{
}
