#pragma once

#include "Quad.h"

#include <vector>


class QuadInstanced : public Quad
{
public:
	QuadInstanced();
	void Render();
	unsigned int CreateEmptyVBO(int floatCount);
	void AddInstancedAttribute(int VAO, int VBO, int attribute, int dataSize, int instancedDataLength, int offset);
	void LoadToVAO();
	void StoreDataInAttributeList(int attributeNumber, int coordinateSize, float* data);
	void UnbindVAO();
	void UpdateVBO(unsigned int VBO, int floatCount, float* data);
	// inline const unsigned int GetVBOInstanced() const { return m_VBO_Instanced; };
	// inline void SetVBOInstanced(unsigned int VBO_Instanced) { m_VBO_Instanced = VBO_Instanced; };
	~QuadInstanced();

private:
	std::vector<float> m_Positions;
	std::vector<float> m_TexCoord;

	std::vector<unsigned int> m_VBOs;
};
