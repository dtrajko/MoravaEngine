#pragma once

#include "Scene.h"


class SceneFramebuffers : public Scene
{

public:
	SceneFramebuffers();
	virtual void Update(float timestep, Window& mainWindow) override;
	virtual void UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults) override;
	virtual void Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms) override;

	void SetGeometry();
	void CleanupGeometry();
	int GetEffectForFrame(int frameID);

	virtual ~SceneFramebuffers() override;

private:
	virtual void SetSkybox() override;
	virtual void SetTextures() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;

	int m_EffectFrame_0;
	int m_EffectFrame_1;
	int m_EffectFrame_2;
	int m_EffectFrame_3;
};
