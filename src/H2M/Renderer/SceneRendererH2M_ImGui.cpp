// SceneRendererH2M's ImGui window: compiled into the editor (see SceneRendererH2M.cpp for the rest of the class)

#include "SceneRendererH2M.h"

#include "H2M/ImGui/ImGuiH2M.h"
#include "H2M/Platform/Vulkan/VulkanRendererH2M.h"
#include "H2M/Renderer/RendererH2M.h"
#include "H2M/Renderer/ShaderH2M.h"

#include "Core/Application.h"
#include "Core/Util.h"

#include <imgui.h>


namespace H2M
{

	void SceneRendererH2M::OnImGuiRender()
	{
		// HZ_PROFILE_FUNC();

		ImGui::Begin("Scene Renderer");

		if (ImGui::TreeNode("Shaders"))
		{
			auto& shaders = ShaderH2M::s_AllShaders;
			for (auto& shader : shaders)
			{
				if (ImGui::TreeNode(shader->GetName().c_str()))
				{
					std::string buttonName = "Reload##" + shader->GetName();
					if (ImGui::Button(buttonName.c_str()))
						shader->Reload(true);
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}

		if (UI::BeginTreeNode("Visualization"))
		{
			UI::BeginPropertyGrid();
			UI::Property("Show Light Complexity", RendererDataUB.ShowLightComplexity);
			UI::Property("Show Shadow Cascades", RendererDataUB.ShowCascades);
			static int maxDrawCall = 1000;
#if defined(GRAPHICS_API_VULKAN)
			UI::PropertySlider("Selected Draw", VulkanRendererH2M::GetSelectedDrawCall(), -1, maxDrawCall);
#endif
			UI::Property("Max Draw Call", maxDrawCall);
			UI::EndPropertyGrid();
			UI::EndTreeNode();
		}

		if (UI::BeginTreeNode("Render Statistics"))
		{
			uint32_t frameIndex = RendererH2M::GetCurrentFrameIndex();
			ImGui::Text("GPU time: %.3fms", m_CommandBuffer->GetExecutionGPUTime(frameIndex));

			ImGui::Text("Shadow Map Pass: %.3fms", m_CommandBuffer->GetExecutionGPUTime(frameIndex, m_GPUTimeQueries.ShadowMapPassQuery));
			ImGui::Text("Depth Pre-Pass: %.3fms", m_CommandBuffer->GetExecutionGPUTime(frameIndex, m_GPUTimeQueries.DepthPrePassQuery));
			ImGui::Text("Light Culling Pass: %.3fms", m_CommandBuffer->GetExecutionGPUTime(frameIndex, m_GPUTimeQueries.LightCullingPassQuery));
			ImGui::Text("Geometry Pass: %.3fms", m_CommandBuffer->GetExecutionGPUTime(frameIndex, m_GPUTimeQueries.GeometryPassQuery));
			ImGui::Text("HBAO Pass: %.3fms", m_CommandBuffer->GetExecutionGPUTime(frameIndex, m_GPUTimeQueries.HBAOPassQuery));
			ImGui::Text("Bloom Pass: %.3fms", m_CommandBuffer->GetExecutionGPUTime(frameIndex, m_GPUTimeQueries.BloomComputePassQuery));
			ImGui::Text("Jump Flood Pass: %.3fms", m_CommandBuffer->GetExecutionGPUTime(frameIndex, m_GPUTimeQueries.JumpFloodPassQuery));
			ImGui::Text("Composite Pass: %.3fms", m_CommandBuffer->GetExecutionGPUTime(frameIndex, m_GPUTimeQueries.CompositePassQuery));

			if (UI::BeginTreeNode("Pipeline Statistics"))
			{
				const PipelineStatisticsH2M& pipelineStats = m_CommandBuffer->GetPipelineStatistics(frameIndex);
				ImGui::Text("Input Assembly Vertices: %llu", (unsigned long long)pipelineStats.InputAssemblyVertices);
				ImGui::Text("Input Assembly Primitives: %llu", (unsigned long long)pipelineStats.InputAssemblyPrimitives);
				ImGui::Text("Vertex Shader Invocations: %llu", (unsigned long long)pipelineStats.VertexShaderInvocations);
				ImGui::Text("Clipping Invocations: %llu", (unsigned long long)pipelineStats.ClippingInvocations);
				ImGui::Text("Clipping Primitives: %llu", (unsigned long long)pipelineStats.ClippingPrimitives);
				ImGui::Text("Fragment Shader Invocations: %llu", (unsigned long long)pipelineStats.FragmentShaderInvocations);
				ImGui::Text("Compute Shader Invocations: %llu", (unsigned long long)pipelineStats.ComputeShaderInvocations);
				UI::EndTreeNode();
			}

			UI::EndTreeNode();
		}

		if (UI::BeginTreeNode("Bloom Settings"))
		{
			UI::BeginPropertyGrid();
			UI::Property("Bloom Enabled", m_BloomSettings.Enabled);
			UI::Property("Threshold", m_BloomSettings.Threshold);
			UI::Property("Knee", m_BloomSettings.Knee);
			UI::Property("Upsample Scale", m_BloomSettings.UpsampleScale);
			UI::Property("Intensity", m_BloomSettings.Intensity, 0.05f, 0.0f, 20.0f);
			UI::Property("Dirt Intensity", m_BloomSettings.DirtIntensity, 0.05f, 0.0f, 20.0f);

			// TODO(Yan): move this to somewhere else
			// UI::Image(m_BloomDirtTexture, ImVec2(64, 64));
			if (ImGui::IsItemHovered())
			{
				if (ImGui::IsItemClicked())
				{
					std::wstring filename = Application::Get()->OpenFile(L"All\0*.*\0");

					if (!filename.empty())
					{
						std::string filenameStr = Util::ToUtf8(filename);
						m_BloomDirtTexture = Texture2D_H2M::Create(filenameStr, false);
					}
				}
			}

			UI::EndPropertyGrid();
			UI::EndTreeNode();
		}

		if (UI::BeginTreeNode("Horizon-Based Ambient Occlusion"))
		{
			UI::BeginPropertyGrid();
			UI::Property("Enable", m_Options.EnableHBAO);
			UI::Property("Intensity", m_Options.HBAOIntensity, 0.05f, 0.1f, 4.0f);
			UI::Property("Radius", m_Options.HBAORadius, 0.05f, 0.0f, 4.0f);
			UI::Property("Bias", m_Options.HBAOBias, 0.02f, 0.0f, 0.95f);
			UI::Property("Blur Sharpness", m_Options.HBAOBlurSharpness, 0.5f, 0.0f, 100.f);
			UI::EndPropertyGrid();

			float size = ImGui::GetContentRegionAvail().x;
			if (m_ResourcesCreated)
			{
				float size = ImGui::GetContentRegionAvail().x;
				auto image = m_GeometryPipeline->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer->GetImage(1);
				// UI::Image(image, { size, size * (1.0f / image->GetAspectRatio()) }, { 0, 1 }, { 1, 0 });
			}
			UI::EndTreeNode();
		}

		if (UI::BeginTreeNode("Shadows"))
		{
			UI::BeginPropertyGrid();
			UI::Property("Soft Shadows", RendererDataUB.SoftShadows);
			UI::Property("DirLight Size", RendererDataUB.LightSize, 0.01f);
			UI::Property("Max Shadow Distance", RendererDataUB.MaxShadowDistance, 1.0f);
			UI::Property("Shadow Fade", RendererDataUB.ShadowFade, 5.0f);
			UI::EndPropertyGrid();
			if (UI::BeginTreeNode("Cascade Settings"))
			{
				UI::BeginPropertyGrid();
				UI::Property("Cascade Fading", RendererDataUB.CascadeFading);
				UI::Property("Cascade Transition Fade", RendererDataUB.CascadeTransitionFade, 0.05f, 0.0f, FLT_MAX);
				UI::Property("Cascade Split", CascadeSplitLambda, 0.01f);
				UI::Property("CascadeNearPlaneOffset", CascadeNearPlaneOffset, 0.1f, -FLT_MAX, 0.0f);
				UI::Property("CascadeFarPlaneOffset", CascadeFarPlaneOffset, 0.1f, 0.0f, FLT_MAX);
				UI::EndPropertyGrid();
				UI::EndTreeNode();
			}

			if (UI::BeginTreeNode("Shadow Map", false))
			{
				static int cascadeIndex = 0;
				auto fb = m_ShadowPassPipelines[cascadeIndex]->GetSpecification().RenderPass->GetSpecification().TargetFramebuffer;
				auto image = fb->GetDepthImage();

				float size = ImGui::GetContentRegionAvail().x; // (float)fb->GetWidth() * 0.5f, (float)fb->GetHeight() * 0.5f
				UI::BeginPropertyGrid();
				UI::PropertySlider("Cascade Index", cascadeIndex, 0, 3);
				UI::EndPropertyGrid();
				if (m_ResourcesCreated)
				{
					// UI::Image(image, (uint32_t)cascadeIndex, { size, size }, { 0, 1 }, { 1, 0 });
				}
				UI::EndTreeNode();
			}

			UI::EndTreeNode();
		}

		if (UI::BeginTreeNode("Compute Bloom"))
		{
			float size = ImGui::GetContentRegionAvail().x;
			if (m_ResourcesCreated)
			{
				static int tex = 0;
				UI::PropertySlider("Texture", tex, 0, 2);
				static int mip = 0;
				auto [mipWidth, mipHeight] = m_BloomComputeTextures[tex]->GetMipSize(mip);
				std::string label = fmt::format("Mip ({0}x{1})", mipWidth, mipHeight);
				UI::PropertySlider(label.c_str(), mip, 0, m_BloomComputeTextures[tex]->GetMipLevelCount() - 1);
				// UI::ImageMip(m_BloomComputeTextures[tex]->GetImage(), mip, { size, size * (1.0f / m_BloomComputeTextures[tex]->GetImage()->GetAspectRatio()) }, { 0, 1 }, { 1, 0 });
			}
			UI::EndTreeNode();
		}

		ImGui::End();
	}

}
