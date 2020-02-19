#define STB_IMAGE_IMPLEMENTATION

#include <stdio.h>
#include <string.h>
#include <cmath>
#include <vector>

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


// Window dimensions
const GLint WIDTH = 1280;
const GLint HEIGHT = 720;
const float toRadians = 3.14159265f / 180.0f;

Window mainWindow;
std::vector <Mesh*> meshList;
std::vector <Shader*> shaderList;

Camera camera;

Texture brickTexture;
Texture pyramidTexture;
Texture sponzaFloorTexture;

Material shinyMaterial;
Material dullMaterial;
Material superShinyMaterial;

DirectionalLight mainLight;
PointLight pointLights[MAX_POINT_LIGHTS];
SpotLight spotLights[MAX_SPOT_LIGHTS];

GLfloat deltaTime = 0.0f;
GLfloat lastTime = 0.0f;

static const char* vShader = "Shaders/shader.vert";
static const char* fShader = "Shaders/shader.frag";


// The Phong shading approach
void calcAverageNormals(unsigned int* indices, unsigned int indiceCount,
	GLfloat* vertices, unsigned int verticeCount,
	unsigned int vLength, unsigned int normalOffset)
{
	for (size_t i = 0; i < indiceCount; i += 3)
	{
		unsigned int in0 = indices[i + 0] * vLength;
		unsigned int in1 = indices[i + 1] * vLength;
		unsigned int in2 = indices[i + 2] * vLength;
		glm::vec3 v1(vertices[in1 + 0] - vertices[in0 + 0], vertices[in1 + 1] - vertices[in0 + 1], vertices[in1 + 2] - vertices[in0 + 2]);
		glm::vec3 v2(vertices[in2 + 0] - vertices[in0 + 0], vertices[in2 + 1] - vertices[in0 + 1], vertices[in2 + 2] - vertices[in0 + 2]);
		glm::vec3 normal = glm::cross(v1, v2);
		normal = glm::normalize(normal);

		in0 += normalOffset;
		in1 += normalOffset;
		in2 += normalOffset;

		vertices[in0 + 0] += normal.x; vertices[in0 + 1] += normal.y; vertices[in0 + 2] += normal.z;
		vertices[in1 + 0] += normal.x; vertices[in1 + 1] += normal.y; vertices[in1 + 2] += normal.z;
		vertices[in2 + 0] += normal.x; vertices[in2 + 1] += normal.y; vertices[in2 + 2] += normal.z;
	}

	for (size_t i = 0; i < verticeCount / vLength; i++)
	{
		unsigned int nOffset = i * vLength + normalOffset;
		glm::vec3 vec(vertices[nOffset + 0], vertices[nOffset + 1], vertices[nOffset + 2]);
		vec = glm::normalize(vec);
		vertices[nOffset + 0] = vec.x; vertices[nOffset + 1] = vec.y; vertices[nOffset + 2] = vec.z;
	}
}

