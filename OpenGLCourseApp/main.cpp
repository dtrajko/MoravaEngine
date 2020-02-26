#define STB_IMAGE_IMPLEMENTATION

#include <stdio.h>
#include <string.h>
#include <cmath>
#include <vector>
#include <map>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "CommonValues.h"
#include "Window.h"
#include "Mesh.h"
#include "Shader.h"
#include "Camera.h"
#include "Texture.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "Material.h"
#include "Model.h"
#include "Vertex.h"
#include "Skybox.h"
#include "SceneCottage.h"
#include "SceneEiffel.h"
#include "SceneSponza.h"
#include "MeshData.h"


// Window dimensions
const GLint WIDTH = 1280;
const GLint HEIGHT = 720;
const float toRadians = 3.14159265f / 180.0f;

Window mainWindow;

Scene* scene;

Camera camera;

std::string currentScene = "sponza"; // "cottage", "sponza", "eiffel"

std::map<std::string, Mesh*> meshes;
std::map<std::string, Shader*> shaders;
std::map<std::string, GLint> uniforms;
std::map<std::string, Texture*> textures;
std::map<std::string, GLuint> textureSlots;
std::map<std::string, Material*> materials;
std::map<std::string, Model*> models;

DirectionalLight mainLight;
PointLight pointLights[MAX_POINT_LIGHTS];
SpotLight spotLights[MAX_SPOT_LIGHTS];

unsigned int pointLightCount = 0;
unsigned int spotLightCount = 0;

GLfloat deltaTime = 0.0f;
GLfloat lastTime = 0.0f;

void SetUniforms(std::map<std::string, GLint>& uniforms)
{
	uniforms.insert(std::make_pair("model", 0));
	uniforms.insert(std::make_pair("view", 0));
	uniforms.insert(std::make_pair("projection", 0));
	uniforms.insert(std::make_pair("eyePosition", 0));
	uniforms.insert(std::make_pair("specularIntensity", 0));
	uniforms.insert(std::make_pair("shininess", 0));
	uniforms.insert(std::make_pair("directionalLightTransform", 0));
	uniforms.insert(std::make_pair("omniLightPos", 0));
	uniforms.insert(std::make_pair("farPlane", 0));
}

void SetTextures(std::map<std::string, Texture*>& textures, std::map<std::string, GLuint>& textureSlots)
{
	// cottage
	textures.insert(std::make_pair("pyramid", new Texture("Textures/pyramid.png")));
	textures.insert(std::make_pair("brick", new Texture("Textures/brick.png")));
	textures.insert(std::make_pair("crateDiffuse", new Texture("Textures/crate.png")));
	textures.insert(std::make_pair("crateNormal", new Texture("Textures/crateNormal.png")));
	textures.insert(std::make_pair("grass", new Texture("Textures/grass.jpg")));
	textures.insert(std::make_pair("normalMapDefault", new Texture("Textures/normal_map_default.png")));
	// sponza
	textures.insert(std::make_pair("sponzaFloorDiffuse", new Texture("Textures/sponza_floor_a_diff.tga")));
	textures.insert(std::make_pair("sponzaFloorNormal", new Texture("Textures/sponza_floor_a_ddn.tga")));
	textures.insert(std::make_pair("sponzaWallDiffuse", new Texture("Textures/sponza_bricks_a_diff.tga")));
	textures.insert(std::make_pair("sponzaWallNormal", new Texture("Textures/sponza_bricks_a_ddn.tga")));
	textures.insert(std::make_pair("sponzaCeilDiffuse", new Texture("Textures/sponza_ceiling_a_diff.tga")));
	textures.insert(std::make_pair("sponzaCeilNormal", new Texture("Textures/sponza_ceiling_a_ddn.tga")));

	textures["pyramid"]->LoadTexture();
	textures["brick"]->LoadTexture();
	textures["crateDiffuse"]->LoadTexture();
	textures["crateNormal"]->LoadTexture();
	textures["grass"]->LoadTexture();
	textures["normalMapDefault"]->LoadTexture();
	textures["sponzaFloorDiffuse"]->LoadTexture();
	textures["sponzaFloorNormal"]->LoadTexture();
	textures["sponzaWallDiffuse"]->LoadTexture();
	textures["sponzaWallNormal"]->LoadTexture();
	textures["sponzaCeilDiffuse"]->LoadTexture();
	textures["sponzaCeilNormal"]->LoadTexture();

	textureSlots.insert(std::make_pair("diffuse", 1));
	textureSlots.insert(std::make_pair("normal", 2));
	textureSlots.insert(std::make_pair("shadow", 3));
	textureSlots.insert(std::make_pair("omniShadow", 4));
}

