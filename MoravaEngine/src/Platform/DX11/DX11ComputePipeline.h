#pragma once

#include "DX11.h"

#include "Hazel/Renderer/HazelShader.h"

// #include "DX11Shader.h"
#include "DX11Texture.h"


class DX11ComputePipeline : public Hazel::RefCounted
{
public:
	DX11ComputePipeline(Hazel::Ref<Hazel::HazelShader> computeShader);
	~DX11ComputePipeline();

	void Execute(VkDescriptorSet* descriptorSets, uint32_t descriptorSetCount, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

	void Begin();
	void Dispatch(VkDescriptorSet descriptorSet, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
	void End();

	Hazel::Ref<Hazel::HazelShader> GetShader() { return m_Shader; }

	void SetPushConstants(const void* data, uint32_t size);

private:
	void CreatePipeline();

private:
	Hazel::Ref<Hazel::HazelShader> m_Shader;

};
