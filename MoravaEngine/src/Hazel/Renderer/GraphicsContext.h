#pragma once

#include "Hazel/Core/Ref.h"
#include "Hazel/Renderer/RendererContext.h"

#include "Core/CommonValues.h"


namespace Hazel {

	class GraphicsContext : public RendererContext
	{
	public:
		static Ref<RendererContext> Create(GLFWwindow* windowHandle);

		void Init();

	private:

	};

}
