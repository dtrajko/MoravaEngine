#include "RendererEditor.h"

#include "SceneEditor.h"
#include "MousePicker.h"
#include "GeometryFactory.h"

#include <stdexcept>


RendererEditor::RendererEditor()
{
}

void RendererEditor::Init(Scene* scene)
{
    SceneEditor* sceneEditor = (SceneEditor*)scene;

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

    SceneEditor* sceneEditor = (SceneEditor*)scene;

    // Override the Projection matrix (update FOV)
    if (mainWindow.GetBufferWidth() > 0 && mainWindow.GetBufferHeight() > 0)
    {
        projectionMatrix = glm::perspective(glm::radians(sceneEditor->GetFOV()),
            (float)mainWindow.GetBufferWidth() / (float)mainWindow.GetBufferHeight(),
            scene->GetSettings().nearPlane, scene->GetSettings().farPlane);
    }

    EnableTransparency();
    EnableCulling();

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
