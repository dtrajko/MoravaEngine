#include "SceneParticles.h"

#include "RendererBasic.h"
#include "Log.h"

#include "ImGuiWrapper.h"


SceneParticles::SceneParticles()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 2.0f, 12.0f);
	sceneSettings.cameraStartYaw = -90.0f;
    sceneSettings.cameraStartPitch = 10.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;
    sceneSettings.enableSkybox = false;
    sceneSettings.enablePointLights = false;
    sceneSettings.enableSpotLights = false;

	SetCamera();
	SetSkybox();
	SetupTextures();
	SetupMeshes();
	SetupModels();
    SetupParticles();

    // Initialize the PBR/IBL Material Workflow component
    m_MaterialWorkflowPBR = new MaterialWorkflowPBR();
    m_MaterialWorkflowPBR->Init("Textures/HDR/greenwich_park_02_1k.hdr");

    m_ShaderFBScene = new Shader("Shaders/framebuffers_scene.vs", "Shaders/framebuffers_scene.fs");
    printf("SceneParticles: m_ShaderFBScene compiled [programID=%d]\n", m_ShaderFBScene->GetProgramID());

    m_ShaderBackground = new Shader("Shaders/LearnOpenGL/2.2.2.background.vs", "Shaders/LearnOpenGL/2.2.2.background.fs");
    Log::GetLogger()->info("SceneParticles: m_ShaderBackground compiled [programID={0}]", m_ShaderBackground->GetProgramID());
}

void SceneParticles::SetSkybox()
{
    skyboxFaces.push_back("Textures/skybox_4/right.png");
    skyboxFaces.push_back("Textures/skybox_4/left.png");
    skyboxFaces.push_back("Textures/skybox_4/top.png");
    skyboxFaces.push_back("Textures/skybox_4/bottom.png");
    skyboxFaces.push_back("Textures/skybox_4/back.png");
    skyboxFaces.push_back("Textures/skybox_4/front.png");
    // m_Skybox = new Skybox(skyboxFaces);

    m_TextureCubeMap = new TextureCubeMap(skyboxFaces);
}

void SceneParticles::SetupTextures()
{
    textures.insert(std::make_pair("water", TextureLoader::Get()->GetTexture("Textures/water.png", false, false)));
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
        m_MouseX = mainWindow.GetMouseX();
        m_MouseY = mainWindow.GetMouseY();

        m_NormalizedMouseX = -1.0f + 2.0f * m_MouseX / mainWindow.GetWidth();
        m_NormalizedMouseY =  1.0f - 2.0f * m_MouseY / mainWindow.GetHeight();

        glm::vec3 particlePosition = m_Camera->GetPosition() + m_Camera->GetFront() * 5.0f;

        m_Particle.Position = { particlePosition.x * m_NormalizedMouseX * 10.0f, particlePosition.y * m_NormalizedMouseY * 6.0f };

        for (int i = 0; i < 10; i++)
            m_ParticleSystem.Emit(m_Particle);
    }

    m_ParticleSystem.OnUpdate(timestep * 0.01f);
}

void SceneParticles::UpdateImGui(float timestep, Window& mainWindow)
{
    bool p_open = true;
    ShowExampleAppDockSpace(&p_open, mainWindow);

    ImGui::Begin("Particles");
    {
        ImGui::SliderFloat("Mouse X", &m_MouseX, -1000.0f, 1000.0f);
        ImGui::SliderFloat("Mouse Y", &m_MouseY, -1000.0f, 1000.0f);
        ImGui::SliderFloat("Normalized X", &m_NormalizedMouseX, -10.0f, 10.0f);
        ImGui::SliderFloat("Normalized Y", &m_NormalizedMouseY, -10.0f, 10.0f);
        ImGui::SliderFloat("Particle position X", &m_Particle.Position.x, -1000.0f, 1000.0f);
        ImGui::SliderFloat("Particle position Y", &m_Particle.Position.y, -1000.0f, 1000.0f);
    }
    ImGui::End();
}

void SceneParticles::Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
    // Skybox shaderBackground
/* Begin backgroundShader */
    {
        m_MaterialWorkflowPBR->SetGlobalRenderState();

        RendererBasic::DisableCulling();
        // render skybox (render as last to prevent overdraw)
        m_ShaderBackground->Bind();
        m_ShaderBackground->setMat4("projection", projectionMatrix);
        m_ShaderBackground->setMat4("view", m_CameraController->CalculateViewMatrix());
        m_ShaderBackground->setMat4("model", glm::mat4(1.0f));
        m_ShaderBackground->setInt("environmentMap", 0);
        m_ShaderBackground->setFloat("u_TextureLOD", 0.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_MaterialWorkflowPBR->GetEnvironmentCubemap());
        // glBindTexture(GL_TEXTURE_CUBE_MAP, m_MaterialWorkflowPBR->GetIrradianceMap()); // display irradiance map
        // glBindTexture(GL_TEXTURE_CUBE_MAP, m_MaterialWorkflowPBR->GetPrefilterMap()); // display prefilter map
        m_MaterialWorkflowPBR->GetSkyboxCube()->Render();
    }
    /* End backgroundShader */

    m_ShaderFBScene->Bind();
    m_ShaderFBScene->setMat4("projection", projectionMatrix);
    m_ShaderFBScene->setMat4("view", m_CameraController->CalculateViewMatrix());

    textures["water"]->Bind(0);
    m_ShaderFBScene->setInt("texture1", 0);

    // Render Particles
    m_ParticleSystem.OnRender(m_Camera, m_ShaderFBScene);
}

SceneParticles::~SceneParticles()
{
}
