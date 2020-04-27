#include "RendererFramebuffers.h"

#include "SceneNanosuit.h"


RendererFramebuffers::RendererFramebuffers()
{
}

void RendererFramebuffers::Init(Scene* scene)
{
	SetUniforms();
	SetShaders();

	// create a framebuffer (FBO)
	unsigned int framebuffer;
	glGenFramebuffers(1, &framebuffer);

	// bind the framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	// generate a texture
	unsigned texColorBuffer;
	glGenTextures(1, &texColorBuffer);

	// bind the texture
	glBindTexture(GL_TEXTURE_2D, texColorBuffer);

	// Specify the target texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// unbind the texture
	glBindTexture(GL_TEXTURE_2D, 0);

	// attach the texture to currently bound framebuffer object
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColorBuffer, 0);

	// create a renderbuffer object (RBO) to be used as a depth and stencil attachment to the framebuffer
	unsigned int rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 800, 600);

	// Unbind the renderbuffer, once we've allocated enough memory for the renderbuffer object
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	// Attach the renderbuffer object to the depth and stencil attachment of the framebuffer
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}

	std::cout << "Framebuffer created successfully." << std::endl;

	// Unbind the framebuffer / back to default framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RendererFramebuffers::SetUniforms()
{
}

void RendererFramebuffers::SetShaders()
{
}

void RendererFramebuffers::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RenderPass(mainWindow, scene, projectionMatrix);
}

void RendererFramebuffers::RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());

	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Render code here

	std::string passType = "main";
	scene->Render(projectionMatrix, passType, shaders, uniforms);
}

RendererFramebuffers::~RendererFramebuffers()
{
}
