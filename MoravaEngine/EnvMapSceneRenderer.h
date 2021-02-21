#pragma once

#include "Hazel/Core/Ref.h"
#include "Hazel/Scene/Entity.h"
#include "Hazel/Renderer/EditorCamera.h"
#include "Hazel/Renderer/HazelMaterial.h"
#include "Hazel/Renderer/HazelTexture.h"
#include "Hazel/Renderer/RenderCommandQueue.h"
#include "Hazel/Renderer/RenderPass.h"
#include "Hazel/Renderer/SceneEnvironment.h"

#include "Camera.h"
#include "HazelFullscreenQuad.h"
#include "Scene.h"


struct SceneRendererOptions
{
	bool ShowGrid = true;
	bool ShowBoundingBoxes = false;
};

struct SceneRendererCamera
{
	Hazel::HazelCamera Camera;
	glm::mat4 ViewMatrix;
};

struct SceneRendererData
{
	// HazelScene* ActiveScene = nullptr;
	struct SceneInfo
	{
		Hazel::HazelCamera* SceneCamera;

		// Resources
		Ref<Hazel::HazelMaterialInstance> HazelSkyboxMaterial;
		Material* SkyboxMaterial;
		Hazel::Environment SceneEnvironment;
		Hazel::HazelLight ActiveLight;
	} SceneData;

	Hazel::Ref<Hazel::HazelTexture2D> BRDFLUT;

	Ref<Shader> CompositeShader;

	Hazel::Ref<Hazel::RenderPass> GeoPass;
	Hazel::Ref<Hazel::RenderPass> CompositePass;
	Hazel::Ref<Hazel::RenderPass> ActiveRenderPass;

	struct DrawCommand
	{
		std::string Name;
		Mesh* Mesh;
		Material* Material;
		glm::mat4 Transform;
	};
	std::vector<DrawCommand> DrawList;
	std::vector<DrawCommand> SelectedMeshDrawList;

	// Grid
	Material* GridMaterial;
	// Ref<HazelShader> HazelGridShader;
	// Ref<Shader> GridShader;
	Ref<Hazel::HazelMaterialInstance> OutlineMaterial;

	SceneRendererOptions Options;

	// Renderer data
	Hazel::RenderCommandQueue* m_CommandQueue;
};


class EnvMapSceneRenderer
{
public:
	EnvMapSceneRenderer() = default;
	EnvMapSceneRenderer(std::string filepath, Hazel::HazelScene* scene);
	~EnvMapSceneRenderer();

	// static void Init(); // TODO
	static void Init(std::string filepath, Hazel::HazelScene* scene); // TODO convert to static

	static void SetViewportSize(uint32_t width, uint32_t height);

	static void BeginScene(Hazel::HazelScene* scene, const Hazel::EditorCamera& camera);
	static void BeginScene(Hazel::HazelScene* scene);
	static void EndScene();

	static void SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f), Ref<Hazel::HazelMaterialInstance> overrideMaterial = nullptr);
	static void SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f));

	static std::pair<Hazel::Ref<Hazel::HazelTextureCube>, Hazel::Ref<Hazel::HazelTextureCube>> CreateEnvironmentMap(const std::string& filepath);

	static Hazel::Ref<Hazel::RenderPass> GetFinalRenderPass();
	// static Ref<HazelTexture2D> GetFinalColorBuffer();
	static FramebufferTexture* GetFinalColorBuffer();

	static void SubmitEntity(Hazel::Entity entity);

	// TODO: Temp
	static uint32_t GetFinalColorBufferRendererID();

	static SceneRendererOptions& GetOptions();


private:
	static void FlushDrawList();
	static void GeometryPass();
	static void CompositePass();

public:
	// From EnvironmentMap
	//static
	static Hazel::Environment Load(const std::string& filepath);

	inline SceneRendererData* GetContextData() { return &s_Data; }
	inline Hazel::HazelLight GetLight() { return s_Data.SceneData.ActiveLight; }
	inline void SetLight(Hazel::HazelLight light) { s_Data.SceneData.ActiveLight = light; }
	static void SetEnvironment(Hazel::Environment environment);
	inline Ref<Shader> GetShaderSkybox() { return m_ShaderSkybox; }
	inline Ref<Shader> GetShaderGrid() { return m_ShaderGrid; }
	inline Ref<Shader> GetShaderComposite() { return s_Data.CompositeShader; }
	inline Hazel::Ref<Hazel::HazelTexture2D> GetEnvEquirect() { return m_EnvEquirect; }
	uint32_t GetFinalColorBufferID();

	// From EnvironmentMap
	static void SetupShaders(); // TODO convert to static

	static SceneRendererData s_Data;
	static std::map<std::string, unsigned int>* m_SamplerSlots;

	// From EnvironmentMap
	static Ref<Shader> m_ShaderEquirectangularConversion;
	static Ref<Shader> m_ShaderEnvFiltering;
	static Ref<Shader> m_ShaderEnvIrradiance;
	static Ref<Shader> m_ShaderGrid;
	static Ref<Shader> m_ShaderSkybox;

	// Intermediate textures
	static Hazel::Ref<Hazel::HazelTextureCube> m_EnvUnfiltered;
	static Hazel::Ref<Hazel::HazelTexture2D> m_EnvEquirect;
	static Hazel::Ref<Hazel::HazelTextureCube> m_EnvFiltered;
	static Hazel::Ref<Hazel::HazelTextureCube> m_IrradianceMap;

	static float s_GridScale;
	static float s_GridSize;

};
