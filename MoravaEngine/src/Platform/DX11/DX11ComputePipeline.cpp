#include "DX11ComputePipeline.h"

#include "Hazel/Core/Timer.h"
#include "Hazel/Renderer/HazelRenderer.h"

#include "Platform/DX11/DX11Context.h"


DX11ComputePipeline::DX11ComputePipeline(Hazel::Ref<Hazel::HazelShader> computeShader)
	: m_Shader(computeShader)
{
	// Hazel::Ref<DX11ComputePipeline> instance = this;
	// Hazel::HazelRenderer::Submit([instance]() mutable
	// {
	// });
	{
		CreatePipeline();
	}
}

DX11ComputePipeline::~DX11ComputePipeline()
{
}

void DX11ComputePipeline::CreatePipeline()
{
	// DX11Device device = DX11Context::GetCurrentDevice()->GetDX11Device();

	// TODO: Abstract into some sort of compute pipeline

	// TODO: CreatePipelineCache
	// TODO: CreateComputePipelines
}

void DX11ComputePipeline::Execute()
{
	// DX11Device device = DX11Context::GetCurrentDevice()->GetDX11Device();

	// TODO: QueueSubmit
}

void DX11ComputePipeline::Begin()
{
	// TODO CmdBindPipeline
}

void DX11ComputePipeline::Dispatch()
{
	// TODO
}

void DX11ComputePipeline::End()
{
	// TODO
}

void DX11ComputePipeline::SetPushConstants(const void* data, uint32_t size)
{
	// TODO CmdPushConstants
}
