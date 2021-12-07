#include "ImGuiWrapper.h"

#include "Core/Application.h"
#include "EnvMap/EnvMapEditorLayer.h"

#include "../../ImGuizmo/ImGuizmo.h"

#include <cwchar>


bool ImGuiWrapper::s_ViewportEnabled = false;
bool ImGuiWrapper::s_ViewportHovered = true;
bool ImGuiWrapper::s_ViewportFocused = true;
bool ImGuiWrapper::s_CanViewportReceiveEvents = true;


bool ImGuiWrapper::DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue, float columnWidth)
{
	ImGuiIO& io = ImGui::GetIO();
	auto boldFont = io.Fonts->Fonts[0];

	ImGui::PushID(label.c_str());

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	ImGui::Text(label.c_str());
	ImGui::NextColumn();

	ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

	float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
	ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	bool isChangedX = false;
	bool isChangedY = false;
	bool isChangedZ = false;

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.20f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });

	ImGui::PushFont(boldFont);
	if (ImGui::Button("X", buttonSize)) {
		values.x = resetValue;
		isChangedX = true;
	}
	ImGui::PopFont();

	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	if (ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f")) {
		isChangedX = true;
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });

	ImGui::PushFont(boldFont);
	if (ImGui::Button("Y", buttonSize)) {
		values.y = resetValue;
		isChangedY = true;
	}
	ImGui::PopFont();

	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	if (ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f")) {
		isChangedY = true;
	}
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });

	ImGui::PushFont(boldFont);
	if (ImGui::Button("Z", buttonSize)) {
		values.z = resetValue;
		isChangedZ = true;
	}
	ImGui::PopFont();

	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	if (ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f")) {
		isChangedZ = true;
	}

	ImGui::PopItemWidth();

	ImGui::PopStyleVar();

	ImGui::Columns(1);

	ImGui::PopID();

	return isChangedX || isChangedY || isChangedZ;
}

static int s_UIContextID = 0;
static uint32_t s_Counter = 0;
static char s_IDBuffer[16];

void ImGuiWrapper::PushID()
{
	ImGui::PushID(s_UIContextID++);
	s_Counter = 0;
}

void ImGuiWrapper::PopID()
{
	ImGui::PopID();
	s_UIContextID--;
}

void ImGuiWrapper::BeginPropertyGrid()
{
	PushID();
	ImGui::Columns(2);
}

bool const ImGuiWrapper::CanViewportReceiveEvents()
{
	if (!s_ViewportEnabled) {
		s_CanViewportReceiveEvents = true;
		return s_CanViewportReceiveEvents;
	}

	if (!s_ViewportFocused && !s_ViewportHovered) {
		s_CanViewportReceiveEvents = false;
	}

	if (s_ViewportHovered && (Input::IsMouseButtonPressed(MouseH2M::ButtonRight) || Input::IsMouseButtonPressed(MouseH2M::ButtonMiddle))) {
		s_CanViewportReceiveEvents = true;
	}

	if (s_ViewportFocused) {
		s_CanViewportReceiveEvents = true;
	}

	return s_CanViewportReceiveEvents;
}

void ImGuiWrapper::DrawInputText(std::string text)
{
	char buffer[256];
	memset(buffer, 0, 256);
	memcpy(buffer, text.c_str(), text.length());
	ImGui::InputText(text.c_str(), buffer, 32);
}

