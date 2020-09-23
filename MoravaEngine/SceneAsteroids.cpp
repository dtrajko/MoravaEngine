#include "SceneAsteroids.h"

#include "ImGuiWrapper.h"
#include "RendererBasic.h"


SceneAsteroids::SceneAsteroids()
{
	sceneSettings.enableSkybox = true;
	sceneSettings.cameraPosition = glm::vec3(0.0f, 30.0f, 220.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraStartPitch = -10.0f;
	sceneSettings.cameraMoveSpeed = 5.0f;
	sceneSettings.directionalLight.base.ambientIntensity = 0.4f;
	sceneSettings.directionalLight.base.diffuseIntensity = 0.8f;
	sceneSettings.directionalLight.direction = glm::vec3(3.0f, -9.0f, -3.0f);
	sceneSettings.nearPlane = 0.01f;
	sceneSettings.farPlane = 500.0f;

	SetupShaders();
	SetSkybox();
	SetupModels();
	SetupGeometry();

	SetWireframeEnabled(false);
}

void SceneAsteroids::SetupShaders()
{
	m_ShaderPlanet = new Shader("Shaders/asteroids_planet.vs", "Shaders/asteroids.fs");
	printf("SceneAsteroids: m_ShaderPlanet shader compiled [programID=%d]\n", m_ShaderPlanet->GetProgramID());

	m_ShaderAsteroids = new Shader("Shaders/asteroids.vs", "Shaders/asteroids.fs");
	printf("SceneAsteroids: m_ShaderAsteroids shader compiled [programID=%d]\n", m_ShaderAsteroids->GetProgramID());
}

void SceneAsteroids::SetupGeometry()
{
	modelMatrices = new glm::mat4[amount];
	srand((unsigned int)glfwGetTime()); // initialize random seed	
	float radius = 120.0;
	float offset = 25.0f;
	for (unsigned int i = 0; i < amount; i++)
	{
		glm::mat4 model = glm::mat4(1.0f);
		// 1. translation: displace along circle with 'radius' in range [-offset, offset]
		float angle = (float)i / (float)amount * 360.0f;
		float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float x = sin(angle) * radius + displacement;
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float y = displacement * 0.2f; // keep height of asteroid field smaller compared to width of x and z
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float z = cos(angle) * radius + displacement;
		model = glm::translate(model, glm::vec3(x, y, z));

		// 2. scale: Scale between 0.05 and 0.25f
		float scale = (float)(rand() % 20) / 100.0f + 0.05f;
		model = glm::scale(model, glm::vec3(scale));

		// 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
		float rotAngle = (float)(rand() % 360);
		model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

		// 4. now add to list of matrices
		modelMatrices[i] = model;
	}

	// configure instanced array
	// -------------------------
	unsigned int buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

	for (unsigned int i = 0; i < models["rock"]->GetMeshes().size(); i++)
	{
		unsigned int VAO = models["rock"]->GetMeshes()[i].GetVAO();
		glBindVertexArray(VAO);
		// set attribute pointers for matrix (4 times vec4)
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(0 * sizeof(glm::vec4)));
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(1 * sizeof(glm::vec4)));
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

		glVertexAttribDivisor(3, 1);
		glVertexAttribDivisor(4, 1);
		glVertexAttribDivisor(5, 1);
		glVertexAttribDivisor(6, 1);

		glBindVertexArray(0);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void SceneAsteroids::SetSkybox()
{
	skyboxFaces.push_back("Textures/skybox_1/right.jpg");
	skyboxFaces.push_back("Textures/skybox_1/left.jpg");
	skyboxFaces.push_back("Textures/skybox_1/top.jpg");
	skyboxFaces.push_back("Textures/skybox_1/bottom.jpg");
	skyboxFaces.push_back("Textures/skybox_1/back.jpg");
	skyboxFaces.push_back("Textures/skybox_1/front.jpg");

	m_Skybox = new Skybox(skyboxFaces);
}

void SceneAsteroids::SetupModels()
{
	ModelJoey* rock = new ModelJoey("Models/rock.obj", "Textures");
	ModelJoey* planet = new ModelJoey("Models/planet.obj", "Textures");
	models.insert(std::make_pair("rock", rock));
	models.insert(std::make_pair("planet", planet));
}

void SceneAsteroids::Update(float timestep, Window* mainWindow)
{
}

void SceneAsteroids::UpdateImGui(float timestep, Window* mainWindow)
{
	ImGui::Begin("Settings");
	ImGui::SliderFloat("FOV", &m_FOV, -60.0f, 180.0f);
	ImGui::End();
}

void SceneAsteroids::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
	// Override the Projection matrix (update FOV)
	projectionMatrix = glm::perspective(glm::radians(m_FOV),
		(float)mainWindow->GetWidth() / (float)mainWindow->GetHeight(),
		sceneSettings.nearPlane, sceneSettings.farPlane);

	RendererBasic::SetProjectionMatrix(projectionMatrix);

	float angleRadians = glm::radians((GLfloat)glfwGetTime());

	if (sceneSettings.enableSkybox)
	{
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::rotate(modelMatrix, angleRadians, glm::vec3(0.0f, 1.0f, 0.0f));
		GetSkybox()->Draw(modelMatrix, m_CameraController->CalculateViewMatrix(), projectionMatrix);
	}

	// configure transformation matrices
	m_ShaderPlanet->Bind();
	m_ShaderPlanet->setMat4("projection", projectionMatrix);
	m_ShaderPlanet->setMat4("view", m_CameraController->CalculateViewMatrix());

	// draw planet
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -20.0f, 0.0f));
	model = glm::rotate(model, angleRadians * 10.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(20.0f));
	m_ShaderPlanet->setMat4("model", model);
	models["planet"]->Draw(m_ShaderPlanet);

	// draw meteorites
	m_ShaderAsteroids->Bind();
	m_ShaderAsteroids->setMat4("projection", projectionMatrix);
	m_ShaderAsteroids->setMat4("view", m_CameraController->CalculateViewMatrix());
	m_ShaderAsteroids->setInt("texture_diffuse1", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, models["rock"]->GetTextures()[0].id); // note: we also made the textures_loaded vector public (instead of private) from the model class.
	for (unsigned int i = 0; i < models["rock"]->GetMeshes().size(); i++)
	{
		glBindVertexArray(models["rock"]->GetMeshes()[i].GetVAO());
		glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)models["rock"]->GetMeshes()[i].GetIndices().size(), GL_UNSIGNED_INT, 0, amount);
		glBindVertexArray(0);
	}
}

SceneAsteroids::~SceneAsteroids()
{
	for (auto& model : models)
		delete model.second;

	models.clear();

	glDeleteVertexArrays(1, &quadVAO);
	glDeleteBuffers(1, &quadVBO);
}
