#pragma once

#include "Mesh/Quad.h"

#include <vector>


class QuadInstanced : public Quad
{
public:
	QuadInstanced();
	virtual ~QuadInstanced() override;

	void Render(unsigned int instanceCount);
	unsigned int CreateEmptyVBO(int floatCount);
	void AddInstancedAttribute(int attribute, int dataSize, int instancedDataLength, int offset);
	unsigned int StoreDataInAttributeList(int attributeNumber, unsigned int floatCount, std::vector<float>* data);
	void UnbindVAO();
	void UpdateVBO(unsigned int floatCount, float* data);
	inline const unsigned int GetVBOInstanced() const { return m_VBO_Instanced; };
	inline void SetVBOInstanced(unsigned int VBO_Instanced) { m_VBO_Instanced = VBO_Instanced; };
	virtual void Clear() override;

private:
	std::vector<float> m_Positions;

	unsigned int m_VBO_Instanced; // a large VBO for instanced data
};
