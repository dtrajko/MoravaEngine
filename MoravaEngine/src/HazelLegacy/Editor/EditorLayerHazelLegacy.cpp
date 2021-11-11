#include "EditorLayerHazelLegacy.h"

#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Editor/AssetEditorPanel.h"
#include "Hazel/Audio/Editor/AudioEventsEditor.h"
#include "Hazel/ImGui/Colours.h"
#include "Hazel/ImGui/ImGuiUtilities.h"
#include "Hazel/Project/Project.h"
#include "Hazel/Utilities/FileSystem.h"
#include "Hazel/Script/ScriptEngine.h"
#include "Hazel/Audio/AudioEngine.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "ImGuizmo.h"


namespace Hazel
{

#define MAX_PROJECT_NAME_LENGTH 255
#define MAX_PROJECT_FILEPATH_LENGTH 512

	static char* s_ProjectNameBuffer = new char[MAX_PROJECT_NAME_LENGTH];
	static char* s_ProjectFilePathBuffer = new char[MAX_PROJECT_FILEPATH_LENGTH];


	EditorLayerHazelLegacy::EditorLayerHazelLegacy(const Ref<UserPreferences>& userPreferences)
		: m_UserPreferences(userPreferences)
	{
		Log::GetLogger()->info("EditorLayerHazelLegacy::EditorLayerHazelLegacy method called!");
	}

	EditorLayerHazelLegacy::~EditorLayerHazelLegacy()
	{
		Log::GetLogger()->info("EditorLayerHazelLegacy::~EditorLayerHazelLegacy method called!");
	}

	auto operator < (const ImVec2& lhs, const ImVec2 rhs)
	{
		return lhs.x < rhs.x&& lhs.y < rhs.y;
	}

	static std::string s_HazelInstallPath = "";

	void EditorLayerHazelLegacy::OnAttach()
	{
		Log::GetLogger()->info("EditorLayerHazelLegacy::OnAttach method called!");

		// using namespace glm;

		// Editor
		TextureProperties clampProps;
		clampProps.SamplerWrap = TextureWrap::Clamp;

		m_CheckerboardTex   = HazelTexture2D::Create("Resources/Editor/Checkerboard.tga");
		m_PlayButtonTex     = HazelTexture2D::Create("Resources/Editor/PlayButton.png");
		m_PauseButtonTex    = HazelTexture2D::Create("Resources/Editor/PauseButton.png");
		m_StopButtonTex     = HazelTexture2D::Create("Resources/Editor/StopButton.png");
		m_SimulateButtonTex = HazelTexture2D::Create("Resources/Editor/SimulateButton.png");
		m_SelectToolTex     = HazelTexture2D::Create("Resources/Editor/icon_pointer.png", clampProps); // TODO: implement clampProps
		m_MoveToolTex       = HazelTexture2D::Create("Resources/Editor/MoveTool.png");
		m_RotateToolTex     = HazelTexture2D::Create("Resources/Editor/icon_rotate.png");
		m_ScaleToolTex      = HazelTexture2D::Create("Resources/Editor/ScaleTool.png", clampProps); // TODO: implement clampProps

		m_LogoTex      = HazelTexture2D::Create("Resources/Editor/H_logo.png", clampProps);
		m_IconMinimize = HazelTexture2D::Create("Resources/Editor/window_minimize.png", clampProps);
		m_IconMaximize = HazelTexture2D::Create("Resources/Editor/window_maximize.png", clampProps);
		m_IconRestore  = HazelTexture2D::Create("Resources/Editor/window_restore.png", clampProps);
		m_IconClose    = HazelTexture2D::Create("Resources/Editor/window_close.png", clampProps);

		m_PointLightIcon = HazelTexture2D::Create("Resources/Editor/Icons/PointLight.png");

		m_SceneHierarchyPanel = CreateScope<SceneHierarchyPanel>(m_EditorScene);
		m_SceneHierarchyPanel->SetSelectionChangedCallback(std::bind(&EditorLayerHazelLegacy::SelectEntity, this, std::placeholders::_1));
		m_SceneHierarchyPanel->SetEntityDeletedCallback(std::bind(&EditorLayerHazelLegacy::OnEntityDeleted, this, std::placeholders::_1));
		m_SceneHierarchyPanel->SetMeshAssetConvertCallback(std::bind(&EditorLayerHazelLegacy::OnCreateMeshFromMeshAsset, this, std::placeholders::_1, std::placeholders::_2));
		m_SceneHierarchyPanel->SetInvalidMetadataCallback(std::bind(&EditorLayerHazelLegacy::SceneHierarchyInvalidMetadataCallback, this, std::placeholders::_1, std::placeholders::_2));

		m_PrefabHierarchyPanel = CreateScope<SceneHierarchyPanel>();

		m_Renderer2D = Ref<Renderer2D>::Create();

		if (!m_UserPreferences->StartupProject.empty())
		{
			OpenProject(m_UserPreferences->StartupProject);
		}
		else
		{
			// HZ_CORE_VERIFY(false, "No project provided!");
			Log::GetLogger()->warn("No project provided!");
		}

		m_ConsolePanel = CreateScope<EditorConsolePanel>();
		m_ViewportRenderer = Ref<SceneRendererHazelLegacy>::Create(m_CurrentScene);
		m_SecondViewportRenderer = Ref<SceneRendererHazelLegacy>::Create(m_CurrentScene);
		m_FocusedRenderer = m_ViewportRenderer;

		AssetEditorPanel::RegisterDefaultEditors(); // TODO: implement the method

		m_Renderer2D->SetLineWidth(m_LineWidth);
		m_ViewportRenderer->SetLineWidth(m_LineWidth); // TODO: implement the method
		UpdateSceneRendererSettings();
		AudioEventsEditor::Init(); // TODO: implement this class and method

		s_HazelInstallPath = FileSystem::GetEnvironmentVariable("HAZEL_DIR");
	}

