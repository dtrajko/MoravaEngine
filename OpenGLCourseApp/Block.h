#pragma once

#include "Mesh.h"

#include <glm/glm.hpp>


class Block : public Mesh
{
public:
	Block();
	Block(glm::vec3 scale);
	void Update(glm::vec3 scale);
	virtual void Render() override;
	inline glm::vec3 GetScale() const { return m_Scale; };
	virtual ~Block() override;

private:
	void Generate(glm::vec3 scale);

private:
	glm::vec3 m_Scale;

};
