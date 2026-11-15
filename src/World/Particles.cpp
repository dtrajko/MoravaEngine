#include "Particles.h"


const char* ParticleEmitterSettings::GetBlendName(BlendMode mode)
{
	return mode == BlendMode::Additive ? "Additive" : "Alpha";
}

bool ParticleEmitterSettings::FindBlend(const std::string& name, BlendMode& mode)
{
	for (BlendMode candidate : { BlendMode::Alpha, BlendMode::Additive })
	{
		if (name == GetBlendName(candidate))
		{
			mode = candidate;
			return true;
		}
	}
	return false;
}