void CreateObjects()
{
	Mesh::CalcAverageNormals(MeshData::indices, MeshData::indexCount, MeshData::vertices, MeshData::vertexCount);
	Mesh::CalcTangentSpace(MeshData::indices, MeshData::indexCount, MeshData::vertices, MeshData::vertexCount);

	Mesh::CalcAverageNormals(MeshData::quadIndices, MeshData::quadIndexCount, MeshData::quadVertices, MeshData::quadVertexCount);
	Mesh::CalcTangentSpace(MeshData::quadIndices, MeshData::quadIndexCount, MeshData::quadVertices, MeshData::quadVertexCount);

	Mesh* cube = new Mesh();
	cube->CreateMesh(MeshData::vertices, MeshData::indices, MeshData::vertexCount, MeshData::indexCount);
	meshes.insert(std::make_pair("cube", cube));

	Mesh* quad = new Mesh();
	quad->CreateMesh(MeshData::quadVertices, MeshData::quadIndices, MeshData::quadVertexCount, MeshData::quadIndexCount);
	meshes.insert(std::make_pair("quad", quad));

	Mesh* quadLarge = new Mesh();
	quadLarge->CreateMesh(MeshData::floorVertices, MeshData::floorIndices, MeshData::floorVertexCount, MeshData::floorIndexCount);
	meshes.insert(std::make_pair("quadLarge", quadLarge));
}

void CreateShaders()
{
	static const char* vertShader = "Shaders/shader.vert";
	static const char* fragShader = "Shaders/shader.frag";

	static const char* vertShaderDirShadowMap = "Shaders/directional_shadow_map.vert";
	static const char* fragShaderDirShadowMap = "Shaders/directional_shadow_map.frag";

	static const char* vertShaderOmniShadowMap = "Shaders/omni_shadow_map.vert";
	static const char* geomShaderOmniShadowMap = "Shaders/omni_shadow_map.geom";
	static const char* fragShaderOmniShadowMap = "Shaders/omni_shadow_map.frag";

	Shader* mainShader = new Shader();
	mainShader->CreateFromFiles(vertShader, fragShader);
	shaders.insert(std::make_pair("main", mainShader));

	Shader* directionalShadowShader = new Shader();
	directionalShadowShader->CreateFromFiles(vertShaderDirShadowMap, fragShaderDirShadowMap);
	shaders.insert(std::make_pair("directionalShadow", directionalShadowShader));

	Shader* omniShadowShader = new Shader();
	omniShadowShader->CreateFromFiles(vertShaderOmniShadowMap, geomShaderOmniShadowMap, fragShaderOmniShadowMap);
	shaders.insert(std::make_pair("omniShadow", omniShadowShader));
}

void UpdateSceneCottage(float now)
{
	glm::vec3 pLightPos = SceneCottage::GetSettings().pLight_0_position;
	float lightRadius = 6.0;
	float lightAngle = now * SceneCottage::GetSettings().shadowSpeed;
	pLightPos.x += (float)cos(lightAngle) * lightRadius;
	pLightPos.z += (float)sin(lightAngle) * lightRadius;
	pLightPos.y += (float)cos(lightAngle * 0.5) * lightRadius * 0.5f;
	pointLights[0].SetPosition(pLightPos);
}

