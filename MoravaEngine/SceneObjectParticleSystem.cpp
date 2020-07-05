#include "SceneObjectParticleSystem.h"
#include "Profiler.h"
#include "ParticleMaster.h"
#include "Timer.h"
#include "Log.h"


int SceneObjectParticleSystem::m_MaxInstances = 10000;

SceneObjectParticleSystem::SceneObjectParticleSystem()
{
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
}

void SceneObjectParticleSystem::Update(bool enabled, glm::vec3 cameraPosition, std::map<std::string, float>* profiler_results)
{
    // Re-generate Particle System
    if (m_Settings != m_SettingsPrev) {
        Regenerate();
        m_SettingsPrev = m_Settings;
    }

    if (enabled && m_System != nullptr) {
        Profiler profiler("SE::ParticleSystemTM::GeneratePatricles");
        m_System->GeneratePatricles(position, scale);
        profiler_results->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
    }

    {
        Profiler profiler("SE::ParticleMaster::Update");
        ParticleMaster::Update(cameraPosition);
        profiler_results->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
    }
}

void SceneObjectParticleSystem::Render()
{
}

void SceneObjectParticleSystem::Render(glm::mat4 viewMatrix, std::map<std::string, float>* profiler_results)
{
    Profiler profiler("SE::ParticleMaster::Render");
    ParticleMaster::Render(viewMatrix);
    profiler_results->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
}

SceneObjectParticleSystem::~SceneObjectParticleSystem()
{
    delete m_Texture;
    delete m_ParticleTexture;
    delete m_System;
}

void SceneObjectParticleSystem::Regenerate()
{
    // Cooldown
    float currentTimestamp = Timer::Get()->GetCurrentTimestamp();
    if (currentTimestamp - m_Regenerate.lastTime < m_Regenerate.cooldown) return;
    m_Regenerate.lastTime = currentTimestamp;

    LOG_INFO("Particle Settings changed, rebuilding the Particle System...");

    ParticleMaster::CleanUp();
    ParticleMaster::Init(m_Settings.instanced, m_MaxInstances);

    Texture* m_Texture = new Texture(m_Settings.textureName.c_str());
    // TODO: Texture* texture = HotLoadTexture(m_Settings.textureName);
    m_ParticleTexture = new ParticleTexture(m_Texture->GetID(), m_Settings.numRows);
    m_System = new ParticleSystemThinMatrix(m_ParticleTexture, m_Settings.PPS,
        m_Settings.direction, m_Settings.intensity, m_Settings.gravityComplient,
        m_Settings.lifeLength, m_Settings.diameter);
}
