#include "Mesh/Ring.h"

#include "Mesh/VertexTBN.h"

#include <GL/glew.h>

#include <vector>


Ring::Ring() : Ring(glm::vec3(1.0f))
{
}

Ring::Ring(glm::vec3 scale) : Mesh(scale)
{
	m_Scale = scale;

	m_Model = new Model("Models/Primitives/ring.obj");
	auto& meshList = m_Model->GetMeshList();
	auto& mesh = meshList.at(0);

	m_VAO = mesh->GetVAO();
	m_VBO = mesh->GetVBO();
	m_IBO = mesh->GetIBO();
	m_IndexCount = mesh->GetIndexCount();

	Generate(scale);
}

void Ring::Generate(glm::vec3 scale)
{
	m_Scale = scale;
}

Ring::~Ring()
{
	delete m_Model;
}
