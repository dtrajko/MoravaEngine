#include "OpenGLContext.h"

#include "Hazel/Core/Assert.h"

#include "Core/Log.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>


namespace Hazel {

	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
	}

	OpenGLContext::~OpenGLContext()
	{
	}

	void OpenGLContext::Create()
	{
		Log::GetLogger()->info("OpenGLContext::Create");

		glfwMakeContextCurrent(m_WindowHandle);

		Log::GetLogger()->info("OpenGL Info:");
		Log::GetLogger()->info("  Vendor: {0}", glGetString(GL_VENDOR));
		Log::GetLogger()->info("  Renderer: {0}", glGetString(GL_RENDERER));
		Log::GetLogger()->info("  Version: {0}", glGetString(GL_VERSION));

#ifdef HZ_ENABLE_ASSERTS
		int versionMajor;
		int versionMinor;
		glGetIntegerv(GL_MAJOR_VERSION, &versionMajor);
		glGetIntegerv(GL_MINOR_VERSION, &versionMinor);

		if (!(versionMajor > 4 || (versionMajor == 4 && versionMinor >= 5)))
		{
			Log::GetLogger()->error("Hazel requires at least OpenGL version 4.5!");
		}
#endif
	}

	void OpenGLContext::SwapBuffers()
	{
		glfwSwapBuffers(m_WindowHandle);
	}

}
