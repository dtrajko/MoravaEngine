#pragma once

#include "RenderPass.h"
#include "HazelTexture.h"
#include "../Scene/Entity.h"


namespace Hazel {


	struct SceneRendererOptions
	{
		bool ShowGrid = true;
		bool ShowBoundingBoxes = false;
	};

	class SceneRenderer
	{
	public:
		static void Init();

		static void SetViewportSize(uint32_t width, uint32_t height);

		static void BeginScene(const HazelScene* scene);
		static void EndScene();

		static void SubmitEntity(Entity* entity);

		static std::pair<Hazel::HazelTextureCube*, Hazel::HazelTextureCube*> CreateEnvironmentMap(const std::string& filepath);

		static Ref<RenderPass> GetFinalRenderPass();
		static Ref<HazelTexture2D> GetFinalColorBuffer();

		// TODO: Temp
		static uint32_t GetFinalColorBufferRendererID();

		static SceneRendererOptions& GetOptions();
	private:
		static void FlushDrawList();
		static void GeometryPass();
		static void CompositePass();
	};

}
