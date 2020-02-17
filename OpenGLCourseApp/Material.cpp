#include "Material.h"



Material::Material()
{
	specularIntensity = 0.0f;
	shininess = 0.0f;
}

Material::Material(GLfloat specIntensity, GLfloat shine)
{
	specularIntensity = specIntensity;
	shininess = shine;
}

void Material::UseMaterial(GLint specularIntensityLocation, GLint shininessLocation)
{
	glUniform1f(specularIntensityLocation, specularIntensity);
	glUniform1f(shininessLocation, shininess);
}

Material::~Material()
{
}
