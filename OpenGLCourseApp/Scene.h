#pragma once

#include <vector>
#include <map>
#include <string>

#include <glm/glm.hpp>

#include "CommonValues.h"
#include "Camera.h"
#include "Mesh.h"
#include "Texture.h"
#include "Material.h"
#include "Model.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "Skybox.h"



class Scene
{

public:
	Scene();
	virtual void Update(float timestep) = 0;
	virtual void Render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, bool shadowPass = false) = 0;
	~Scene();

private:
	Camera camera;

	std::vector<std::string> skyboxFaces;
	Skybox skybox;

	DirectionalLight directionalLight;
	PointLight pointLights[MAX_POINT_LIGHTS];
	SpotLight spotLights[MAX_SPOT_LIGHTS];

	std::map<std::string, Mesh*> listMeshes;
	std::map<std::string, Shader*> listShaders;
	std::map<std::string, Texture*> listTextures;
	std::map<std::string, Material*> listMaterials;
	std::map<std::string, Model*> listModels;

	unsigned int shadowMapWidth;
	unsigned int shadowMapHeight;
};
