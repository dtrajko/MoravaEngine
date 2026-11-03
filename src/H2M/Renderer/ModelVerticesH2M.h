/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>


namespace H2M
{

	struct VertexH2M
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec3 Binormal;
		glm::vec2 Texcoord;

		bool operator==(const VertexH2M& other) const
		{
			return Position == other.Position && Normal == other.Normal && Texcoord == other.Texcoord;
		}
	};

	struct AnimatedVertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec3 Binormal;
		glm::vec2 Texcoord;

		uint32_t IDs[4] = { 0, 0,0, 0 };
		float Weights[4]{ 0.0f, 0.0f, 0.0f, 0.0f };

		// The four largest weights are kept (the skinning shaders read four): false when a weight was left out, after which
		// NormalizeWeights makes the kept ones add up to 1 again
		bool AddBoneData(uint32_t BoneID, float Weight)
		{
			size_t smallest = 0;
			for (size_t i = 0; i < 4; i++)
			{
				if (Weights[i] == 0.0)
				{
					IDs[i] = BoneID;
					Weights[i] = Weight;
					return true;
				}
				if (Weights[i] < Weights[smallest])
				{
					smallest = i;
				}
			}
			if (Weight > Weights[smallest])
			{
				IDs[smallest] = BoneID;
				Weights[smallest] = Weight;
			}
			return false;
		}

		void NormalizeWeights()
		{
			const float sum = Weights[0] + Weights[1] + Weights[2] + Weights[3];
			if (sum > 0.0f)
			{
				for (float& weight : Weights)
				{
					weight /= sum;
				}
			}
		}
	};

	struct Index
	{
		uint32_t V1, V2, V3;
	};

	static_assert(sizeof(Index) == 3 * sizeof(uint32_t));
}
