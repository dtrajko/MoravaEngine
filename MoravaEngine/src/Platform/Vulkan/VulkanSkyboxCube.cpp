#include "Platform/Vulkan/VulkanSkyboxCube.h"

#include "Mesh/VertexTBN.h"

#include <vector>


VulkanSkyboxCube::VulkanSkyboxCube()
{
	struct VertexSkyboxCube
	{
		glm::vec3 Position;
		glm::vec2 TexCoord;
	};

	float sizeX = 1.0f;
	float sizeY = 1.0f;
	float sizeZ = 1.0f;

	float txCoX = 1.0f;
	float txCoY = 1.0f;
	float txCoZ = 1.0f;

	std::vector<VertexSkyboxCube> vertices = {
		// --------------- Position XYZ ----------------- TexCoords UV
		// back face
		VertexSkyboxCube { glm::vec3(-sizeX,  sizeY, -sizeZ), glm::vec2(txCoX,  0.0f) },
		VertexSkyboxCube { glm::vec3(-sizeX, -sizeY, -sizeZ), glm::vec2(txCoX, txCoY) },
		VertexSkyboxCube { glm::vec3( sizeX, -sizeY, -sizeZ), glm::vec2(0.0f,  txCoY) },
		VertexSkyboxCube { glm::vec3( sizeX,  sizeY, -sizeZ), glm::vec2(0.0f,   0.0f) },
		// front face
		VertexSkyboxCube { glm::vec3(-sizeX,  sizeY,  sizeZ), glm::vec2(0.0f,   0.0f) },
		VertexSkyboxCube { glm::vec3(-sizeX, -sizeY,  sizeZ), glm::vec2(0.0f,  txCoY) },
		VertexSkyboxCube { glm::vec3( sizeX, -sizeY,  sizeZ), glm::vec2(txCoX, txCoY) },
		VertexSkyboxCube { glm::vec3( sizeX,  sizeY,  sizeZ), glm::vec2(txCoX,  0.0f) },
		// right face
		VertexSkyboxCube { glm::vec3( sizeX,  sizeY, -sizeZ), glm::vec2(txCoZ,  0.0f) },
		VertexSkyboxCube { glm::vec3( sizeX, -sizeY, -sizeZ), glm::vec2(txCoZ, txCoY) },
		VertexSkyboxCube { glm::vec3( sizeX, -sizeY,  sizeZ), glm::vec2(0.0f,  txCoY) },
		VertexSkyboxCube { glm::vec3( sizeX,  sizeY,  sizeZ), glm::vec2(0.0f,   0.0f) },
		// left face
		VertexSkyboxCube { glm::vec3(-sizeX,  sizeY, -sizeZ), glm::vec2(0.0f,   0.0f) },
		VertexSkyboxCube { glm::vec3(-sizeX, -sizeY, -sizeZ), glm::vec2(0.0f,  txCoY) },
		VertexSkyboxCube { glm::vec3(-sizeX, -sizeY,  sizeZ), glm::vec2(txCoZ, txCoY) },
		VertexSkyboxCube { glm::vec3(-sizeX,  sizeY,  sizeZ), glm::vec2(txCoZ,  0.0f) },
		// top face
		VertexSkyboxCube { glm::vec3(-sizeX,  sizeY,  sizeZ), glm::vec2(0.0f,  txCoZ) },
		VertexSkyboxCube { glm::vec3(-sizeX,  sizeY, -sizeZ), glm::vec2(0.0f,   0.0f) },
		VertexSkyboxCube { glm::vec3( sizeX,  sizeY, -sizeZ), glm::vec2(txCoX,  0.0f) },
		VertexSkyboxCube { glm::vec3( sizeX,  sizeY,  sizeZ), glm::vec2(txCoX, txCoZ) },
		// bottom face
		VertexSkyboxCube { glm::vec3(-sizeX, -sizeY,  sizeZ), glm::vec2(0.0f,   0.0f) },
		VertexSkyboxCube { glm::vec3(-sizeX, -sizeY, -sizeZ), glm::vec2(0.0f,  txCoZ) },
		VertexSkyboxCube { glm::vec3( sizeX, -sizeY, -sizeZ), glm::vec2(txCoX, txCoZ) },
		VertexSkyboxCube { glm::vec3( sizeX, -sizeY,  sizeZ), glm::vec2(txCoX,  0.0f) },
	};

	std::vector<uint32_t> indices =
	{
		 0,  3,  1, // back
		 3,  2,  1, // back
		 4,  5,  7, // front
		 7,  5,  6, // front
		 8, 11,  9, // right
		11, 10,  9, // right
		12, 13, 15, // left
		15, 13, 14, // left
		16, 19, 17, // top
		19, 18, 17, // top
		21, 22, 23, // bottom
		20, 21, 23, // bottom
	};

	m_VertexBuffer = H2M::VertexBuffer::Create(vertices.data(), (uint32_t)vertices.size() * sizeof(VertexSkyboxCube));
	m_IndexBuffer = H2M::IndexBuffer::Create(indices.data(), (uint32_t)indices.size() * sizeof(uint32_t));

	m_IndexCount = (uint32_t)indices.size();
}

VulkanSkyboxCube::~VulkanSkyboxCube()
{
}
