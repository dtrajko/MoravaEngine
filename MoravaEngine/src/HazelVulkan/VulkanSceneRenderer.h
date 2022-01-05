#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Editor/EditorCameraH2M.h"
#include "H2M/Renderer/MaterialH2M.h"
#include "H2M/Renderer/TextureH2M.h"
#include "H2M/Renderer/RenderCommandQueueH2M.h"
#include "H2M/Renderer/RenderPassH2M.h"
#include "H2M/Renderer/SceneEnvironmentH2M.h"
#include "H2M/Renderer/SceneRendererH2M.h"
#include "H2M/Scene/EntityH2M.h"

#include "Camera/Camera.h"
#include "EnvMap/EnvMapSharedData.h"
#include "Mesh/HazelFullscreenQuad.h"


class VulkanSceneRenderer
{
public:
	static void Init(std::string filepath, H2M::SceneH2M* scene);

	static void SetViewportSize(uint32_t width, uint32_t height);

	static void BeginScene(H2M::SceneH2M* scene, const H2M::SceneRendererCameraH2M& camera);
	static void EndScene();

	static void SubmitMesh(H2M::RefH2M<H2M::MeshH2M> mesh, const glm::mat4& transform = glm::mat4(1.0f), H2M::RefH2M<H2M::MaterialH2M> overrideMaterial = H2M::RefH2M<H2M::MaterialH2M>());
	static void SubmitSelectedMesh(H2M::RefH2M<H2M::MeshH2M> mesh, const glm::mat4& transform = glm::mat4(1.0f));

	static std::pair<H2M::RefH2M<H2M::TextureCubeH2M>, H2M::RefH2M<H2M::TextureCubeH2M>> CreateEnvironmentMap(const std::string& filepath);

	static H2M::RefH2M<H2M::RenderPassH2M> GetFinalRenderPass();
	static FramebufferTexture* GetFinalColorBuffer(); // originally returns H2M::RefH2M<H2M::Texture2D_H2M>

	// TODO: Temp
	static uint32_t GetFinalColorBufferRendererID();

	static H2M::SceneRendererOptionsH2M& GetOptions();

	// Temporary methods from EnvMapEditorLayer
	static H2M::RefH2M<H2M::TextureCubeH2M> GetRadianceMap();
	static H2M::RefH2M<H2M::TextureCubeH2M> GetIrradianceMap();
	static H2M::RefH2M<H2M::Texture2D_H2M> GetBRDFLUT();
	static H2M::RefH2M<H2M::ShaderH2M> GetShaderComposite();
	static H2M::RefH2M<H2M::RenderPassH2M> GetGeoPass();
	static H2M::RefH2M<H2M::RenderPassH2M> GetCompositePass();
	static void CreateDrawCommand(std::string fileNameNoExt, H2M::MeshH2M* mesh);
	static H2M::LightH2M& GetActiveLight();
	static void SetActiveLight(H2M::LightH2M& light);
	static void AddToDrawList(std::string name, H2M::RefH2M<H2M::MeshH2M> mesh, H2M::EntityH2M entity, glm::mat4 transform);
	static H2M::EnvironmentH2M Load(const std::string& filepath);
	static void SetEnvironment(H2M::EnvironmentH2M environment);
	static H2M::RefH2M<H2M::ShaderH2M> GetShaderSkybox() { return s_ShaderSkybox; }
	static H2M::RefH2M<H2M::ShaderH2M> GetShaderGrid() { return s_ShaderGrid; }
	static H2M::RefH2M<H2M::Texture2D_H2M> GetEnvEquirect() { return s_EnvEquirect; }
	static void SetupShaders();
	static H2M::SceneRendererCameraH2M& GetCamera();

	static void SubmitEntity(H2M::EntityH2M entity);

private:
	static void FlushDrawList();

// Moved from EnvMapEditorLayer back to EnvMapSceneRenderer
public:
	static void GeometryPass();
	static void CompositePass();

	static void SubmitEntityEnvMap(H2M::EntityH2M entity);
	static glm::mat4 GetViewProjection();

private:
	static void RenderSkybox();
	static void RenderHazelGrid();
	static void RenderOutline(H2M::RefH2M<H2M::ShaderH2M> shader, H2M::EntityH2M entity, const glm::mat4& entityTransform, H2M::SubmeshH2M& submesh);
	static void UpdateShaderPBRUniforms(H2M::RefH2M<H2M::ShaderH2M> shaderHazelPBR, H2M::RefH2M<EnvMapMaterial> envMapMaterial);

public:
	// From EnvironmentMap
	static H2M::RefH2M<H2M::ShaderH2M> s_ShaderEquirectangularConversion;
	static H2M::RefH2M<H2M::ShaderH2M> s_ShaderEnvFiltering;
	static H2M::RefH2M<H2M::ShaderH2M> s_ShaderEnvIrradiance;
	static H2M::RefH2M<H2M::ShaderH2M> s_ShaderGrid;
	static H2M::RefH2M<H2M::ShaderH2M> s_ShaderSkybox;

	// Intermediate textures
	static H2M::RefH2M<H2M::TextureCubeH2M> s_EnvUnfiltered;
	static H2M::RefH2M<H2M::Texture2D_H2M> s_EnvEquirect;
	static H2M::RefH2M<H2M::TextureCubeH2M> s_EnvFiltered;
	static H2M::RefH2M<H2M::TextureCubeH2M> s_IrradianceMap;

	static float s_GridScale;
	static float s_GridSize;

	static uint32_t s_FramebufferWidth;
	static uint32_t s_FramebufferHeight;

};
