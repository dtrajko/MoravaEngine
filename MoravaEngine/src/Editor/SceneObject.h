#pragma once

#include "Core/AABB.h"
#include "Mesh/Mesh.h"
#include "Mesh/Model.h"
#include "Editor/Pivot.h"

#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>

#include <string>


// reserved ID range for meshes 0-999
const int MESH_TYPE_CUBE     =  0;
const int MESH_TYPE_PYRAMID  =  1;
const int MESH_TYPE_SPHERE   =  2;
const int MESH_TYPE_CYLINDER =  3;
const int MESH_TYPE_CONE     =  4;
const int MESH_TYPE_RING     =  5;
const int MESH_TYPE_BOB_LAMP =  6;
const int MESH_TYPE_ANIM_BOY =  7;
const int MESH_TYPE_TERRAIN  =  8;
const int MESH_TYPE_WATER    =  9;
const int MESH_TYPE_DRONE    = 10;
const int MESH_TYPE_M1911    = 11;

// Models start at 1000
const int MODEL_STONE_CARVED   = 1000;
const int MODEL_OLD_STOVE      = 1001;
const int MODEL_BUDDHA         = 1002;
const int MODEL_HHELI          = 1003;
const int MODEL_JEEP           = 1004;
const int MODEL_DAMAGED_HELMET = 1005;
const int MODEL_SF_HELMET      = 1006;
const int MODEL_CERBERUS       = 1007;
const int MODEL_PINE           = 1008;
const int MODEL_BOULDER        = 1009;

// Particle systems start at 2000
const int PARTICLE_SYSTEM      = 2000;


class SceneObject
{
public:
	SceneObject();
	~SceneObject();

	virtual void Render();

	inline AABB* const GetAABB() const { return m_AABB; };
	inline void SetAABB(AABB* inAABB) { m_AABB = inAABB; };

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

	std::string m_Type;

	Mesh* mesh;
	Model* model;
	Pivot* pivot;

	int m_TypeID;
	std::string textureName;
	std::string materialName;

	float tilingFactor;
	float tilingFMaterial;

	bool castShadow;
	bool receiveShadows;

private:
	AABB* m_AABB;

};
