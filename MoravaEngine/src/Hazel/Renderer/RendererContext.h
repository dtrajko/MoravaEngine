#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Core/Ref.h"

struct GLFWwindow;


namespace Hazel {

	class RendererContext : public RefCounted
	{
	public:
		RendererContext() = default;
		virtual ~RendererContext() = default;

		virtual void Create() = 0;
		virtual void BeginFrame() = 0;
		virtual void SwapBuffers() = 0;

		virtual void OnResize(uint32_t width, uint32_t height) = 0;

		static Ref<RendererContext> Create(GLFWwindow* windowHandle);
	};

}
