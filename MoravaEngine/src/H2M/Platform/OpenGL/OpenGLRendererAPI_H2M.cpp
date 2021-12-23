/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "H2M/Renderer/RendererAPI_H2M.h"

#include <gl/glew.h>

#include "H2M/Renderer/ShaderH2M.h"


namespace H2M
{

	static void OpenGLLogMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
	{
		switch (severity)
		{
			case GL_DEBUG_SEVERITY_HIGH:
				H2M_CORE_ERROR("[OpenGL Debug HIGH] {0}", message);
				H2M_CORE_ASSERT(false, "GL_DEBUG_SEVERITY_HIGH");
				break;
			case GL_DEBUG_SEVERITY_MEDIUM:
				H2M_CORE_WARN("[OpenGL Debug MEDIUM] {0}", message);
				break;
			case GL_DEBUG_SEVERITY_LOW:
				H2M_CORE_INFO("[OpenGL Debug LOW] {0}", message);
				break;
			case GL_DEBUG_SEVERITY_NOTIFICATION:
				// HZ_CORE_TRACE("[OpenGL Debug NOTIFICATION] {0}", message);
				break;
		}
	}

	void RendererAPI_H2M::Init()
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

		glEnable(GL_MULTISAMPLE); // Probably won't work at this point - MSAA must be enabled before window creation

		glEnable(GL_STENCIL_TEST);

		auto& caps = GetCapabilities();

		caps.Vendor = (const char*)glGetString(GL_VENDOR);
		caps.Device = (const char*)glGetString(GL_RENDERER);
		caps.Version = (const char*)glGetString(GL_VERSION);

		glGetIntegerv(GL_MAX_SAMPLES, &caps.MaxSamples);
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &caps.MaxAnisotropy);

		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &caps.MaxTextureUnits);

		GLenum error = glGetError();
		while (error != GL_NO_ERROR)
		{
			H2M_CORE_ERROR("OpenGL Error {0}", error);
			error = glGetError();
		}

		LoadRequiredAssets();
	}

	void RendererAPI_H2M::Shutdown()
	{
	}

	void RendererAPI_H2M::LoadRequiredAssets()
	{
	}

	void RendererAPI_H2M::Clear(float r, float g, float b, float a)
	{
		glClearColor(r, g, b, a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	void RendererAPI_H2M::SetClearColor(float r, float g, float b, float a)
	{
		glClearColor(r, g, b, a);
	}

	void RendererAPI_H2M::SetLineThickness(float thickness)
	{
		glLineWidth(thickness);
	}

	void RendererAPI_H2M::DrawIndexed(uint32_t indexCount, PrimitiveTypeH2M type, bool depthTest)
	{
		if (!depthTest)
		{
			glDisable(GL_DEPTH_TEST);
		}

		GLenum glPrimitiveType = 0;
		switch (type)
		{
			case PrimitiveTypeH2M::Triangles:
				glPrimitiveType = GL_TRIANGLES;
				break;
			case PrimitiveTypeH2M::Lines:
				glPrimitiveType = GL_LINES;
				break;
		}

		glDrawElements(glPrimitiveType, indexCount, GL_UNSIGNED_INT, nullptr);

		if (!depthTest)
		{
			glEnable(GL_DEPTH_TEST);
		}
	}

	void RendererAPI_H2M::DrawLines(RefH2M<VertexArrayH2M> vertexArray, uint32_t vertexCount)
	{
		vertexArray->Bind();
		glDrawArrays(GL_LINES, 0, vertexCount);
	}

	void RendererAPI_H2M::SetLineWidth(float width)
	{
		glLineWidth(width);
	}

}
