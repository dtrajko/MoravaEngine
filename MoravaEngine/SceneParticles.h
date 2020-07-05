#pragma once

#include "Scene.h"

#include "ParticleSystemCherno.h"
#include "Grid.h"
#include "Pivot.h"
#include "MaterialWorkflowPBR.h"
#include "TextureCubemap.h"


class SceneParticles : public Scene
{

public:
	SceneParticles();
	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void UpdateImGui(float timestep, Window& mainWindow) override;
	virtual void Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;
	virtual ~SceneParticles() override;

private:
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;
	void SetupParticles();

	MaterialWorkflowPBR* m_MaterialWorkflowPBR;

	TextureCubeMap* m_TextureCubeMap;

	// Particle System
	ParticleSystemCherno m_ParticleSystem;
	ParticleProps m_Particle;

	Grid* m_Grid;
	Pivot* m_PivotScene;

	Shader* m_ShaderFBScene;
};
