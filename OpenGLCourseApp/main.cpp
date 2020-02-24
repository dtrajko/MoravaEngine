#define STB_IMAGE_IMPLEMENTATION

#include <stdio.h>
#include <string.h>
#include <cmath>
#include <vector>
#include <map>

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



// Window dimensions
const GLint WIDTH = 1280;
const GLint HEIGHT = 720;
const float toRadians = 3.14159265f / 180.0f;

Window mainWindow;


struct SceneSettings
{
	glm::vec3 cameraPosition;
	glm::vec3 lightDirection;
	float cameraStartYaw;
	float ambientIntensity;
	float diffuseIntensity;
	unsigned int shadowMapWidth;
	unsigned int shadowMapHeight;
	float shadowSpeed;
	glm::vec3 pLight_0_color;
	glm::vec3 pLight_0_position;
	float pLight_0_diffuseIntensity;
	glm::vec3 pLight_1_color;
	glm::vec3 pLight_1_position;
	float pLight_1_diffuseIntensity;
	glm::vec3 pLight_2_color;
	glm::vec3 pLight_2_position;
	float pLight_2_diffuseIntensity;
	glm::mat4 lightProjectionMatrix;
};

std::map<std::string, SceneSettings> sceneSettings;
std::string currentScene = "cottage"; // "cottage", "sponza", "eiffel"


GLint uniformModel = 0;
GLint uniformView = 0;
GLint uniformProjection = 0;
GLint uniformEyePosition = 0;
GLint uniformSpecularIntensity = 0;
GLint uniformShininess = 0;
GLint uniformDirectionalLightTransform = 0;
GLint uniformOmniLightPos = 0;
GLint uniformFarPlane = 0;

std::vector <Mesh*> meshList;
std::vector <Shader*> shaderList;

Shader directionalShadowShader;
Shader omniShadowShader;

Camera camera;

Texture brickTexture;
Texture pyramidTexture;
Texture crateTextureDiffuse;
Texture crateTextureNormal;
Texture grassTexture;
// Sponza textures
Texture textureSponzaFloorDiffuse;
Texture textureSponzaFloorNormal;
Texture textureSponzaWallDiffuse;
Texture textureSponzaWallNormal;
Texture textureSponzaCeilDiffuse;
Texture textureSponzaCeilNormal;
Texture normalMapDefault;

// Avoid the default GL_TEXTURE0, always be explicit
GLuint txSlotDiffuse = 1;
GLuint txSlotNormal = 2;
GLuint txSlotShadow = 3;
GLuint txSlotOmniShadow = 4;

Material shinyMaterial;
Material dullMaterial;
Material superShinyMaterial;

Model sponza;
Model cottage;
Model eiffel;
Model watchtower;

DirectionalLight mainLight;
PointLight pointLights[MAX_POINT_LIGHTS];
SpotLight spotLights[MAX_SPOT_LIGHTS];

unsigned int pointLightCount = 0;
unsigned int spotLightCount = 0;

GLfloat deltaTime = 0.0f;
GLfloat lastTime = 0.0f;

static const char* vertShader = "Shaders/shader.vert";
static const char* fragShader = "Shaders/shader.frag";

static const char* vertShaderDirShadowMap = "Shaders/directional_shadow_map.vert";
static const char* fragShaderDirShadowMap = "Shaders/directional_shadow_map.frag";

static const char* vertShaderOmniShadowMap = "Shaders/omni_shadow_map.vert";
static const char* geomShaderOmniShadowMap = "Shaders/omni_shadow_map.geom";
static const char* fragShaderOmniShadowMap = "Shaders/omni_shadow_map.frag";


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

	for (unsigned int i = 0; i < verticeCount / vLength; i++)
	{
		unsigned int nOffset = i * vLength + normalOffset;
		glm::vec3 vec(vertices[nOffset + 0], vertices[nOffset + 1], vertices[nOffset + 2]);
		vec = glm::normalize(vec);
		vertices[nOffset + 0] = vec.x; vertices[nOffset + 1] = vec.y; vertices[nOffset + 2] = vec.z;
	}
}

