#include "Scene/SceneBloom.h"

#include "Core/Application.h"
#include "Core/Input.h"


SceneBloom::SceneBloom()
{
    sceneSettings.cameraPosition = glm::vec3(-10.0f, 2.0f, 0.0f);
    sceneSettings.cameraStartYaw = 0.0f;
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
    SetupShaders();
    SetupFramebuffers();
    SetupLights();
    SetupMeshes();
    SetupModels();
}

SceneBloom::~SceneBloom()
{
    Release();
}

void SceneBloom::SetupTextures()
{
    // m_TextureWood = H2M::Texture2DH2M::Create("Textures/wood.png", true, H2M::TextureWrap::Clamp);

    m_TextureWood = loadTexture("Textures/wood.png", true); // note that we're loading the texture as an SRGB texture
    m_TextureContainer = loadTexture("Textures/container/container2.png", true); // note that we're loading the texture as an SRGB texture
}

void SceneBloom::SetupTextureSlots()
{
}

void SceneBloom::SetupMeshes()
{
}

void SceneBloom::SetupModels()
{
}

void SceneBloom::SetupShaders()
{
    m_ShaderBloom = MoravaShader::Create("Shaders/LearnOpenGL/7.bloom.vs", "Shaders/LearnOpenGL/7.bloom.fs");
    Log::GetLogger()->info("SceneBloom: m_ShaderBloom compiled [programID={0}]", m_ShaderBloom->GetProgramID());

    m_ShaderLightBox = MoravaShader::Create("Shaders/LearnOpenGL/7.bloom.vs", "Shaders/LearnOpenGL/7.light_box.fs");
    Log::GetLogger()->info("SceneBloom: m_ShaderLightBox compiled [programID={0}]", m_ShaderLightBox->GetProgramID());

    m_ShaderBlur = MoravaShader::Create("Shaders/LearnOpenGL/7.blur.vs", "Shaders/LearnOpenGL/7.blur.fs");
    Log::GetLogger()->info("SceneBloom: m_ShaderBlur compiled [programID={0}]", m_ShaderBlur->GetProgramID());

    m_ShaderBloomFinal = MoravaShader::Create("Shaders/LearnOpenGL/7.bloom_final.vs", "Shaders/LearnOpenGL/7.bloom_final.fs");
    Log::GetLogger()->info("SceneBloom: m_ShaderBloomFinal compiled [programID={0}]", m_ShaderBloomFinal->GetProgramID());

    m_ShaderBloom->Bind();
    m_ShaderBloom->SetInt("diffuseTexture", 0);
    m_ShaderBlur->Bind();
    m_ShaderBlur->SetInt("image", 0);
    m_ShaderBloomFinal->Bind();
    m_ShaderBloomFinal->SetInt("scene", 0);
    m_ShaderBloomFinal->SetInt("bloomBlur", 1);
}

void SceneBloom::SetupFramebuffers()
{
    ResetHandlers();
    GenerateConditional();
}

void SceneBloom::GenerateConditional()
{
    m_Width = Application::Get()->GetWindow()->GetWidth();
    m_Height = Application::Get()->GetWindow()->GetHeight();

    if (m_Width != m_WidthPrev || m_Height != m_HeightPrev)
    {
        Release();
        Generate();

        m_WidthPrev = m_Width;
        m_HeightPrev = m_Height;
    }
}

void SceneBloom::Generate()
{
    // configure (floating point) framebuffers
    // ---------------------------------------
    glGenFramebuffers(1, &m_HDR_FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_HDR_FBO);

    // create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)
    glGenTextures(2, m_ColorBuffers);

    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, m_ColorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Width, m_Height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, m_ColorBuffers[i], 0);
    }

    // create and attach depth buffer (renderbuffer)
    glGenRenderbuffers(1, &m_RBO_Depth);
    glBindRenderbuffer(GL_RENDERBUFFER, m_RBO_Depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_Width, m_Height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_RBO_Depth);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer not complete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    glGenFramebuffers(2, m_PingPongFBO);
    glGenTextures(2, m_PingPongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_PingPongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, m_PingPongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_Width, m_Height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_PingPongColorbuffers[i], 0);

        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "Framebuffer not complete!" << std::endl;
        }
    }
}

