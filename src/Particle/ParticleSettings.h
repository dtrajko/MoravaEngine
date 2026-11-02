#pragma once

#include <glm/glm.hpp>

#include <string>


struct ParticleSettings {
	std::string textureName;
	int numRows;
	int PPS;
	glm::vec3 direction;
	float intensity;
	float gravityComplient;
	float lifeLength;
	float diameter;
	bool instanced;

	inline bool operator!=(const ParticleSettings& other)
	{
		return textureName != other.textureName ||
			numRows != other.numRows ||
			PPS != other.PPS ||
			direction.x != other.direction.x ||
			direction.y != other.direction.y ||
			direction.z != other.direction.z ||
			intensity != other.intensity ||
			gravityComplient != other.gravityComplient ||
			lifeLength != other.lifeLength ||
			diameter != other.diameter ||
			instanced != other.instanced;
	}
};