void calcTangentSpace(unsigned int* indices, unsigned int indiceCount,
	GLfloat* vertices, unsigned int verticeCount)
{
	unsigned int vLength = sizeof(Vertex) / sizeof(float);

	for (size_t i = 0; i < indiceCount; i += 3)
	{
		unsigned int in0 = indices[i + 0] * vLength;
		unsigned int in1 = indices[i + 1] * vLength;
		unsigned int in2 = indices[i + 2] * vLength;
		glm::vec3 v0(vertices[in0 + 0], vertices[in0 + 1], vertices[in0 + 2]);
		glm::vec3 v1(vertices[in1 + 0], vertices[in1 + 1], vertices[in1 + 2]);
		glm::vec3 v2(vertices[in0 + 2], vertices[in2 + 1], vertices[in2 + 2]);

		glm::vec2 uv0(vertices[in0 + 3], vertices[in0 + 4]);
		glm::vec2 uv1(vertices[in1 + 3], vertices[in1 + 4]);
		glm::vec2 uv2(vertices[in2 + 3], vertices[in2 + 4]);

		// Edges of the triangle : position delta
		glm::vec3 deltaPos1 = v1 - v0;
		glm::vec3 deltaPos2 = v2 - v1;

		// UV delta
		glm::vec2 deltaUV1 = uv1 - uv0;
		glm::vec2 deltaUV2 = uv2 - uv0;

		float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
		glm::vec3 tangent   = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
		glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

		// write tangents
		vertices[in0 + 8] = tangent.x; vertices[in0 + 9] = tangent.y; vertices[in0 + 10] = tangent.z;
		vertices[in1 + 8] = tangent.x; vertices[in1 + 9] = tangent.y; vertices[in1 + 10] = tangent.z;
		vertices[in2 + 8] = tangent.x; vertices[in2 + 9] = tangent.y; vertices[in2 + 10] = tangent.z;

		// write bitangents
		vertices[in0 + 11] = bitangent.x; vertices[in0 + 12] = bitangent.y; vertices[in0 + 13] = bitangent.z;
		vertices[in1 + 11] = bitangent.x; vertices[in1 + 12] = bitangent.y; vertices[in1 + 13] = bitangent.z;
		vertices[in2 + 11] = bitangent.x; vertices[in2 + 12] = bitangent.y; vertices[in2 + 13] = bitangent.z;
	}
}


