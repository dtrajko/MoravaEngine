#pragma once

#include "Hazel/Renderer/HazelTexture.h"

// ImGui
#define _CRT_SECURE_NO_WARNINGS
#include "../ImGui/ImGui.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <filesystem>


namespace Hazel
{

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();
		~ContentBrowserPanel();

		void OnImGuiRender(bool* p_open = (bool*)0);

	private:
		std::filesystem::path m_CurrentDirectory;

		Ref<HazelTexture2D> m_TextureDirectory;
		Ref<HazelTexture2D> m_TextureFile;

	};

}
