#include "Mesh/Ring90.h"

#include "Mesh/VertexTBN.h"

#include <GL/glew.h>

#include <vector>


Ring90::Ring90() : Ring90(glm::vec3(1.0f))
{
}

Ring90::Ring90(glm::vec3 scale) : Mesh(scale)
{
	m_Scale = scale;

	m_Model = new Model("Models/Primitives/ring_90.obj");
	auto& meshList = m_Model->GetMeshList();
	auto& mesh = meshList.at(0);

	m_VAO = mesh->GetVAO();
	m_VBO = mesh->GetVBO();
	m_IBO = mesh->GetIBO();
	m_IndexCount = mesh->GetIndexCount();

	Generate(scale);
}

void Ring90::Generate(glm::vec3 scale)
{
	m_Scale = scale;
}

Ring90::~Ring90()
{
	delete m_Model;
}
