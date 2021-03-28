#include "../../Renderer/RendererAPI.h"
#include "../../Core/Base.h"
#include "../../Core/Assert.h"

#include "Core/Log.h"

#include <GL/glew.h>


namespace Hazel {

	static void OpenGLLogMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
	{
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:
			Log::GetLogger()->error("[OpenGL Debug HIGH] {0}", message);
			// HZ_CORE_ASSERT(false, "GL_DEBUG_SEVERITY_HIGH");
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			Log::GetLogger()->warn("[OpenGL Debug MEDIUM] {0}", message);
			break;
		case GL_DEBUG_SEVERITY_LOW:
			Log::GetLogger()->info("[OpenGL Debug LOW] {0}", message);
			break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			HZ_CORE_TRACE("[OpenGL Debug NOTIFICATION] {0}", message);
			break;
		}
	}

	void RendererAPI::Init()
	{
		glDebugMessageCallback(OpenGLLogMessage, nullptr);
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

		unsigned int vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glEnable(GL_DEPTH_TEST);
		//glEnable(GL_CULL_FACE);
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		glFrontFace(GL_CCW);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_MULTISAMPLE);

		auto& caps = RendererAPI::GetCapabilities();

		caps.Vendor = (const char*)glGetString(GL_VENDOR);
		caps.Renderer = (const char*)glGetString(GL_RENDERER);
		caps.Version = (const char*)glGetString(GL_VERSION);

		glGetIntegerv(GL_MAX_SAMPLES, &caps.MaxSamples);
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &caps.MaxAnisotropy);

		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &caps.MaxTextureUnits);

		GLenum error = glGetError();
		while (error != GL_NO_ERROR)
		{
			Log::GetLogger()->error("OpenGL Error {0}", error);
			error = glGetError();
		}

		LoadRequiredAssets();
	}

	void RendererAPI::Shutdown()
	{
	}

	void RendererAPI::LoadRequiredAssets()
	{
	}

	void RendererAPI::Clear(float r, float g, float b, float a)
	{
		glClearColor(r, g, b, a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void RendererAPI::SetClearColor(float r, float g, float b, float a)
	{
		glClearColor(r, g, b, a);
	}

	void RendererAPI::DrawIndexed(uint32_t count, PrimitiveType type, bool depthTest)
	{
		if (!depthTest) {
			glDisable(GL_DEPTH_TEST);
		}

		GLenum glPrimitiveType = 0;
		switch (type)
		{
		case PrimitiveType::Triangles:
			glPrimitiveType = GL_TRIANGLES;
			break;
		case PrimitiveType::Lines:
			glPrimitiveType = GL_LINES;
			break;
		}

		glDrawElements(glPrimitiveType, count, GL_UNSIGNED_INT, nullptr);

		if (!depthTest) {
			glEnable(GL_DEPTH_TEST);
		}
	}

	void RendererAPI::SetLineThickness(float thickness)
	{
		glLineWidth(thickness);
	}

}
