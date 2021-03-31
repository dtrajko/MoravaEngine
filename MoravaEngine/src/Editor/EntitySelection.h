#pragma once

#include "Hazel/Scene/Entity.h"
#include "Hazel/Scene/HazelScene.h"

#include <vector>


struct SelectedSubmesh
{
	Hazel::Entity Entity;
	Hazel::Submesh* Submesh;
	float Distance;
};

class EntitySelection
{

public:
	static std::vector<SelectedSubmesh> s_SelectionContext;

};
