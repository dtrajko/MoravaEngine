#pragma once

#include "../Core/Ref.h"
#include "../Scene/Entity.h"
#include "../Renderer/SceneEnvironment.h"
#include "EditorCamera.h"
#include "HazelMaterial.h"
#include "HazelTexture.h"
#include "RenderCommandQueue.h"
#include "RenderPass.h"

#include "../../Camera.h"
#include "../../HazelFullscreenQuad.h"
#include "../../Scene.h"


namespace Hazel {

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
			HazelCamera* SceneCamera;

			// Resources
			Ref<HazelMaterialInstance> HazelSkyboxMaterial;
			Material* SkyboxMaterial;
			Environment SceneEnvironment;
			HazelLight ActiveLight;
		} SceneData;

		Ref<HazelTexture2D> BRDFLUT;

		::Ref<Shader> CompositeShader;

		Ref<RenderPass> GeoPass;
		Ref<RenderPass> CompositePass;
		Ref<RenderPass> ActiveRenderPass;

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
		Ref<HazelMaterialInstance> OutlineMaterial;

		SceneRendererOptions Options;

		// Renderer data
		RenderCommandQueue* m_CommandQueue;;
	};

	class SceneRenderer
	{
	public:
		SceneRenderer() = default;
		SceneRenderer(std::string filepath, HazelScene* scene);
		~SceneRenderer();

		void Init(std::string filepath, HazelScene* scene); // TODO convert to static

		static void Init(); // TODO

		static void SetViewportSize(uint32_t width, uint32_t height);

		static void BeginScene(HazelScene* scene, const EditorCamera& camera);
		static void BeginScene(HazelScene* scene);
		static void EndScene();

		static void SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f), Ref<HazelMaterialInstance> overrideMaterial = nullptr);
		static void SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f));

		static std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>> CreateEnvironmentMap(const std::string& filepath);

		static Ref<RenderPass> GetFinalRenderPass();
		static Ref<HazelTexture2D> GetFinalColorBuffer();

		static void SubmitEntity(Entity entity);

		// TODO: Temp
		static uint32_t GetFinalColorBufferRendererID();

		static SceneRendererOptions& GetOptions();

		// static FramebufferTexture* GetFinalColorBuffer();

	private:
		static void FlushDrawList();
		static void GeometryPass();
		static void CompositePass();

	public:
		// From EnvironmentMap
		inline SceneRendererData* GetContextData() { return &s_Data; }
		inline HazelLight GetLight() { return s_Data.SceneData.ActiveLight; }
		inline void SetLight(HazelLight light) { s_Data.SceneData.ActiveLight = light; }
		Environment Load(const std::string& filepath);
		static void SetEnvironment(Environment environment);
		inline ::Ref<Shader> GetShaderSkybox() { return m_ShaderSkybox; }
		inline ::Ref<Shader> GetShaderGrid() { return m_ShaderGrid; }
		inline ::Ref<Shader> GetShaderComposite() { return s_Data.CompositeShader; }
		inline Ref<HazelTexture2D> GetEnvEquirect() { return m_EnvEquirect; }
		uint32_t GetFinalColorBufferID();

		// From EnvironmentMap
		void SetupShaders(); // TODO convert to static

		static SceneRendererData s_Data;
		static std::map<std::string, unsigned int>* m_SamplerSlots;

		// From EnvironmentMap
		static ::Ref<Shader> m_ShaderEquirectangularConversion;
		static ::Ref<Shader> m_ShaderEnvFiltering;
		static ::Ref<Shader> m_ShaderEnvIrradiance;
		static ::Ref<Shader> m_ShaderGrid;
		static ::Ref<Shader> m_ShaderSkybox;

		// Intermediate textures
		static Ref<HazelTextureCube> m_EnvUnfiltered;
		static Ref<HazelTexture2D> m_EnvEquirect;
		static Ref<HazelTextureCube> m_EnvFiltered;
		static Ref<HazelTextureCube> m_IrradianceMap;

		float m_GridScale = 16.025f;
		float m_GridSize = 0.025f;

	};

}
