#include "SceneObjectParticleSystem.h"
#include "Profiler.h"
#include "ParticleMaster.h"
#include "Timer.h"
#include "Log.h"
#include "AABB.h"


SceneObjectParticleSystem::SceneObjectParticleSystem()
{
    m_Settings = {};

    m_SettingsPrev = m_Settings;
    m_SettingsPrev.textureName = "none";

    position = glm::vec3(0.0f, 0.0f, 0.0f);
    scale = glm::vec3(0.0f, 0.0f, 0.0f);

    pivot = new Pivot(position, scale);

    m_MaxInstances = 10000;
    m_Master = new ParticleMaster();
    m_Texture = nullptr;
    m_Camera = nullptr;
}

SceneObjectParticleSystem::SceneObjectParticleSystem(bool instancedRendering, int maxInstances, Camera* camera) : SceneObject()
{
    m_Camera = camera;

    m_Settings.textureName = "particle_atlas";
    m_Settings.numRows = 4;
    m_Settings.PPS = 40;
    m_Settings.direction = glm::vec3(0.0f, 1.0f, 0.0f);
    m_Settings.intensity = 4.0f;
    m_Settings.diameter = 0.2f;
    m_Settings.gravityComplient = 0.2f;
    m_Settings.lifeLength = 2.0f;
    m_Settings.instanced = true;

    m_SettingsPrev = m_Settings;
    m_SettingsPrev.textureName = "none";

    position = glm::vec3(0.0f, 0.0f, 0.0f);
    scale = glm::vec3(0.0f, 0.0f, 0.0f);

    pivot = new Pivot(position, scale);

    m_MaxInstances = maxInstances;
    m_Master = new ParticleMaster(instancedRendering, maxInstances);
}

void SceneObjectParticleSystem::Update(bool enabled, std::map<std::string, float>* profiler_results, Texture* texture)
{
    m_Texture = texture;
    glm::vec3 cameraPosition = m_Camera->GetPosition();

    // Re-generate Particle System
    if (m_Settings != m_SettingsPrev)
    {    
        Regenerate(m_Texture);
        m_SettingsPrev = m_Settings;
    }

    if (enabled && m_System != nullptr) {
        Profiler profiler("SOPS::ParticleSystemTM::GeneratePatricles");
        m_System->GenerateParticles(position, scale, m_Master);
        profiler_results->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
    }

    {
        Profiler profiler("SOPS::ParticleMaster::Update");
        m_Master->Update(cameraPosition);
        profiler_results->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
    }
}

void SceneObjectParticleSystem::Render()
{
    glm::mat4 viewMatrix = m_Camera->CalculateViewMatrix();
    m_Master->Render(viewMatrix);
}

SceneObjectParticleSystem::~SceneObjectParticleSystem()
{
    delete m_Texture;
    delete m_ParticleTexture;
    delete m_System;
    delete m_Master;
}

void SceneObjectParticleSystem::Regenerate(Texture* texture)
{
    // Cooldown
    float currentTimestamp = Timer::Get()->GetCurrentTimestamp();
    if (currentTimestamp - m_Regenerate.lastTime < m_Regenerate.cooldown) return;
    m_Regenerate.lastTime = currentTimestamp;

    LOG_INFO("Particle Settings changed, rebuilding the Particle System...");

    delete m_Master;
    m_Master = new ParticleMaster(m_Settings.instanced, m_MaxInstances);

    m_Texture = texture;
    // TODO: Texture* texture = HotLoadTexture(m_Settings.textureName);
    m_ParticleTexture = new ParticleTexture(m_Texture->GetID(), m_Settings.numRows);
    m_System = new ParticleSystemThinMatrix(m_ParticleTexture, m_Settings.PPS,
        m_Settings.direction, m_Settings.intensity, m_Settings.gravityComplient,
        m_Settings.lifeLength, m_Settings.diameter);
}
