#pragma once

#include "Scene.h"
#include "EnvironmentMap.h"
#include "Hazel/Renderer/HazelMesh.h"
#include "Framebuffer.h"
#include "Texture.h"
#include "Hazel/Panels/SceneHierarchyPanel.h"
#include "Hazel/Scene/Entity.h"
#include "Grid.h"
#include "Pivot.h"
#include "Hazel/Events/KeyEvent.h"
#include "Hazel/Scene/Entity.h"

#include <map>
#include <string>


const int MAX_LIGHTS_ENV_MAP = 4 + 4; // (4 x point lights) + (4 x spot lights)

class SceneHazelEnvMap : public Scene
{

public:
	SceneHazelEnvMap();
	virtual ~SceneHazelEnvMap() override;

	virtual void Update(float timestep, Window* mainWindow) override;
	virtual void UpdateImGui(float timestep, Window* mainWindow) override;
	void UpdateImGuizmo(Window* mainWindow);
	virtual void ShowExampleAppDockSpace(bool* p_open, Window* mainWindow) override;
	virtual void Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
		std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms) override;
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

	void SetupRenderFramebuffer();
	void ResizeViewport(glm::vec2 viewportPanelSize, Framebuffer* renderFramebuffer);
	void CheckIntersection(Window* mainWindow);
	void RenderLineElements(Shader* shaderBasic, glm::mat4 projectionMatrix);

	bool OnKeyPressed(KeyPressedEvent& e);

	void NewScene();
	void OpenScene();
	void SaveSceneAs();

	// Temporary/experimental
	virtual void OnEntitySelected(Hazel::Entity* entity) override;

private:
	EnvironmentMap* m_EnvironmentMap;

	struct Viewport
	{
		int X;
		int Y;
		int Width;
		int Height;
		int MouseX;
		int MouseY;
	};

	Viewport m_ImGuiViewport;
	Viewport m_ImGuiViewportEnvMap;

	// viewports
	// -- viewport main
	Framebuffer* m_RenderFramebuffer;
	int m_ImGuiViewportMainX;
	int m_ImGuiViewportMainY;
	bool m_IsViewportEnabled;
	bool m_ViewportFocused;
	bool m_ViewportHovered;
	glm::vec2 m_ViewportMainSize;
	// -- viewport environment map
	int m_ImGuiViewportEnvMapX;
	int m_ImGuiViewportEnvMapY;
	bool m_IsViewportEnvMapEnabled;
	bool m_ViewportEnvMapFocused;
	bool m_ViewportEnvMapHovered;
	glm::vec2 m_ViewportEnvMapSize;

	// Used in EnvironmentMap::CastRay
	glm::vec2 m_ViewportBounds[2];

	Shader* m_ShaderBackground;
	Shader* m_ShaderBasic;

	struct Entity {
		struct Transform {
			glm::vec3 Translation;
			glm::quat Rotation;
			glm::vec3 Scale;
			glm::mat4 Transform;
		} Transform;
		struct Init {
			struct Transform {
				glm::vec3 Scale;
			} Transform;
			struct AABBTransform {
				struct Transform {
					glm::vec3 Scale;
				} Transform;
			} AABB;
		} Init;
		struct AABBTransform {
			glm::vec3 Translation;
			glm::quat Rotation;
			glm::vec3 Scale;
			glm::mat4 Transform;
		} AABBTransform;

		AABB AABB;
		glm::vec3 OriginOffset;
		bool Intersecting;
		bool Enabled;
	};

	std::map<std::string, Entity> m_Entities;

	int m_HDRI_Edit;
	int m_HDRI_Edit_Prev;

	float m_CurrentTimestamp;
	EventCooldown m_ResizeViewport;

	bool m_VisibleAABBs;

	int m_ImGuizmoType = -1; // -1 = no gizmo

	glm::vec3 m_Translation_ImGuizmo;
	glm::mat4* m_Transform_ImGuizmo;

	Hazel::SceneHierarchyPanel* m_SceneHierarchyPanel;

	bool m_DisplayLineElements;
	Grid* m_Grid;
	Pivot* m_PivotScene;

	// Hazelnut/EditorLayer
	float m_SnapValue = 0.5f;

};