void ImGuiWrapper::DrawMaterialUI(H2M::RefH2M<EnvMapMaterial> material, H2M::RefH2M<H2M::Texture2D_H2M> checkerboardTexture)
{
	// Display Material UUID
	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, 70.0f);
	ImGui::SetColumnWidth(1, 200.0f);
	ImGui::Text("UUID");
	ImGui::NextColumn();
	ImGui::Text(material->GetUUID().c_str());
	ImGui::Columns(1);

	auto materialName = material->GetName();

	char buffer[256];
	memset(buffer, 0, 256);
	memcpy(buffer, materialName.c_str(), materialName.length());
	if (ImGui::InputText("##MaterialName", buffer, 256))
	{
		materialName = std::string(buffer);
		material->SetName(materialName);
	}

	// Tiling Factor
	// ImGui::SliderFloat("Tiling Factor", &material->GetTilingFactor(), 0.0f, 20.0f);
	ImGui::DragFloat("Tiling Factor", &material->GetTilingFactor(), 0.1f, 0.0f, 20.0f, "%.2f");

	{
		// Albedo
		std::string textureLabel = material->GetName() + " Albedo";
		if (ImGui::CollapsingHeader(textureLabel.c_str(), nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::OpenGL)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
				ImGui::Image(material->GetAlbedoInput().TextureMap ?
					(void*)(intptr_t)material->GetAlbedoInput().TextureMap->GetImTextureID() :
					(void*)(intptr_t)checkerboardTexture->GetImTextureID(), ImVec2(64, 64));
				ImGui::PopStyleVar();
			}
			else
			{
				DrawInputText("Albedo");
			}

			DragAndDropTarget(material->GetAlbedoInput().TextureMap, material->GetAlbedoInput().SRGB);

			if (ImGui::IsItemHovered())
			{
				if (material->GetAlbedoInput().TextureMap)
				{
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
					ImGui::TextUnformatted(material->GetAlbedoInput().TextureMap->GetPath().c_str());
					ImGui::PopTextWrapPos();

					if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::OpenGL)
					{
						ImGui::Image((void*)(intptr_t)material->GetAlbedoInput().TextureMap->GetImTextureID(), ImVec2(384, 384));
					}
					else
					{
						DrawInputText("Albedo");
					}

					ImGui::EndTooltip();
				}
				if (ImGui::IsItemClicked())
				{
					std::string filename = Application::Get()->OpenFile();
					if (filename != "")
					{
						// material->GetAlbedoInput().TextureMap = H2M::Texture2D_H2M::Create(filename, material->GetAlbedoInput().SRGB);
						// H2M::TexturePropertiesH2M textureProperties = { H2M::TextureWrapH2M::Repeat, H2M::TextureFilterH2M::Linear, true, material->GetAlbedoInput().SRGB, false, "AlbedoMap" };
						material->GetAlbedoInput().TextureMap = H2M::Texture2D_H2M::Create(filename, material->GetAlbedoInput().SRGB);
					}
				}
			}
			ImGui::SameLine();
			ImGui::BeginGroup();
			{
				std::string checkboxLabel = "Use##" + material->GetName() + "AlbedoMap";
				ImGui::Checkbox(checkboxLabel.c_str(), &material->GetAlbedoInput().UseTexture);

				std::string checkboxLabelSRGB = "sRGB##" + material->GetName() + "AlbedoMap";
				if (ImGui::Checkbox(checkboxLabelSRGB.c_str(), &material->GetAlbedoInput().SRGB))
				{
					if (material->GetAlbedoInput().TextureMap)
					{
						// H2M::TextureProperties textureProperties = { H2M::TextureWrap::Repeat, H2M::TextureFilter::Linear, true, material->GetAlbedoInput().SRGB, false, "AlbedoMap" };
						material->GetAlbedoInput().TextureMap = H2M::Texture2D_H2M::Create(material->GetAlbedoInput().TextureMap->GetPath(), material->GetAlbedoInput().SRGB);
					}
				}
			}
			ImGui::EndGroup();
			ImGui::SameLine();
			std::string colorLabel = "Color##" + material->GetName() + "Albedo";
			ImGui::ColorEdit3(colorLabel.c_str(), glm::value_ptr(material->GetAlbedoInput().Color), ImGuiColorEditFlags_NoInputs);
		}
	}
	{
		// Normals
		std::string textureLabel = material->GetName() + " Normals";
		if (ImGui::CollapsingHeader(textureLabel.c_str(), nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::OpenGL)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
				ImGui::Image(material->GetNormalInput().TextureMap ?
					(void*)(intptr_t)material->GetNormalInput().TextureMap->GetImTextureID() :
					(void*)(intptr_t)checkerboardTexture->GetImTextureID(), ImVec2(64, 64));
				ImGui::PopStyleVar();
			}
			else
			{
				DrawInputText("Normal");
			}

			DragAndDropTarget(material->GetNormalInput().TextureMap, false);

			if (ImGui::IsItemHovered())
			{
				if (material->GetNormalInput().TextureMap)
				{
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
					ImGui::TextUnformatted(material->GetNormalInput().TextureMap->GetPath().c_str());
					ImGui::PopTextWrapPos();

					if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::OpenGL)
					{
						ImGui::Image((void*)(intptr_t)material->GetNormalInput().TextureMap->GetImTextureID(), ImVec2(384, 384));
					}
					else
					{
						DrawInputText("Normal");
					}

					ImGui::EndTooltip();
				}
				if (ImGui::IsItemClicked())
				{
					std::string filename = Application::Get()->OpenFile();
					if (filename != "")
					{
						// H2M::TextureProperties textureProperties = { H2M::TextureWrap::Repeat, H2M::TextureFilter::Linear, true, false, false, "NormalMap" };
						material->GetNormalInput().TextureMap = H2M::Texture2D_H2M::Create(filename, false);
					}
				}
			}
			ImGui::SameLine();
			std::string checkboxLabel = "Use##" + material->GetName() + "NormalMap";
			ImGui::Checkbox(checkboxLabel.c_str(), &material->GetNormalInput().UseTexture);
		}
	}
	{
		// Metalness
		std::string textureLabel = material->GetName() + " Metalness";
		if (ImGui::CollapsingHeader(textureLabel.c_str(), nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::OpenGL)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
				ImGui::Image(material->GetMetalnessInput().TextureMap ?
					(void*)(intptr_t)material->GetMetalnessInput().TextureMap->GetImTextureID() :
					(void*)(intptr_t)checkerboardTexture->GetImTextureID(), ImVec2(64, 64));
				ImGui::PopStyleVar();
			}
			else
			{
				DrawInputText("Metalness");
			}

			DragAndDropTarget(material->GetMetalnessInput().TextureMap, false);

			if (ImGui::IsItemHovered())
			{
				if (material->GetMetalnessInput().TextureMap)
				{
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
					ImGui::TextUnformatted(material->GetMetalnessInput().TextureMap->GetPath().c_str());
					ImGui::PopTextWrapPos();

					if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::OpenGL)
					{
						ImGui::Image((void*)(intptr_t)material->GetMetalnessInput().TextureMap->GetImTextureID(), ImVec2(384, 384));
					}
					else
					{
						DrawInputText("Metalness");
					}

					ImGui::EndTooltip();
				}
				if (ImGui::IsItemClicked())
				{
					std::string filename = Application::Get()->OpenFile();
					if (filename != "")
					{
						// H2M::TextureProperties textureProperties = { H2M::TextureWrap::Repeat, H2M::TextureFilter::Linear, true, false, false, "MetalnessMap" };
						material->GetMetalnessInput().TextureMap = H2M::Texture2D_H2M::Create(filename, false);
					}
				}
			}
			ImGui::SameLine();
			std::string checkboxLabel = "Use##" + material->GetName() + "MetalnessMap";
			ImGui::Checkbox(checkboxLabel.c_str(), &material->GetMetalnessInput().UseTexture);
			// ImGui::SameLine();
			std::string sliderLabel = "Value##" + material->GetName() + "MetalnessInput";
			ImGui::SliderFloat(sliderLabel.c_str(), &material->GetMetalnessInput().Value, 0.0f, 1.0f);
		}
	}
	{
		// Roughness
		std::string textureLabel = material->GetName() + " Roughness";
		if (ImGui::CollapsingHeader(textureLabel.c_str(), nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::OpenGL)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
				ImGui::Image(material->GetRoughnessInput().TextureMap ?
					(void*)(intptr_t)material->GetRoughnessInput().TextureMap->GetImTextureID() :
					(void*)(intptr_t)checkerboardTexture->GetImTextureID(), ImVec2(64, 64));
				ImGui::PopStyleVar();
			}
			else
			{
				DrawInputText("Roughness");
			}

			DragAndDropTarget(material->GetRoughnessInput().TextureMap, false);

			if (ImGui::IsItemHovered())
			{
				if (material->GetRoughnessInput().TextureMap)
				{
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
					ImGui::TextUnformatted(material->GetRoughnessInput().TextureMap->GetPath().c_str());
					ImGui::PopTextWrapPos();

					if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::OpenGL)
					{
						ImGui::Image((void*)(intptr_t)material->GetRoughnessInput().TextureMap->GetImTextureID(), ImVec2(384, 384));
					}
					else
					{
						DrawInputText("Roughness");
					}

					ImGui::EndTooltip();
				}
				if (ImGui::IsItemClicked())
				{
					std::string filename = Application::Get()->OpenFile();
					if (filename != "")
					{
						// H2M::TexturePropertiesH2M textureProperties = { H2M::TextureWrapH2M::Repeat, H2M::TextureFilterH2M::Linear, true, false, false, "RoughnessMap" };
						material->GetRoughnessInput().TextureMap = H2M::Texture2D_H2M::Create(filename, false);
					}
				}
			}
			ImGui::SameLine();
			std::string checkboxLabel = "Use##" + material->GetName() + "RoughnessMap";
			ImGui::Checkbox(checkboxLabel.c_str(), &material->GetRoughnessInput().UseTexture);
			// ImGui::SameLine();
			std::string sliderLabel = "Value##" + material->GetName() + "RoughnessInput";
			ImGui::SliderFloat(sliderLabel.c_str(), &material->GetRoughnessInput().Value, 0.0f, 1.0f);
		}
	}
	{
		// AO (Ambient Occlusion)
		std::string textureLabel = material->GetName() + " AO";
		if (ImGui::CollapsingHeader(textureLabel.c_str(), nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::OpenGL)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
				ImGui::Image(material->GetAOInput().TextureMap ?
					(void*)(intptr_t)material->GetAOInput().TextureMap->GetImTextureID() :
					(void*)(intptr_t)checkerboardTexture->GetImTextureID(), ImVec2(64, 64));
				ImGui::PopStyleVar();
			}
			else
			{
				DrawInputText("AO");
			}

			DragAndDropTarget(material->GetAOInput().TextureMap, false);

			if (ImGui::IsItemHovered())
			{
				if (material->GetAOInput().TextureMap)
				{
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
					ImGui::TextUnformatted(material->GetAOInput().TextureMap->GetPath().c_str());
					ImGui::PopTextWrapPos();

					if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::OpenGL)
					{
						ImGui::Image((void*)(intptr_t)material->GetAOInput().TextureMap->GetImTextureID(), ImVec2(384, 384));
					}
					else
					{
						DrawInputText("AO");
					}

					ImGui::EndTooltip();
				}
				if (ImGui::IsItemClicked())
				{
					std::string filename = Application::Get()->OpenFile();
					if (filename != "")
					{
						// H2M::TexturePropertiesH2M textureProperties = { H2M::TextureWrapH2M::Repeat, H2M::TextureFilterH2M::Linear, true, false, false, "AOMap" };
						material->GetAOInput().TextureMap = H2M::Texture2D_H2M::Create(filename, false);
					}
				}
			}
			ImGui::SameLine();
			std::string checkboxLabel = "Use##" + material->GetName() + "AOMap";
			ImGui::Checkbox(checkboxLabel.c_str(), &material->GetAOInput().UseTexture);
			// ImGui::SameLine();
			std::string sliderLabel = "Value##" + material->GetName() + "AOInput";
			ImGui::SliderFloat(sliderLabel.c_str(), &material->GetAOInput().Value, 0.0f, 1.0f);
		}
	}
	{
		// Emissive
		std::string textureLabel = material->GetName() + " Emissive";
		if (ImGui::CollapsingHeader(textureLabel.c_str(), nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::OpenGL)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
				ImGui::Image(material->GetEmissiveInput().TextureMap ?
					(void*)(intptr_t)material->GetEmissiveInput().TextureMap->GetImTextureID() :
					(void*)(intptr_t)checkerboardTexture->GetImTextureID(), ImVec2(64, 64));
				ImGui::PopStyleVar();
			}
			else
			{
				DrawInputText("Emissive");
			}

			DragAndDropTarget(material->GetEmissiveInput().TextureMap, material->GetEmissiveInput().SRGB);

			if (ImGui::IsItemHovered())
			{
				if (material->GetEmissiveInput().TextureMap)
				{
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
					ImGui::TextUnformatted(material->GetEmissiveInput().TextureMap->GetPath().c_str());
					ImGui::PopTextWrapPos();

					if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::OpenGL)
					{
						ImGui::Image((void*)(intptr_t)material->GetEmissiveInput().TextureMap->GetImTextureID(), ImVec2(384, 384));
					}
					else
					{
						DrawInputText("Emissive");
					}

					ImGui::EndTooltip();
				}
				if (ImGui::IsItemClicked())
				{
					std::string filename = Application::Get()->OpenFile();
					if (filename != "")
					{
						// H2M::TextureProperties textureProperties = { H2M::TextureWrap::Repeat, H2M::TextureFilter::Linear, true, material->GetEmissiveInput().SRGB, false, "EmissiveMap" };
						material->GetEmissiveInput().TextureMap = H2M::Texture2D_H2M::Create(filename, material->GetEmissiveInput().SRGB);
					}
				}
			}
			ImGui::SameLine();
			ImGui::BeginGroup();
			{
				std::string checkboxLabel = "Use##" + material->GetName() + "EmissiveMap";
				ImGui::Checkbox(checkboxLabel.c_str(), &material->GetEmissiveInput().UseTexture);
				std::string checkboxLabelSRGB = "sRGB##" + material->GetName() + "EmissiveMap";
				if (ImGui::Checkbox(checkboxLabelSRGB.c_str(), &material->GetEmissiveInput().SRGB))
				{
					if (material->GetEmissiveInput().TextureMap)
					{
						// H2M::TextureProperties textureProperties = { H2M::TextureWrap::Repeat, H2M::TextureFilter::Linear, true, material->GetEmissiveInput().SRGB, false, "EmissiveMap" };
						material->GetEmissiveInput().TextureMap = H2M::Texture2D_H2M::Create(material->GetEmissiveInput().TextureMap->GetPath(), material->GetEmissiveInput().SRGB);
					}
				}
			}
			ImGui::EndGroup();
			// ImGui::SameLine();
			std::string sliderLabel = "Value##" + material->GetName() + "EmissiveInput";
			ImGui::SliderFloat(sliderLabel.c_str(), &material->GetEmissiveInput().Value, 0.0f, 1.0f);
		}
	}
	// END PBR Textures
}

