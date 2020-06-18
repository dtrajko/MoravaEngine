#pragma once

#include "Quad.h"

#include <vector>


class QuadInstanced : public Quad
{
public:
	QuadInstanced();
	void Render(unsigned int instanceCount);
	unsigned int CreateEmptyVBO(int floatCount);
	void AddInstancedAttribute(int VAO, int VBO, int attribute, int dataSize, int instancedDataLength, int offset);
	void LoadToVAO();
	void StoreDataInAttributeList(int attributeNumber, int coordinateSize, std::vector<float>* data);
	void UnbindVAO();
	void UpdateVBO(unsigned int VBO, unsigned int floatCount, std::vector<float>* data);
	virtual ~QuadInstanced();

private:
	std::vector<float> m_Positions;

	std::vector<unsigned int> m_VBOs;
};
