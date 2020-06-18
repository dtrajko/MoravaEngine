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
	unsigned int StoreDataInAttributeList(int attributeNumber, unsigned int floatCount, std::vector<float>* data);
	void UnbindVAO();
	void UpdateVBO(unsigned int VBO, unsigned int floatCount, std::vector<float>* data);
	inline const unsigned int GetVBOInstanced() const { return m_VBO_Instanced; };
	inline void SetVBOInstanced(unsigned int VBO_Instanced) { m_VBO_Instanced = VBO_Instanced; };
	virtual void Clear() override;
	virtual ~QuadInstanced();

private:
	std::vector<float> m_Positions;

	unsigned int m_VBO_Instanced; // a large VBO for instanced data
};