	void EditorLayerHazelLegacy::OnDetach()
	{
		Log::GetLogger()->info("EditorLayerHazelLegacy::OnAttach method called!");

		AudioEventsEditor::Shutdown(); // TODO: implement method
		CloseProject(false);
		FileSystem::StopWatching(); // TODO: implement method
		AssetEditorPanel::UnregisterAllEditors(); // TODO implement method
	}

	void EditorLayerHazelLegacy::OnScenePlay()
	{
		m_SelectionContext.clear();

		m_SceneState = SceneState::Play;
		UI::SetMouseEnabled(true);
		Input::SetCursorMode(CursorMode::Normal);

		m_ConsolePanel->OnScenePlay();

		if (Project::GetActive()->GetConfig().ReloadAssemblyOnPlay)
		{
			ScriptEngine::ReloadAssembly((Project::GetScriptModuleFilePath()).string());

			m_RuntimeScene = Ref<HazelScene>::Create();
			m_EditorScene->CopyTo(m_RuntimeScene);

			m_RuntimeScene->OnRuntimeStart();
			m_SceneHierarchyPanel->SetContext(m_RuntimeScene);
			m_CurrentScene = m_RuntimeScene;
		}
	}

	void EditorLayerHazelLegacy::OnSceneStop()
	{
		m_RuntimeScene->OnRuntimeStop();
		m_SceneState = SceneState::Edit;
		Input::SetCursorMode(CursorMode::Normal);
		UI::SetMouseEnabled(true);

		// Unload runtime scene
		m_RuntimeScene = nullptr;

		m_SelectionContext.clear();
		ScriptEngine::SetSceneContext(m_EditorScene);
		MiniAudioEngine::SetSceneContext(m_EditorScene);
		m_SceneHierarchyPanel->SetContext(m_EditorScene);
		m_CurrentScene = m_EditorScene;
	}

	void EditorLayerHazelLegacy::OnSceneStartSimulation()
	{
		m_SelectionContext.clear();

		m_SceneState = SceneState::Simulate;

		m_SimulationScene = Ref<HazelScene>::Create();
		m_EditorScene->CopyTo(m_SimulationScene);

		m_SimulationScene->OnSimulationStart();
		m_SceneHierarchyPanel->SetContext(m_SimulationScene);
		m_CurrentScene = m_SimulationScene;
	}

