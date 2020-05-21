#include "RendererEditor.h"

#include "SceneParticles.h"
#include "MousePicker.h"
#include "GeometryFactory.h"

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
    Shader* shaderEditorObject = new Shader("Shaders/editor_object.vs", "Shaders/editor_object.fs");
    shaders.insert(std::make_pair("editor_object", shaderEditorObject));
    printf("RendererEditor: shaderEditorObject compiled [programID=%d]\n", shaderEditorObject->GetProgramID());

    Shader* shaderBasic = new Shader("Shaders/basic.vs", "Shaders/basic.fs");
    shaders.insert(std::make_pair("basic", shaderBasic));
    printf("RendererEditor: shaderBasic compiled [programID=%d]\n", shaderBasic->GetProgramID());

    Shader* shaderFramebuffersScene = new Shader("Shaders/framebuffers_scene.vs", "Shaders/framebuffers_scene.fs");
    shaders.insert(std::make_pair("framebuffers_scene", shaderFramebuffersScene));
    printf("RendererEditor: shaderFramebuffersScene compiled [programID=%d]\n", shaderFramebuffersScene->GetProgramID());

    Shader* shaderPBR = new Shader("Shaders/learnopengl/2.2.2.pbr.vs", "Shaders/learnopengl/2.2.2.pbr.fs");
    shaders.insert(std::make_pair("shaderPBR", shaderPBR));
    printf("RendererEditor: shaderPBR compiled [programID=%d]\n", shaderPBR->GetProgramID());
}

void RendererEditor::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RenderPass(mainWindow, scene, projectionMatrix);
}

void RendererEditor::RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());

	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Override the Projection matrix (update FOV)
    if (mainWindow.GetBufferWidth() > 0 && mainWindow.GetBufferHeight() > 0)
    {
        projectionMatrix = glm::perspective(glm::radians(scene->GetFOV()),
            (float)mainWindow.GetBufferWidth() / (float)mainWindow.GetBufferHeight(),
            scene->GetSettings().nearPlane, scene->GetSettings().farPlane);
    }

    EnableTransparency();
    EnableCulling();

    Shader* shaderEditor = shaders["editor_object"];

    shaderEditor->Bind();
    shaderEditor->setMat4("projection", projectionMatrix);
    shaderEditor->setMat4("view", scene->GetCamera()->CalculateViewMatrix());

    // Directional Light
    shaderEditor->setBool("directionalLight.base.enabled", scene->GetLightManager()->directionalLight.GetEnabled());
    shaderEditor->setVec3("directionalLight.base.color", scene->GetLightManager()->directionalLight.GetColor());
    shaderEditor->setFloat("directionalLight.base.ambientIntensity", scene->GetLightManager()->directionalLight.GetAmbientIntensity());
    shaderEditor->setFloat("directionalLight.base.diffuseIntensity", scene->GetLightManager()->directionalLight.GetDiffuseIntensity());
    shaderEditor->setVec3("directionalLight.direction", scene->GetLightManager()->directionalLight.GetDirection());

    // Point Lights
    for (unsigned int i = 0; i < scene->GetLightManager()->pointLightCount; i++)
    {
        char locBuff[100] = { '\0' };

        snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.enabled", i);
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

    if (scene->GetSettings().enableSkybox)
    {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        float angleRadians = glm::radians((GLfloat)glfwGetTime());
        modelMatrix = glm::rotate(modelMatrix, angleRadians, glm::vec3(0.0f, 1.0f, 0.0f));
        scene->GetSkybox()->Draw(modelMatrix, scene->GetCamera()->CalculateViewMatrix(), projectionMatrix);
    }
        
    std::string passType = "main";
    scene->Render(projectionMatrix, passType, shaders, uniforms);
}

RendererEditor::~RendererEditor()
{
}
