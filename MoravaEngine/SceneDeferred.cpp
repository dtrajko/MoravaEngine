#include "SceneDeferred.h"

#include "ResourceManager.h"
#include "Block.h"
#include "Cube.h"
#include "Application.h"
#include "Log.h"

#include "ImGuiWrapper.h"


SceneDeferred::SceneDeferred()
{
    sceneSettings.cameraPosition = glm::vec3(0.0f, 0.0f, 8.0f);
    sceneSettings.cameraStartYaw = -90.0f;
    sceneSettings.cameraStartPitch = 0.0f;
    sceneSettings.cameraMoveSpeed = 1.0f;
    sceneSettings.enablePointLights  = false;
    sceneSettings.enableSpotLights   = false;
    sceneSettings.enableOmniShadows  = false;
    sceneSettings.enableSkybox       = false;
    sceneSettings.enableShadows      = false;
    sceneSettings.enableWaterEffects = false;
    sceneSettings.enableParticles    = false;

    ResourceManager::Init();

    SetCamera();
    SetupShaders();
    SetupMeshes();
    SetupModels();
    SetupFramebuffers();
    SetupLights();

    m_UpdateCooldown = { 0.0f, 0.2f };
}

void SceneDeferred::SetupMeshes()
{
    m_Quad = new QuadSSAO();
    m_Cube = new Cube();
}

void SceneDeferred::SetupModels()
{
    stbi_set_flip_vertically_on_load(true);

    m_Backpack = new ModelSSAO("Models/backpack/backpack.obj", "Models/backpack");

    m_ObjectPositions.push_back(glm::vec3(-3.0f, -0.5f, -3.0f));
    m_ObjectPositions.push_back(glm::vec3( 0.0f, -0.5f, -3.0f));
    m_ObjectPositions.push_back(glm::vec3( 3.0f, -0.5f, -3.0f));
    m_ObjectPositions.push_back(glm::vec3(-3.0f, -0.5f,  0.0f));
    m_ObjectPositions.push_back(glm::vec3( 0.0f, -0.5f,  0.0f));
    m_ObjectPositions.push_back(glm::vec3( 3.0f, -0.5f,  0.0f));
    m_ObjectPositions.push_back(glm::vec3(-3.0f, -0.5f,  3.0f));
    m_ObjectPositions.push_back(glm::vec3( 0.0f, -0.5f,  3.0f));
    m_ObjectPositions.push_back(glm::vec3( 3.0f, -0.5f,  3.0f));
}

void SceneDeferred::SetupFramebuffers()
{
    m_Width = (int)Application::Get()->GetWindow()->GetWidth();
    m_Height = (int)Application::Get()->GetWindow()->GetHeight();

    GenerateFramebuffers(m_Width, m_Height);
}

void SceneDeferred::SetupShaders()
{
    m_ShaderGeometryPass = new Shader("Shaders/LearnOpenGL/8.2.g_buffer.vs", "Shaders/LearnOpenGL/8.2.g_buffer.fs");
    Log::GetLogger()->info("SceneDeferred: m_ShaderGeometryPass compiled [programID={0}]", m_ShaderGeometryPass->GetProgramID());

    m_ShaderLightingPass = new Shader("Shaders/LearnOpenGL/8.2.deferred_shading.vs", "Shaders/LearnOpenGL/8.2.deferred_shading.fs");
    Log::GetLogger()->info("SceneDeferred: m_ShaderLightingPass compiled [programID={0}]", m_ShaderLightingPass->GetProgramID());

    m_ShaderLightBox = new Shader("Shaders/LearnOpenGL/8.2.deferred_light_box.vs", "Shaders/LearnOpenGL/8.2.deferred_light_box.fs");
    Log::GetLogger()->info("SceneDeferred: m_ShaderLightBox compiled [programID={0}]", m_ShaderLightBox->GetProgramID());

    m_ShaderLightingPass->Bind();
    m_ShaderLightingPass->setInt("gPosition",   0);
    m_ShaderLightingPass->setInt("gNormal",     1);
    m_ShaderLightingPass->setInt("gAlbedoSpec", 2);
}