void SceneBloom::SetupLights()
{
    // lighting info
    // -------------

    // positions
    m_LightPositions.push_back(glm::vec3(0.0f, 0.5f, 1.5f));
    m_LightPositions.push_back(glm::vec3(-4.0f, 0.5f, -3.0f));
    m_LightPositions.push_back(glm::vec3(3.0f, 0.5f, 1.0f));
    m_LightPositions.push_back(glm::vec3(-.8f, 2.4f, -1.0f));

    // colors
    m_LightColors.push_back(glm::vec3(5.0f, 5.0f, 5.0f));
    m_LightColors.push_back(glm::vec3(10.0f, 0.0f, 0.0f));
    m_LightColors.push_back(glm::vec3(0.0f, 0.0f, 15.0f));
    m_LightColors.push_back(glm::vec3(0.0f, 5.0f, 0.0f));
}

void SceneBloom::Update(float timestep, Window* mainWindow)
{
    Scene::Update(timestep, mainWindow);

    GenerateConditional();
}

void SceneBloom::UpdateImGui(float timestep, Window* mainWindow)
{
    bool p_open = true;
    ShowExampleAppDockSpace(&p_open, mainWindow);

    ImGui::Begin("Framebuffers");
    {
        ImVec2 imageSize(128.0f, 128.0f);

        ImGui::Text("Color Buffer 0");
        ImGui::Image((void*)(intptr_t)m_ColorBuffers[0], imageSize);

        ImGui::Text("Color Buffer 1");
        ImGui::Image((void*)(intptr_t)m_ColorBuffers[1], imageSize);

        ImGui::Text("Ping Pong\nColor Buffer 0");
        ImGui::Image((void*)(intptr_t)m_PingPongColorbuffers[0], imageSize);

        ImGui::Text("Ping Pong\nColor Buffer 1");
        ImGui::Image((void*)(intptr_t)m_PingPongColorbuffers[1], imageSize);
    }
    ImGui::End();

    ImGui::Begin("Settings");
    {
        ImGui::Checkbox("Bloom Enabled", &m_BloomEnabled);
        ImGui::DragFloat("Exposure", &m_Exposure, 0.01f, 0.0f, 4.0f);
    }
    ImGui::End();
}

