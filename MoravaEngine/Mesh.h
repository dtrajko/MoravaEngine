#pragma once

#include <glm/glm.hpp>


class Mesh
{

public:

	Mesh();
	Mesh(glm::vec3 scale);
	virtual void Create();
	virtual void Create(float* vertices, unsigned int* indices, unsigned int vertexCount, unsigned int indexCount);
	virtual void Generate(glm::vec3 scale);
	virtual void Update(glm::vec3 scale);
	virtual void Render();
	virtual void Clear();
	inline glm::vec3 GetScale() const { return m_Scale; };

	inline const unsigned int GetVertexCount() const { return m_VertexCount; };
	inline const unsigned int GetIndexCount() const { return m_IndexCount; };

	inline const unsigned int GetVAO() const { return m_VAO; };
	inline const unsigned int GetVBO() const { return m_VBO; };
	inline const unsigned int GetIBO() const { return m_IBO; };

	static void CalcAverageNormals(float* vertices, unsigned int vertexCount, unsigned int* indices, unsigned int indexCount);
	static void CalcTangentSpace(float* vertices, unsigned int vertexCount, unsigned int* indices, unsigned int indexCount);


	virtual ~Mesh();

public:
	float* m_Vertices;
	unsigned int* m_Indices;

protected:
	unsigned int m_VAO;
	unsigned int m_VBO;
	unsigned int m_IBO;
	unsigned int m_VertexCount;
	unsigned int m_IndexCount;

	glm::vec3 m_Scale;

};
