#include "SceneParticles.h"

#include "ImGuiWrapper.h"


SceneParticles::SceneParticles()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 2.0f, 12.0f);
	sceneSettings.cameraStartYaw = -90.0f;
    sceneSettings.cameraStartPitch = 0.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;
    sceneSettings.enableSkybox = true;
    sceneSettings.enablePointLights = false;
    sceneSettings.enableSpotLights = false;

	SetCamera();
	SetSkybox();
	SetTextures();
	SetupMeshes();
	SetupModels();
    SetupParticles();

    m_Grid = new Grid(10);
    m_PivotScene = new Pivot(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(40.0f, 40.0f, 40.0f));
}

void SceneParticles::SetSkybox()
{
    skyboxFaces.push_back("Textures/skybox_4/right.png");
    skyboxFaces.push_back("Textures/skybox_4/left.png");
    skyboxFaces.push_back("Textures/skybox_4/top.png");
    skyboxFaces.push_back("Textures/skybox_4/bottom.png");
    skyboxFaces.push_back("Textures/skybox_4/back.png");
    skyboxFaces.push_back("Textures/skybox_4/front.png");

    m_Skybox = new Skybox(skyboxFaces);
}

void SceneParticles::SetTextures()
{
}

void SceneParticles::SetupMeshes()
{
}

void SceneParticles::SetupModels()
{
}

void SceneParticles::SetupParticles()
{
    // Init Particle System
    m_Particle.ColorBegin = { 254 / 255.0f, 212 / 255.0f, 123 / 255.0f, 1.0f };
    m_Particle.ColorEnd = { 254 / 255.0f, 109 / 255.0f, 41 / 255.0f, 1.0f };
    m_Particle.SizeBegin = 0.5f, m_Particle.SizeVariation = 0.3f, m_Particle.SizeEnd = 0.0f;
    m_Particle.LifeTime = 4.0f;
    m_Particle.Velocity = { 0.0f, 0.0f };
    m_Particle.VelocityVariation = { 3.0f, 1.0f };
    m_Particle.Position = { 0.0f, 0.0f };

    m_ParticleSystem.OnStart();
}

void SceneParticles::Update(float timestep, Window& mainWindow)
{
    // Update Particle System
    if (mainWindow.getMouseButtons()[GLFW_MOUSE_BUTTON_1])
    {
        float normalizedMouseX = -1.0f + 2.0f * mainWindow.GetMouseX() / mainWindow.GetBufferWidth();
        float normalizedMouseY =  1.0f - 2.0f * mainWindow.GetMouseY() / mainWindow.GetBufferHeight();

        glm::vec3 particlePosition = m_Camera->GetPosition() + m_Camera->GetFront() * 5.0f;

        m_Particle.Position = { particlePosition.x * normalizedMouseX, particlePosition.y * normalizedMouseY};

        // printf("SceneEditor::Update MouseCoords [ %.2ff %.2ff ] Particle Position [ %.2ff %.2ff ]\n", normalizedMouseX, normalizedMouseY, particlePosition.x, particlePosition.y);

        for (int i = 0; i < 10; i++)
            m_ParticleSystem.Emit(m_Particle);
    }

    m_ParticleSystem.OnUpdate(timestep * 0.01f);
}

void SceneParticles::UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults)
{
    // ImGui::Begin("Particles");
    // ImGui::SliderFloat("FOV", &m_FOV, 1.0f, 120.0f);
    // ImGui::End();
}

void SceneParticles::Render(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
    Shader* shaderEditor = shaders["editor_object"];
    shaderEditor->Bind();

    // Render Particles
    m_ParticleSystem.OnRender(m_Camera, shaderEditor);

    m_Grid->Draw(shaders["basic"], projectionMatrix, m_Camera->CalculateViewMatrix());
    m_PivotScene->Draw(shaders["basic"], projectionMatrix, m_Camera->CalculateViewMatrix());
}


SceneParticles::~SceneParticles()
{
    delete m_PivotScene;
    delete m_Grid;
}
