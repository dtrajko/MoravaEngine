#pragma once

#include "Scene/Scene.h"

#include "H2M/Core/Events/KeyEventH2M.h"

#include "Framebuffer/MoravaFramebuffer.h"
#include "Mesh/Grid.h"
#include "Texture/MoravaTexture.h"

#include <map>
#include <string>


class SceneDX11 : public Scene
{
public:
	SceneDX11();
	virtual ~SceneDX11() override;

	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void UpdateImGui(float timestep, Window* mainWindow) override;
	virtual void ShowExampleAppDockSpace(bool* p_open, Window* mainWindow) override;
	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, H2M::RefH2M<MoravaShader>> shaders, std::map<std::string, int> uniforms) override;
	void SetupUniforms();

private:
	virtual void SetLightManager() override;
	virtual void SetWaterManager(int width, int height) override;
	virtual void SetupTextures() override;
	virtual void SetupTextureSlots() override;
	virtual void SetupMaterials() override;
	virtual void SetupMeshes() override;
	virtual void SetupModels() override;
	virtual void SetupFramebuffers() override;

	void SetupShaders(); // Usually in Renderer* classes

	bool OnKeyPressed(H2M::KeyPressedEventH2M& e);

	// Temporary/experimental
	virtual void OnEntitySelected(H2M::EntityH2M entity) override;

private:
	H2M::RefH2M<MoravaShader> m_ShaderBackground;
	H2M::RefH2M<MoravaShader> m_ShaderBasic;

};
