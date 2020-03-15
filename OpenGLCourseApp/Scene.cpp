#include "Scene.h"

#include "MeshData.h"
#include "Tile2D.h"


SceneSettings Scene::sceneSettings;

Scene::Scene()
{
	sceneSettings.enableShadows      = false;
	sceneSettings.enableOmniShadows  = false;
	sceneSettings.enablePointLights  = false;
	sceneSettings.enableSpotLights   = false;
	sceneSettings.enableWaterEffects = false;
	sceneSettings.enableSkybox       = false;
	sceneSettings.enableNormalMaps   = false;
	sceneSettings.cameraPosition = glm::vec3(0.0f, 6.0f, 20.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 4.0f;
	sceneSettings.nearPlane = 0.01f;
	sceneSettings.farPlane = 200.0f;
	sceneSettings.ambientIntensity = 0.2f;
	sceneSettings.diffuseIntensity = 0.8f;
	sceneSettings.lightDirection = glm::vec3(3.0f, -9.0f, -3.0f);
	sceneSettings.lightProjectionMatrix = glm::ortho(-16.0f, 16.0f, -16.0f, 16.0f, 0.1f, 32.0f);
	sceneSettings.pLight_0_color = glm::vec3(1.0f, 0.0f, 1.0f);
	sceneSettings.pLight_0_position = glm::vec3(0.0f, 20.0f, 0.0f);
	sceneSettings.pLight_0_diffuseIntensity = 6.0f;
	sceneSettings.pLight_1_color = glm::vec3(1.0f, 0.0f, 0.0f);
	sceneSettings.pLight_1_position = glm::vec3(-2.0f, 9.6f, 0.0f);
	sceneSettings.pLight_1_diffuseIntensity = 6.0f;
	sceneSettings.pLight_2_color = glm::vec3(0.8f, 0.8f, 0.5f);
	sceneSettings.pLight_2_position = glm::vec3(-2.0f, 4.0f, 0.0f);
	sceneSettings.pLight_2_diffuseIntensity = 6.0f;
	sceneSettings.shadowMapWidth = 2048;
	sceneSettings.shadowMapHeight = 2048;
	sceneSettings.shadowSpeed = 0.4f;
	sceneSettings.waterHeight = 1.6f;
	sceneSettings.waterWaveSpeed = 0.005f;

	shadowMapWidth = 1024;
	shadowMapHeight = 1024;

	SetTextures();
	SetTextureSlots();
	SetupMaterials();
	SetupMeshes();
}

void Scene::SetTextureSlots()
{
	textureSlots.insert(std::make_pair("diffuse",    1));
	textureSlots.insert(std::make_pair("normal",     2));
	textureSlots.insert(std::make_pair("shadow",     3));
	textureSlots.insert(std::make_pair("omniShadow", 4));
	textureSlots.insert(std::make_pair("reflection", 5));
	textureSlots.insert(std::make_pair("refraction", 6));
	textureSlots.insert(std::make_pair("depth",      7));
	textureSlots.insert(std::make_pair("DuDv",       8));

	// PBR
	textureSlots.insert(std::make_pair("albedo",     1));
	textureSlots.insert(std::make_pair("normal",     2));
	textureSlots.insert(std::make_pair("metallic",   3));
	textureSlots.insert(std::make_pair("roughness",  4));
	textureSlots.insert(std::make_pair("ao",         5));

	// environment cubemap
	textureSlots.insert(std::make_pair("equirectangularMap", 6));
	textureSlots.insert(std::make_pair("environmentMap", 7));
}

void Scene::SetupMaterials()
{
	materials.insert(std::make_pair("shiny", new Material(1.0f, 128.0f)));
	materials.insert(std::make_pair("dull", new Material(1.0f, 64.0f)));
	materials.insert(std::make_pair("superShiny", new Material(1.0f, 1024.0f)));
	materials.insert(std::make_pair("superDull", new Material(1.0f, 16.0f)));
}

void Scene::SetupMeshes()
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

	float vertices[] = { -1, -1, -1, 1, 1, -1, 1, -1, -1, 1, 1, 1 };
	unsigned int indices[] = { 0, 1, 2, 3, 4, 5 };
	Tile2D* m_Tile2D = new Tile2D();
	m_Tile2D->CreateMesh(&vertices[0], &indices[0], 12, 6);
	meshes.insert(std::make_pair("water", m_Tile2D));
}

void Scene::SetTextures()
{
	textures.insert(std::make_pair("normalMapDefault", new Texture("Textures/normal_map_default.png")));
	textures.insert(std::make_pair("shadowMapDefault", new Texture("Textures/shadow_map_default.png")));
	textures.insert(std::make_pair("waterDuDv", new Texture("Textures/water/waterDuDv.png")));
	textures.insert(std::make_pair("waterNormal", new Texture("Textures/water/waterNormal.png")));

	textures["normalMapDefault"]->Load();
	textures["waterDuDv"]->Load();
	textures["waterNormal"]->Load();
}

void Scene::SetCamera()
{
	m_Camera = new Camera(sceneSettings.cameraPosition, glm::vec3(0.0f, 1.0f, 0.0f), sceneSettings.cameraStartYaw, 0.0f, sceneSettings.cameraMoveSpeed, 0.1f);
}

Scene::~Scene()
{
	for (auto& texture : textures)
		delete texture.second;

	for (auto& mesh : meshes)
		delete mesh.second;

	for (auto& material : materials)
		delete material.second;

	for (auto& model : models)
		delete model.second;

	skyboxFaces.clear();
	textures.clear();
	textureSlots.clear();
	materials.clear();
	meshes.clear();
	models.clear();

	delete m_Skybox;
	delete m_Camera;
}