void CreateObjects()
{
	GLfloat vertices[] =
	{
		//  X      Y      Z       U     V       NX     NY     NZ
		-0.5f,  0.5f, -0.5f,   1.0f, 1.0f,   -0.5f,  0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,   1.0f, 0.0f,   -0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,   0.0f, 0.0f,    0.5f, -0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,   0.0f, 1.0f,    0.5f,  0.5f, -0.5f,

		-0.5f,  0.5f,  0.5f,   1.0f, 1.0f,   -0.5f,  0.5f,  0.5f,
		-0.5f, -0.5f,  0.5f,   1.0f, 0.0f,   -0.5f, -0.5f,  0.5f,
		 0.5f, -0.5f,  0.5f,   0.0f, 0.0f,    0.5f, -0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,   0.0f, 1.0f,    0.5f,  0.5f,  0.5f,

		 0.5f,  0.5f, -0.5f,   1.0f, 1.0f,    0.5f,  0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,   1.0f, 0.0f,    0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f,  0.5f,   0.0f, 0.0f,    0.5f, -0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,   0.0f, 1.0f,    0.5f,  0.5f,  0.5f,

		-0.5f,  0.5f, -0.5f,   1.0f, 1.0f,   -0.5f,  0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,   1.0f, 0.0f,   -0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f,  0.5f,   0.0f, 0.0f,   -0.5f, -0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,   0.0f, 1.0f,   -0.5f,  0.5f,  0.5f,

		-0.5f,  0.5f,  0.5f,   1.0f, 1.0f,   -0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f, -0.5f,   1.0f, 0.0f,   -0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,   0.0f, 0.0f,    0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f,  0.5f,   0.0f, 1.0f,    0.5f,  0.5f,  0.5f,

		-0.5f, -0.5f,  0.5f,   1.0f, 1.0f,   -0.5f, -0.5f,  0.5f,
		-0.5f, -0.5f, -0.5f,   1.0f, 0.0f,   -0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,   0.0f, 0.0f,    0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f,  0.5f,   0.0f, 1.0f,    0.5f, -0.5f,  0.5f,
	};

	unsigned int vertexCount = 8 * 4 * 6;

	printf("Size of vertices array: %.2d\n", vertexCount);

	unsigned int indices[] =
	{
		 0,  1,  3,
		 3,  1,  2,
		 4,  5,  7,
		 7,  5,  6,
		 8,  9, 11,
		11,  9, 10,
		12, 13, 15,
		15, 13, 14,
		16, 17, 19,
		19, 17, 18,
		20, 21, 23,
		23, 21, 22,
	};

	unsigned int indexCount = 12 * 3;

	calcAverageNormals(indices, indexCount, vertices, vertexCount, 8, 5);

	Mesh* obj1 = new Mesh();
	obj1->CreateMesh(vertices, indices, vertexCount, indexCount);
	meshList.push_back(obj1);

	Mesh* obj2 = new Mesh();
	obj2->CreateMesh(vertices, indices, vertexCount, indexCount);
	meshList.push_back(obj2);

	/* Floor Mesh */
	GLfloat floorVertices[] =
	{
		-10.0f, 0.0f, -10.0f,    0.0f,  0.0f,   0.0f, -1.0f, 0.0f,
		 10.0f, 0.0f, -10.0f,   10.0f,  0.0f,   0.0f, -1.0f, 0.0f,
		-10.0f, 0.0f,  10.0f,    0.0f, 10.0f,   0.0f, -1.0f, 0.0f,
		 10.0f, 0.0f,  10.0f,   10.0f, 10.0f,   0.0f, -1.0f, 0.0f,
	};

	unsigned int floorVertexCount = 32;

	unsigned int floorIndices[] =
	{
		0, 2, 1,
		1, 2, 3,
	};

	unsigned int floorIndexCount = 6;

	Mesh* floor = new Mesh();
	floor->CreateMesh(floorVertices, floorIndices, floorVertexCount, floorIndexCount);
	meshList.push_back(floor);

	Mesh* wallRight = new Mesh();
	wallRight->CreateMesh(floorVertices, floorIndices, floorVertexCount, floorIndexCount);
	meshList.push_back(wallRight);

	Mesh* wallBack = new Mesh();
	wallBack->CreateMesh(floorVertices, floorIndices, floorVertexCount, floorIndexCount);
	meshList.push_back(wallBack);

	Mesh* ceil = new Mesh();
	ceil->CreateMesh(floorVertices, floorIndices, floorVertexCount, floorIndexCount);
	meshList.push_back(ceil);
}

void CreateShaders()
{
	Shader* shader1 = new Shader();
	shader1->CreateFromFiles(vShader, fShader);
	shaderList.push_back(shader1);
}


