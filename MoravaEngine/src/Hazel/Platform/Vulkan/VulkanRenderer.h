#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Renderer/HazelMesh.h"
#include "Hazel/Renderer/HazelCamera.h"


namespace Hazel {

	class VulkanRenderer
	{
	public:
		static void SubmitMesh(const Ref<HazelMesh>& mesh);

		static void OnResize(uint32_t width, uint32_t height);
		static void Init();
		static void Draw(HazelCamera* camera); // TODO: there should be no parameters
	};

}
