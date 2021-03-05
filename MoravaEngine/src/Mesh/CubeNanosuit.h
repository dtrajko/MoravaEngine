#pragma once

#include "LearnOpenGL/MeshJoey.h"
#include "Mesh/Cube.h"

#include <string>
#include <vector>


class CubeNanosuit
{
public:
	CubeNanosuit();
	CubeNanosuit(std::string const& textureDirectory);
	MeshJoey* processMesh();
	std::vector<TextureData> loadMaterialTextures(std::string fileName, std::string typeName);
	inline MeshJoey* GetMesh() { return m_Mesh; };
	void Draw(Shader* shader);
	virtual ~CubeNanosuit();

private:
	MeshJoey* m_Mesh;
	std::string m_TextureDirectory;

	std::vector<std::vector<float>> m_Vertices;
	std::vector<unsigned int> m_Indices;

};
