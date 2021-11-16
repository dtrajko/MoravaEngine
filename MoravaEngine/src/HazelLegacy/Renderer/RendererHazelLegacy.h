#pragma once

#include "HazelLegacy/Renderer/MeshHazelLegacy.h"

#include <glm/glm.hpp>


namespace Hazel
{

	class RendererHazelLegacy
	{
	public:
		static void RenderMesh(Ref<Pipeline> pipeline, Ref<MeshHazelLegacy> mesh, const glm::mat4& transform);
		static void RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<MeshHazelLegacy> mesh, const glm::mat4& transform);
		static void RenderQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material, const glm::mat4& transform);

		static void DrawAABB(Ref<MeshHazelLegacy> mesh, const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f));
		static void DrawAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f));

	};

}
