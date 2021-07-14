#pragma once

#include "Scene/Scene.h"

#include "Framebuffer/MoravaFramebuffer.h"


class SceneFramebuffers : public Scene
{

public:
	SceneFramebuffers();
	virtual ~SceneFramebuffers() override;

	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void UpdateImGui(float timestep, Window* mainWindow) override;
	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, MoravaShader*> shaders, std::map<std::string, int> uniforms) override;

	void SetGeometry();
	void CleanupGeometry();
	int GetEffectForFrame(int frameID);


private:
	virtual void SetupTextures() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;
	virtual void SetupFramebuffers() override;

	void SetupShaders();

	int m_EffectFrame_0;
	int m_EffectFrame_1;
	int m_EffectFrame_2;
	int m_EffectFrame_3;

public:
	unsigned int SCR_WIDTH = 1280;
	unsigned int SCR_HEIGHT = 720;

	MoravaFramebuffer* m_Framebuffer;

	MoravaShader* m_ShaderFramebuffersScene;
	MoravaShader* m_ShaderFramebuffersScreen;

};
