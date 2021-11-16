#pragma once

#include "Scene/Scene.h"

#include "Hazel/Editor/SceneHierarchyPanel.h"
#include "Hazel/Core/Events/KeyEvent.h"
#include "Hazel/Renderer/HazelMesh.h"
#include "Hazel/Scene/Entity.h"

#include "HazelLegacy/Platform/Vulkan/VulkanTestLayer.h"

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
		std::map<std::string, Hazel::Ref<MoravaShader>> shaders, std::map<std::string, int> uniforms) override;
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
	virtual void OnEntitySelected(Hazel::EntityHazelLegacy entity) override;

public:
	// std::unique_ptr<Hazel::VulkanTestLayer> m_VulkanTestLayer;

private:
	Hazel::Ref<MoravaShader> m_ShaderBackground;
	Hazel::Ref<MoravaShader> m_ShaderBasic;

	Grid* m_Grid;
	Pivot* m_PivotScene;

	friend class EnvMapEditorLayer;

};
