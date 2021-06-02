#include "Scene/SceneSSAO.h"

#include "Core/Application.h"
#include "Mesh/Block.h"
#include "Mesh/Cube.h"
#include "Mesh/QuadSSAO.h"


SceneSSAO::SceneSSAO()
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
    SetupMeshes();
    SetupModels();
    SetupSSAO();

    m_RenderTarget = (int)RenderTarget::SSAO_Composite;
}

void SceneSSAO::SetupTextures()
{
    ResourceManager::LoadTexture("crate",       "Textures/crate.png");
    ResourceManager::LoadTexture("crateNormal", "Textures/crateNormal.png");
}

void SceneSSAO::SetupTextureSlots()
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

void SceneSSAO::SetupMeshes()
{
    // SSAO meshes
    Cube* cube = new Cube();
    meshes.insert(std::make_pair("cube", cube));
}

void SceneSSAO::SetupModels()
{
    ModelSSAO* gladiator = new ModelSSAO("Models/Gladiator/Gladiator.fbx", "IgnoreTextures");
    modelsSSAO.insert(std::make_pair("gladiator", gladiator));
}

void SceneSSAO::SetupFramebuffers()
{
}

void SceneSSAO::SetupSSAO()
{
    m_SSAO.Init();
}

void SceneSSAO::Update(float timestep, Window* mainWindow)
{
    Scene::Update(timestep, mainWindow);

    m_SSAO.Update();
}

void SceneSSAO::UpdateImGui(float timestep, Window* mainWindow)
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

void SceneSSAO::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, MoravaShader*> shaders, std::map<std::string, int> uniforms)
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

SceneSSAO::~SceneSSAO()
{
    delete meshes["block"];
    delete meshes["floor"];
    delete meshes["cube"];
    delete modelsSSAO["gladiator"];
}
