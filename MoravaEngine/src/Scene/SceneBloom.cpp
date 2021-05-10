#include "Scene/SceneBloom.h"

#include "Core/Application.h"
#include "Mesh/Block.h"
#include "Mesh/Cube.h"
#include "Mesh/QuadSSAO.h"


SceneBloom::SceneBloom()
{
    sceneSettings.cameraPosition = glm::vec3(0.0f, 5.0f, 10.0f);
    sceneSettings.cameraStartYaw = -90.0f;
    sceneSettings.cameraStartPitch = 0.0f;
    sceneSettings.cameraMoveSpeed = 1.0f;
    sceneSettings.waterHeight = 0.0f;
    sceneSettings.waterWaveSpeed = 0.05f;
    sceneSettings.enablePointLights  = true;
    sceneSettings.enableSpotLights   = true;
    sceneSettings.enableOmniShadows  = false;
    sceneSettings.enableSkybox       = false;
    sceneSettings.enableShadows      = false;
    sceneSettings.enableWaterEffects = false;
    sceneSettings.enableParticles    = false;

    ResourceManager::Init();

    SetCamera();
    SetLightManager();
    SetupTextureSlots();
    SetupTextures();
    SetupFramebuffers();
    SetupShaders();
    SetupMeshes();
    SetupModels();
    SetupSSAO();

    m_RenderTarget = (int)RenderTarget::SSAO_Composite;
}

void SceneBloom::SetupTextures()
{
    std::string filepathWood = "Textures/wood.png";
    Hazel::Ref<Hazel::HazelTexture2D> textureWood = Hazel::HazelTexture2D::Create(filepathWood, true, Hazel::HazelTextureWrap::Clamp);
    Log::GetLogger()->info("SceneBloom: HazelTexture2D loaded '{0}'", filepathWood);

    std::string filepathContainer = "Textures/container/container2.png";
    Hazel::Ref<Hazel::HazelTexture2D> textureContainer = Hazel::HazelTexture2D::Create(filepathContainer, true, Hazel::HazelTextureWrap::Clamp);
    Log::GetLogger()->info("SceneBloom: HazelTexture2D loaded '{0}'", filepathContainer);
}

void SceneBloom::SetupTextureSlots()
{
    textureSlots.insert(std::make_pair("diffuse",    1));
    textureSlots.insert(std::make_pair("normal",     2));
    textureSlots.insert(std::make_pair("shadow",     3));
    textureSlots.insert(std::make_pair("omniShadow", 4));
    textureSlots.insert(std::make_pair("reflection", 5));
    textureSlots.insert(std::make_pair("refraction", 6));
    textureSlots.insert(std::make_pair("depth",      7));
    textureSlots.insert(std::make_pair("DuDv",       8));
}

void SceneBloom::SetupMeshes()
{
    // SSAO meshes
    Cube* cube = new Cube();
    meshes.insert(std::make_pair("cube", cube));
}

void SceneBloom::SetupModels()
{
    ModelSSAO* gladiator = new ModelSSAO("Models/Gladiator/Gladiator.fbx", "IgnoreTextures");
    modelsSSAO.insert(std::make_pair("gladiator", gladiator));
}

void SceneBloom::SetupFramebuffers()
{
    GLsizei scrWidth = static_cast<GLsizei>(Application::Get()->GetWindow()->GetWidth());
    GLsizei scrHeight = static_cast<GLsizei>(Application::Get()->GetWindow()->GetHeight());

    // configure (floating point) framebuffers
    // ---------------------------------------
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

    // create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);

    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scrWidth, scrHeight, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }

    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, scrWidth, scrHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, scrWidth, scrHeight, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }
}

void SceneBloom::SetupShaders()
{
    m_ShaderBloom = Hazel::Ref<Shader>::Create("Shaders/LearnOpenGL/7.bloom.vs", "Shaders/LearnOpenGL/7.bloom.fs");
    Log::GetLogger()->info("SceneBloom: m_ShaderBloom compiled [programID={0}]", m_ShaderBloom->GetProgramID());

    m_ShaderLightBox = Hazel::Ref<Shader>::Create("Shaders/LearnOpenGL/7.bloom.vs", "Shaders/LearnOpenGL/7.light_box.fs");
    Log::GetLogger()->info("SceneBloom: m_ShaderLightBox compiled [programID={0}]", m_ShaderLightBox->GetProgramID());

    m_ShaderBlur = Hazel::Ref<Shader>::Create("Shaders/LearnOpenGL/7.blur.vs", "Shaders/LearnOpenGL/7.blur.fs");
    Log::GetLogger()->info("SceneBloom: m_ShaderBlur compiled [programID={0}]", m_ShaderBlur->GetProgramID());

    m_ShaderBloomFinal = Hazel::Ref<Shader>::Create("Shaders/LearnOpenGL/7.bloom_final.vs", "Shaders/LearnOpenGL/7.bloom_final.fs");
    Log::GetLogger()->info("SceneBloom: m_ShaderBloomFinal compiled [programID={0}]", m_ShaderBloomFinal->GetProgramID());
}

void SceneBloom::SetupSSAO()
{
    m_SSAO.Init();
}

void SceneBloom::Update(float timestep, Window* mainWindow)
{
    Scene::Update(timestep, mainWindow);

    m_SSAO.Update();
}