void CreateObjects()
{
	GLfloat vertices[] =
	{
		//  X      Y      Z        U     V        NX     NY     NZ        TX     TY     TZ        BX     BY     BZ
		-0.5f,  0.5f, -0.5f,    1.0f, 1.0f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,    1.0f, 0.0f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,    0.0f, 0.0f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,    0.0f, 1.0f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,

		-0.5f,  0.5f,  0.5f,    0.0f, 1.0f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,
		-0.5f, -0.5f,  0.5f,    0.0f, 0.0f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,
		 0.5f, -0.5f,  0.5f,    1.0f, 0.0f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,    1.0f, 1.0f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,

		 0.5f,  0.5f, -0.5f,    1.0f, 1.0f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,    1.0f, 0.0f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f,  0.5f,    0.0f, 0.0f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,    0.0f, 1.0f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,

		-0.5f,  0.5f, -0.5f,    0.0f, 1.0f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,    0.0f, 0.0f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f,  0.5f,    1.0f, 0.0f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,    1.0f, 1.0f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,

		-0.5f,  0.5f,  0.5f,    1.0f, 1.0f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f, -0.5f,    1.0f, 0.0f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,    0.0f, 0.0f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,
		 0.5f,  0.5f,  0.5f,    0.0f, 1.0f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,

		-0.5f, -0.5f,  0.5f,    0.0f, 1.0f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,
		-0.5f, -0.5f, -0.5f,    0.0f, 0.0f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,    1.0f, 0.0f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f,  0.5f,    1.0f, 1.0f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,
	};

	unsigned int vertexCount = 14 * 4 * 6;

	printf("Size of vertices array: %.2d\n", vertexCount);

	unsigned int indices[] =
	{
		 0,  3,  1,
		 3,  2,  1,
		 4,  5,  7,
		 7,  5,  6,
		 8, 11,  9,
		11, 10,  9,
		12, 13, 15,
		15, 13, 14,
		16, 19, 17,
		19, 18, 17,
		20, 21, 23,
		23, 21, 22,
	};

	unsigned int indexCount = 6 * 6;

	calcAverageNormals(indices, indexCount, vertices, vertexCount, sizeof(Vertex) / sizeof(float), offsetof(Vertex, Normal) / sizeof(float));
	calcTangentSpace(indices, indexCount, vertices, vertexCount);

	/* Floor Mesh */
	GLfloat floorVertices[] =
	{
		// position               tex coords      normal               tangent              bitangent
		-10.0f, 0.0f, -10.0f,     0.0f,  0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f, -1.0f,
		 10.0f, 0.0f, -10.0f,    10.0f,  0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f, -1.0f,
		-10.0f, 0.0f,  10.0f,     0.0f, 10.0f,    0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f, -1.0f,
		 10.0f, 0.0f,  10.0f,    10.0f, 10.0f,    0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f, -1.0f,
	};

	unsigned int floorVertexCount = 14 * 4;

	unsigned int floorIndices[] =
	{
		0, 2, 1,
		1, 2, 3,
	};

	unsigned int floorIndexCount = 6;


	/* Basic Quad mesh */
	GLfloat quadVertices[] =
	{
		// position            tex coords     normal               tangent              bitangent 
		-1.0f, 0.0f, -1.0f,    0.0f, 0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f, -1.0f,
		 1.0f, 0.0f, -1.0f,    1.0f, 0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f, -1.0f,
		-1.0f, 0.0f,  1.0f,    0.0f, 1.0f,    0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f, -1.0f,
		 1.0f, 0.0f,  1.0f,    1.0f, 1.0f,    0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f, -1.0f,
	};

	unsigned int quadVertexCount = 14 * 4;

	unsigned int quadIndices[] =
	{
		0, 2, 1,
		1, 2, 3,
	};

	unsigned int quadIndexCount = 6;

	calcAverageNormals(quadIndices, quadIndexCount, quadVertices, quadVertexCount, sizeof(Vertex) / sizeof(float), offsetof(Vertex, Normal) / sizeof(float));
	calcTangentSpace(quadIndices, quadIndexCount, quadVertices, quadVertexCount);


	Mesh* obj1 = new Mesh();
	obj1->CreateMesh(vertices, indices, vertexCount, indexCount);
	meshList.push_back(obj1);

	Mesh* obj2 = new Mesh();
	obj2->CreateMesh(vertices, indices, vertexCount, indexCount);
	meshList.push_back(obj2);

	Mesh* floor = new Mesh();
	floor->CreateMesh(floorVertices, floorIndices, floorVertexCount, floorIndexCount);
	meshList.push_back(floor);

	Mesh* wall = new Mesh();
	wall->CreateMesh(floorVertices, floorIndices, floorVertexCount, floorIndexCount);
	meshList.push_back(wall);

	Mesh* ceil = new Mesh();
	ceil->CreateMesh(floorVertices, floorIndices, floorVertexCount, floorIndexCount);
	meshList.push_back(ceil);

	Mesh* shadowMapDisplay = new Mesh();
	shadowMapDisplay->CreateMesh(quadVertices, quadIndices, quadVertexCount, quadIndexCount);
	meshList.push_back(shadowMapDisplay);
}

void CreateShaders()
{
	Shader* shader1 = new Shader();
	shader1->CreateFromFiles(vertShader, fragShader);
	shaderList.push_back(shader1);

	directionalShadowShader = Shader();
	directionalShadowShader.CreateFromFiles(vertShaderDirShadowMap, fragShaderDirShadowMap);

	omniShadowShader = Shader();
	omniShadowShader.CreateFromFiles(vertShaderOmniShadowMap, geomShaderOmniShadowMap, fragShaderOmniShadowMap);
}

