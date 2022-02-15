/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "OpenGLContextH2M.h"

#include "H2M/Core/AssertH2M.h"

#include "Core/CommonValues.h"
#include "Core/Log.h"


namespace H2M {

	OpenGLContextH2M::OpenGLContextH2M(Window* window) : m_Window(window) {}

	OpenGLContextH2M::~OpenGLContextH2M() {}

	void OpenGLContextH2M::Init()
	{
		Log::GetLogger()->info("OpenGLContextH2M::Init");

		// Set context for GLEW to use
		glfwMakeContextCurrent(m_Window->GetHandle());

		// Allow modern extension features
		glewExperimental = GL_TRUE;

		if (glewInit() != GLEW_OK)
		{
			glfwDestroyWindow(m_Window->GetHandle());
			glfwTerminate();
			throw std::runtime_error("GLEW initialization failed!");
		}

		Log::GetLogger()->info("GLEW initialized.");

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

	void OpenGLContextH2M::SwapBuffers()
	{
		glfwSwapBuffers(m_Window->GetHandle());
	}

}
