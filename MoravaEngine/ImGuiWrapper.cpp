#include "ImGuiWrapper.h"

#include "Application.h"
#include "EnvMapEditorLayer.h"

#include "../vendor/cross-platform/ImGuizmo/ImGuizmo.h"


Window* ImGuiWrapper::s_Window;
float ImGuiWrapper::s_Time;
std::string  ImGuiWrapper::s_MaterialNameNew;

void ImGuiWrapper::Init(Window* window)
{
	s_Window = window;

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

	// Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
	io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
	io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
	io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
	io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
	io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
	io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
	io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
	io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
	io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
	io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
	io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
	io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
	io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
	io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
	io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
	io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
	io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

	io.Fonts->AddFontFromFileTTF("Fonts/opensans/OpenSans-Bold.ttf", 16.0f);
	io.FontDefault = io.Fonts->AddFontFromFileTTF("Fonts/opensans/OpenSans-Regular.ttf", 16.0f);

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window->GetHandle(), true);
	ImGui_ImplOpenGL3_Init("#version 330");
}

void ImGuiWrapper::Begin()
{
	ImGuiIO& io = ImGui::GetIO();

	float time = (float)glfwGetTime();
	io.DeltaTime = s_Time > 0.0f ? (time - s_Time) : (1.0f / 60.0f);
	s_Time = time;

	// ImGui Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGuizmo::BeginFrame();
}

void ImGuiWrapper::End()
{
	// ImGui Rendering
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)s_Window->GetWidth(), (float)s_Window->GetHeight());

	// Rendering
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}

}