void ImGuiWrapper::DragAndDropTarget(H2M::RefH2M<H2M::Texture2D_H2M>& texture, bool srgb)
{
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
		{
			std::wstring itemPath = std::wstring((const wchar_t*)payload->Data);
			size_t itemSize = payload->DataSize;
			Log::GetLogger()->debug("END DRAG & DROP FILE '{0}', size: {1}", Util::to_str(itemPath.c_str()).c_str(), itemSize);

			std::string filename(itemPath.begin(), itemPath.end());
			if (filename != "")
			{
				// H2M::TextureProperties textureProperties = { H2M::TextureWrap::Repeat, H2M::TextureFilter::Linear, true, srgb, false, "DragAndDropTarget" };
				texture = H2M::Texture2D_H2M::Create(filename, srgb);
			}
		}
		ImGui::EndDragDropTarget();
	}
}

bool ImGuiWrapper::Property(const std::string& name, bool& value)
{
	bool modified = false;

	ImGui::Text(name.c_str());
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);

	std::string id = "##" + name;
	modified = ImGui::Checkbox(id.c_str(), &value);

	ImGui::PopItemWidth();
	ImGui::NextColumn();

	return modified;
}

bool ImGuiWrapper::Property(const std::string& name, float& value, float min, float max, PropertyFlag flags)
{
	ImGui::Text(name.c_str());
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);

	std::string id = "##" + name;

	bool changed = false;
	if (flags == PropertyFlag::SliderProperty) {
		changed = ImGui::SliderFloat(id.c_str(), &value, min, max);
	}
	else {
		changed = ImGui::DragFloat(id.c_str(), &value, 1.0f, min, max);
	}

	ImGui::PopItemWidth();
	ImGui::NextColumn();

	return changed;
}

