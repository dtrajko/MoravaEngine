#include "DX11Mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "DX11VertexLayout.h"

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

	std::string mtldir = inputfile.substr(0, inputfile.find_last_of("\\/"));

	bool res = tinyobj::LoadObj(&attribs, &shapes, &materials, &warn, &err, inputfile.c_str(), mtldir.c_str());

	if (!err.empty()) throw std::exception("Mesh not created successfully");

	if (!res) throw std::exception("Mesh not created successfully");

	std::vector<DX11VertexLayout> list_vertices;
	std::vector<uint32_t> list_indices;

	size_t size_vertex_index_lists = 0;

	for (size_t s = 0; s < shapes.size(); s++)
	{
		size_vertex_index_lists += shapes[s].mesh.indices.size();
	}

	list_vertices.reserve(size_vertex_index_lists);
	list_indices.reserve(size_vertex_index_lists);

	// if .OBJ contains no materials, add a default material
	bool useMaterials = true;
	if (materials.size() == 0)
	{
		useMaterials = false;
		materials.push_back(tinyobj::material_t());
	}

	m_MaterialSlots.resize(materials.size());

	size_t index_global_offset = 0;

	for (size_t m = 0; m < materials.size(); m++)
	{
		m_MaterialSlots[m].StartIndex = index_global_offset;
		m_MaterialSlots[m].MaterialID = m;

		for (size_t s = 0; s < shapes.size(); s++)
		{
			size_t index_offset = 0;

			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
			{
				unsigned char num_face_verts = shapes[s].mesh.num_face_vertices[f];

				if (useMaterials && shapes[s].mesh.material_ids[f] != m)
				{
					index_offset += num_face_verts;
					continue;
				}

				glm::vec3 vertices_face[3];
				glm::vec2 texcoords_face[3];

				for (unsigned char v = 0; v < num_face_verts; v++)
				{
					tinyobj::index_t index = shapes[s].mesh.indices[index_offset + v];

					tinyobj::real_t vx = attribs.vertices[index.vertex_index * 3 + 0];
					tinyobj::real_t vy = attribs.vertices[index.vertex_index * 3 + 1];
					tinyobj::real_t vz = attribs.vertices[index.vertex_index * 3 + 2];

					tinyobj::real_t tx = 0;
					tinyobj::real_t ty = 0;
					if (attribs.texcoords.size())
					{
						tx = attribs.texcoords[index.texcoord_index * 2 + 0];
						ty = attribs.texcoords[index.texcoord_index * 2 + 1];
					}
					vertices_face[v] = glm::vec3(vx, vy, vz);
					texcoords_face[v] = glm::vec2(tx, ty);
				}

				glm::vec3 tangent;
				glm::vec3 binormal;

				ComputeTangents(
					vertices_face[0], vertices_face[1], vertices_face[2],
					texcoords_face[0], texcoords_face[1], texcoords_face[2],
					tangent, binormal);

				for (unsigned char v = 0; v < num_face_verts; v++)
				{
					tinyobj::index_t index = shapes[s].mesh.indices[index_offset + v];

					tinyobj::real_t vx = attribs.vertices[index.vertex_index * 3 + 0];
					tinyobj::real_t vy = attribs.vertices[index.vertex_index * 3 + 1];
					tinyobj::real_t vz = attribs.vertices[index.vertex_index * 3 + 2];

					tinyobj::real_t tx = 0;
					tinyobj::real_t ty = 0;
					if (attribs.texcoords.size())
					{
						tx = attribs.texcoords[index.texcoord_index * 2 + 0];
						ty = attribs.texcoords[index.texcoord_index * 2 + 1];
					}

					tinyobj::real_t nx = 0;
					tinyobj::real_t ny = 0;
					tinyobj::real_t nz = 0;
					if (attribs.normals.size())
					{
						nx = attribs.normals[index.normal_index * 3 + 0];
						ny = attribs.normals[index.normal_index * 3 + 1];
						nz = attribs.normals[index.normal_index * 3 + 2];
					}

					glm::vec3 v_tangent, v_binormal;
					v_binormal = glm::cross(glm::vec3(nx, ny, nz), tangent);
					v_tangent = glm::cross(v_binormal, glm::vec3(nx, ny, nz));

					DX11VertexLayout vertex(glm::vec3(vx, vy, vz), glm::vec3(nx, ny, nz), v_tangent, v_binormal, glm::vec2(tx, ty));
					list_vertices.push_back(vertex);

					list_indices.push_back((unsigned int)index_global_offset + v);
				}

				index_offset += num_face_verts;
				index_global_offset += num_face_verts;
			}
		}

		m_MaterialSlots[m].NumIndices = index_global_offset - m_MaterialSlots[m].StartIndex;
	}

	m_VertexBuffer = H2M::VertexBuffer::Create(&list_vertices[0], sizeof(DX11VertexLayout), (uint32_t)list_vertices.size());
	m_IndexBuffer = H2M::IndexBuffer::Create(&list_indices[0], (uint32_t)list_indices.size() * sizeof(uint32_t));
	// m_IndexBuffer = IndexBuffer::Create(m_Indices.data(), (uint32_t)m_Indices.size() * sizeof(Index)); // HazelMesh
	// s_IndexBuffer = H2M::RefH2M<DX11IndexBuffer>::Create(indexList, indexCount * sizeof(uint32_t)); // DX11Renderer

	Log::GetLogger()->info("DX11Mesh '{0}' successfully created!", inputfile);
}

const DX11MaterialSlot DX11Mesh::GetMaterialSlot(uint32_t slot)
{
	if (slot >= m_MaterialSlots.size()) return DX11MaterialSlot();
	return m_MaterialSlots[slot];
}

size_t DX11Mesh::GetNumMaterialSlots()
{
	return m_MaterialSlots.size();
}

void DX11Mesh::ComputeTangents(
	const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
	const glm::vec2& t0, const glm::vec2& t1, const glm::vec2& t2,
	glm::vec3& tangent, glm::vec3& binormal)
{
	glm::vec3 deltaPos1 = v1 - v0;
	glm::vec3 deltaPos2 = v2 - v0;

	glm::vec2 deltaUV1 = t1 - t0;
	glm::vec2 deltaUV2 = t2 - t0;

	float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
	tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y);
	tangent = glm::normalize(tangent);
	binormal = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x);
	binormal = glm::normalize(binormal);
}

DX11Mesh::~DX11Mesh()
{
}
