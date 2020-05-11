#pragma once


#include "Mesh.h"


class Pyramid : public Mesh
{
public:
	Pyramid();
	Pyramid(glm::vec3 scale);
	virtual void Update(glm::vec3 scale) override;
	virtual void Render() override;
	inline glm::vec3 GetScale() const { return m_Scale; };
	virtual ~Pyramid() override;

private:
	void Generate(glm::vec3 scale);

private:
	glm::vec3 m_Scale;
};
