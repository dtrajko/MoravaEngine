#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Core/Layer.h"
#include "Hazel/Core/Timestep.h"
#include "Hazel/Editor/ContentBrowserPanel.h"
#include "Hazel/Editor/SceneHierarchyPanel.h"
#include "Hazel/Events/Event.h"
#include "Hazel/Scene/Entity.h"

#include "DX11Pipeline.h"
#include "DX11Shader.h"
#include "DX11Texture2D.h"
#include "DX11VertexBuffer.h"
#include "DX11InputListener.h"
#include "DX11CameraFP.h"
#include "DX11Mesh.h"

#include "Core/Window.h"
#include "Scene/Scene.h"
#include "Editor/EntitySelection.h"
#include "Editor/MaterialEditorPanel.h"

#include "ImGuizmo.h"


struct RenderObject
{
	enum class PipelineType
	{
		None = 0,
		Unlit,
		Light,
	};
	Hazel::Ref<Hazel::HazelMesh> Mesh;
	Hazel::Ref<DX11Mesh> MeshDX11;
	std::vector<Hazel::Ref<Hazel::HazelTexture2D>> Textures;
	glm::mat4 Transform;
	PipelineType PipelineType;
	Hazel::Entity Entity;
	// std::vector<Hazel::Ref<DX11Material>> Materials; // we probably need some kind of <Submesh, Material> map here
};

class DX11TestLayer : public Hazel::Layer, public DX11InputListener
{
public:
	DX11TestLayer();
	DX11TestLayer(const std::string& name);
	virtual ~DX11TestLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	virtual void OnUpdate(Hazel::Timestep ts) override;
	void OnImGuiRender(Window* mainWindow, Scene* scene);

	virtual void OnEvent(Event& event) override;

	void ShowExampleAppDockSpace(bool* p_open, Window* mainWindow);
	void OnRender(Window* mainWindow);

	void Render(const glm::vec4& clearColor, std::shared_ptr<DX11CameraFP> camera);

	static inline std::shared_ptr<DX11CameraFP> GetCamera() { return s_Camera; }
	static inline const glm::vec2& GetStartMousePosition() { return s_StartMousePosition; }

	// Inherited via DX11InputListener
	virtual void OnKeyDown(int key) override;
	virtual void OnKeyUp(int key) override;

	// MOUSE pure virtual callback functions
	virtual void OnMouseMove(const glm::vec2& mousePosDelta, const glm::vec2& mousePosAbs) override;

	virtual void OnLeftMouseDown(const glm::vec2& mousePos) override;
	virtual void OnRightMouseDown(const glm::vec2& mousePos) override;

	virtual void OnLeftMouseUp(const glm::vec2& mousePos) override;
	virtual void OnRightMouseUp(const glm::vec2& mousePos) override;

	bool OnLeftMouseDownEventHandler(const glm::vec2& mousePos);
	std::pair<float, float> GetMouseViewportSpace();
	std::pair<glm::vec3, glm::vec3> CastRay(float mx, float my);
	void AddSubmeshToSelectionContext(SelectedSubmesh submesh);
	void OnSelected(const SelectedSubmesh& selectionContext);
	Ref<Hazel::Entity> GetMeshEntity();

public:
	static Hazel::Ref<DX11Mesh> s_Mesh;
	static Hazel::Ref<Hazel::HazelMesh> s_MeshLight;
	static Hazel::Ref<Hazel::HazelMesh> s_SkyboxSphere;
	// render meshes with materials
	static std::vector<RenderObject> s_RenderObjectsWithMaterials;
	static std::vector<Hazel::Ref<DX11Material>> s_ListMaterials;

	static ImGuizmo::OPERATION s_ImGuizmoType;
	static bool s_LeftControlKeyPressed;

	static bool s_ShowWindowSceneHierarchy;
	static bool s_ShowWindowAssetManager;
	static bool s_ShowWindowMaterialEditor;

	static Hazel::Ref<Hazel::HazelScene> s_Scene; // the Scene object provides the ECS registry

	static glm::mat4 s_CurrentlySelectedTransform;

	static float s_ViewportWidth;
	static float s_ViewportHeight;
	static glm::vec2 s_ViewportBounds[2];
	static bool s_AllowViewportCameraEvents; // EditorLayer (Raypicking)

	static Hazel::SceneHierarchyPanel* s_SceneHierarchyPanel;
	static Hazel::ContentBrowserPanel* s_ContentBrowserPanel;
	static MaterialEditorPanel* s_MaterialEditorPanel;

private:
	static std::shared_ptr<DX11CameraFP> s_Camera;
	static glm::vec2 s_StartMousePosition;

	std::vector<RenderObject> m_RenderObjects;
	bool m_ShowMouseCursor;

	bool m_FullscreenEnabled = false;

};
