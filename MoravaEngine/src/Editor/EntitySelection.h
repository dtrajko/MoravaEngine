#pragma once

#include "H2M/Renderer/ModelH2M.h"
#include "H2M/Scene/EntityH2M.h"
#include "H2M/Scene/SceneH2M.h"

#include <vector>


struct SelectedSubmesh
{
	H2M::EntityH2M Entity;
	H2M::RefH2M<H2M::MeshH2M> Mesh;
	float Distance;
};

class EntitySelection
{

public:
	static std::vector<SelectedSubmesh> s_SelectionContext;

};
