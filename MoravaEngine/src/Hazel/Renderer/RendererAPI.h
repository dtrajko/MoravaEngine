#pragma once

#include <cstdint>
#include <string>


namespace Hazel {

	using RendererID = uint32_t;

	enum class RendererAPIType
	{
		None,
		Vulkan,
		OpenGL
	};

	enum class PrimitiveType
	{
		None = 0, Triangles, Lines
	};

	struct RenderAPICapabilities
	{
		std::string Vendor;
		std::string Renderer;
		std::string Version;

		int MaxSamples = 0;
		float MaxAnisotropy = 0.0f;
		int MaxTextureUnits = 0;
	};

	class RendererAPI
	{
	public:

		static void Init();
		static void Shutdown();

		static void Clear(float r, float g, float b, float a);
		static void SetClearColor(float r, float g, float b, float a);

		static void DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest = true);
		static void SetLineThickness(float thickness);

		static RenderAPICapabilities& GetCapabilities()
		{
			static RenderAPICapabilities capabilities;
			return capabilities;
		}

		// Currently hard-coded to OpenGL
		static RendererAPIType Current() { s_CurrentRendererAPI = RendererAPIType::OpenGL; return s_CurrentRendererAPI; }

	private:
		static void LoadRequiredAssets();

	private:
		static RendererAPIType s_CurrentRendererAPI;

	};

}
