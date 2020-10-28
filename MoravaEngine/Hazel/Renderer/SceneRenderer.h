#pragma once

#include "RenderPass.h"
#include "HazelTexture.h"
#include "RenderCommandQueue.h"
#include "../Scene/Entity.h"

#include "../../Camera.h"
#include "../../HazelFullscreenQuad.h"
#include "../../Scene.h"

namespace Hazel {


	struct SceneRendererOptions
	{
		bool ShowGrid = true;
		bool ShowBoundingBoxes = false;
	};

	struct SceneRendererData
	{
		Hazel::HazelScene* ActiveScene = nullptr;
		struct SceneInfo
		{
			Camera* SceneCamera;

			// Resources
			Material* SkyboxMaterial;
			Hazel::Environment SceneEnvironment;
			Hazel::HazelLight ActiveLight;
		} SceneData;

		Hazel::HazelTexture2D* BRDFLUT;

		Hazel::RenderPass* GeoPass;
		Hazel::RenderPass* CompositePass;
		Hazel::RenderPass* ActiveRenderPass;

		struct DrawCommand
		{
			std::string Name;
			Mesh* Mesh;
			Material* Material;
			glm::mat4 Transform;
		};
		std::vector<DrawCommand> DrawList;

		// Grid
		Material* GridMaterial;

		SceneRendererOptions Options;

		// Renderer data
		Hazel::RenderCommandQueue* m_CommandQueue;

		unsigned int FullscreenQuadVAO;
		unsigned int FullscreenQuadVBO;
		unsigned int FullscreenQuadIBO;
	};


	class SceneRenderer
	{
	public:
		SceneRenderer() = default;
		SceneRenderer(std::string filepath, Scene* scene);
		~SceneRenderer();

		void Init(std::string filepath, Scene* scene);

		static void SetViewportSize(uint32_t width, uint32_t height);

		void BeginScene(HazelScene* scene); // static
		void EndScene(); // static

		static void SubmitEntity(Entity* entity);

		// static RenderPass* GetFinalRenderPass();
		// static Ref<HazelTexture2D> GetFinalColorBuffer();

		// TODO: Temp
		// static uint32_t GetFinalColorBufferRendererID();

		static SceneRendererOptions& GetOptions();

		// From EnvironmentMap
		inline SceneRendererData* GetContextData() { return &s_Data; }
		inline HazelLight GetLight() { return s_Data.SceneData.ActiveLight; }
		inline void SetLight(HazelLight light) { s_Data.SceneData.ActiveLight = light; }
		Environment Load(const std::string& filepath);
		void SetEnvironment(Hazel::Environment environment);
		std::pair<Hazel::HazelTextureCube*, Hazel::HazelTextureCube*> CreateEnvironmentMap(const std::string& filepath);
		inline Shader* GetShaderSkybox() { return m_ShaderSkybox; }
		inline Shader* GetShaderGrid() { return m_ShaderGrid; }
		inline Shader* GetShaderComposite() { return m_ShaderComposite; }
		inline Hazel::HazelTexture2D* GetEnvEquirect() { return m_EnvEquirect; }
		Hazel::RenderPass* GetFinalRenderPass();
		FramebufferTexture* GetFinalColorBuffer();
		uint32_t GetFinalColorBufferID();
		HazelFullscreenQuad* GetFullscreenQuad() { return m_HazelFullscreenQuad; }
		// From EnvironmentMap (Renderer methods)
		void Renderer_BeginRenderPass(Hazel::RenderPass* renderPass, bool clear);
		void Renderer_EndRenderPass();
		void Renderer_SubmitFullscreenQuad(Material* material);

	private:
		void FlushDrawList(); // TODO: static
		void GeometryPass();  // TODO: static
		void CompositePass(); // TODO: static

		// From EnvironmentMap
		void SetupShaders();

	public:
		static SceneRendererData s_Data;

		// From EnvironmentMap
		Shader* m_ShaderEquirectangularConversion;
		Shader* m_ShaderEnvFiltering;
		Shader* m_ShaderEnvIrradiance;
		Shader* m_ShaderGrid;
		Shader* m_ShaderSkybox;
		Shader* m_ShaderComposite;

		// Intermediate textures
		Hazel::HazelTextureCube* m_EnvUnfiltered;
		Hazel::HazelTexture2D* m_EnvEquirect;
		Hazel::HazelTextureCube* m_EnvFiltered;
		Hazel::HazelTextureCube* m_IrradianceMap;

		HazelFullscreenQuad* m_HazelFullscreenQuad;

		std::map<std::string, unsigned int>* m_SamplerSlots;
	};

}
