#include "DX11Mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "DX11VertexMesh.h"

#include <locale>
#include <codecvt>


DX11Mesh::DX11Mesh(const wchar_t* fullPath)
{
	tinyobj::attrib_t attribs;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	std::string inputfile = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(fullPath);

	bool res = tinyobj::LoadObj(&attribs, &shapes, &materials, &warn, &err, inputfile.c_str());

	if (!err.empty()) {
		throw std::exception("Mesh not created successfully!");
	}

	if (!res) {
		throw std::exception("Mesh not created successfully!");
	}

	if (shapes.size() > 1) {
		throw std::exception("Multiple shapes in a mesh not supported!");
	}

	std::vector<DX11VertexMesh> list_vertices;
	std::vector<uint32_t> list_indices;

	size_t size_vertex_index_lists = 0;

	for (size_t s = 0; s < shapes.size(); s++)
	{
		size_vertex_index_lists += shapes[s].mesh.indices.size();
	}

	list_vertices.reserve(size_vertex_index_lists);
	list_indices.reserve(size_vertex_index_lists);

	for (size_t s = 0; s < shapes.size(); s++)
	{
		size_t index_offset = 0;

		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
		{
			unsigned char num_face_verts = shapes[s].mesh.num_face_vertices[f];

			for (unsigned char v = 0; v < num_face_verts; v++)
			{
				tinyobj::index_t index = shapes[s].mesh.indices[index_offset + v];

				tinyobj::real_t vx = attribs.vertices[index.vertex_index * 3 + 0];
				tinyobj::real_t vy = attribs.vertices[index.vertex_index * 3 + 1];
				tinyobj::real_t vz = attribs.vertices[index.vertex_index * 3 + 2];

				tinyobj::real_t tx = attribs.vertices[index.texcoord_index * 2 + 0];
				tinyobj::real_t ty = attribs.vertices[index.texcoord_index * 2 + 1];

				DX11VertexMesh vertex(glm::vec3(vx, vy, vz), glm::vec2(tx, ty));
				list_vertices.push_back(vertex);
				list_indices.push_back((uint32_t)(index_offset + v));
			}
		}
	}

	m_VertexBuffer = Hazel::VertexBuffer::Create(&list_vertices[0], sizeof(DX11VertexMesh), (uint32_t)list_vertices.size());
	m_IndexBuffer = Hazel::IndexBuffer::Create(&list_indices[0], (uint32_t)list_indices.size() * sizeof(uint32_t));
	// m_IndexBuffer = IndexBuffer::Create(m_Indices.data(), (uint32_t)m_Indices.size() * sizeof(Index)); // HazelMesh
	// s_IndexBuffer = Hazel::Ref<DX11IndexBuffer>::Create(indexList, indexCount * sizeof(uint32_t)); // DX11Renderer

	Log::GetLogger()->info("DX11Mesh '{0}' successfully created!", inputfile);
}

DX11Mesh::~DX11Mesh()
{
}