/**
 * by dtrajko: custom version of the method with an additional 'speed' parameter
 */
bool ImGuiWrapper::Property(const std::string& name, float& value, float speed, float min, float max, PropertyFlag flags)
{
	ImGui::Text(name.c_str());
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);

	std::string id = "##" + name;

	bool changed = false;
	if (flags == PropertyFlag::SliderProperty) {
		changed = ImGui::SliderFloat(id.c_str(), &value, min, max);
	}
	else {
		changed = ImGui::DragFloat(id.c_str(), &value, speed, min, max);
	}

	ImGui::PopItemWidth();
	ImGui::NextColumn();

	return changed;
}

bool ImGuiWrapper::Property(const std::string& name, glm::vec2& value, PropertyFlag flags)
{
	return Property(name, value, -1.0f, 1.0f, flags);
}

bool ImGuiWrapper::Property(const std::string& name, glm::vec2& value, float min, float max, PropertyFlag flags)
{
	ImGui::Text(name.c_str());
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);

	std::string id = "##" + name;

	bool changed = false;
	if (flags == PropertyFlag::SliderProperty) {
		changed = ImGui::SliderFloat2(id.c_str(), glm::value_ptr(value), min, max);
	}
	else {
		changed = ImGui::DragFloat2(id.c_str(), glm::value_ptr(value), 1.0f, min, max);
	}

	ImGui::PopItemWidth();
	ImGui::NextColumn();

	return changed;
}

