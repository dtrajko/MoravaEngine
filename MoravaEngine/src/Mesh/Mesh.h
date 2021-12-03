#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Core/TimestepH2M.h"

#include "Core/Transform.h"
#include "Material/Material.h"

#include <glm/glm.hpp>

#include <vector>


struct aiScene;

class Mesh : public H2M::RefCountedH2M
{

public:

	Mesh();
	Mesh(glm::vec3 scale);
	virtual ~Mesh();

	virtual void Create();
	virtual void Create(float* vertices, unsigned int* indices, unsigned int vertexCount, unsigned int indexCount);
	virtual void Generate(glm::vec3 scale);
	virtual void Update(glm::vec3 scale);
	virtual void OnUpdate(H2M::TimestepH2M ts, bool debug); // HazelMesh Update method
	virtual void Render();
	virtual void Clear();

	// Getters
	inline glm::vec3 GetScale() const { return m_Scale; };
	inline const unsigned int GetVertexCount() const { return m_VertexCount; };
	inline const unsigned int GetIndexCount() const { return m_IndexCount; };
	inline const unsigned int GetVAO() const { return m_VAO; };
	inline const unsigned int GetVBO() const { return m_VBO; };
	inline const unsigned int GetIBO() const { return m_IBO; };
	Material* GetBaseMaterial() { return m_BaseMaterial; }
	std::vector<Material*>* GetMaterials() { return &m_Materials; }
	void SetFilePath(const std::string& filePath) { m_FilePath = filePath; }
	const std::string& GetFilePath() const { return m_FilePath; }
	const aiScene* GetSceneAssimp() { return m_Scene; }

	void BindVertexArray();

	virtual void RecalculateNormals();
	virtual void RecalculateTangentSpace();



public:
	float* m_Vertices;
	unsigned int* m_Indices;

	Transform m_Transform;

protected:
	unsigned int m_VAO;
	unsigned int m_VBO;
	unsigned int m_IBO;
	unsigned int m_VertexCount;
	unsigned int m_IndexCount;

	glm::vec3 m_Scale;

	Material* m_BaseMaterial;
	std::vector<Material*> m_Materials;

	// Hazel/Renderer/Mesh
	std::string m_FilePath;

	const aiScene* m_Scene; // Moved from HazelMesh

	uint32_t m_VertexAttribArrayCount = 0;

};