	void EditorLayerHazelLegacy::OnSceneStopSimulation()
	{
		m_SimulationScene->OnSimulationEnd();
		m_SceneState = SceneState::Edit;

		m_SimulationScene = nullptr;

		m_SelectionContext.clear();
		ScriptEngine::SetSceneContext(m_EditorScene);
		m_SceneHierarchyPanel->SetContext(m_EditorScene);
		m_CurrentScene = m_EditorScene;
	}

	void EditorLayerHazelLegacy::UpdateWindowTitle(const std::string& sceneName)
	{
		std::string rendererAPI = "";
		
		switch (RendererAPI::Current())
		{
			case RendererAPIType::OpenGL:
				rendererAPI = "OpenGL";
				break;
			case RendererAPIType::Vulkan:
				rendererAPI = "Vulkan";
				break;
			case RendererAPIType::DX11:
				rendererAPI = "DirectX 11";
				break;
		}

		std::string title = fmt::format("{0} ({1}) - MoravaEngine - {2} ({3}) Renderer: {4}", sceneName, 
			Project::GetActive()->GetConfig().Name, Application::GetPlatformName(), Application::GetConfigurationName(), rendererAPI);
		Application::Get()->GetWindow()->SetTitle(title);
	}

	void EditorLayerHazelLegacy::UI_DrawMenubar()
	{
		const ImRect menuBarRect = { ImGui::GetCursorPos(), {ImGui::GetContentRegionAvail().x, ImGui::GetFrameHeightWithSpacing()} };

		ImGui::BeginGroup();

		if (UI::BeginMenuBar(menuBarRect))
		{
			bool menuOpen = false; // ImGui::IsPopupOpen("##menubar", ImGuiPopupFlags_AnyPopupId); // TODO: find the source code

			if (menuOpen)
			{
				const ImU32 colActive = UI::ColourWithSaturation(Colours::Theme::accent, 0.5f); // TODO: locate ColourWithSaturation
				ImGui::PushStyleColor(ImGuiCol_Header, colActive);
				ImGui::PushStyleColor(ImGuiCol_HeaderHovered, colActive);
			}

			auto popItemHighlight = [&menuOpen]
			{
				if (menuOpen)
				{
					ImGui::PopStyleColor(3);
					menuOpen = false;
				}
			};

			auto pushDarkTextIfActive = [](const char* menuName)
			{
				if (ImGui::IsPopupOpen(menuName))
				{
					ImGui::PushStyleColor(ImGuiCol_Text, Colours::Theme::backgroundDark);
					return true;
				}
				return false;
			};

			const ImU32 colHovered = IM_COL32(0, 0, 0, 80);

			{
				bool colourPushed = pushDarkTextIfActive("File");

				if (ImGui::BeginMenu("File"))
				{
					popItemHighlight();
					colourPushed = false;

					ImGui::PushStyleColor(ImGuiCol_HeaderHovered, colHovered);

					if (ImGui::MenuItem("Create Project..."))
					{
						m_ShowCreateNewProjectPopup = true;
					}
					if (ImGui::MenuItem("Save Project"))
					{
						SaveProject();
					}
					if (ImGui::MenuItem("Open Project...", "Ctrl+0"))
					{
						OpenProject();
					}
					if (ImGui::BeginMenu("Open Recent"))
					{
						size_t i = 0;
						for (auto it = m_UserPreferences->RecentProjects.begin(); it != m_UserPreferences->RecentProjects.end(); it++)
						{
							if (i > 10) break;

							if (ImGui::MenuItem(it->second.Name.c_str()))
							{
								// stash filepath away and defer actual opening of project until it is "safe" to do so
								strcpy(s_ProjectFilePathBuffer, it->second.FilePath.data());

								RecentProject projectEntry;
								projectEntry.Name = it->second.Name;
								projectEntry.FilePath = it->second.FilePath;
								projectEntry.LastOpened = time(NULL);

								it = m_UserPreferences->RecentProjects.erase(it);

								m_UserPreferences->RecentProjects[projectEntry.LastOpened] = projectEntry;

								UserPreferencesSerializer preferencesSerializer(m_UserPreferences);
								preferencesSerializer.Serialize(m_UserPreferences->FilePath);

								break;
							}

							i++;
						}
						ImGui::EndMenu();
					}

					ImGui::Separator();

					if (ImGui::MenuItem("New Scene", "Ctrl+N"))
					{
						m_ShowNewScenePopup = true;
					}
					if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
					{
						SaveScene();
					}
					if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
					{
						SaveSceneAs();
					}

					ImGui::Separator();

					if (ImGui::MenuItem("Exit", "Alt + F4"))
					{
						// Application::Get()->Close(); // TODO: Implement method
					}

					ImGui::PopStyleColor();
					ImGui::EndMenu();
				}

				if (colourPushed)
				{
					ImGui::PopStyleColor();
				}
			}

			{
				bool colourPushed = pushDarkTextIfActive("Edit");

				if (ImGui::BeginMenu("Edit"))
				{
					popItemHighlight();
					colourPushed = false;

					ImGui::PushStyleColor(ImGuiCol_HeaderHovered, colHovered);

					ImGui::MenuItem("Project Settings", nullptr, &m_ShowProjectSettings);
					ImGui::MenuItem("Second Viewport", nullptr, &m_ShowSecondViewport);
					if (ImGui::MenuItem("Reload C# Assembly"))
						ScriptEngine::ReloadAssembly((Project::GetScriptModuleFilePath()).string());

					ImGui::PopStyleColor();
					ImGui::EndMenu();
				}

				if (colourPushed)
				{
					ImGui::PopStyleColor();
				}
			}

			{
				bool colourPushed = pushDarkTextIfActive("View");

				if (ImGui::BeginMenu("View"))
				{
					popItemHighlight();
					colourPushed = false;

					ImGui::PushStyleColor(ImGuiCol_HeaderHovered, colHovered);

					ImGui::MenuItem("Audio Events Editor", nullptr, &m_ShowAudioEventsEditor);
					ImGui::MenuItem("Asset Manager", nullptr, &m_AssetManagerPanelOpen);

					ImGui::PopStyleColor();
					ImGui::EndMenu();
				}

				if (colourPushed)
				{
					ImGui::PopStyleColor();
				}
			}

#if 0 // #ifdef HZ_DEBUG
			{
				bool colourPushed = pushDarkTextIfActive("Debug");

				if (ImGui::BeginMenu("Debug"))
				{
					popItemHighlight();
					colourPushed = false;

					ImGui::PushStyleColor(ImGuiCol_HeaderHovered, colHovered);

					if (PhysXDebugger::IsDebugging())
					{
						if (ImGui::MenuItem("Stop PhysX Debugging"))
							PhysXDebugger::StopDebugging();
					}
					else
					{
						if (ImGui::MenuItem("Start PhysX Debugging"))
							PhysXDebugger::StartDebugging((Project::GetActive()->GetProjectDirectory() / "PhysXDebugInfo").string(), Physics::GetSettings().DebugType == DebugType::LiveDebug);
					}

					ImGui::PopStyleColor();
					ImGui::EndMenu();
				}

				if (colourPushed)
					ImGui::PopStyleColor();
			}
#endif

			{
				bool colourPushed = pushDarkTextIfActive("Help");

				if (ImGui::BeginMenu("Help"))
				{
					popItemHighlight();
					colourPushed = false;

					ImGui::PushStyleColor(ImGuiCol_HeaderHovered, colHovered);

					if (ImGui::MenuItem("About"))
						m_ShowAboutPopup = true;

					ImGui::PopStyleColor();
					ImGui::EndMenu();
				}

				if (colourPushed)
				{
					ImGui::PopStyleColor();
				}
			}

			if (menuOpen)
			{
				ImGui::PopStyleColor(2);
			}

		}

		UI::EndMenuBar(); // TODO: implement the method

		ImGui::EndGroup();

	}

