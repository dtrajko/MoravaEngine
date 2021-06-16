#include "Hazel/Editor/ContentBrowserPanel.h"

#include "Core/Log.h"
#include "ImGui/ImGuiWrapper.h"


namespace Hazel
{
	// Once we have projects, change this
	static const std::filesystem::path s_AssetPath = ".";

	ContentBrowserPanel::ContentBrowserPanel()
		: m_CurrentDirectory(s_AssetPath)
	{
		m_TextureDirectory = HazelTexture2D::Create("Textures/UI/directory_transparent.png", false);
		m_TextureFile = HazelTexture2D::Create("Textures/UI/file_transparent.png", false);
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

		ImGui::Columns(3);
		// ImGui::AlignTextToFramePadding();

		ImGui::SetColumnWidth(0, panelSize.x - 200.0f);
		ImGui::SetColumnWidth(1, 100.0f);
		ImGui::SetColumnWidth(2, 100.0f);

		// Column #1: The breadcrumb control can go here
		// ImGui::Text("Breadcrumb Control");
		// ImGui::SameLine();
		// ImGui::Text(m_CurrentDirectory.string().c_str());
		// ImGui::SameLine();

		std::filesystem::path breadcrumbPath = m_CurrentDirectory;
		for (auto it = m_CurrentDirectory.begin(); it != m_CurrentDirectory.end(); it++)
		{
			if (ImGui::Button(it->filename().string().c_str()))
			{
				breadcrumbPath /= *it;
			}
			ImGui::SameLine();
		}
		m_CurrentDirectory = breadcrumbPath;

		ImGui::NextColumn(); // goto column #2

		float tableCellWidthStep = 4.0f;
		float tableCellWidthMin = 32.0f;
		float tableCellWidthMax = 192.0f;

		// This Property control takes 2 table columns (#2 and #3)
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

		ImGui::Columns(columnCount);

		for (uint32_t i = 0; i < columnCount; i++)
		{
			ImGui::SetColumnWidth(i, m_TableCellWidth);
		}

		uint32_t imageButtonID = 0;

		if (m_CurrentDirectory != std::filesystem::path(s_AssetPath))
		{
			ImGui::PushID(imageButtonID++);
			if (ImGui::ImageButton((void*)(intptr_t)m_TextureDirectory->GetID(), iconSize, iconUV0, iconUV1, iconFramePadding, iconBgColor, iconTintColor))
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
				Log::GetLogger()->info("m_CurrentDirectory: '{0}'", m_CurrentDirectory.string().c_str());
			}
			ImGui::PopID();

			ImGui::Text("<-");

			//	ImGui::Image((void*)(intptr_t)m_TextureDirectory->GetID(), ImVec2{ 64.0f, 64.0f });
			//	if (ImGui::Button("<-"))
			//	{
			//		m_CurrentDirectory = m_CurrentDirectory.parent_path();
			//		Log::GetLogger()->info("m_CurrentDirectory: '{0}'", m_CurrentDirectory.string().c_str());
			//	}

			ImGui::Dummy(verticalSeparator);

			ImGui::NextColumn();
		}

		for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
		{
			const auto& path = directoryEntry.path();
			auto relativePath = std::filesystem::relative(path, s_AssetPath);
			std::string filenameString = relativePath.filename().string();

			if (directoryEntry.is_directory())
			{
				ImGui::PushID(imageButtonID++);
				if (ImGui::ImageButton((void*)(intptr_t)m_TextureDirectory->GetID(), iconSize, iconUV0, iconUV1, iconFramePadding, iconBgColor, iconTintColor))
				{
					m_CurrentDirectory /= path.filename();
					Log::GetLogger()->info("ImageButton (Directory) clicked: '{0}'", m_CurrentDirectory.string().c_str());
				}
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

				//	ImGui::Image((void*)(intptr_t)m_TextureDirectory->GetID(), ImVec2{ 64.0f, 64.0f });
				//	if (ImGui::Button(filenameString.c_str()))
				//	{
				//		m_CurrentDirectory /= path.filename();
				//		Log::GetLogger()->info("Button (Directory) clicked: '{0}'", m_CurrentDirectory.string().c_str());
				//	}
			}
			else
			{
				ImGui::PushID(imageButtonID++);
				if (ImGui::ImageButton((void*)(intptr_t)m_TextureFile->GetID(), iconSize, iconUV0, iconUV1, iconFramePadding, iconBgColor, iconTintColor))
				{
					Log::GetLogger()->info("ImageButton (File) clicked: '{0}'", filenameString.c_str());
				}
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

				//	ImGui::Image((void*)(intptr_t)m_TextureFile->GetID(), ImVec2{ 64.0f, 64.0f });
				//	if (ImGui::Button(filenameString.c_str()))
				//	{
				//	}
			}

			ImGui::Dummy(verticalSeparator);

			ImGui::NextColumn();
		}

		/**** END Table #2 (thumbnails, variable number of columns) ****/

		ImGui::End();
	}

}
