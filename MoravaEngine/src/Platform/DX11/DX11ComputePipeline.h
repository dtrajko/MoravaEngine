#pragma once

#include "DX11.h"

#include "Hazel/Renderer/HazelShader.h"

#include "DX11Shader.h"
#include "DX11Texture2D.h"


class DX11ComputePipeline : public Hazel::RefCounted
{
public:
	DX11ComputePipeline(Hazel::Ref<Hazel::HazelShader> computeShader);
	~DX11ComputePipeline();

	void Execute();

	void Begin();
	void Dispatch();
	void End();

	Hazel::Ref<Hazel::HazelShader> GetShader() { return m_Shader; }

	void SetPushConstants(const void* data, uint32_t size);

private:
	void CreatePipeline();

private:
	Hazel::Ref<Hazel::HazelShader> m_Shader;

};
