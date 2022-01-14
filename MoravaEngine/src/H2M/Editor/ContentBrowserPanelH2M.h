#pragma once

#include "H2M/Renderer/TextureH2M.h"
#include "Core/CommonStructs.h"

// ImGui
#define _CRT_SECURE_NO_WARNINGS
#include "../ImGui/ImGui.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <filesystem>


namespace H2M
{

	class ContentBrowserPanelH2M : public RefCountedH2M // TODO: ContentBrowserPanelH2M => EditorPanelH2M => RefCountedH2M
	{
	public:
		ContentBrowserPanelH2M();
		~ContentBrowserPanelH2M();

		void OnImGuiRender(bool* p_open = (bool*)0);

	private:
		std::vector<std::filesystem::directory_entry> GetDirectoryEntriesCached(std::filesystem::path currentDirectory);

	private:
		std::filesystem::path m_CurrentDirectory;
		std::filesystem::path m_CurrentDirectoryOld;

		RefH2M<Texture2D_H2M> m_TextureDirectory;
		RefH2M<Texture2D_H2M> m_TextureFile;

		float m_TableCellWidth = 48.0f;

		// EventCooldown m_FilesystemIO = { 0.0f, 0.5f };
		// std::filesystem::directory_iterator m_DirectoryIteratorCached;

		std::vector<std::filesystem::directory_entry> m_DirectoryEntriesCached;

		bool m_RefreshPressed = false;

	};

}