	float EditorLayerHazelLegacy::UI_DrawTitlebar()
	{
		return 0.0f;
	}

	void EditorLayerHazelLegacy::UI_HandleManualWindowResize()
	{
		auto* window = static_cast<GLFWwindow*>(Application::Get()->GetWindow()->GetHandle());
		const bool maximized = glfwGetWindowAttrib(window, GLFW_MAXIMIZED);

		ImVec2 newSize, newPosition;
		if (!maximized && UI::UpdateWindowManualResize(ImGui::GetCurrentWindow(), newSize, newPosition))
		{
			// On Windows we hook into the GLFW win32 window internals
#ifndef HZ_PLATFORM_WINDOWS
			glfwSetWindowPos(window, newPosition.x, newPosition.y);
			glfwSetWindowSize(window, newSize.x, newSize.y);
#endif
		}
	}

	bool EditorLayerHazelLegacy::UI_TitleBarHitTest(int x, int y) const
	{
		return m_TitleBarHovered;
	}

	float EditorLayerHazelLegacy::GetSnapValue()
	{
		switch (m_GizmoType)
		{
			case ImGuizmo::OPERATION::TRANSLATE: return 0.5f;
			case ImGuizmo::OPERATION::ROTATE: return 45.0f;
			case ImGuizmo::OPERATION::SCALE: return 0.5f;
		}
		return 0.0f;
	}