void UpdateSceneEiffel(float now)
{
	// Shadow rotation
	glm::vec3 lightDirection = SceneEiffel::GetSettings().lightDirection;
	float lightRadius = abs(lightDirection.x);
	float lightAngle = now * SceneEiffel::GetSettings().shadowSpeed;
	lightDirection.x = (float)cos(lightAngle) * lightRadius;
	lightDirection.z = (float)sin(lightAngle) * lightRadius;
	mainLight.SetDirection(lightDirection);
}

void UpdateSceneSponza(float now)
{
	// Shadow rotation
	glm::vec3 lightDirection = SceneSponza::GetSettings().lightDirection;
	float lightRadius = abs(lightDirection.x);
	float lightAngle = now * SceneSponza::GetSettings().shadowSpeed;
	lightDirection.x = (float)cos(lightAngle) * lightRadius;
	lightDirection.z = (float)sin(lightAngle) * lightRadius;
	mainLight.SetDirection(lightDirection);
}

void UpdateScene(float now)
{
	if (currentScene == "cottage")
	{
		UpdateSceneCottage(now);
	}

	if (currentScene == "eiffel")
	{
		UpdateSceneEiffel(now);
	}

	if (currentScene == "sponza")
	{
		UpdateSceneSponza(now);
	}
}

void RenderScene(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, bool shadowPass = false)
{
	scene->Render(viewMatrix, projectionMatrix, shadowPass, shaders, uniforms, textures, textureSlots, meshes, materials, models);
}