bool ImGuiWrapper::Property(const std::string& name, glm::vec3& value, PropertyFlag flags)
{
	return Property(name, value, -1.0f, 1.0f, flags);
}

bool ImGuiWrapper::Property(const std::string& name, glm::vec3& value, float min, float max, PropertyFlag flags)
{
	return Property(name, value, 1.0f, min, max, flags);
}

bool ImGuiWrapper::Property(const std::string& name, glm::vec3& value, float speed, float min, float max, PropertyFlag flags)
{
	ImGui::Text(name.c_str());
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);

	std::string id = "##" + name;

	bool changed = false;
	if ((int)flags & (int)PropertyFlag::ColorProperty)
	{
		changed = ImGui::ColorEdit3(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
	}
	else if (flags == PropertyFlag::SliderProperty)
	{
		changed = ImGui::SliderFloat3(id.c_str(), glm::value_ptr(value), min, max);
	}
	else
	{
		changed = ImGui::DragFloat3(id.c_str(), glm::value_ptr(value), speed, min, max);
	}

	ImGui::PopItemWidth();
	ImGui::NextColumn();

	return changed;
}

bool ImGuiWrapper::Property(const std::string& name, glm::vec4& value, PropertyFlag flags)
{
	return Property(name, value, -1.0f, 1.0f, flags);
}