void RenderSceneCottage()
{
	glm::mat4 sceneOrigin = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

	// Model matrix
	glm::mat4 model;

	/* Cube Left */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-5.0f, 3.0f, -5.0f)) * sceneOrigin;
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(2.0f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	brickTexture.Bind(txSlotDiffuse);
	normalMapDefault.Bind(txSlotNormal);
	shinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	meshList[0]->RenderMesh();

	/* Cube Right */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(5.0f, 3.0f, -5.0f)) * sceneOrigin;
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(2.0f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	crateTextureDiffuse.Bind(txSlotDiffuse);
	crateTextureNormal.Bind(txSlotNormal);
	superShinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	meshList[1]->RenderMesh();

	/* Floor */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)) * sceneOrigin;
	model = glm::scale(model, glm::vec3(1.0f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	textureSponzaFloorDiffuse.Bind(txSlotDiffuse);
	textureSponzaFloorNormal.Bind(txSlotNormal);
	superShinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	meshList[2]->RenderMesh();

	/* Floor 2nd */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 10.0f, 0.0f)) * sceneOrigin;
	model = glm::scale(model, glm::vec3(1.0f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	textureSponzaFloorDiffuse.Bind(txSlotDiffuse);
	textureSponzaFloorNormal.Bind(txSlotNormal);
	superShinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	meshList[2]->RenderMesh();

	/* Floor 3nd */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 20.0f, 0.0f)) * sceneOrigin;
	model = glm::scale(model, glm::vec3(1.0f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	grassTexture.Bind(txSlotDiffuse);
	shinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	meshList[2]->RenderMesh();

	/* Wall Right */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(10.0f, 10.0f, 0.0f)) * sceneOrigin;
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(1.0f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	textureSponzaWallDiffuse.Bind(txSlotDiffuse);
	textureSponzaWallNormal.Bind(txSlotNormal);
	shinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	meshList[3]->RenderMesh();

	/* Wall Left */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-10.0f, 10.0f, 0.0f)) * sceneOrigin;
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(1.0f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	textureSponzaWallDiffuse.Bind(txSlotDiffuse);
	textureSponzaWallNormal.Bind(txSlotNormal);
	shinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	meshList[3]->RenderMesh();

	/* Wall Back */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 10.0f, -10.0f)) * sceneOrigin;
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(1.0f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	textureSponzaWallDiffuse.Bind(txSlotDiffuse);
	textureSponzaWallNormal.Bind(txSlotNormal);
	shinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	meshList[3]->RenderMesh();

	/* Ceil */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 9.99f, 0.0f)) * sceneOrigin;
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(1.0f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	textureSponzaCeilDiffuse.Bind(txSlotDiffuse);
	textureSponzaCeilNormal.Bind(txSlotNormal);
	superShinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	meshList[4]->RenderMesh();

	/* Ceil 2nd */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 19.99f, 0.0f)) * sceneOrigin;
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(1.0f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	textureSponzaCeilDiffuse.Bind(txSlotDiffuse);
	textureSponzaCeilNormal.Bind(txSlotNormal);
	superShinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	meshList[4]->RenderMesh();

	/* Cottage */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 20.0f, -5.0f)) * sceneOrigin;
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(1.0f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	superShinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	cottage.RenderModel(txSlotDiffuse, txSlotNormal);
}

