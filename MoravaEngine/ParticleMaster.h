#pragma once

#include "Particle.h"

#include <vector>


class ParticleMaster
{
public:
	ParticleMaster();
	~ParticleMaster();

private:
	std::vector<Particle*> m_Particles;

};
