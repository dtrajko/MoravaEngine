#pragma once

#include "Mesh/Mesh.h"


/**
 * Quad mesh compatible with LearnOpenGL code / shaders
 * Vertex buffer structure as follows:
 * layout (location = 0) in vec3 aPosition;
 * layout (location = 1) in vec2 aTexCoord;
 */
class HazelFullscreenQuad : public Mesh
{
public:
	HazelFullscreenQuad();
	virtual ~HazelFullscreenQuad();

	virtual void Render() override;

private:
	void SetupFullscreenQuad();

private:
	unsigned int m_FullscreenQuadVAO;
	unsigned int m_FullscreenQuadVBO;
	unsigned int m_FullscreenQuadIBO;

};