void RenderSceneSponza(bool shadowPass = false)
{
	/* Sponza scene */
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(0.008f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	superShinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	sponza.RenderModel(txSlotDiffuse, txSlotNormal);

	if (!shadowPass)
	{
		/* ShadowMap display */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 16.0f, -0.5f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(1.2f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		shaderList[0]->SetTexture(txSlotShadow);
		shaderList[0]->SetNormalMap(txSlotShadow);
		dullMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
		meshList[5]->RenderMesh();
	}
}

void RenderSceneEiffel(bool shadowPass = false)
{
	glm::mat4 model;

	/* Floor */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(3.0f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	textureSponzaCeilDiffuse.Bind(txSlotDiffuse);
	textureSponzaCeilNormal.Bind(txSlotNormal);
	superShinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	if (!shadowPass)
	{
		meshList[2]->RenderMesh();
	}

	/* Eiffel model */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-2.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(0.0003f, 0.0003f, 0.0003f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	superShinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	eiffel.RenderModel(txSlotDiffuse, txSlotNormal);

	/* Watchtower model */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(4.0f, -0.35f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(0.5f));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
	superShinyMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
	watchtower.RenderModel(txSlotDiffuse, txSlotNormal);

	if (!shadowPass)
	{
		/* ShadowMap display */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 20.0f, -40.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(10.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		shaderList[0]->SetTexture(txSlotShadow);
		shaderList[0]->SetNormalMap(txSlotShadow);
		dullMaterial.UseMaterial(uniformSpecularIntensity, uniformShininess);
		meshList[5]->RenderMesh();
	}
}

void RenderScene(bool shadowPass = false)
{
	if (currentScene == "cottage")
	{
		RenderSceneCottage();
	}

	if (currentScene == "sponza")
	{
		RenderSceneSponza(shadowPass);
	}

	if (currentScene == "eiffel")
	{
		RenderSceneEiffel(shadowPass);
	}
}

void DirectionalShadowMapPass(DirectionalLight* light)
{
	directionalShadowShader.Bind();

	glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

	light->GetShadowMap()->Write();

	glClear(GL_DEPTH_BUFFER_BIT);

	uniformModel = directionalShadowShader.GetModelLocation();
	directionalShadowShader.SetDirectionalLightTransform(&light->CalculateLightTransform());

	directionalShadowShader.Validate();

	bool shadowPass = true;
	RenderScene(shadowPass);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OmniShadowMapPass(PointLight* light)
{
	omniShadowShader.Bind();

	glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

	light->GetShadowMap()->Write();

	glClear(GL_DEPTH_BUFFER_BIT);

	uniformModel = omniShadowShader.GetModelLocation();
	uniformOmniLightPos = omniShadowShader.GetUniformLocationOmniLightPos();
	uniformFarPlane = omniShadowShader.GetUniformLocationFarPlane();

	glUniform3f(uniformOmniLightPos, light->GetPosition().x, light->GetPosition().y, light->GetPosition().z);
	glUniform1f(uniformFarPlane, light->GetFarPlane());

	omniShadowShader.SetLightMatrices(light->CalculateLightTransform());

	omniShadowShader.Validate();

	bool shadowPass = true;
	RenderScene(shadowPass);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderPass(glm::mat4 projectionMatrix, glm::mat4 viewMatrix)
{
	shaderList[0]->Bind();

	uniformModel = shaderList[0]->GetModelLocation();
	uniformProjection = shaderList[0]->GetProjectionLocation();
	uniformView = shaderList[0]->GetViewLocation();
	uniformEyePosition = shaderList[0]->GetUniformLocationEyePosition();
	uniformSpecularIntensity = shaderList[0]->GetUniformLocationSpecularIntensity();
	uniformShininess = shaderList[0]->GetUniformLocationShininess();

	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());

	// Clear the window
	glClearColor(135.0f / 255.0f, 206.0f / 255.0f, 235.0f / 255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(viewMatrix));
	glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniform3f(uniformEyePosition, camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);

	shaderList[0]->SetDirectionalLight(&mainLight);
	shaderList[0]->SetPointLights(pointLights, pointLightCount, txSlotOmniShadow, 0);
	shaderList[0]->SetSpotLights(spotLights, spotLightCount, txSlotOmniShadow, pointLightCount);
	shaderList[0]->SetDirectionalLightTransform(&mainLight.CalculateLightTransform());

	mainLight.GetShadowMap()->Read(txSlotShadow);
	shaderList[0]->SetTexture(txSlotDiffuse);
	shaderList[0]->SetNormalMap(txSlotNormal);
	shaderList[0]->SetDirectionalShadowMap(txSlotShadow);

	glm::vec3 lowerLight = camera.getCameraPosition();
	lowerLight.y -= 0.2f;
	spotLights[2].SetFlash(lowerLight, camera.getCameraDirection());

	shaderList[0]->Validate();

	RenderScene();
}

void setupLight()
{
	mainLight = DirectionalLight(sceneSettings[currentScene].shadowMapWidth, sceneSettings[currentScene].shadowMapHeight, { 1.0f, 1.0f, 1.0f },
		sceneSettings[currentScene].ambientIntensity, sceneSettings[currentScene].diffuseIntensity, sceneSettings[currentScene].lightDirection);
	mainLight.SetLightProjection(sceneSettings[currentScene].lightProjectionMatrix);

	pointLights[0] = PointLight(1024, 1024, 0.01f, 100.0f, sceneSettings[currentScene].pLight_0_color, 0.5f, 6.0f, sceneSettings[currentScene].pLight_0_position, 0.3f, 0.2f, 0.1f);
	pointLightCount++;
	pointLights[1] = PointLight(1024, 1024, 0.01f, 100.0f, sceneSettings[currentScene].pLight_1_color, 0.4f, sceneSettings[currentScene].pLight_1_diffuseIntensity, sceneSettings[currentScene].pLight_1_position, 0.3f, 0.2f, 0.1f);
	pointLightCount++;
	pointLights[2] = PointLight(1024, 1024, 0.01f, 100.0f, { 0.0f, 0.0f, 1.0f }, 0.1f, 1.0f, { 10.0f, 2.0f, 10.0f }, 0.3f, 0.2f, 0.1f);
	pointLightCount++;

	spotLights[0] = SpotLight(1024, 1024, 0.01f, 100.0f, { 1.0f, 1.0f, 1.0f }, 1.0f, 10.0f, { 0.0f, 20.0f, -28.0f }, { 0.0f, 0.0f, -1.0f }, 0.3f, 0.2f, 0.1f, 160.0f);
	spotLightCount++;
	spotLights[1] = SpotLight(1024, 1024, 0.01f, 100.0f, { 0.8f, 0.8f, 1.0f }, 0.3f, 6.0f, { 8.0f, 2.0f, 0.0f }, { -0.25f, 0.5f, -0.5f }, 0.3f, 0.2f, 0.1f, 45.0f);
	spotLightCount++;
	spotLights[2] = SpotLight(1024, 1024, 0.01f, 100.0f, { 1.0f, 1.0f, 1.0f }, 0.4f, 1.6f, glm::vec3(), glm::vec3(), 0.4f, 0.3f, 0.2f, 35.0f);
	spotLightCount++;
}


int main()
{
	mainWindow = Window(WIDTH, HEIGHT);
	mainWindow.Initialize();

	/* Start scene settings */
	sceneSettings.insert(std::make_pair("cottage", SceneSettings()));
	sceneSettings.insert(std::make_pair("sponza", SceneSettings()));
	sceneSettings.insert(std::make_pair("eiffel", SceneSettings()));

	sceneSettings["cottage"].cameraPosition = glm::vec3(0.0f, 25.0f, 15.0f);
	sceneSettings["cottage"].lightDirection = glm::vec3(0.0f, -60.0f, -20.0f);
	sceneSettings["cottage"].cameraStartYaw = -90.0f;
	sceneSettings["cottage"].ambientIntensity = 0.05f;
	sceneSettings["cottage"].diffuseIntensity = 0.2f;
	sceneSettings["cottage"].shadowMapWidth = 1024;
	sceneSettings["cottage"].shadowMapHeight = 1024;
	sceneSettings["cottage"].shadowSpeed = 2.0f;
	sceneSettings["cottage"].pLight_0_color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings["cottage"].pLight_0_position = glm::vec3(0.0f, 6.0f, 0.0f);
	sceneSettings["cottage"].pLight_0_diffuseIntensity = 6.0f;
	sceneSettings["cottage"].pLight_1_color = glm::vec3(1.0f, 0.0f, 1.0f);
	sceneSettings["cottage"].pLight_1_position = glm::vec3(-5.0f, 8.0f, -5.0f);
	sceneSettings["cottage"].pLight_1_diffuseIntensity = 6.0f;
	sceneSettings["cottage"].pLight_2_color = glm::vec3(0.0f, 0.0f, 1.0f);
	sceneSettings["cottage"].pLight_2_position = glm::vec3(10.0f, 2.0f, 10.0f);
	sceneSettings["cottage"].pLight_2_diffuseIntensity = 6.0f;
	sceneSettings["cottage"].lightProjectionMatrix = glm::ortho(-16.0f, 16.0f, -16.0f, 16.0f, 0.1f, 32.0f);

	sceneSettings["eiffel"].cameraPosition = glm::vec3(0.0f, 6.0f, 20.0f);
	sceneSettings["eiffel"].lightDirection = glm::vec3(3.0f, -9.0f, -3.0f);
	sceneSettings["eiffel"].cameraStartYaw = -90.0f;
	sceneSettings["eiffel"].ambientIntensity = 0.2f;
	sceneSettings["eiffel"].diffuseIntensity = 0.8f;
	sceneSettings["eiffel"].shadowMapWidth = 2048;
	sceneSettings["eiffel"].shadowMapHeight = 2048;
	sceneSettings["eiffel"].shadowSpeed = 1.0f;
	sceneSettings["eiffel"].pLight_0_color = glm::vec3(1.0f, 0.0f, 1.0f);
	sceneSettings["eiffel"].pLight_0_position = glm::vec3(0.0f, 20.0f, 0.0f);
	sceneSettings["eiffel"].pLight_0_diffuseIntensity = 6.0f;
	sceneSettings["eiffel"].pLight_1_color = glm::vec3(1.0f, 0.0f, 0.0f);
	sceneSettings["eiffel"].pLight_1_position = glm::vec3(-2.0f, 9.6f, 0.0f);
	sceneSettings["eiffel"].pLight_1_diffuseIntensity = 6.0f;
	sceneSettings["eiffel"].pLight_2_color = glm::vec3(0.0f, 0.0f, 1.0f);
	sceneSettings["eiffel"].pLight_2_position = glm::vec3(10.0f, 2.0f, 10.0f);
	sceneSettings["eiffel"].pLight_2_diffuseIntensity = 6.0f;
	sceneSettings["eiffel"].lightProjectionMatrix = glm::ortho(-16.0f, 16.0f, -16.0f, 16.0f, 0.1f, 32.0f);

	sceneSettings["sponza"].cameraPosition = glm::vec3(-4.0f, 10.0f, -0.5f);
	sceneSettings["sponza"].lightDirection = glm::vec3(1.2f, -14.0f, 1.2f);
	sceneSettings["sponza"].cameraStartYaw = 0.0f;
	sceneSettings["sponza"].ambientIntensity = 0.2f;
	sceneSettings["sponza"].diffuseIntensity = 1.0f;
	sceneSettings["sponza"].shadowMapWidth = 4096;
	sceneSettings["sponza"].shadowMapHeight = 4096;
	sceneSettings["sponza"].shadowSpeed = 0.1f;
	sceneSettings["sponza"].pLight_0_color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings["sponza"].pLight_0_position = glm::vec3(0.0f, 20.0f, 0.0f);
	sceneSettings["sponza"].pLight_0_diffuseIntensity = 2.0f;
	sceneSettings["sponza"].pLight_1_color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings["sponza"].pLight_1_position = glm::vec3(8.92f, 2.75f, -0.85f);
	sceneSettings["sponza"].pLight_1_diffuseIntensity = 2.0f;
	sceneSettings["sponza"].pLight_2_color = glm::vec3(0.0f, 0.0f, 1.0f);
	sceneSettings["sponza"].pLight_2_position = glm::vec3(10.0f, 2.0f, 10.0f);
	sceneSettings["sponza"].pLight_2_diffuseIntensity = 2.0f;
	sceneSettings["sponza"].lightProjectionMatrix = glm::ortho(-36.0f, 36.0f, -36.0f, 36.0f, 0.1f, 36.0f);
	/* End scene settings */

	CreateObjects();
	CreateShaders();

	camera = Camera(sceneSettings[currentScene].cameraPosition, glm::vec3(0.0f, 1.0f, 0.0f), sceneSettings[currentScene].cameraStartYaw, 0.0f, 4.0f, 0.1f);

	brickTexture = Texture("Textures/brick.png");
	brickTexture.LoadTexture();
	pyramidTexture = Texture("Textures/pyramid.png");
	pyramidTexture.LoadTexture();

	// Sponza textures
	textureSponzaFloorDiffuse = Texture("Textures/sponza_floor_a_diff.tga");
	textureSponzaFloorDiffuse.LoadTexture();
	textureSponzaFloorNormal = Texture("Textures/sponza_floor_a_ddn.tga");
	textureSponzaFloorNormal.LoadTexture();
	textureSponzaWallDiffuse = Texture("Textures/sponza_bricks_a_diff.tga");
	textureSponzaWallDiffuse.LoadTexture();
	textureSponzaWallNormal = Texture("Textures/sponza_bricks_a_ddn.tga");
	textureSponzaWallNormal.LoadTexture();
	textureSponzaCeilDiffuse = Texture("Textures/sponza_ceiling_a_diff.tga");
	textureSponzaCeilDiffuse.LoadTexture();
	textureSponzaCeilNormal = Texture("Textures/sponza_ceiling_a_ddn.tga");
	textureSponzaCeilNormal.LoadTexture();

	crateTextureDiffuse = Texture("Textures/crate.png");
	crateTextureDiffuse.LoadTexture(true);
	crateTextureNormal = Texture("Textures/crateNormal.png");
	crateTextureNormal.LoadTexture(true);
	grassTexture = Texture("Textures/grass.jpg");
	grassTexture.LoadTexture();
	normalMapDefault = Texture("Textures/normal_map_default.png");
	normalMapDefault.LoadTexture();


	shinyMaterial = Material(1.0f, 128.0f);
	dullMaterial = Material(1.0f, 64.0f);
	superShinyMaterial = Material(1.0f, 1024.0f);

	if (currentScene == "sponza")
	{
		sponza = Model();
		sponza.LoadModel("Models/sponza.obj");
	}

	if (currentScene == "cottage")
	{
		cottage = Model();
		cottage.LoadModel("Models/cottage.obj");
	}

	if (currentScene == "eiffel")
	{
		eiffel = Model();
		eiffel.LoadModel("Models/Eiffel_Tower.obj");
		watchtower = Model();
		watchtower.LoadModel("Models/wooden_watch_tower.obj");
	}

	setupLight();

	// Projection matrix
	glm::mat4 projection = glm::perspective(glm::radians(60.0f), mainWindow.GetBufferWidth() / mainWindow.GetBufferHeight(), 0.1f, 200.0f);

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
		camera.mouseScrollControl(mainWindow.getKeys(), deltaTime, mainWindow.getXMouseScrollOffset(), mainWindow.getYMouseScrollOffset());

		// Shadow rotation
		glm::vec3 lightDirection = sceneSettings[currentScene].lightDirection;
		float lightRadius = abs(lightDirection.x);
		float lightAngle = now * sceneSettings[currentScene].shadowSpeed;
		lightDirection.x = cos(lightAngle) * lightRadius;
		lightDirection.z = sin(lightAngle) * lightRadius;
		mainLight.SetDirection(lightDirection);

		if (currentScene == "cottage")
		{
			glm::vec3 pLightPos = sceneSettings[currentScene].pLight_0_position;
			float lightRadius = 6.0;
			float lightAngle = now * sceneSettings[currentScene].shadowSpeed;
			pLightPos.x += cos(lightAngle) * lightRadius;
			pLightPos.z += sin(lightAngle) * lightRadius;
			pLightPos.y += cos(lightAngle * 0.5) * lightRadius * 0.5f;
			pointLights[0].SetPosition(pLightPos);
		}

		DirectionalShadowMapPass(&mainLight);

		for (size_t i = 0; i < pointLightCount; i++)
		{
			OmniShadowMapPass(&pointLights[i]);
		}

		for (size_t i = 0; i < spotLightCount; i++)
		{
			OmniShadowMapPass((PointLight*)&spotLights[i]);
		}

		RenderPass(projection, camera.CalculateViewMatrix());

		shaderList[0]->Unbind();

		mainWindow.SwapBuffers();
	}

	return 0;
}
