#include "Scene.h"

#include "MeshData.h"
#include "Tile2D.h"


SceneSettings Scene::sceneSettings;

Scene::Scene()
{
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
}

void Scene::SetupMaterials()
{
	materials.insert(std::make_pair("shiny", new Material(1.0f, 128.0f)));
	materials.insert(std::make_pair("dull", new Material(1.0f, 64.0f)));
	materials.insert(std::make_pair("superShiny", new Material(1.0f, 1024.0f)));
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
	unsigned int indices[] = { 0, 1, 2, 3, 5, 4 };
	Tile2D* m_Tile2D = new Tile2D();
	m_Tile2D->CreateMesh(&vertices[0], &indices[0], 12, 6);
	meshes.insert(std::make_pair("water", m_Tile2D));
}

void Scene::SetTextures()
{
	textures.insert(std::make_pair("normalMapDefault", new Texture("Textures/normal_map_default.png")));
	textures.insert(std::make_pair("waterDuDv", new Texture("Textures/water/waterDuDv.png")));
	textures.insert(std::make_pair("waterNormal", new Texture("Textures/water/waterNormal.png")));

	textures["normalMapDefault"]->LoadTexture();
	textures["waterDuDv"]->LoadTexture();
	textures["waterNormal"]->LoadTexture();
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

	delete skybox;
}