bool ImGuiWrapper::Property(const std::string& name, glm::vec4& value, float min, float max, PropertyFlag flags)
{
	ImGui::Text(name.c_str());
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);

	std::string id = "##" + name;

	bool changed = false;
	if ((int)flags & (int)PropertyFlag::ColorProperty)
	{
		changed = ImGui::ColorEdit4(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
	}
	else if (flags == PropertyFlag::SliderProperty)
	{
		changed = ImGui::SliderFloat4(id.c_str(), glm::value_ptr(value), min, max);
	}
	else
	{
		changed = ImGui::DragFloat4(id.c_str(), glm::value_ptr(value), 1.0f, min, max);
	}

	ImGui::PopItemWidth();
	ImGui::NextColumn();

	return changed;
}

bool ImGuiWrapper::Property(const char* label, const char* value)
{
	bool modified = false;

	ImGui::Text(label);
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);

	s_IDBuffer[0] = '#';
	s_IDBuffer[1] = '#';
	memset(s_IDBuffer + 2, 0, 14);
	// _itoa_s(s_Counter++, s_IDBuffer + 2, 16, 16);
	sprintf(s_IDBuffer + 2, "%d", s_Counter++);
	modified = ImGui::InputText(s_IDBuffer, (char*)value, 256, ImGuiInputTextFlags_ReadOnly);

	ImGui::PopItemWidth();
	ImGui::NextColumn();

	return modified;
}

bool ImGuiWrapper::Property(const char* label, int& value)
{
	bool modified = false;

	ImGui::Text(label);
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);

	s_IDBuffer[0] = '#';
	s_IDBuffer[1] = '#';
	memset(s_IDBuffer + 2, 0, 14);
	// _itoa_s(s_Counter++, s_IDBuffer + 2, 16, 16);
	sprintf(s_IDBuffer + 2, "%d", s_Counter++);
	if (ImGui::DragInt(s_IDBuffer, &value)) {
		modified = true;
	}

	ImGui::PopItemWidth();
	ImGui::NextColumn();

	return modified;
}

bool ImGuiWrapper::Property(const char* label, float& value, float delta)
{
	bool modified = false;

	ImGui::Text(label);
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);

	s_IDBuffer[0] = '#';
	s_IDBuffer[1] = '#';
	memset(s_IDBuffer + 2, 0, 14);
	// _itoa_s(s_Counter++, s_IDBuffer + 2, 16, 16);
	sprintf(s_IDBuffer + 2, "%d", s_Counter++);
	if (ImGui::DragFloat(s_IDBuffer, &value, delta))
	{
		modified = true;
	}

	ImGui::PopItemWidth();
	ImGui::NextColumn();

	return modified;
}

bool ImGuiWrapper::Property(const char* label, glm::vec2& value, float delta)
{
	bool modified = false;

	ImGui::Text(label);
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);

	s_IDBuffer[0] = '#';
	s_IDBuffer[1] = '#';
	memset(s_IDBuffer + 2, 0, 14);
	// _itoa_s(s_Counter++, s_IDBuffer + 2, 16, 16);
	sprintf(s_IDBuffer + 2, "%d", s_Counter++);
	if (ImGui::DragFloat2(s_IDBuffer, glm::value_ptr(value), delta))
	{
		modified = true;
	}

	ImGui::PopItemWidth();
	ImGui::NextColumn();

	return modified;
}
