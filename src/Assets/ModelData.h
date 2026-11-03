#pragma once

#include "Animation/AnimationClip.h"
#include "H2M/Renderer/ModelVerticesH2M.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>


/**
 * What a model file holds, as plain data (docs/engine/Game_Engine_Runtime_Guide.html, 10.2): the import fills it from the
 * source file through assimp, the cooked .mmesh file holds the same bytes, and ModelH2M is built from it whichever it was.
 */
struct ModelMeshData
{
	std::string Name;
	uint32_t BaseVertex = 0;
	uint32_t BaseIndex = 0;
	uint32_t IndexCount = 0;
	uint32_t VertexCount = 0;
	uint32_t TriangleCount = 0; // in ModelData::Indices, after the earlier meshes' triangles
	uint32_t MaterialIndex = 0;
	bool IsRigged = false;
	glm::vec3 BoundsMin = glm::vec3(0.0f);
	glm::vec3 BoundsMax = glm::vec3(0.0f);
};

// Values and maps as the file has them: which of them a graphics API uses is decided when the model is built. A map's path
// is as written in the file (relative to the model), empty: none
struct ModelMaterialData
{
	std::string Name;

	bool HasDiffuseColor = false;
	glm::vec3 DiffuseColor = glm::vec3(0.0f);
	bool HasEmissiveColor = false;
	glm::vec3 EmissiveColor = glm::vec3(0.0f);
	bool HasShininess = false;
	float Shininess = 0.0f;
	bool HasReflectivity = false;
	float Reflectivity = 0.0f;
	bool HasMetallicFactor = false;
	float MetallicFactor = 0.0f;
	bool HasRoughnessFactor = false;
	float RoughnessFactor = 0.0f;

	std::string AlbedoMap;
	std::string NormalMap;
	std::string ShininessMap;
	std::string DiffuseRoughnessMap; // glTF: the metallicRoughness map
	std::string ReflectionFactorMap; // FBX: "$raw.ReflectionFactor|file"
	std::string MetalnessMap;
	std::string EmissiveMap;
	std::string AmbientOcclusionMap; // or the lightmap: glTF's occlusion map arrives as one
};

struct ModelData
{
	bool IsAnimated = false; // the file has animations: AnimatedVertices, else StaticVertices
	std::vector<H2M::VertexH2M> StaticVertices;
	std::vector<H2M::AnimatedVertex> AnimatedVertices;
	std::vector<H2M::Index> Indices;
	std::vector<ModelMeshData> Meshes;
	std::vector<ModelMaterialData> Materials;

	// The file's node tree, each parent before its children (Nodes[0] is the root), and the meshes at each node
	std::vector<AnimationSkeleton::SourceNode> Nodes;
	std::vector<std::vector<uint32_t>> NodeMeshes;

	std::vector<std::string> BoneNames;
	std::vector<glm::mat4> BoneOffsets;
	std::vector<AnimationClip> Clips;
};