void SceneBloom::UpdateImGui(float timestep, Window* mainWindow)
{
    bool p_open = true;
    ShowExampleAppDockSpace(&p_open, mainWindow);

    ImGui::Begin("Framebuffers");
    {
        ImVec2 imageSize(96.0f, 96.0f);

        ImGui::Text("gPosition");
        ImGui::Image((void*)(intptr_t)m_SSAO.m_GBufferSSAO.m_GBufferPosition, imageSize);

        ImGui::Text("gNormal");
        ImGui::Image((void*)(intptr_t)m_SSAO.m_GBufferSSAO.m_GBufferNormal, imageSize);

        ImGui::Text("gAlbedo");
        ImGui::Image((void*)(intptr_t)m_SSAO.m_GBufferSSAO.m_GBufferAlbedo, imageSize);

        ImGui::Text("gTexCoord");
        ImGui::Image((void*)(intptr_t)m_SSAO.m_GBufferSSAO.m_GBufferTexCoord, imageSize);

        ImGui::Text("m_SSAO_ColorBuffer");
        ImGui::Image((void*)(intptr_t)m_SSAO.m_SSAO_ColorBuffer, imageSize);

        if (m_SSAO.m_BlurEnabled)
        {
            ImGui::Text("m_SSAO_ColorBufferBlur");
            ImGui::Image((void*)(intptr_t)m_SSAO.m_SSAO_ColorBufferBlur, imageSize);
        }

        ImGui::Text("m_NoiseTexture");
        ImGui::Image((void*)(intptr_t)m_SSAO.m_NoiseTexture, imageSize);
    }
    ImGui::End();

    ImGui::Begin("SSAO Settings");
    {
        ImGui::SliderFloat3("Light Position", glm::value_ptr(m_SSAO.m_LightPos), -10.0f, 10.0f);
        ImGui::SliderInt("KernelSize", (int*)&m_SSAO.m_KernelSize, 0, 128);
        ImGui::SliderFloat("Radius", &m_SSAO.m_KernelRadius, 0.0f, 10.0f);
        ImGui::SliderFloat("Bias", &m_SSAO.m_KernelBias, -1.0f, 1.0f);
        if (ImGui::Checkbox("Blur Enabled", &m_SSAO.m_BlurEnabled)) {
            if (m_RenderTarget == (int)RenderTarget::SSAO_ColorBuffer || m_RenderTarget == (int)RenderTarget::SSAO_ColorBufferBlur) {
                m_RenderTarget = m_SSAO.m_BlurEnabled ? (int)RenderTarget::SSAO_ColorBufferBlur : (int)RenderTarget::SSAO_ColorBuffer;
            }
        }

        ImGui::Separator();

        ImGui::Text("Render Targets");
        ImGui::RadioButton("SSAO Composite",    &m_RenderTarget, (int)RenderTarget::SSAO_Composite);
        ImGui::RadioButton("G-Buffer Position", &m_RenderTarget, (int)RenderTarget::GBuffer_Position);
        ImGui::RadioButton("G-Buffer Normal",   &m_RenderTarget, (int)RenderTarget::GBuffer_Normal);
        ImGui::RadioButton("G-Buffer Albedo",   &m_RenderTarget, (int)RenderTarget::GBuffer_Albedo);
        ImGui::RadioButton("G-Buffer TexCoord", &m_RenderTarget, (int)RenderTarget::GBuffer_TexCoord);
        ImGui::RadioButton("SSAO Color",        &m_RenderTarget, (int)RenderTarget::SSAO_ColorBuffer);
        if (m_SSAO.m_BlurEnabled)
        {
            ImGui::RadioButton("SSAO Color Blur", &m_RenderTarget, (int)RenderTarget::SSAO_ColorBufferBlur);
        }
    }
    ImGui::End();
}

void SceneBloom::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
    m_SSAO.Render(projectionMatrix, m_Camera->GetViewMatrix(), meshes, &modelsSSAO);

    if (m_RenderTarget != (int)RenderTarget::SSAO_Composite)
    {
        if (m_RenderTarget == (int)RenderTarget::GBuffer_Position)
        {
            m_SSAO.m_GBufferSSAO.BindForReading();
            m_SSAO.m_GBufferSSAO.SetReadBuffer((int)RenderTarget::GBuffer_Position);
        }
        else if (m_RenderTarget == (int)RenderTarget::GBuffer_Normal)
        {
            m_SSAO.m_GBufferSSAO.BindForReading();
            m_SSAO.m_GBufferSSAO.SetReadBuffer((int)RenderTarget::GBuffer_Normal);
        }
        else if (m_RenderTarget == (int)RenderTarget::GBuffer_Albedo)
        {
            m_SSAO.m_GBufferSSAO.BindForReading();
            m_SSAO.m_GBufferSSAO.SetReadBuffer((int)RenderTarget::GBuffer_Albedo);
        }
        else if (m_RenderTarget == (int)RenderTarget::GBuffer_TexCoord)
        {
            m_SSAO.m_GBufferSSAO.BindForReading();
            m_SSAO.m_GBufferSSAO.SetReadBuffer((int)RenderTarget::GBuffer_TexCoord);
        }
        else if (m_RenderTarget == (int)RenderTarget::SSAO_ColorBuffer)
        {
            m_SSAO.BindFramebufferSSAO();
            m_SSAO.BindColorAttachment();
        }
        else if (m_RenderTarget == (int)RenderTarget::SSAO_ColorBufferBlur)
        {
            m_SSAO.BindFramebufferSSAOBlur();
            m_SSAO.BindColorAttachment();
        }

        glBlitFramebuffer(0, 0, m_SSAO.m_GBufferSSAO.GetWidth(), m_SSAO.m_GBufferSSAO.GetHeight(),
            0, 0, m_SSAO.m_GBufferSSAO.GetWidth(), m_SSAO.m_GBufferSSAO.GetHeight(), GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
}

SceneBloom::~SceneBloom()
{
    delete meshes["block"];
    delete meshes["floor"];
    delete meshes["cube"];
    delete modelsSSAO["gladiator"];
}
