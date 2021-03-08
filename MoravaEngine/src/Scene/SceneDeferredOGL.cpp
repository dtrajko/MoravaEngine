#include "Scene/SceneDeferredOGL.h"

#include "Mesh/Block.h"


SceneDeferredOGL::SceneDeferredOGL()
{
    sceneSettings.cameraPosition = glm::vec3(0.0f, 0.0f, 2.0f);
    sceneSettings.cameraStartYaw = -90.0f;
    sceneSettings.cameraStartPitch = 0.0f;
    sceneSettings.cameraMoveSpeed = 1.0f;

    // directional light
    sceneSettings.directionalLight.base.enabled = true;
    sceneSettings.directionalLight.base.ambientIntensity = 0.6f;
    sceneSettings.directionalLight.base.diffuseIntensity = 0.4f;

    ResourceManager::Init();

    SetCamera();
    SetLightManager();
    SetupTextureSlots();
    SetupTextures();
    SetupMeshes();
}

SceneDeferredOGL::~SceneDeferredOGL()
{
    delete meshes["cube"];
}

void SceneDeferredOGL::SetupTextureSlots()
{
    textureSlots.insert(std::make_pair("diffuse", 1));
    textureSlots.insert(std::make_pair("normal",  2));
    textureSlots.insert(std::make_pair("depth",   3));
}

void SceneDeferredOGL::SetupTextures()
{
    ResourceManager::LoadTexture("crate_diffuse", "Textures/crate.png");
    ResourceManager::LoadTexture("crate_normal",  "Textures/crateNormal.png");
}

void SceneDeferredOGL::SetupMeshes()
{
    Block* cube = new Block(glm::vec3(1.0f, 1.0f, 1.0f));
    meshes.insert(std::make_pair("cube", cube));
}

void SceneDeferredOGL::Update(float timestep, Window* mainWindow)
{
    m_Camera->OnUpdate(timestep);
}

void SceneDeferredOGL::UpdateImGui(float timestep, Window* mainWindow)
{
    bool p_open = true;
    ShowExampleAppDockSpace(&p_open, mainWindow);
}

void SceneDeferredOGL::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
    Shader* shaderForwardBasic = shaders["forward_basic"];

    glm::mat4 model = glm::mat4(1.0f);

    shaderForwardBasic->Bind();

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    shaderForwardBasic->setMat4("model", model);
    ResourceManager::GetTexture("crate_diffuse")->Bind(textureSlots["diffuse"]);
    ResourceManager::GetTexture("crate_normal")->Bind(textureSlots["normal"]);
    meshes["cube"]->Render();
}