void SceneDeferred::SetupLights()
{
    // lighting info
    // -------------
    srand(13);
    for (unsigned int i = 0; i < NR_LIGHTS; i++)
    {
        // calculate slightly random offsets
        float xPos = ((rand() % 100) / 100.0f) * 6.0f - 3.0f;
        float yPos = ((rand() % 100) / 100.0f) * 6.0f - 4.0f;
        float zPos = ((rand() % 100) / 100.0f) * 6.0f - 3.0f;
        m_LightPositions.push_back(glm::vec3(xPos, yPos, zPos));
        // also calculate random color
        float rColor = ((rand() % 100) / 200.0f) + 0.5f; // between 0.5 and 1.0
        float gColor = ((rand() % 100) / 200.0f) + 0.5f; // between 0.5 and 1.0
        float bColor = ((rand() % 100) / 200.0f) + 0.5f; // between 0.5 and 1.0
        m_LightColors.push_back(glm::vec3(rColor, gColor, bColor));
    }
}

void SceneDeferred::Update(float timestep, Window* mainWindow)
{
    UpdateCooldown(timestep);
}

void SceneDeferred::UpdateCooldown(float timestep)
{
    // Cooldown
    if (timestep - m_UpdateCooldown.lastTime < m_UpdateCooldown.cooldown) return;
    m_UpdateCooldown.lastTime = timestep;

    m_Width = Application::Get()->GetWindow()->GetWidth();
    m_Height = Application::Get()->GetWindow()->GetHeight();

    if (m_Width != m_WidthPrev || m_Height != m_HeightPrev)
    {
        ResetHandlers();
        GenerateFramebuffers(m_Width, m_Height);

        m_WidthPrev = m_Width;
        m_HeightPrev = m_Height;
    }
}

void SceneDeferred::ResetHandlers()
{
    // Framebuffers
    gBuffer = 0;

    // textures / framebuffer attachments
    gPosition = 0;
    gNormal = 0;
    gAlbedoSpec = 0;
    rboDepth = 0;    
}

void SceneDeferred::GenerateFramebuffers(int width, int height)
{
    ResetHandlers();

    // configure g-buffer framebuffer
    // ------------------------------
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

    // position color buffer
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Width, m_Height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

    // normal color buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Width, m_Height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    // color + specular color buffer
    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);

    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
    attachments = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments.data());

    // create and attach depth buffer (renderbuffer)
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_Width, m_Height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneDeferred::UpdateImGui(float timestep, Window* mainWindow)
{
    bool p_open = true;
    ShowExampleAppDockSpace(&p_open, mainWindow);

    ImGui::Begin("Framebuffers");
    {
        ImVec2 imageSize(192.0f, 192.0f);

        std::string gPositionText = std::string("gPosition [TextureID=") + std::to_string(gPosition) + std::string("]");
        ImGui::Text(gPositionText.c_str());
        ImGui::Image((void*)(intptr_t)gPosition, imageSize);

        std::string gNormalText = std::string("gNormal [TextureID=") + std::to_string(gNormal) + std::string("]");
        ImGui::Text(gNormalText.c_str());
        ImGui::Image((void*)(intptr_t)gNormal, imageSize);

        std::string gAlbedoSpecText = std::string("gAlbedoSpec [TextureID=") + std::to_string(gAlbedoSpec) + std::string("]");
        ImGui::Text(gAlbedoSpecText.c_str());
        ImGui::Image((void*)(intptr_t)gAlbedoSpec, imageSize);
    }
    ImGui::End();
}