	void EditorLayerHazelLegacy::DeleteEntity(Entity entity)
	{
		auto children = entity.Children();
		for (auto childId : children)
		{
			DeleteEntity(m_EditorScene->FindEntityByUUID(childId));
		}

		m_EditorScene->UnparentEntity(entity);
		m_EditorScene->DestroyEntity(entity);
	}

	void EditorLayerHazelLegacy::UpdateSceneRendererSettings()
	{
		std::array<Ref<SceneRendererHazelLegacy>, 2> renderers = { m_ViewportRenderer, m_SecondViewportRenderer };

		for (Ref<SceneRendererHazelLegacy> renderer : renderers)
		{
			SceneRendererOptionsVulkan& options = renderer->GetOptions();
			options.ShowSelectedInWireframe = m_ShowSelectedWireframe;

			SceneRendererOptionsVulkan::PhysicsColliderView colliderView = SceneRendererOptionsVulkan::PhysicsColliderView::None;
			if (m_ShowPhysicsCollidersWireframe)
			{
				colliderView = m_ShowPhysicsCollidersWireframeOnTop ? SceneRendererOptionsVulkan::PhysicsColliderView::OnTop : SceneRendererOptionsVulkan::PhysicsColliderView::Normal;
			}
			options.ShowPhysicsColliders = colliderView;
		}
	}

	void EditorLayerHazelLegacy::OnUpdate(Timestep ts)
	{
		switch (m_SceneState)
		{
			case SceneState::Edit:
			{
				m_EditorCamera.SetActive(m_AllowViewportCameraEvents || Input::GetCursorMode() == CursorMode::Locked);
				m_EditorCamera.OnUpdate(ts);
				UI::SetMouseEnabled(true);
				//	m_EditorScene->OnRenderEditor(m_ViewportRenderer, ts, m_EditorCamera);

				//	if (m_ShowSecondViewport)
				//	{
				//		m_SecondEditorCamera.SetActive(m_ViewportPanel2Focused);
				//		m_SecondEditorCamera.OnUpdate(ts);
				//		m_EditorScene->OnRenderEditor(m_SecondViewportRenderer, ts, m_SecondEditorCamera);
				//	}

				//	OnRender2D();
				break;
			}
			case SceneState::Play:
			{
				m_RuntimeScene->OnUpdate(ts);
				m_RuntimeScene->OnRenderRuntime(m_ViewportRenderer, ts);
				break;
			}
			case SceneState::Simulate:
			{
				m_EditorCamera.SetActive(m_AllowViewportCameraEvents || Input::GetCursorMode() == CursorMode::Locked);
				m_EditorCamera.OnUpdate(ts);
				m_SimulationScene->OnUpdate(ts);
				m_SimulationScene->OnRenderSimulation(m_ViewportRenderer, ts, m_EditorCamera);
				break;
			}
			case SceneState::Pause:
			{
				m_EditorCamera.SetActive(m_ViewportPanelMouseOver);
				UI::SetMouseEnabled(true);
				m_EditorCamera.OnUpdate(ts);

				m_RuntimeScene->OnRenderRuntime(m_ViewportRenderer, ts);
				break;
			}
		}

		AssetEditorPanel::OnUpdate(ts);

		SceneRendererHazelLegacy::WaitForThreads();
	}

