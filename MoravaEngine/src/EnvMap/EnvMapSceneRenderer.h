#pragma once

#include "Hazel/Core/Ref.h"
#include "Hazel/Editor/EditorCamera.h"
#include "Hazel/Renderer/HazelMaterial.h"
#include "Hazel/Renderer/HazelTexture.h"
#include "Hazel/Renderer/RenderCommandQueue.h"
#include "Hazel/Renderer/RenderPass.h"
#include "Hazel/Renderer/SceneEnvironment.h"
#include "Hazel/Scene/Entity.h"

#include "Camera/Camera.h"
#include "EnvMap/EnvMapRenderPass.h"
#include "Mesh/HazelFullscreenQuad.h"


struct SceneRendererOptions
{
	bool ShowGrid = true;
	bool ShowBoundingBoxes = false;
};

struct SceneRendererCamera
{
	Hazel::HazelCamera Camera;
	glm::mat4 ViewMatrix;
	float Near, Far;
	float FOV;
};

class EnvMapSceneRenderer
{
public:
	static void Init(std::string filepath, Hazel::HazelScene* scene);

	static void SetViewportSize(uint32_t width, uint32_t height);

	static void BeginScene(Hazel::HazelScene* scene, const SceneRendererCamera& camera);
	static void EndScene();

	static void SubmitMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f), Ref<Hazel::HazelMaterial> overrideMaterial = nullptr);
	static void SubmitSelectedMesh(Ref<Mesh> mesh, const glm::mat4& transform = glm::mat4(1.0f));

	static std::pair<Hazel::Ref<Hazel::HazelTextureCube>, Hazel::Ref<Hazel::HazelTextureCube>> CreateEnvironmentMap(const std::string& filepath);

	static Hazel::Ref<Hazel::RenderPass> GetFinalRenderPass();
	static FramebufferTexture* GetFinalColorBuffer(); // originally returns Hazel::Ref<Hazel::HazelTexture2D>

	// TODO: Temp
	static uint32_t GetFinalColorBufferRendererID();

	static SceneRendererOptions& GetOptions();

	// Temporary methods from EnvMapEditorLayer
	static Hazel::Ref<Hazel::HazelTextureCube> GetRadianceMap();
	static Hazel::Ref<Hazel::HazelTextureCube> GetIrradianceMap();
	static Hazel::Ref<Hazel::HazelTexture2D> GetBRDFLUT();
	static Hazel::Ref<Shader> GetShaderComposite();
	static Hazel::Ref<EnvMapRenderPass> GetGeoPass();
	static Hazel::Ref<EnvMapRenderPass> GetCompositePass();
	static void CreateDrawCommand(std::string fileNameNoExt, Hazel::HazelMesh* mesh);
	static Hazel::HazelLight& GetActiveLight();
	static void SetActiveLight(Hazel::HazelLight& light);
	static void AddToDrawList(std::string name, Hazel::Ref<Hazel::HazelMesh> mesh, Hazel::Entity entity, glm::mat4 transform);
	static Hazel::Environment Load(const std::string& filepath);
	static void SetEnvironment(Hazel::Environment environment);
	static Hazel::Ref<Shader> GetShaderSkybox() { return s_ShaderSkybox; }
	static Hazel::Ref<Shader> GetShaderGrid() { return s_ShaderGrid; }
	static Hazel::Ref<Hazel::HazelTexture2D> GetEnvEquirect() { return s_EnvEquirect; }
	static void SetupShaders();
	static void SubmitEntity(Hazel::Entity entity);

private:
	static void FlushDrawList();
	static void GeometryPass();
	static void CompositePass();

public:
	static std::map<std::string, unsigned int>* m_SamplerSlots;

	// From EnvironmentMap
	static Hazel::Ref<Shader> s_ShaderEquirectangularConversion;
	static Hazel::Ref<Shader> s_ShaderEnvFiltering;
	static Hazel::Ref<Shader> s_ShaderEnvIrradiance;
	static Hazel::Ref<Shader> s_ShaderGrid;
	static Hazel::Ref<Shader> s_ShaderSkybox;

	// Intermediate textures
	static Hazel::Ref<Hazel::HazelTextureCube> s_EnvUnfiltered;
	static Hazel::Ref<Hazel::HazelTexture2D> s_EnvEquirect;
	static Hazel::Ref<Hazel::HazelTextureCube> s_EnvFiltered;
	static Hazel::Ref<Hazel::HazelTextureCube> s_IrradianceMap;

	static float s_GridScale;
	static float s_GridSize;

	static uint32_t s_FramebufferWidth;
	static uint32_t s_FramebufferHeight;

};
