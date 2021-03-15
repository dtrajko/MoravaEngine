#include "Scene/SceneFramebuffers.h"

#include "Mesh/GeometryFactory.h"


SceneFramebuffers::SceneFramebuffers()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 1.0f, 5.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;
	sceneSettings.directionalLight.base.ambientIntensity = 0.4f;
	sceneSettings.directionalLight.base.diffuseIntensity = 0.8f;
	sceneSettings.directionalLight.direction = glm::vec3(3.0f, -9.0f, -3.0f);

	m_EffectFrame_0 = 0; // default colors (diffuse)
	m_EffectFrame_1 = 1; // inverse colors
	m_EffectFrame_2 = 3; // nightvision
	m_EffectFrame_3 = 4; // kernel sharpen

	SetCamera();
	SetupTextures();
	SetupShaders();
	SetupFramebuffers();
	SetupMeshes();
	SetupModels();
	SetGeometry();
}

void SceneFramebuffers::SetupTextures()
{
	textures.insert(std::make_pair("floor_metal", TextureLoader::Get()->GetTexture("Textures/metal.png", false, false)));
	textures.insert(std::make_pair("cube_marble", TextureLoader::Get()->GetTexture("Textures/marble.jpg", false, false)));
	textures.insert(std::make_pair("cube_wood",   TextureLoader::Get()->GetTexture("Textures/container/container2.png", false, false)));
}

void SceneFramebuffers::SetupShaders()
{
	m_ShaderFramebuffersScene = new Shader("Shaders/framebuffers_scene.vs", "Shaders/framebuffers_scene.fs");
	printf("SceneFramebuffers: m_ShaderFramebuffersScene compiled [programID=%d]\n", m_ShaderFramebuffersScene->GetProgramID());

	m_ShaderFramebuffersScreen = new Shader("Shaders/framebuffers_screen.vs", "Shaders/framebuffers_screen.fs");
	printf("SceneFramebuffers: m_ShaderFramebuffersScreen compiled [programID=%d]\n", m_ShaderFramebuffersScreen->GetProgramID());

	// shader configuration
	m_ShaderFramebuffersScene->Bind();
	m_ShaderFramebuffersScene->setInt("texture1", 0);

	m_ShaderFramebuffersScreen->Bind();
	m_ShaderFramebuffersScreen->setInt("screenTexture", 0);
}

void SceneFramebuffers::SetupFramebuffers()
{
	// Framebuffer configuration
	// -- create a framebuffer (FBO)
	m_Framebuffer = new Framebuffer(SCR_WIDTH, SCR_HEIGHT);

	// -- generate a color texture attachment
	m_Framebuffer->AddColorAttachmentSpecification(SCR_WIDTH, SCR_HEIGHT, AttachmentType::Texture, AttachmentFormat::Color);

	// -- create a renderbuffer object (RBO) to be used as a depth and stencil attachment to the framebuffer
	//  (we won't be sampling these)
	m_Framebuffer->AddDepthAttachmentSpecification(SCR_WIDTH, SCR_HEIGHT, AttachmentType::Renderbuffer, AttachmentFormat::DepthStencil);

	m_Framebuffer->Generate(SCR_WIDTH, SCR_HEIGHT);

	if (!m_Framebuffer->CheckStatus())
		throw std::runtime_error("ERROR: Framebuffer is not complete!");

	std::cout << "Framebuffer created successfully." << std::endl;

	// -- Unbind the framebuffer / back to default framebuffer
	m_Framebuffer->Unbind(SCR_WIDTH, SCR_HEIGHT);
}

void SceneFramebuffers::SetupMeshes()
{
}

void SceneFramebuffers::SetupModels()
{
}

void SceneFramebuffers::SetGeometry()
{
	GeometryFactory::CubeTexCoords::Create();
	GeometryFactory::Plane::Create();
	GeometryFactory::Quad::Create();
}

void SceneFramebuffers::Update(float timestep, Window* mainWindow)
{
	m_Camera->OnUpdate(timestep);
}

void SceneFramebuffers::UpdateImGui(float timestep, Window* mainWindow)
{
	bool p_open = true;
	ShowExampleAppDockSpace(&p_open, mainWindow);

	ImGui::Begin("Effects");
	{
		ImGui::SliderInt("Effect Frame 0", &m_EffectFrame_0, 0, 5);
		ImGui::SliderInt("Effect Frame 1", &m_EffectFrame_1, 0, 5);
		ImGui::SliderInt("Effect Frame 2", &m_EffectFrame_2, 0, 5);
		ImGui::SliderInt("Effect Frame 3", &m_EffectFrame_3, 0, 5);
	}
	ImGui::End();

	ImGui::Begin("Framebuffers");
	{
		ImVec2 imageSize(128.0f, 128.0f);

		ImGui::Text("Texture Attachment");
		ImGui::Image((void*)(intptr_t)m_Framebuffer->GetTextureAttachmentColor(0)->GetID(), imageSize);

		ImGui::Text("Depth and Stencil");
		ImGui::Image((void*)(intptr_t)m_Framebuffer->GetAttachmentDepthAndStencil()->GetID(), imageSize);	
	}
	ImGui::End();
}

