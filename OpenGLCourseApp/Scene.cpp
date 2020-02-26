#include "Scene.h"

#include "MeshData.h"


SceneSettings Scene::sceneSettings;

Scene::Scene()
{
	shadowMapWidth = 1024;
	shadowMapHeight = 1024;

	SetTextures();
	SetupMaterials();
	SetupMeshes();
}

void Scene::SetTextures()
{
	textureSlots.insert(std::make_pair("diffuse", 1));
	textureSlots.insert(std::make_pair("normal", 2));
	textureSlots.insert(std::make_pair("shadow", 3));
	textureSlots.insert(std::make_pair("omniShadow", 4));
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
