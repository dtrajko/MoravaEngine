#include "RendererFramebuffers.h"

#include "SceneFramebuffers.h"


RendererFramebuffers::RendererFramebuffers()
{
}

void RendererFramebuffers::Init(Scene* scene)
{
	SetUniforms();
	SetShaders();
	SetFramebuffers();
}

void RendererFramebuffers::SetUniforms()
{
}

void RendererFramebuffers::SetShaders()
{
	Shader* shaderFramebuffersScene = new Shader("Shaders/framebuffers_scene.vs", "Shaders/framebuffers_scene.fs");
	shaders.insert(std::make_pair("framebuffers_scene", shaderFramebuffersScene));
	printf("RendererFramebuffers: shaderFramebuffersScene compiled [programID=%d]\n", shaderFramebuffersScene->GetProgramID());

	Shader* shaderFramebuffersScreen = new Shader("Shaders/framebuffers_screen.vs", "Shaders/framebuffers_screen.fs");
	shaders.insert(std::make_pair("framebuffers_screen", shaderFramebuffersScreen));
	printf("RendererFramebuffers: shaderFramebuffersScreen compiled [programID=%d]\n", shaderFramebuffersScreen->GetProgramID());

	// shader configuration
	shaders["framebuffers_scene"]->Bind();
	shaders["framebuffers_scene"]->setInt("texture1", 0);

	shaders["framebuffers_screen"]->Bind();
	shaders["framebuffers_screen"]->setInt("screenTexture", 0);
}

void RendererFramebuffers::SetFramebuffers()
{
	// Framebuffer configuration
	// -- create a framebuffer (FBO)
	m_Framebuffer = new Framebuffer();

	// -- generate a color texture attachment
	m_Framebuffer->CreateTextureAttachmentColor(SCR_WIDTH, SCR_HEIGHT);

	// -- create a renderbuffer object (RBO) to be used as a depth and stencil attachment to the framebuffer
	//      (we won't be sampling these)
	m_Framebuffer->CreateBufferAttachmentDepthAndStencil(SCR_WIDTH, SCR_HEIGHT);

	if (!m_Framebuffer->CheckStatus())
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

	std::cout << "Framebuffer created successfully." << std::endl;

	// -- Unbind the framebuffer / back to default framebuffer
	m_Framebuffer->Unbind();
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

	{
		// First Render Pass
		m_Framebuffer->Bind();
		glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

		// -- make sure we clear the framebuffer's content
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		m_Framebuffer->Clear(); // we're not using the stencil buffer now
	}

	{
		std::string passType = "main";
		scene->Render(projectionMatrix, passType, shaders, uniforms);
	}

	{
		// Second Render Pass
		// -- now bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
		m_Framebuffer->Unbind();
		glDisable(GL_DEPTH_TEST);

		// -- clear all relevant buffers
		// -- set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		shaders["framebuffers_screen"]->Bind();
		SceneFramebuffers* sceneFramebuffers = (SceneFramebuffers*)scene;
		glBindVertexArray(sceneFramebuffers->GetQuadVAO());

		// -- use the color attachment texture as the texture of the quad plane
		m_Framebuffer->GetTextureAttachmentColor(0)->Bind();
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
}

RendererFramebuffers::~RendererFramebuffers()
{
	delete m_Framebuffer;
}
