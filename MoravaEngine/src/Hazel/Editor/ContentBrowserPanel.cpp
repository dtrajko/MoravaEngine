#include "Hazel/Editor/ContentBrowserPanel.h"

#include "Core/Log.h"
#include "Core/Timer.h"
#include "Core/Util.h"
#include "ImGui/ImGuiWrapper.h"
#include "Platform/DX11/DX11Texture2D.h"


namespace Hazel
{
	// Once we have projects, change this
	static const std::filesystem::path s_AssetPath = ".";

	ContentBrowserPanel::ContentBrowserPanel()
		: m_CurrentDirectory(s_AssetPath), m_CurrentDirectoryOld("")
	{
		// m_TextureDirectory = HazelTexture2D::Create("Textures/UI/directory_transparent.png", false);
		// m_TextureFile = HazelTexture2D::Create("Textures/UI/file_transparent.png", false);
		m_TextureDirectory = HazelTexture2D::Create("Textures/UI/directory_modern_transparent.png", false);
		m_TextureFile = HazelTexture2D::Create("Textures/UI/file_modern_transparent.png", false);
	}

	ContentBrowserPanel::~ContentBrowserPanel()
	{
	}

	void ContentBrowserPanel::OnImGuiRender(bool* p_open)
	{
		ImGui::Begin("Content Browser", p_open);

		ImVec2 panelSize = ImGui::GetWindowSize();

		const ImVec2 verticalSeparator = ImVec2(0.0f, 10.0f);

		/**** BEGIN Table #1 (3 columns) ****/

		ImGui::Columns(4, 0, false);
		// ImGui::AlignTextToFramePadding();

		ImGui::SetColumnWidth(0, panelSize.x - 300.0f);
		ImGui::SetColumnWidth(1, 80.0f);
		ImGui::SetColumnWidth(2, 110.0f);
		ImGui::SetColumnWidth(3, 110.0f);

		// Column #1: The breadcrumb control can go here
		// ImGui::Text("Breadcrumb Control");
		// ImGui::SameLine();
		// ImGui::Text(m_CurrentDirectory.string().c_str());
		// ImGui::SameLine();

		std::filesystem::path breadcrumbPath = s_AssetPath;
		std::filesystem::path selectedDirectoryPath = m_CurrentDirectory;
		for (auto it = m_CurrentDirectory.begin(); it != m_CurrentDirectory.end(); it++)
		{
			if (it->filename() != s_AssetPath)
			{
				breadcrumbPath /= it->filename();
			}
			if (ImGui::Button(it->filename().string().c_str()))
			{
				selectedDirectoryPath = breadcrumbPath;
			}
			ImGui::SameLine();
		}
		m_CurrentDirectory = selectedDirectoryPath;

		ImGui::NextColumn(); // goto column #2

		// Refresh button
		m_RefreshPressed = ImGui::Button("Refresh") ? true : false;

		ImGui::NextColumn(); // goto column #3

		float tableCellWidthStep = 4.0f;
		float tableCellWidthMin = 32.0f;
		float tableCellWidthMax = 192.0f;

		// This Property control takes 2 table columns (#3 and #4)
		ImGuiWrapper::Property("Thumbnail Size", m_TableCellWidth, tableCellWidthStep, tableCellWidthMin, tableCellWidthMax, PropertyFlag::DragProperty);

		if (m_TableCellWidth < tableCellWidthMin) m_TableCellWidth = tableCellWidthMin;
		if (m_TableCellWidth > tableCellWidthMax) m_TableCellWidth = tableCellWidthMax;

		/**** END Table #1 (3 columns) ****/

		// ImGui::Dummy(verticalSeparator);

		ImGui::Separator();
		ImGui::Dummy(verticalSeparator);

		/**** BEGIN Table #2 (thumbnails, variable number of columns) ****/

		const float cellMarginWidth = 8.0f * 2;

		const ImVec2 iconSize = ImVec2{ m_TableCellWidth - cellMarginWidth, m_TableCellWidth - cellMarginWidth };
		const ImVec2 iconUV0 = ImVec2(0, 0);
		const ImVec2 iconUV1 = ImVec2(1, 1);
		const int iconFramePadding = -1;
		const ImVec4 iconBgColor = ImVec4(0, 0, 0, 0);
		const ImVec4 iconTintColor = ImVec4(1, 1, 1, 1);

		uint32_t columnCount = (uint32_t)panelSize.x / (uint32_t)m_TableCellWidth;

		if (columnCount >= 1)
		{
			ImGui::Columns(columnCount, 0, false);

			for (uint32_t i = 0; i < columnCount; i++)
			{
				ImGui::SetColumnWidth(i, m_TableCellWidth);
			}
		}

		uint32_t imageButtonID = 0;
		if (m_CurrentDirectory != std::filesystem::path(s_AssetPath))
		{
			ImGui::PushID(imageButtonID++);
			ImTextureID dirIconTextureID = m_TextureDirectory->GetImTextureID();
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			if (ImGui::ImageButton(dirIconTextureID, iconSize, iconUV0, iconUV1, iconFramePadding/*, iconBgColor, iconTintColor*/))
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
				Log::GetLogger()->info("m_CurrentDirectory: '{0}'", m_CurrentDirectory.string().c_str());
			}
			ImGui::PopStyleColor();
			ImGui::PopID();

			ImGui::Text("<-");

			ImGui::Dummy(verticalSeparator);

			ImGui::NextColumn();
		}

