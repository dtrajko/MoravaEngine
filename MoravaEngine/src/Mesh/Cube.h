#pragma once

#include "Mesh/Mesh.h"


/**
 * Cube mesh compatible with LearnOpenGL code / shaders
 * Vertex buffer structure as follows:
 * layout (location = 0) in vec3 aPos;
 * layout (location = 1) in vec3 aNormal;
 * layout (location = 2) in vec2 aTexCoords;
 */
class Cube : public Mesh
{
public:
	Cube();
	virtual void Render() override;
	virtual ~Cube() override;

};
