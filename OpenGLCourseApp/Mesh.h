#pragma once

#include <glm/glm.hpp>


class Mesh
{

public:

	Mesh();

	virtual void Create(float* vertices, unsigned int* indices, unsigned int vertexCount, unsigned int indexCount);
	virtual void Update(glm::vec3 scale);
	virtual void Render();
	virtual void Clear();

	static void CalcAverageNormals(float* vertices, unsigned int vertexCount, unsigned int* indices, unsigned int indexCount);
	static void CalcTangentSpace(float* vertices, unsigned int vertexCount, unsigned int* indices, unsigned int indexCount);

	inline glm::vec3 GetScale() const { return m_Scale; };

	virtual ~Mesh();

protected:
	unsigned int m_VAO;
	unsigned int m_VBO;
	unsigned int m_IBO;
	unsigned int m_IndexCount;

	glm::vec3 m_Scale;

};