		for (auto& directoryEntry : GetDirectoryEntriesCached(m_CurrentDirectory))
		{
			const auto& path = directoryEntry.path();
			auto relativePath = std::filesystem::relative(path, s_AssetPath);
			std::string filenameString = relativePath.filename().string();

			{
				ImGui::PushID(imageButtonID++);

				Hazel::Ref<Hazel::HazelTexture2D> iconTexture = directoryEntry.is_directory() ? m_TextureDirectory : m_TextureFile;
				ImTextureID iconTextureID = iconTexture->GetImTextureID();

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
				ImGui::ImageButton(iconTextureID, iconSize, iconUV0, iconUV1, iconFramePadding, iconBgColor, iconTintColor);

				if (ImGui::BeginDragDropSource())
				{
					std::wstring itemPath = relativePath.wstring();
					// size_t itemSize = relativePath.native().size();
					size_t itemSize = (itemPath.size() + 1) * sizeof(wchar_t);

					// Log::GetLogger()->debug("Begin drag & drop file '{0}', size: {1}", Util::to_str(itemPath.c_str()).c_str(), itemSize);

					ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPath.c_str(), itemSize, ImGuiCond_Once);

					ImGui::EndDragDropSource();
				}

				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					if (directoryEntry.is_directory())
					{
						m_CurrentDirectory /= path.filename();
					}

					/**** BEGIN LOG ****/
					std::string entryType = directoryEntry.is_directory() ? "Directory" : "File";
					std::string entryPath = directoryEntry.is_directory() ? 
						m_CurrentDirectory.string() :
						m_CurrentDirectory.string() + Util::DirectorySeparator() + filenameString;
					Log::GetLogger()->info("ImageButton ({0}) clicked: '{1}'", entryType.c_str(), entryPath.c_str());
					/**** END LOG ****/
				}
				ImGui::PopStyleColor();
				ImGui::PopID();

				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::TextUnformatted(filenameString.c_str());
					ImGui::EndTooltip();
				}

				ImGui::Text(filenameString.c_str());

				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::TextUnformatted(filenameString.c_str());
					ImGui::EndTooltip();
				}
			}

			ImGui::Dummy(verticalSeparator);

			ImGui::NextColumn();
		}

		/**** END Table #2 (thumbnails, variable number of columns) ****/

		ImGui::End();
	}

	std::vector<std::filesystem::directory_entry> ContentBrowserPanel::GetDirectoryEntriesCached(std::filesystem::path currentDirectory)
	{
		bool cacheExpired = false;

		float currentTimestamp = Timer::Get()->GetCurrentTimestamp();

		// if (currentTimestamp - m_FilesystemIO.lastTime < m_FilesystemIO.cooldown) return std::filesystem::directory_iterator(currentDirectory);
		// m_FilesystemIO.lastTime = currentTimestamp;

		cacheExpired = m_RefreshPressed;

		if (currentDirectory != m_CurrentDirectoryOld)
		{
			cacheExpired = true;
			m_CurrentDirectoryOld = currentDirectory;
		}

		// return cached data if cache is not expired
		if (!cacheExpired)
		{
			return m_DirectoryEntriesCached;
		}

		// Cache is expired, update it
		m_DirectoryEntriesCached.clear();
		for (std::filesystem::directory_entry directoryEntry : std::filesystem::directory_iterator(currentDirectory))
		{
			m_DirectoryEntriesCached.push_back(directoryEntry);
		}

		Log::GetLogger()->debug("DIRECTORY ENTRIES - UPDATING THE CACHE! Timestamp: {0}", currentTimestamp);

		return m_DirectoryEntriesCached;
	}

}
