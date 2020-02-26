#include "Scene.h"



SceneSettings Scene::sceneSettings;

Scene::Scene()
{
	shadowMapWidth = 1024;
	shadowMapHeight = 1024;

	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_rt.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_lf.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_up.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_dn.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_bk.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_ft.tga");
	skybox = new Skybox(skyboxFaces);
}

void Scene::Update(float timestep)
{
}

Scene::~Scene()
{
	delete skybox;
	skyboxFaces.clear();
}