void DirectionalShadowMapPass(DirectionalLight* light, glm::mat4 viewMatrix, glm::mat4 projectionMatrix)
{
	shaders["directionalShadow"]->Bind();

	glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

	light->GetShadowMap()->Write();

	glClear(GL_DEPTH_BUFFER_BIT);

	uniforms["model"] = shaders["directionalShadow"]->GetModelLocation();
	shaders["directionalShadow"]->SetDirectionalLightTransform(&light->CalculateLightTransform());

	shaders["directionalShadow"]->Validate();

	bool shadowPass = true;
	RenderScene(viewMatrix, projectionMatrix, shadowPass);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OmniShadowMapPass(PointLight* light, glm::mat4 viewMatrix, glm::mat4 projectionMatrix)
{
	shaders["omniShadow"]->Bind();

	glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

	light->GetShadowMap()->Write();

	glClear(GL_DEPTH_BUFFER_BIT);

	uniforms["model"] = shaders["omniShadow"]->GetModelLocation();
	uniforms["omniLightPos"] = shaders["omniShadow"]->GetUniformLocationOmniLightPos();
	uniforms["farPlane"] = shaders["omniShadow"]->GetUniformLocationFarPlane();

	glUniform3f(uniforms["omniLightPos"] , light->GetPosition().x, light->GetPosition().y, light->GetPosition().z);
	glUniform1f(uniforms["farPlane"] , light->GetFarPlane());

	shaders["omniShadow"]->SetLightMatrices(light->CalculateLightTransform());

	shaders["omniShadow"]->Validate();

	bool shadowPass = true;
	RenderScene(viewMatrix, projectionMatrix, shadowPass);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPass(glm::mat4 viewMatrix, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());

	// Clear the window
	glClearColor(135.0f / 255.0f, 206.0f / 255.0f, 235.0f / 255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 modelMatrix = glm::mat4(1.0f);

	if (currentScene == "eiffel")
	{
		glm::vec3 dLightDir = mainLight.GetDirection();
		float angleRadians = atan2(dLightDir.x, dLightDir.z) + 0.6f;
		modelMatrix = glm::rotate(modelMatrix, angleRadians, glm::vec3(0.0f, 1.0f, 0.0f));
	}

	scene->GetSkybox()->Draw(modelMatrix, viewMatrix, projectionMatrix);

	shaders["main"]->Bind();

	uniforms["model"] = shaders["main"]->GetModelLocation();
	uniforms["projection"] = shaders["main"]->GetProjectionLocation();
	uniforms["view"] = shaders["main"]->GetViewLocation();
	uniforms["eyePosition"] = shaders["main"]->GetUniformLocationEyePosition();
	uniforms["specularIntensity"] = shaders["main"]->GetUniformLocationSpecularIntensity();
	uniforms["shininess"] = shaders["main"]->GetUniformLocationShininess();

	glUniformMatrix4fv(uniforms["view"], 1, GL_FALSE, glm::value_ptr(viewMatrix));
	glUniformMatrix4fv(uniforms["projection"], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniform3f(uniforms["eyePosition"], camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);

	shaders["main"]->SetDirectionalLight(&mainLight);
	shaders["main"]->SetPointLights(pointLights, pointLightCount, textureSlots["omniShadow"], 0);
	shaders["main"]->SetSpotLights(spotLights, spotLightCount, textureSlots["omniShadow"], pointLightCount);
	shaders["main"]->SetDirectionalLightTransform(&mainLight.CalculateLightTransform());

	mainLight.GetShadowMap()->Read(textureSlots["shadow"]);
	shaders["main"]->SetTexture(textureSlots["diffuse"]);
	shaders["main"]->SetNormalMap(textureSlots["normal"]);
	shaders["main"]->SetDirectionalShadowMap(textureSlots["shadow"]);

	glm::vec3 lowerLight = camera.getCameraPosition();
	lowerLight.y -= 0.2f;
	spotLights[2].SetFlash(lowerLight, camera.getCameraDirection());

	shaders["main"]->Validate();

	RenderScene(viewMatrix, projectionMatrix);
}

void setupLight()
{
	mainLight = DirectionalLight(scene->GetSettings().shadowMapWidth, scene->GetSettings().shadowMapHeight, { 1.0f, 1.0f, 1.0f },
		scene->GetSettings().ambientIntensity, scene->GetSettings().diffuseIntensity, scene->GetSettings().lightDirection);
	mainLight.SetLightProjection(scene->GetSettings().lightProjectionMatrix);

	pointLights[0] = PointLight(1024, 1024, 0.01f, 100.0f, 
		scene->GetSettings().pLight_0_color, 0.4f, 
		scene->GetSettings().pLight_0_diffuseIntensity, 
		scene->GetSettings().pLight_0_position, 0.3f, 0.2f, 0.1f);
	pointLightCount++;
	pointLights[1] = PointLight(1024, 1024, 0.01f, 100.0f, 
		scene->GetSettings().pLight_1_color, 0.4f,
		scene->GetSettings().pLight_1_diffuseIntensity,
		scene->GetSettings().pLight_1_position, 0.3f, 0.2f, 0.1f);
	pointLightCount++;
	pointLights[2] = PointLight(1024, 1024, 0.01f, 100.0f,
		scene->GetSettings().pLight_2_color, 0.4f,
		scene->GetSettings().pLight_2_diffuseIntensity,
		scene->GetSettings().pLight_2_position, 0.3f, 0.2f, 0.1f);
	pointLightCount++;

	spotLights[0] = SpotLight(1024, 1024, 0.01f, 100.0f, { 1.0f, 1.0f, 1.0f }, 1.0f, 10.0f, { 0.0f, 20.0f, -28.0f }, { 0.0f, 0.0f, -1.0f }, 0.3f, 0.2f, 0.1f, 160.0f);
	spotLightCount++;
	spotLights[1] = SpotLight(1024, 1024, 0.01f, 100.0f, { 0.8f, 0.8f, 1.0f }, 0.3f, 6.0f, { 8.0f, 2.0f, 0.0f }, { -0.25f, 0.5f, -0.5f }, 0.3f, 0.2f, 0.1f, 45.0f);
	spotLightCount++;
	spotLights[2] = SpotLight(1024, 1024, 0.01f, 100.0f, { 1.0f, 1.0f, 1.0f }, 0.4f, 1.6f, glm::vec3(), glm::vec3(), 0.4f, 0.3f, 0.2f, 35.0f);
	spotLightCount++;
}

void Cleanup()
{
	for (auto& mesh : meshes)
		delete mesh.second;

	for (auto& shader : shaders)
		delete shader.second;

	for (auto& texture : textures)
		delete texture.second;

	for (auto& material : materials)
		delete material.second;

	for (auto& model : models)
		delete model.second;

	meshes.clear();
	shaders.clear();
	textures.clear();
	materials.clear();
	models.clear();

	uniforms.clear();	
	textureSlots.clear();

	delete scene;
}

int main()
{
	mainWindow = Window(WIDTH, HEIGHT);
	mainWindow.Initialize();

	printf("OpenGL Info:\n");
	printf("   Vendor: %s\n", glGetString(GL_VENDOR));
	printf("   Renderer: %s\n", glGetString(GL_RENDERER));
	printf("   Version: %s\n", glGetString(GL_VERSION));

	if (currentScene == "cottage")
	{
		scene = new SceneCottage();
	}
	else if (currentScene == "eiffel")
	{
		scene = new SceneEiffel();
	}
	else if (currentScene == "sponza")
	{
		scene = new SceneSponza();
	}

	CreateObjects();
	CreateShaders();

	camera = Camera(scene->GetSettings().cameraPosition, glm::vec3(0.0f, 1.0f, 0.0f), scene->GetSettings().cameraStartYaw, 0.0f, 4.0f, 0.1f);

	// Projection matrix
	glm::mat4 projection = glm::perspective(glm::radians(60.0f), mainWindow.GetBufferWidth() / mainWindow.GetBufferHeight(), 0.1f, 200.0f);

	setupLight();

	SetUniforms(uniforms);
	SetTextures(textures, textureSlots);

	materials.insert(std::make_pair("shiny", new Material(1.0f, 128.0f)));
	materials.insert(std::make_pair("dull", new Material(1.0f, 64.0f)));
	materials.insert(std::make_pair("superShiny", new Material(1.0f, 1024.0f)));

	if (currentScene == "sponza")
	{
		Model* sponza = new Model();
		sponza->LoadModel("Models/sponza.obj");
		models.insert(std::make_pair("sponza", sponza));
	}

	if (currentScene == "cottage")
	{
		Model* cottage = new Model();
		cottage->LoadModel("Models/cottage.obj");
		models.insert(std::make_pair("cottage", cottage));
	}

	if (currentScene == "eiffel")
	{
		Model* eiffel = new Model();
		eiffel->LoadModel("Models/Eiffel_Tower.obj");
		models.insert(std::make_pair("eiffel", eiffel));

		Model* watchtower = new Model();
		watchtower->LoadModel("Models/wooden_watch_tower.obj");
		models.insert(std::make_pair("watchtower", watchtower));
	}

	// Loop until window closed
	while (!mainWindow.GetShouldClose())
	{
		GLfloat now = (GLfloat)glfwGetTime();
		deltaTime = now - lastTime;
		lastTime = now;

		// Get and handle user input events
		glfwPollEvents();

		camera.keyControl(mainWindow.getKeys(), deltaTime);
		camera.mouseControl(mainWindow.getMouseButtons(), mainWindow.getXChange(), mainWindow.getYChange());
		// camera.mouseScrollControl(mainWindow.getKeys(), deltaTime, mainWindow.getXMouseScrollOffset(), mainWindow.getYMouseScrollOffset());

		if (mainWindow.getKeys()[GLFW_KEY_L])
		{
			spotLights[2].Toggle();
			mainWindow.getKeys()[GLFW_KEY_L] = false;
		}

		UpdateScene(now);

		DirectionalShadowMapPass(&mainLight, camera.CalculateViewMatrix(), projection);

		for (size_t i = 0; i < pointLightCount; i++)
		{
			OmniShadowMapPass(&pointLights[i], camera.CalculateViewMatrix(), projection);
		}

		for (size_t i = 0; i < spotLightCount; i++)
		{
			OmniShadowMapPass((PointLight*)&spotLights[i], camera.CalculateViewMatrix(), projection);
		}

		RenderPass(camera.CalculateViewMatrix(), projection);

		shaders["main"]->Unbind();

		mainWindow.SwapBuffers();
	}

	Cleanup();
	return 0;
}
