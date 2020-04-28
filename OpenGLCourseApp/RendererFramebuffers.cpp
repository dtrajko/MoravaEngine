#include "RendererFramebuffers.h"

#include "SceneNanosuit.h"


RendererFramebuffers::RendererFramebuffers()
{
}

void RendererFramebuffers::Init(Scene* scene)
{
	SetUniforms();
	SetShaders();

	// set up vertex data (and buffer(s)) and configure vertex attributes
	float cubeVertices[] =
	{
		 // positions           // texture Coords
		-0.5f, -0.5f, -0.5f,    0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,    1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,    1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,    1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,    0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,    0.0f, 0.0f,

		-0.5f, -0.5f,  0.5f,    0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,    1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,    1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,    1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,    0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,    0.0f, 0.0f,

		-0.5f,  0.5f,  0.5f,    1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,    1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,    0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,    0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,    0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,    1.0f, 0.0f,

		 0.5f,  0.5f,  0.5f,    1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,    1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,    0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,    0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,    0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,    1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,    0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,    1.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,    1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,    1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,    0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,    0.0f, 1.0f,

		-0.5f,  0.5f, -0.5f,    0.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,    1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,    1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,    1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,    0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,    0.0f, 1.0f
	};

	float planeVertices[] =
	{
		 // positions           // texture Coords 
		 5.0f, -0.5f,  5.0f,    2.0f, 0.0f,
		-5.0f, -0.5f,  5.0f,    0.0f, 0.0f,
		-5.0f, -0.5f, -5.0f,    0.0f, 2.0f,

		 5.0f, -0.5f,  5.0f,    2.0f, 0.0f,
		-5.0f, -0.5f, -5.0f,    0.0f, 2.0f,
		 5.0f, -0.5f, -5.0f,    2.0f, 2.0f
	};

	// vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates
	float quadVertices[] =
	{
		// positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f, 1.0f
	};

	// cube VAO
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);
	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	// plane VAO
	glGenVertexArrays(1, &planeVAO);
	glGenBuffers(1, &planeVBO);
	glBindVertexArray(planeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), &planeVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	// screen quad VAO
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	// shader configuration
	shaders["framebuffers_scene"]->Bind();
	shaders["framebuffers_scene"]->setInt("texture1", 0);

	shaders["framebuffers_screen"]->Bind();
	shaders["framebuffers_screen"]->setInt("screenTexture", 0);

	// Framebuffer configuration

	// create a framebuffer (FBO)
	// glGenFramebuffers(1, &m_FBO);
	m_Framebuffer = new Framebuffer();

	// bind the framebuffer
	m_Framebuffer->Bind();
	// glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

	// generate a texture
	// glGenTextures(1, &textureColorbuffer);
	m_Framebuffer->CreateTextureAttachmentColor(SCR_WIDTH, SCR_HEIGHT);

	// bind the texture
	// glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
	// m_Framebuffer->GetTextureAttachmentColor(0)->Bind();

	// Specify the target texture
	// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// unbind the texture
	// glBindTexture(GL_TEXTURE_2D, 0);
	// m_Framebuffer->GetTextureAttachmentColor(0)->Unbind();

	// attach the texture to currently bound framebuffer object
	// glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);

	// create a renderbuffer object (RBO) to be used as a depth and stencil attachment to the framebuffer
	// (we won't be sampling these)
	m_Framebuffer->CreateBufferAttachmentDepthAndStencil(SCR_WIDTH, SCR_HEIGHT);

	// glGenRenderbuffers(1, &m_RBO);
	// glBindRenderbuffer(GL_RENDERBUFFER, m_RBO);
	// glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);

	// Unbind the renderbuffer, once we've allocated enough memory for the renderbuffer object
	// glBindRenderbuffer(GL_RENDERBUFFER, 0);

	// Attach the renderbuffer object to the depth and stencil attachment of the framebuffer
	// glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_RBO);

	// if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	if (m_Framebuffer->CheckStatus())
	{
		std::cout << "Framebuffer created successfully." << std::endl;
	}
	else
	{
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}

	// Unbind the framebuffer / back to default framebuffer
	// glBindFramebuffer(GL_FRAMEBUFFER, 0);
	m_Framebuffer->Unbind();
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
}

void RendererFramebuffers::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RenderPass(mainWindow, scene, projectionMatrix);
}

void RendererFramebuffers::RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());

	// draw as wireframe
	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// First render pass
	// glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	m_Framebuffer->Bind();
	glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

	// make sure we clear the framebuffer's content
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // // we're not using the stencil buffer now
	
	// BEGIN to render scene
	shaders["framebuffers_scene"]->Bind();

	glm::mat4 model = glm::mat4(1.0f);
	shaders["framebuffers_scene"]->setMat4("view", scene->GetCamera()->CalculateViewMatrix());
	shaders["framebuffers_scene"]->setMat4("projection", projectionMatrix);

	// cubes
	glBindVertexArray(cubeVAO);
	glActiveTexture(GL_TEXTURE0);

	scene->GetTextures()["cube_wood"]->Bind(0);

	model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
	shaders["framebuffers_scene"]->setMat4("model", model);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
	shaders["framebuffers_scene"]->setMat4("model", model);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	// floor
	glBindVertexArray(planeVAO);

	scene->GetTextures()["floor_metal"]->Bind(0);

	shaders["framebuffers_scene"]->setMat4("model", glm::mat4(1.0f));
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	// END to render scene

	// Second Pass

	// now bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
	// glBindFramebuffer(GL_FRAMEBUFFER, 0);
	m_Framebuffer->Unbind();
	glDisable(GL_DEPTH_TEST);

	// clear all relevant buffers
	// set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	shaders["framebuffers_screen"]->Bind();
	glBindVertexArray(quadVAO);

	// use the color attachment texture as the texture of the quad plane
	// glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
	m_Framebuffer->GetTextureAttachmentColor(0)->Bind();
	glDrawArrays(GL_TRIANGLES, 0, 6);

	std::string passType = "main";
	scene->Render(projectionMatrix, passType, shaders, uniforms);
}

RendererFramebuffers::~RendererFramebuffers()
{
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &planeVAO);
	glDeleteVertexArrays(1, &quadVAO);
	glDeleteBuffers(1, &cubeVBO);
	glDeleteBuffers(1, &planeVBO);
	glDeleteBuffers(1, &quadVBO);

	// OOP
	delete m_Framebuffer;

	// glDeleteTextures(1, &textureColorbuffer);
	// glDeleteRenderbuffers(1, &m_RBO);
	// glDeleteFramebuffers(1, &m_FBO);
}
