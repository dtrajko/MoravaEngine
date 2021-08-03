#include "MoravaTinyMesh.h"

#include "Core/Util.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>


template <>
struct std::hash<Hazel::Vertex>
{
	size_t operator()(Hazel::Vertex const& vertex) const
	{
		size_t seed = 0;
		Util::HashCombine(seed, vertex.Position, vertex.Normal, vertex.Texcoord);
		return seed;
	}
};

MoravaTinyMesh::MoravaTinyMesh(const std::string& filepath, Hazel::Ref<MoravaShader> shader, Hazel::Ref<Hazel::HazelMaterial> material, bool isAnimated)
	: Hazel::HazelMesh(filepath, shader, material, isAnimated)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str()))
	{
		throw std::runtime_error(warn + err);
	}

	vertices.clear();
	indices.clear();

	std::unordered_map<Hazel::Vertex, uint32_t> uniqueVertices{};

	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Hazel::Vertex vertex{};

			if (index.vertex_index >= 0)
			{
				vertex.Position = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2],
				};
			}

			if (index.normal_index >= 0)
			{
				vertex.Normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2],
				};
			}

			if (index.texcoord_index >= 0)
			{
				vertex.Texcoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					attrib.texcoords[2 * index.texcoord_index + 1],
				};
			}

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}
}