void SceneDeferred::RenderPassGeometry(glm::mat4 projectionMatrix)
{
    // 1. geometry pass: render scene's geometry/color data into gbuffer
        // -----------------------------------------------------------------
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_ShaderGeometryPass->Bind();
    m_ShaderGeometryPass->setMat4("projection", projectionMatrix);
    m_ShaderGeometryPass->setMat4("view", m_CameraController->CalculateViewMatrix());

    glm::mat4 model = glm::mat4(1.0f);
    for (unsigned int i = 0; i < m_ObjectPositions.size(); i++)
    {
        model = glm::mat4(1.0f);
        model = glm::translate(model, m_ObjectPositions[i]);
        model = glm::scale(model, glm::vec3(0.25f));
        m_ShaderGeometryPass->setMat4("model", model);
        m_Backpack->Draw(m_ShaderGeometryPass);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneDeferred::RenderPassLighting()
{
    // 2. lighting pass: calculate lighting by iterating over a screen filled quad pixel-by-pixel using the gbuffer's content.
    // -----------------------------------------------------------------------------------------------------------------------
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_ShaderLightingPass->Bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);

    // send light relevant uniforms
    for (unsigned int i = 0; i < m_LightPositions.size(); i++)
    {
        m_ShaderLightingPass->setVec3("lights[" + std::to_string(i) + "].Position", m_LightPositions[i]);
        m_ShaderLightingPass->setVec3("lights[" + std::to_string(i) + "].Color", m_LightColors[i]);
        // update attenuation parameters and calculate radius
        const float constant = 1.0f; // note that we don't send this to the shader, we assume it is always 1.0 (in our case)
        const float linear = 0.7f;
        const float quadratic = 1.8f;
        m_ShaderLightingPass->setFloat("lights[" + std::to_string(i) + "].Linear", linear);
        m_ShaderLightingPass->setFloat("lights[" + std::to_string(i) + "].Quadratic", quadratic);
        // then calculate radius of light volume/sphere
        const float maxBrightness = std::fmaxf(std::fmaxf(m_LightColors[i].r, m_LightColors[i].g), m_LightColors[i].b);
        float radius = (-linear + std::sqrt(linear * linear - 4 * quadratic * (constant - (256.0f / 5.0f) * maxBrightness))) / (2.0f * quadratic);
        m_ShaderLightingPass->setFloat("lights[" + std::to_string(i) + "].Radius", radius);
    }
    m_ShaderLightingPass->setVec3("viewPos", m_Camera->GetPosition());

    // finally render quad
    m_Quad->Render();
}

void SceneDeferred::RenderPassForward(glm::mat4 projectionMatrix)
{
    // 2.5. copy content of geometry's depth buffer to default framebuffer's depth buffer
    // ----------------------------------------------------------------------------------
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
    // blit to default framebuffer. Note that this may or may not work as the internal formats of both the FBO and default framebuffer have to match.
    // the internal formats are implementation defined. This works on all of my systems, but if it doesn't on yours you'll likely have to write to the 		
    // depth buffer in another shader stage (or somehow see to match the default framebuffer's internal format with the FBO's internal format).
    glBlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 3. render lights on top of scene
    // --------------------------------
    m_ShaderLightBox->Bind();
    m_ShaderLightBox->setMat4("projection", projectionMatrix);
    m_ShaderLightBox->setMat4("view", m_CameraController->CalculateViewMatrix());
    glm::mat4 model = glm::mat4(1.0f);
    for (unsigned int i = 0; i < m_LightPositions.size(); i++)
    {
        model = glm::mat4(1.0f);
        model = glm::translate(model, m_LightPositions[i]);
        model = glm::scale(model, glm::vec3(0.125f));
        m_ShaderLightBox->setMat4("model", model);
        m_ShaderLightBox->setVec3("lightColor", m_LightColors[i]);
        m_Cube->Render();
    }
}

void SceneDeferred::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
    m_Width = (int)Application::Get()->GetWindow()->GetWidth();
    m_Height = (int)Application::Get()->GetWindow()->GetHeight();

    glEnable(GL_DEPTH_TEST);

    RenderPassGeometry(projectionMatrix);
    RenderPassLighting();
    RenderPassForward(projectionMatrix);
}

SceneDeferred::~SceneDeferred()
{
    delete m_Quad;
    delete m_Cube;
    delete m_Backpack;

    delete m_ShaderGeometryPass;
    delete m_ShaderLightingPass;
    delete m_ShaderLightBox;
}
