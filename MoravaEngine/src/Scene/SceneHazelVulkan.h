#pragma once

#include "Scene/Scene.h"

#include "H2M/Editor/SceneHierarchyPanel.h"
#include "H2M/Core/Events/KeyEvent.h"
#include "H2M/Renderer/HazelMesh.h"
#include "H2M/Scene/Entity.h"

#include "H2M/Platform/Vulkan/VulkanTestLayer.h"

#include "Framebuffer/MoravaFramebuffer.h"
#include "HazelVulkan/VulkanWeekRenderer.h"
#include "Mesh/Grid.h"
#include "Texture/MoravaTexture.h"

#include <map>
#include <string>


class VulkanTestLayer;

class SceneHazelVulkan : public Scene
{
public:
	SceneHazelVulkan();
	virtual ~SceneHazelVulkan() override;

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

	bool OnKeyPressed(KeyPressedEvent& e);

	// Temporary/experimental
	virtual void OnEntitySelected(H2M::Entity entity) override;

public:
	// std::unique_ptr<H2M::VulkanTestLayer> m_VulkanTestLayer;

private:
	H2M::RefH2M<MoravaShader> m_ShaderBackground;
	H2M::RefH2M<MoravaShader> m_ShaderBasic;

	Grid* m_Grid;
	Pivot* m_PivotScene;

	friend class EnvMapEditorLayer;

};
