#pragma once

#include "Mesh.h"
#include "Model.h"
#include "AABB.h"
#include "Pivot.h"

#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>

#include <string>


const int MESH_TYPE_CUBE = 0;
const int MESH_TYPE_PYRAMID = 1;
const int MESH_TYPE_SPHERE = 2;
const int MESH_TYPE_CYLINDER = 3;
const int MESH_TYPE_CONE = 4;
const int MESH_TYPE_RING = 5;
const int MESH_TYPE_BOB_LAMP = 6;
const int MESH_TYPE_ANIM_BOY = 7;


class SceneObject
{
public:
	SceneObject();
	void Render();
	~SceneObject();

public:

	int id;
	std::string name;
	bool isSelected;

	glm::mat4 transform;
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
	glm::vec3 positionAABB;
	glm::vec3 scaleAABB;

	glm::vec4 color;

	std::string objectType;

	Mesh* mesh;
	Model* model;
	AABB* AABB;
	Pivot* pivot;

	int meshType;
	int modelType;
	std::string textureName;
	std::string materialName;

	float tilingFactor;
	float tilingFMaterial;

};