int main()
{
	mainWindow = Window(WIDTH, HEIGHT);
	mainWindow.Initialize();

	CreateObjects();
	CreateShaders();

	camera = Camera(glm::vec3(0.0f, 2.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f, 4.0f, 0.1f);

	brickTexture = Texture("Textures/brick.png");
	brickTexture.LoadTexture();
	pyramidTexture = Texture("Textures/pyramid.png");
	pyramidTexture.LoadTexture();
	sponzaFloorTexture = Texture("Textures/sponza_floor.jpg");
	sponzaFloorTexture.LoadTexture();

	shinyMaterial = Material(1.0f, 128.0f);
	dullMaterial = Material(1.0f, 64.0f);
	superShinyMaterial = Material(1.0f, 256.0f);

	mainLight = DirectionalLight({ 1.0f, 1.0f, 1.0f }, 0.1f, 0.2f, { -0.5f, -0.3f, 0.5f });

	unsigned int pointLightCount = 0;
	pointLights[0] = PointLight({ 1.0f, 0.0f, 0.0f }, 0.1f, 1.0f, {  8.0f, 5.0f, -8.0f }, 0.3f, 0.2f, 0.1f);
	pointLightCount++;															 
	pointLights[1] = PointLight({ 0.0f, 1.0f, 0.0f }, 0.1f, 1.0f, { -2.0f, 2.0f, 2.0f }, 0.3f, 0.2f, 0.1f);
	pointLightCount++;															 
	pointLights[2] = PointLight({ 0.0f, 0.0f, 1.0f }, 0.1f, 1.0f, {  -6.0f, 6.0f, -8.0f }, 0.3f, 0.2f, 0.1f);
	pointLightCount++;

	unsigned int spotLightCount = 0;
	spotLights[0] = SpotLight({ 1.0f, 0.0f, 1.0f }, 0.1f, 1.0f, { 9.0f, 2.0f, 0.0f }, { 1.5f, 1.5f, 0.0f }, 0.3f, 0.2f, 0.1f, 10.0f);
	spotLightCount++;
	spotLights[1] = SpotLight({ 0.4f, 0.4f, 1.0f }, 0.1f, 2.0f, { 0.0f, 2.0f, -9.0f }, { 0.0f, 1.5f, -1.5f }, 0.3f, 0.2f, 0.1f, 10.0f);
	spotLightCount++;

	GLint uniformModel = 0;
	GLint uniformView = 0;
	GLint uniformProjection = 0;
	GLint uniformEyePosition = 0;
	GLint uniformSpecularIntensity = 0;
	GLint uniformShininess = 0;

	// Projection matrix
	glm::mat4 projection = glm::perspective(45.0f, mainWindow.GetBufferWidth() / mainWindow.GetBufferHeight(), 0.1f, 100.0f);

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

		// Clear the window
		glClearColor(0.2f, 0.0f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shaderList[0]->Bind();
		uniformModel             = shaderList[0]->GetModelLocation();
		uniformProjection        = shaderList[0]->GetProjectionLocation();
		uniformView              = shaderList[0]->GetViewLocation();
		uniformEyePosition       = shaderList[0]->GetUniformLocationEyePosition();
		uniformSpecularIntensity = shaderList[0]->GetUniformLocationSpecularIntensity();
		uniformShininess         = shaderList[0]->GetUniformLocationShininess();

		shaderList[0]->SetDirectionalLight(&mainLight);
		shaderList[0]->SetPointLights(pointLights, pointLightCount);
		shaderList[0]->SetSpotLights(spotLights, spotLightCount);

		glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(camera.CalculateViewMatrix()));
		glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniform3f(uniformEyePosition, camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);

		// Model matrix
		glm::mat4 model;

		/* Cube Left */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-2.0f, 2.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(2.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		brickTexture.UseTexture();
		shinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
		meshList[0]->RenderMesh();

		/* Cube Right */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.0f, 2.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(2.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		pyramidTexture.UseTexture();
		dullMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
		meshList[1]->RenderMesh();

		/* Floor */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		sponzaFloorTexture.UseTexture();
		superShinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
		meshList[2]->RenderMesh();

		/* Wall Right */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(10.0f, 10.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(1.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		brickTexture.UseTexture();
		shinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
		meshList[3]->RenderMesh();

		/* Wall Back */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 10.0f, -10.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(1.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		brickTexture.UseTexture();
		shinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
		meshList[4]->RenderMesh();

		/* Ceil */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 8.0f, 0.0f));
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		sponzaFloorTexture.UseTexture();
		superShinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
		meshList[5]->RenderMesh();

		shaderList[0]->Unbind();

		mainWindow.SwapBuffers();
	}

	return 0;
}
