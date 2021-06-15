#include "Hazel/Editor/ContentBrowserPanel.h"

#include "Core/Log.h"


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

		const ImVec2 iconSize = ImVec2{ 48.0f, 48.0f };
		const ImVec2 iconUV0 = ImVec2(0, 0);
		const ImVec2 iconUV1 = ImVec2(1, 1);
		const int iconFramePadding = -1;
		const ImVec4 iconBgColor = ImVec4(0, 0, 0, 0);
		const ImVec4 iconTintColor = ImVec4(1, 1, 1, 1);
		const ImVec2 verticalSeparator = ImVec2(0.0f, 10.0f);

		const uint32_t columnWidth = 64;
		ImVec2 windowSize = ImGui::GetWindowSize();
		uint32_t columns = (uint32_t)windowSize.x / columnWidth;

		ImGui::Columns(columns);

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

				ImGui::Text(filenameString.c_str());

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

				ImGui::Text(filenameString.c_str());

				//	ImGui::Image((void*)(intptr_t)m_TextureFile->GetID(), ImVec2{ 64.0f, 64.0f });
				//	if (ImGui::Button(filenameString.c_str()))
				//	{
				//	}
			}

			ImGui::Dummy(verticalSeparator);

			ImGui::NextColumn();
		}

		ImGui::End();
	}

}
