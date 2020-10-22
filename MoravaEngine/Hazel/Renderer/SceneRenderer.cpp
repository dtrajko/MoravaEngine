#include "SceneRenderer.h"

#include "HazelMaterial.h"
#include "HazelShader.h"
#include "../Core/Assert.h"
#include "../Scene/HazelScene.h"
#include "../Platform/OpenGL/OpenGLRenderPass.h"

#include "../../Log.h"


namespace Hazel {

    struct SceneRendererData
    {
        const HazelScene* ActiveScene = nullptr;
        struct SceneInfo
        {
            HazelCamera SceneCamera;

            // Resources
            Ref<HazelMaterialInstance> SkyboxMaterial;
            Environment SceneEnvironment;
            Light ActiveLight;
        } SceneData;

        Ref<HazelTexture2D> BRDFLUT;
        Ref<Shader> CompositeShader;

        Ref<RenderPass> GeoPass;
        Ref<RenderPass> CompositePass;

        struct DrawCommand
        {
            Ref<Mesh> Mesh;
            Ref<HazelMaterialInstance> Material;
            glm::mat4 Transform;
        };
        std::vector<DrawCommand> DrawList;

        // Grid
        Ref<HazelMaterialInstance> GridMaterial;

        SceneRendererOptions Options;
    };

    static SceneRendererData s_Data;

    void SceneRenderer::Init()
    {
        // TODO: Setup framebuffers and render passes
    }

    void SceneRenderer::SetViewportSize(uint32_t width, uint32_t height)
    {
        s_Data.GeoPass->GetSpecification().TargetFramebuffer->Resize(width, height);
        s_Data.CompositePass->GetSpecification().TargetFramebuffer->Resize(width, height);
    }

    void SceneRenderer::BeginScene(const HazelScene* scene)
    {
        HZ_CORE_ASSERT(!s_Data.ActiveScene, "");

        s_Data.ActiveScene = scene;

        //  s_Data.SceneData.SceneCamera = scene->m_Camera;
        //  s_Data.SceneData.SkyboxMaterial = scene->m_SkyboxMaterial;
        //  s_Data.SceneData.SceneEnvironment = scene->m_Environment;
        //  s_Data.SceneData.ActiveLight = scene->m_Light;
    }

    void SceneRenderer::EndScene()
    {
        HZ_CORE_ASSERT(s_Data.ActiveScene, "");

        s_Data.ActiveScene = nullptr;

        FlushDrawList();
    }

    void SceneRenderer::SubmitEntity(Entity* entity)
    {
        // TODO: Culling, sorting, etc.

        auto mesh = entity->GetMesh();
        if (!mesh)
            return;

        // TODO: s_Data.DrawList.push_back({ mesh, entity->GetMaterial(), entity->GetTransform() });
    }

    static Ref<HazelShader> equirectangularConversionShader, envFilteringShader, envIrradianceShader;

    std::pair<HazelTextureCube*, HazelTextureCube*> SceneRenderer::CreateEnvironmentMap(const std::string& filepath)
    {
        return std::pair<HazelTextureCube*, HazelTextureCube*>();
    }

    void SceneRenderer::GeometryPass()
    {
    }

    void SceneRenderer::CompositePass()
    {
    }

    void SceneRenderer::FlushDrawList()
    {
        HZ_CORE_ASSERT(!s_Data.ActiveScene, "");

        GeometryPass();
        CompositePass();

        s_Data.DrawList.clear();
        s_Data.SceneData = {};
    }

    Ref<HazelTexture2D> SceneRenderer::GetFinalColorBuffer()
    {
        // return s_Data.CompositePass->GetSpecification().TargetFramebuffer;
        Log::GetLogger()->error("Not implemented");
        return nullptr;
    }

    Ref<RenderPass> SceneRenderer::GetFinalRenderPass()
    {
        return s_Data.CompositePass;
    }

    uint32_t SceneRenderer::GetFinalColorBufferRendererID()
    {
        return s_Data.CompositePass->GetSpecification().TargetFramebuffer->GetTextureAttachmentColor()->GetID();
    }

    SceneRendererOptions& SceneRenderer::GetOptions()
    {
        return s_Data.Options;
    }

}
