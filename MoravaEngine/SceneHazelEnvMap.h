#pragma once

#include "Scene.h"
#include "EnvMapEditorLayer.h"
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

	bool OnKeyPressed(KeyPressedEvent& e);

	// Temporary/experimental
	virtual void OnEntitySelected(Hazel::Entity entity) override;

private:
	EnvMapEditorLayer* m_EnvironmentMap;

	Ref<Shader> m_ShaderBackground;
	Ref<Shader> m_ShaderBasic;

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

	Grid* m_Grid;
	Pivot* m_PivotScene;

	friend class EnvMapEditorLayer;

};
