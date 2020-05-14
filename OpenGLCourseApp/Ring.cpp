#include "Ring.h"

#include <GL/glew.h>

#include <vector>

#include "VertexTBN.h"


Ring::Ring() : Ring(glm::vec3(1.0f))
{
}

Ring::Ring(glm::vec3 scale) : Mesh(scale)
{
	m_Scale = scale;

	m_Model = new Model();
	m_Model->LoadModel("Models/ring.obj");
	auto& meshList = m_Model->GetMeshList();
	auto& mesh = meshList.at(0);

	m_VAO = mesh->m_VAO;
	m_IBO = mesh->m_IBO;
	m_VBO = mesh->m_VBO;
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