void SceneFramebuffers::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
	{
		// -- BEGIN First Render Pass render target m_Framebuffer
		m_Framebuffer->Bind(SCR_WIDTH, SCR_HEIGHT);

		// Clear the window
		glm::vec4 bgColor = RendererBasic::s_BgColor;
		glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

		m_ShaderFramebuffersScene->Bind();

		glm::mat4 model = glm::mat4(1.0f);
		m_ShaderFramebuffersScene->setMat4("view", m_Camera->GetViewMatrix());
		m_ShaderFramebuffersScene->setMat4("projection", projectionMatrix);

		// -- cubes
		glBindVertexArray(GeometryFactory::CubeTexCoords::GetVAO());
		glActiveTexture(GL_TEXTURE0);

		textures["cube_wood"]->Bind(0);

		model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
		m_ShaderFramebuffersScene->setMat4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
		m_ShaderFramebuffersScene->setMat4("model", model);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		// -- floor
		glBindVertexArray(GeometryFactory::Plane::GetVAO());

		textures["floor_metal"]->Bind(0);

		m_ShaderFramebuffersScene->setMat4("model", glm::mat4(1.0f));
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}
	// -- END First Render Pass render target m_Framebuffer

	// -- BEGIN Second Render Pass render target default framebuffer
	{
		// -- now bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
		m_Framebuffer->Unbind((GLsizei)mainWindow->GetWidth(), (GLsizei)mainWindow->GetHeight());

		// -- clear all relevant buffers
		// -- set clear color to white (not really necessery actually, since we won't be able to see behind the quad anyways)
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glDisable(GL_DEPTH_TEST);

		m_ShaderFramebuffersScreen->Bind();
		glBindVertexArray(GeometryFactory::Quad::GetVAO());

		// -- use the color attachment texture as the texture of the quad plane
		m_Framebuffer->GetTextureAttachmentColor(0)->Bind();

		glm::mat4 model;

		model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, 0.5f, 0.0f));
		model = glm::scale(model, glm::vec3(0.5f));
		m_ShaderFramebuffersScreen->setMat4("model", model);
		m_ShaderFramebuffersScreen->setInt("effect", GetEffectForFrame(0)); // diffuse (default)
		glDrawArrays(GL_TRIANGLES, 0, 6);

		model = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.0f));
		model = glm::scale(model, glm::vec3(0.5f));
		m_ShaderFramebuffersScreen->setMat4("model", model);
		m_ShaderFramebuffersScreen->setInt("effect", GetEffectForFrame(1)); // inverse color
		glDrawArrays(GL_TRIANGLES, 0, 6);

		model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, -0.5f, 0.0f));
		model = glm::scale(model, glm::vec3(0.5f));
		m_ShaderFramebuffersScreen->setMat4("model", model);
		m_ShaderFramebuffersScreen->setInt("effect", GetEffectForFrame(2)); // nightvision
		glDrawArrays(GL_TRIANGLES, 0, 6);

		model = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, -0.5f, 0.0f));
		model = glm::scale(model, glm::vec3(0.5f));
		m_ShaderFramebuffersScreen->setMat4("model", model);
		m_ShaderFramebuffersScreen->setInt("effect", GetEffectForFrame(3)); // kernel sharp
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	// -- END Second Render Pass render target default framebuffer
}

void SceneFramebuffers::CleanupGeometry()
{
	// Geometry cleanup
	GeometryFactory::CubeTexCoords::Destroy();
	GeometryFactory::Plane::Destroy();
	GeometryFactory::Quad::Destroy();
}

int SceneFramebuffers::GetEffectForFrame(int frameID)
{
	switch (frameID)
	{
	case 0:
		return m_EffectFrame_0;
	case 1:
		return m_EffectFrame_1;
	case 2:
		return m_EffectFrame_2;
	case 3:
		return m_EffectFrame_3;
	default:
		return 0;
	}
}

SceneFramebuffers::~SceneFramebuffers()
{
	CleanupGeometry();

	delete m_Framebuffer;
}