void ImGuiWrapper::Cleanup()
{
	// ImGui Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

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

void ImGuiWrapper::DrawMaterialUI(EnvMapMaterial* material, Hazel::Ref<Hazel::HazelTexture2D> checkerboardTexture)
{
	// Display Material UUID
	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, 70.0f);
	ImGui::SetColumnWidth(1, 200.0f);
	ImGui::Text("UUID");
	ImGui::NextColumn();
	ImGui::Text(material->GetUUID().c_str());
	ImGui::Columns(1);

	// Rename material
	std::string materialNameOld = material->GetName();

	char buffer[256];
	memset(buffer, 0, 256);
	memcpy(buffer, material->GetName().c_str(), material->GetName().length());
	if (ImGui::InputText("##MaterialName", buffer, 256))
	{
		s_MaterialNameNew = std::string(buffer);
	}

	ImGui::SameLine();

	std::string buttonName = "Rename";
	if (ImGui::Button(buttonName.c_str())) {
		if (s_MaterialNameNew != materialNameOld) {
			EnvMapEditorLayer::RenameMaterial(material, s_MaterialNameNew);
			s_MaterialNameNew = "";
		}
	}

	// Tiling Factor
	// ImGui::SliderFloat("Tiling Factor", &material->GetTilingFactor(), 0.0f, 20.0f);
	ImGui::DragFloat("Tiling Factor", &material->GetTilingFactor(), 0.1f, 0.0f, 20.0f, "%.2f");

	{
		// Albedo
		std::string textureLabel = material->GetName() + " Albedo";
		if (ImGui::CollapsingHeader(textureLabel.c_str(), nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
			ImGui::Image(material->GetAlbedoInput().TextureMap ?
				(void*)(intptr_t)material->GetAlbedoInput().TextureMap->GetID() :
				(void*)(intptr_t)checkerboardTexture->GetID(), ImVec2(64, 64));
			ImGui::PopStyleVar();
			if (ImGui::IsItemHovered())
			{
				if (material->GetAlbedoInput().TextureMap)
				{
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
					ImGui::TextUnformatted(material->GetAlbedoInput().TextureMap->GetPath().c_str());
					ImGui::PopTextWrapPos();
					ImGui::Image((void*)(intptr_t)material->GetAlbedoInput().TextureMap->GetID(), ImVec2(384, 384));
					ImGui::EndTooltip();
				}
				if (ImGui::IsItemClicked())
				{
					std::string filename = Application::Get()->OpenFile();
					if (filename != "")
						material->GetAlbedoInput().TextureMap = Hazel::HazelTexture2D::Create(filename, material->GetAlbedoInput().SRGB, Hazel::HazelTextureWrap::Repeat);
				}
			}
			ImGui::SameLine();
			ImGui::BeginGroup();

			std::string checkboxLabel = "Use##" + material->GetName() + "AlbedoMap";
			ImGui::Checkbox(checkboxLabel.c_str(), &material->GetAlbedoInput().UseTexture);

			std::string checkboxLabelSRGB = "sRGB##" + material->GetName() + "AlbedoMap";
			if (ImGui::Checkbox(checkboxLabelSRGB.c_str(), &material->GetAlbedoInput().SRGB))
			{
				if (material->GetAlbedoInput().TextureMap)
					material->GetAlbedoInput().TextureMap = Hazel::HazelTexture2D::Create(
						material->GetAlbedoInput().TextureMap->GetPath(),
						material->GetAlbedoInput().SRGB,
						Hazel::HazelTextureWrap::Repeat);
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
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
			ImGui::Image(material->GetNormalInput().TextureMap ?
				(void*)(intptr_t)material->GetNormalInput().TextureMap->GetID() :
				(void*)(intptr_t)checkerboardTexture->GetID(), ImVec2(64, 64));
			ImGui::PopStyleVar();
			if (ImGui::IsItemHovered())
			{
				if (material->GetNormalInput().TextureMap)
				{
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
					ImGui::TextUnformatted(material->GetNormalInput().TextureMap->GetPath().c_str());
					ImGui::PopTextWrapPos();
					ImGui::Image((void*)(intptr_t)material->GetNormalInput().TextureMap->GetID(), ImVec2(384, 384));
					ImGui::EndTooltip();
				}
				if (ImGui::IsItemClicked())
				{
					std::string filename = Application::Get()->OpenFile();
					if (filename != "")
						material->GetNormalInput().TextureMap = Hazel::HazelTexture2D::Create(filename, false, Hazel::HazelTextureWrap::Repeat);
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
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
			ImGui::Image(material->GetMetalnessInput().TextureMap ?
				(void*)(intptr_t)material->GetMetalnessInput().TextureMap->GetID() :
				(void*)(intptr_t)checkerboardTexture->GetID(), ImVec2(64, 64));
			ImGui::PopStyleVar();
			if (ImGui::IsItemHovered())
			{
				if (material->GetMetalnessInput().TextureMap)
				{
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
					ImGui::TextUnformatted(material->GetMetalnessInput().TextureMap->GetPath().c_str());
					ImGui::PopTextWrapPos();
					ImGui::Image((void*)(intptr_t)material->GetMetalnessInput().TextureMap->GetID(), ImVec2(384, 384));
					ImGui::EndTooltip();
				}
				if (ImGui::IsItemClicked())
				{
					std::string filename = Application::Get()->OpenFile();
					if (filename != "")
						material->GetMetalnessInput().TextureMap = Hazel::HazelTexture2D::Create(filename, false, Hazel::HazelTextureWrap::Repeat);
				}
			}
			ImGui::SameLine();
			std::string checkboxLabel = "Use##" + material->GetName() + "MetalnessMap";
			ImGui::Checkbox(checkboxLabel.c_str(), &material->GetMetalnessInput().UseTexture);
			ImGui::SameLine();
			std::string sliderLabel = "Value##" + material->GetName() + "MetalnessInput";
			ImGui::SliderFloat(sliderLabel.c_str(), &material->GetMetalnessInput().Value, 0.0f, 1.0f);
		}
	}
	{
		// Roughness
		std::string textureLabel = material->GetName() + " Roughness";
		if (ImGui::CollapsingHeader(textureLabel.c_str(), nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
			ImGui::Image(material->GetRoughnessInput().TextureMap ?
				(void*)(intptr_t)material->GetRoughnessInput().TextureMap->GetID() :
				(void*)(intptr_t)checkerboardTexture->GetID(), ImVec2(64, 64));
			ImGui::PopStyleVar();
			if (ImGui::IsItemHovered())
			{
				if (material->GetRoughnessInput().TextureMap)
				{
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
					ImGui::TextUnformatted(material->GetRoughnessInput().TextureMap->GetPath().c_str());
					ImGui::PopTextWrapPos();
					ImGui::Image((void*)(intptr_t)material->GetRoughnessInput().TextureMap->GetID(), ImVec2(384, 384));
					ImGui::EndTooltip();
				}
				if (ImGui::IsItemClicked())
				{
					std::string filename = Application::Get()->OpenFile();
					if (filename != "")
						material->GetRoughnessInput().TextureMap = Hazel::HazelTexture2D::Create(filename, false, Hazel::HazelTextureWrap::Repeat);
				}
			}
			ImGui::SameLine();
			std::string checkboxLabel = "Use##" + material->GetName() + "RoughnessMap";
			ImGui::Checkbox(checkboxLabel.c_str(), &material->GetRoughnessInput().UseTexture);
			ImGui::SameLine();
			std::string sliderLabel = "Value##" + material->GetName() + "RoughnessInput";
			ImGui::SliderFloat(sliderLabel.c_str(), &material->GetRoughnessInput().Value, 0.0f, 1.0f);
		}
	}
	{
		// Emissive
		std::string textureLabel = material->GetName() + " Emissive";
		if (ImGui::CollapsingHeader(textureLabel.c_str(), nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
			ImGui::Image(material->GetEmissiveInput().TextureMap ?
				(void*)(intptr_t)material->GetEmissiveInput().TextureMap->GetID() :
				(void*)(intptr_t)checkerboardTexture->GetID(), ImVec2(64, 64));
			ImGui::PopStyleVar();
			if (ImGui::IsItemHovered())
			{
				if (material->GetEmissiveInput().TextureMap)
				{
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
					ImGui::TextUnformatted(material->GetEmissiveInput().TextureMap->GetPath().c_str());
					ImGui::PopTextWrapPos();
					ImGui::Image((void*)(intptr_t)material->GetEmissiveInput().TextureMap->GetID(), ImVec2(384, 384));
					ImGui::EndTooltip();
				}
				if (ImGui::IsItemClicked())
				{
					std::string filename = Application::Get()->OpenFile();
					if (filename != "")
						material->GetEmissiveInput().TextureMap = Hazel::HazelTexture2D::Create(filename, material->GetEmissiveInput().SRGB, Hazel::HazelTextureWrap::Repeat);
				}
			}
			ImGui::SameLine();
			ImGui::BeginGroup();

			std::string checkboxLabel = "Use##" + material->GetName() + "EmissiveMap";
			ImGui::Checkbox(checkboxLabel.c_str(), &material->GetEmissiveInput().UseTexture);
			ImGui::SameLine();
			std::string sliderLabel = "Value##" + material->GetName() + "EmissiveInput";
			ImGui::SliderFloat(sliderLabel.c_str(), &material->GetEmissiveInput().Value, 0.0f, 1.0f);

			std::string checkboxLabelSRGB = "sRGB##" + material->GetName() + "EmissiveMap";
			if (ImGui::Checkbox(checkboxLabelSRGB.c_str(), &material->GetEmissiveInput().SRGB))
			{
				if (material->GetEmissiveInput().TextureMap)
					material->GetEmissiveInput().TextureMap = Hazel::HazelTexture2D::Create(
						material->GetEmissiveInput().TextureMap->GetPath(),
						material->GetEmissiveInput().SRGB,
						Hazel::HazelTextureWrap::Repeat);
			}
			ImGui::EndGroup();
		}
	}
	{
		// AO (Ambient Occlusion)
		std::string textureLabel = material->GetName() + " AO";
		if (ImGui::CollapsingHeader(textureLabel.c_str(), nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
			ImGui::Image(material->GetAOInput().TextureMap ?
				(void*)(intptr_t)material->GetAOInput().TextureMap->GetID() :
				(void*)(intptr_t)checkerboardTexture->GetID(), ImVec2(64, 64));
			ImGui::PopStyleVar();
			if (ImGui::IsItemHovered())
			{
				if (material->GetAOInput().TextureMap)
				{
					ImGui::BeginTooltip();
					ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
					ImGui::TextUnformatted(material->GetAOInput().TextureMap->GetPath().c_str());
					ImGui::PopTextWrapPos();
					ImGui::Image((void*)(intptr_t)material->GetAOInput().TextureMap->GetID(), ImVec2(384, 384));
					ImGui::EndTooltip();
				}
				if (ImGui::IsItemClicked())
				{
					std::string filename = Application::Get()->OpenFile();
					if (filename != "")
						material->GetAOInput().TextureMap = Hazel::HazelTexture2D::Create(filename, false, Hazel::HazelTextureWrap::Repeat);
				}
			}
			ImGui::SameLine();
			std::string checkboxLabel = "Use##" + material->GetName() + "AOMap";
			ImGui::Checkbox(checkboxLabel.c_str(), &material->GetAOInput().UseTexture);
			ImGui::SameLine();
			std::string sliderLabel = "Value##" + material->GetName() + "AOInput";
			ImGui::SliderFloat(sliderLabel.c_str(), &material->GetAOInput().Value, 0.0f, 1.0f);
		}
	}
	// END PBR Textures
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
	ImGui::Text(name.c_str());
	ImGui::NextColumn();
	ImGui::PushItemWidth(-1);

	std::string id = "##" + name;

	bool changed = false;
	if ((int)flags & (int)PropertyFlag::ColorProperty)
		changed = ImGui::ColorEdit3(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
	else if (flags == PropertyFlag::SliderProperty) {
		changed = ImGui::SliderFloat3(id.c_str(), glm::value_ptr(value), min, max);
	}
	else {
		changed = ImGui::DragFloat3(id.c_str(), glm::value_ptr(value), 1.0f, min, max);
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
		changed = ImGui::ColorEdit4(id.c_str(), glm::value_ptr(value), ImGuiColorEditFlags_NoInputs);
	else if (flags == PropertyFlag::SliderProperty) {
		changed = ImGui::SliderFloat4(id.c_str(), glm::value_ptr(value), min, max);
	}
	else {
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
	_itoa_s(s_Counter++, s_IDBuffer + 2, 16, 16);
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
	_itoa_s(s_Counter++, s_IDBuffer + 2, 16, 16);
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
	_itoa_s(s_Counter++, s_IDBuffer + 2, 16, 16);
	if (ImGui::DragFloat(s_IDBuffer, &value, delta)) {
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
	_itoa_s(s_Counter++, s_IDBuffer + 2, 16, 16);
	if (ImGui::DragFloat2(s_IDBuffer, glm::value_ptr(value), delta)) {
		modified = true;
	}

	ImGui::PopItemWidth();
	ImGui::NextColumn();

	return modified;
}
