#pragma once

#include "DX11.h"

#include "H2M/Renderer/ShaderH2M.h"

#include "DX11Shader.h"
#include "DX11Texture2D.h"


class DX11ComputePipeline : public H2M::RefCountedH2M
{
public:
	DX11ComputePipeline(H2M::RefH2M<H2M::ShaderH2M> computeShader);
	~DX11ComputePipeline();

	void Execute();

	void Begin();
	void Dispatch();
	void End();

	H2M::RefH2M<H2M::ShaderH2M> GetShader() { return m_Shader; }

	void SetPushConstants(const void* data, uint32_t size);

private:
	void CreatePipeline();

private:
	H2M::RefH2M<H2M::ShaderH2M> m_Shader;

};
