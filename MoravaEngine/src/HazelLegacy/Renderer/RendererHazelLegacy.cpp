#include "RendererHazelLegacy.h"

#include "HazelLegacy/Renderer/RendererAPIHazelLegacy.h"


namespace Hazel
{

	void RendererHazelLegacy::RenderMesh(Ref<Pipeline> pipeline, Ref<MeshHazelLegacy> mesh, const glm::mat4& transform)
	{
		// s_RendererAPI->RenderMesh(pipeline, mesh, transform);
	}

	void RendererHazelLegacy::RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<MeshHazelLegacy> mesh, const glm::mat4& transform)
	{
		// s_RendererAPI->RenderMeshWithoutMaterial(pipeline, mesh, transform);
	}

	void RendererHazelLegacy::RenderQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material, const glm::mat4& transform)
	{
		// s_RendererAPI->RenderQuad(pipeline, material, transform);
	}

	void RendererHazelLegacy::DrawAABB(Ref<MeshHazelLegacy> mesh, const glm::mat4& transform, const glm::vec4& color)
	{
		for (Ref<SubmeshHazelLegacy> submesh : mesh->GetSubmeshes())
		{
			auto& aabb = submesh->BoundingBox;
			auto aabbTransform = transform * submesh->Transform;
			DrawAABB(aabb, aabbTransform);
		}
	}

	void RendererHazelLegacy::DrawAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color /*= glm::vec4(1.0f)*/)
	{
		glm::vec4 min = { aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f };
		glm::vec4 max = { aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f };

		glm::vec4 corners[8] =
		{
			transform * glm::vec4 { aabb.Min.x, aabb.Min.y, aabb.Max.z, 1.0f },
			transform * glm::vec4 { aabb.Min.x, aabb.Max.y, aabb.Max.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Min.y, aabb.Max.z, 1.0f },

			transform * glm::vec4 { aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f },
			transform * glm::vec4 { aabb.Min.x, aabb.Max.y, aabb.Min.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Max.y, aabb.Min.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Min.y, aabb.Min.z, 1.0f }
		};

		for (uint32_t i = 0; i < 4; i++)
			Renderer2D::DrawLine(corners[i], corners[(i + 1) % 4], color);

		for (uint32_t i = 0; i < 4; i++)
			Renderer2D::DrawLine(corners[i + 4], corners[((i + 1) % 4) + 4], color);

		for (uint32_t i = 0; i < 4; i++)
			Renderer2D::DrawLine(corners[i], corners[i + 4], color);
	}

}
