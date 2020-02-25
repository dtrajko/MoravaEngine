#include "Scene.h"



Scene::Scene()
{
	shadowMapWidth = 1024;
	shadowMapHeight = 1024;
}

void Scene::Update(float timestep)
{
}

void Scene::Render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, bool shadowPass)
{
}

Scene::~Scene()
{
}
