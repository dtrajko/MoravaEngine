#include "SceneFramebuffers.h"

#include "ImGuiWrapper.h"


SceneFramebuffers::SceneFramebuffers()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 1.0f, 5.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;
	sceneSettings.ambientIntensity = 0.4f;
	sceneSettings.diffuseIntensity = 0.8f;
	sceneSettings.lightDirection = glm::vec3(3.0f, -9.0f, -3.0f);

	m_EffectFrame_0 = 0; // default colors (diffuse)
	m_EffectFrame_1 = 1; // inverse colors
	m_EffectFrame_2 = 3; // nightvision
	m_EffectFrame_3 = 4; // kernel sharpen

	SetCamera();
	SetSkybox();
	SetTextures();
	SetupMeshes();
	SetupModels();
	SetGeometry();
}

void SceneFramebuffers::SetSkybox()
{
}

void SceneFramebuffers::SetTextures()
{
	textures.insert(std::make_pair("floor_metal", new Texture("Textures/metal.png")));
	textures.insert(std::make_pair("cube_marble", new Texture("Textures/marble.jpg")));
	textures.insert(std::make_pair("cube_wood", new Texture("Textures/container/container2.png")));	
}

void SceneFramebuffers::SetupMeshes()
{
}

void SceneFramebuffers::SetupModels()
{
}

void SceneFramebuffers::SetGeometry()
{
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
		 1.0f,  1.0f,  1.0f, 1.0f,
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
}

void SceneFramebuffers::Update(float timestep, Window& mainWindow)
{
}

void SceneFramebuffers::UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults)
{
	ImGui::Begin("Effects");
	ImGui::SliderInt("Effect Frame 0", &m_EffectFrame_0, 0, 5);
	ImGui::SliderInt("Effect Frame 1", &m_EffectFrame_1, 0, 5);
	ImGui::SliderInt("Effect Frame 2", &m_EffectFrame_2, 0, 5);
	ImGui::SliderInt("Effect Frame 3", &m_EffectFrame_3, 0, 5);
	ImGui::End();
}

void SceneFramebuffers::Render(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
	shaders["framebuffers_scene"]->Bind();

	glm::mat4 model = glm::mat4(1.0f);
	shaders["framebuffers_scene"]->setMat4("view", m_Camera->CalculateViewMatrix());
	shaders["framebuffers_scene"]->setMat4("projection", projectionMatrix);

	// -- cubes
	glBindVertexArray(cubeVAO);
	glActiveTexture(GL_TEXTURE0);

	textures["cube_wood"]->Bind(0);

	model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
	shaders["framebuffers_scene"]->setMat4("model", model);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
	shaders["framebuffers_scene"]->setMat4("model", model);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	// -- floor
	glBindVertexArray(planeVAO);

	textures["floor_metal"]->Bind(0);

	shaders["framebuffers_scene"]->setMat4("model", glm::mat4(1.0f));
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void SceneFramebuffers::CleanupGeometry()
{
	// Geometry cleanup
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteBuffers(1, &cubeVBO);
	glDeleteVertexArrays(1, &planeVAO);
	glDeleteBuffers(1, &planeVBO);
	glDeleteVertexArrays(1, &quadVAO);
	glDeleteBuffers(1, &quadVBO);
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
}