	void EditorLayerHazelLegacy::OnImGuiRender()
	{
		// Log::GetLogger()->info("EditorLayerHazelLegacy::OnImGuiRender method called!");
	}

	void EditorLayerHazelLegacy::OnEvent(Event& e)
	{
		// Log::GetLogger()->info("EditorLayerHazelLegacy::OnEvent method called!");
	}

	// this method seems to be obsolete
	void EditorLayerHazelLegacy::OnRender()
	{
	}

	void EditorLayerHazelLegacy::OnRender2D()
	{
	}

	bool EditorLayerHazelLegacy::OnKeyPressedEvent(KeyPressedEvent& e)
	{
		return false;
	}

	bool EditorLayerHazelLegacy::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		return false;
	}

	void EditorLayerHazelLegacy::SelectEntity(Entity entity)
	{
	}

	void EditorLayerHazelLegacy::OpenProject()
	{
	}

	void EditorLayerHazelLegacy::OpenProject(const std::string& filepath)
	{
	}

	void EditorLayerHazelLegacy::CreateProject(std::filesystem::path projectPath)
	{
	}

	void EditorLayerHazelLegacy::SaveProject()
	{
	}

	void EditorLayerHazelLegacy::CloseProject(bool unloadProject)
	{
	}

	void EditorLayerHazelLegacy::NewScene(const std::string& name)
	{
	}

	void EditorLayerHazelLegacy::OpenScene()
	{
	}

	void EditorLayerHazelLegacy::OpenScene(const std::string& filepath)
	{
	}

	void EditorLayerHazelLegacy::OpenScene(const AssetMetadata& assetMetadata)
	{
	}

	void EditorLayerHazelLegacy::SaveScene()
	{
	}

	void EditorLayerHazelLegacy::SaveSceneAs()
	{
	}

	void EditorLayerHazelLegacy::UI_WelcomePopup()
	{
	}

	void EditorLayerHazelLegacy::UI_AboutPopup()
	{
	}

	void EditorLayerHazelLegacy::UI_CreateNewMeshPopup()
	{
	}

	void EditorLayerHazelLegacy::UI_InvalidAssetMetadataPopup()
	{
	}

	void EditorLayerHazelLegacy::UI_NewScene()
	{
	}

	std::pair<float, float> EditorLayerHazelLegacy::GetMouseViewportSpace(bool primaryViewport)
	{
		return std::pair<float, float>();
	}

	std::pair<glm::vec3, glm::vec3> EditorLayerHazelLegacy::CastRay(const EditorCamera& camera, float mx, float my)
	{
		return std::pair<glm::vec3, glm::vec3>();
	}

	void EditorLayerHazelLegacy::OnCreateMeshFromMeshAsset(Entity entity, Ref<MeshAsset> meshAsset)
	{
		m_ShowCreateNewMeshPopup = true;
		m_CreateNewMeshPopupData.MeshToCreate = meshAsset;
		m_CreateNewMeshPopupData.TargetEntity = entity;
	}

	void EditorLayerHazelLegacy::SceneHierarchyInvalidMetadataCallback(Entity entity, AssetHandle handle)
	{
		m_ShowInvalidAssetMetadataPopup = true;
		m_InvalidAssetMetadataPopupData.Metadata = AssetManager::GetMetadata(handle);
	}

	void EditorLayerHazelLegacy::OnSelected(const SelectedSubmesh& selectionContext)
	{
	}

	void EditorLayerHazelLegacy::OnEntityDeleted(Entity e)
	{
	}

	Ray EditorLayerHazelLegacy::CastMouseRay()
	{
#if 0
		auto [mouseX, mouseY] = GetMouseViewportSpace();
		if (mouseX > -1.0f && mouseX < 1.0f && mouseY > -1.0f && mouseY < 1.0f)
		{
			auto [origin, direction] = CastRay(mouseX, mouseY);
			return Ray(origin, direction);
		}
#endif
		HZ_CORE_ASSERT(false);

		return Ray::Zero();
	}

}