void SceneBloom::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, H2M::RefH2M<MoravaShader>> shaders, std::map<std::string, int> uniforms)
{
    if (m_Width == 0 || m_Height == 0) return;

    // 1. render scene into floating point framebuffer
    // -----------------------------------------------
    glBindFramebuffer(GL_FRAMEBUFFER, m_HDR_FBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), (float)m_Width / (float)m_Height, 0.1f, 1000.0f);
    glm::mat4 view = m_Camera->GetViewMatrix();
    glm::mat4 model = glm::mat4(1.0f);
    m_ShaderBloom->Bind();
    m_ShaderBloom->SetMat4("projection", projection);
    m_ShaderBloom->SetMat4("view", view);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_TextureWood);

    // set lighting uniforms
    for (unsigned int i = 0; i < m_LightPositions.size(); i++)
    {
        m_ShaderBloom->SetFloat3("lights[" + std::to_string(i) + "].Position", m_LightPositions[i]);
        m_ShaderBloom->SetFloat3("lights[" + std::to_string(i) + "].Color", m_LightColors[i]);
    }

    // m_ShaderBloom->SetFloat3("viewPos", m_Camera->GetPosition());

    // create one large cube that acts as the floor
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0));
    model = glm::scale(model, glm::vec3(12.5f, 0.5f, 12.5f));
    m_ShaderBloom->SetMat4("model", model);
    renderCube();

    // then create multiple cubes as the scenery
    glBindTexture(GL_TEXTURE_2D, m_TextureContainer);
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0));
    model = glm::scale(model, glm::vec3(0.5f));
    m_ShaderBloom->SetMat4("model", model);
    renderCube();

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.0f, 0.0f, 1.0));
    model = glm::scale(model, glm::vec3(0.5f));
    m_ShaderBloom->SetMat4("model", model);
    renderCube();

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.0f, -1.0f, 2.0));
    model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    m_ShaderBloom->SetMat4("model", model);
    renderCube();

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 2.7f, 4.0));
    model = glm::rotate(model, glm::radians(23.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    model = glm::scale(model, glm::vec3(1.25));
    m_ShaderBloom->SetMat4("model", model);
    renderCube();

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-2.0f, 1.0f, -3.0));
    model = glm::rotate(model, glm::radians(124.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    m_ShaderBloom->SetMat4("model", model);
    renderCube();

    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-3.0f, 0.0f, 0.0));
    model = glm::scale(model, glm::vec3(0.5f));
    m_ShaderBloom->SetMat4("model", model);
    renderCube();

    // finally show all the light sources as bright cubes
    m_ShaderLightBox->Bind();
    m_ShaderLightBox->SetMat4("projection", projection);
    m_ShaderLightBox->SetMat4("view", view);

    for (unsigned int i = 0; i < m_LightPositions.size(); i++)
    {
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(m_LightPositions[i]));
        model = glm::scale(model, glm::vec3(0.25f));
        m_ShaderLightBox->SetMat4("model", model);
        m_ShaderLightBox->SetFloat3("lightColor", m_LightColors[i]);
        renderCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 2. blur bright fragments with two-pass Gaussian Blur 
    // --------------------------------------------------
    bool horizontal = true, first_iteration = true;
    unsigned int amount = 16;
    m_ShaderBlur->Bind();
    for (unsigned int i = 0; i < amount; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_PingPongFBO[horizontal]);
        m_ShaderBlur->SetInt("horizontal", horizontal);
        glBindTexture(GL_TEXTURE_2D, first_iteration ? m_ColorBuffers[1] : m_PingPongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
        renderQuad();
        horizontal = !horizontal;
        if (first_iteration) {
            first_iteration = false;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 3. now render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
    // --------------------------------------------------------------------------------------------------------------------------
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_ShaderBloomFinal->Bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_ColorBuffers[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_PingPongColorbuffers[!horizontal]);

    m_ShaderBloomFinal->SetInt("bloom", m_BloomEnabled);
    m_ShaderBloomFinal->SetFloat("exposure", m_Exposure);

    renderQuad();
}

void SceneBloom::renderCube()
{
    // initialize (if necessary)
    if (m_CubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };

        glGenVertexArrays(1, &m_CubeVAO);
        glGenBuffers(1, &m_CubeVBO);

        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, m_CubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // link vertex attributes
        glBindVertexArray(m_CubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(m_CubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void SceneBloom::renderQuad()
{
    if (m_QuadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &m_QuadVAO);
        glGenBuffers(1, &m_QuadVBO);
        glBindVertexArray(m_QuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(m_QuadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

unsigned int SceneBloom::loadTexture(char const* path, bool gammaCorrection)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum internalFormat;
        GLenum dataFormat;
        if (nrComponents == 1)
        {
            internalFormat = dataFormat = GL_RED;
        }
        else if (nrComponents == 3)
        {
            internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrComponents == 4)
        {
            internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void SceneBloom::Release()
{
    if (m_HDR_FBO > 0) {
        glDeleteFramebuffers(1, &m_HDR_FBO);
    }

    glDeleteRenderbuffers(1, &m_RBO_Depth);

    for (unsigned int i = 0; i < 2; i++)
    {
        if (m_PingPongFBO[i] > 0) {
            glDeleteFramebuffers(1, &m_PingPongFBO[i]);
        }

        if (m_ColorBuffers[i] > 0) {
            glDeleteTextures(1, &m_ColorBuffers[i]);
        }

        if (m_PingPongColorbuffers[i] > 0) {
            glDeleteTextures(1, &m_PingPongColorbuffers[i]);
        }
    }

    ResetHandlers();
}

void SceneBloom::ResetHandlers()
{
    m_HDR_FBO = 0;

    m_RBO_Depth = 0;

    for (unsigned int i = 0; i < 2; i++)
    {
        m_PingPongFBO[i] = 0;

        m_ColorBuffers[i] = 0;
        m_PingPongColorbuffers[i] = 0;
    }
}
