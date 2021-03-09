#pragma once

#include "Mesh/Mesh.h"


/**
 * Vertex buffer layout:
 * layout (location = 0) in vec3 aPos;
 * layout (location = 1) in vec2 aTexCoords;
 */
class QuadSSAO : public Mesh
{
public:
	QuadSSAO();
	virtual ~QuadSSAO();

	virtual void Render() override;

};
