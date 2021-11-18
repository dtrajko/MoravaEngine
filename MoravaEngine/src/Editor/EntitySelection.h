#pragma once

#include "Hazel/Scene/HazelScene.h"

#include "HazelLegacy/Renderer/MeshHazelLegacy.h"
#include "HazelLegacy/Scene/EntityHazelLegacy.h"

#include <vector>


struct SelectedSubmesh
{
	Hazel::EntityHazelLegacy Entity;
	Hazel::Ref<Hazel::SubmeshHazelLegacy> Mesh;
	float Distance;
};

class EntitySelection
{

public:
	static std::vector<SelectedSubmesh> s_SelectionContext;

};
