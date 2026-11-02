#pragma once

#include <glm/glm.hpp>


struct EventCooldown
{
	float lastTime;
	float cooldown;
};

struct SLight
{
	bool enabled;
	glm::vec3 color;
	float ambientIntensity;
	float diffuseIntensity;
};

struct SDirectionalLight
{
	SLight base;
	glm::vec3 direction;
};

struct SPointLight
{
	SLight base;
	glm::vec3 position;
	float constant;
	float linear;
	float exponent;
};

struct SSpotLight
{
	SPointLight base;
	glm::vec3 direction;
	float edge;
};

struct PointSpotLight
{
	bool enabled;
	glm::vec3 position;
	glm::vec3 color;
	float exponent;
	float linear;
	float constant;
};
