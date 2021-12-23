#include "DX11RendererBasic.h"

#include "Core/Application.h"
#include "Core/CommonValues.h"
#include "Core/Util.h"
#include "DX11Context.h"


DX11RendererBasic::DX11RendererBasic()
{
}

DX11RendererBasic::~DX11RendererBasic()
{
}

void DX11RendererBasic::Init(Scene* scene)
{
	SetUniforms();
	SetShaders();

	s_BgColor = glm::vec4(0.0, 0.0, 0.0, 1.0);

	InitDebug();
}

void DX11RendererBasic::SetUniforms()
{
}

void DX11RendererBasic::SetShaders()
{
}

void DX11RendererBasic::RendererInfo(WindowProps& windowProps)
{
	windowProps.Title += " [Renderer: DirectX 11]";
}

void DX11RendererBasic::InitDebug()
{
	Log::GetLogger()->warn("DX11RendererBasic::InitDebug: Method not yet supported!");
}

// this API is adequate. In DirectX 11 one can choose between front (D3D11_CULL_FRONT) and back (D3D11_CULL_BACK) culling
void DX11RendererBasic::EnableCulling()
{
	DX11Context::Get()->SetRasterizerState(DX11CullMode::Front); // or DX11CullMode::Back !?
}

void DX11RendererBasic::DisableCulling()
{
	DX11Context::Get()->SetRasterizerState(DX11CullMode::None);
}

void DX11RendererBasic::EnableTransparency()
{
	Log::GetLogger()->warn("DX11RendererBasic::EnableTransparency: Method not yet supported!");
}

void DX11RendererBasic::DisableTransparency()
{
	Log::GetLogger()->warn("DX11RendererBasic::DisableTransparency: Method not yet supported!");
}

void DX11RendererBasic::EnableDepthBuffer()
{
	Log::GetLogger()->warn("DX11RendererBasic::EnableDepthBuffer: Method not yet supported!");
}

void DX11RendererBasic::DisableDepthBuffer()
{
	Log::GetLogger()->warn("DX11RendererBasic::DisableDepthBuffer: Method not yet supported!");
}

void DX11RendererBasic::ClearDepthBuffer()
{
	Log::GetLogger()->warn("DX11RendererBasic::ClearDepthBuffer: Method not yet supported!");
}

void DX11RendererBasic::EnableDepthTest()
{
	Log::GetLogger()->warn("DX11RendererBasic::EnableDepthTest: Method not yet supported!");
}

void DX11RendererBasic::DisableDepthTest()
{
	Log::GetLogger()->warn("DX11RendererBasic::DisableDepthTest: Method not yet supported!");
}

void DX11RendererBasic::EnableMSAA()
{
	Log::GetLogger()->warn("DX11RendererBasic::EnableMSAA: Method not yet supported!");
}

void DX11RendererBasic::DisableMSAA()
{
	Log::GetLogger()->warn("DX11RendererBasic::DisableMSAA: Method not yet supported!");
}

void DX11RendererBasic::EnableBlend()
{
	Log::GetLogger()->warn("DX11RendererBasic::EnableBlend: Method not yet supported!");
}

void DX11RendererBasic::DisableBlend()
{
	Log::GetLogger()->warn("DX11RendererBasic::DisableBlend: Method not yet supported!");
}

void DX11RendererBasic::EnableWireframe()
{
	// throw std::logic_error("The method or operation is not implemented.");
	// Log::GetLogger()->warn("DX11RendererBasic::EnableWireframe: Method not yet supported!");
}

void DX11RendererBasic::DisableWireframe()
{
	// throw std::logic_error("The method or operation is not implemented.");
	// Log::GetLogger()->warn("DX11RendererBasic::DisableWireframe: Method not yet supported!");
}

void DX11RendererBasic::SetViewportSize(uint32_t width, uint32_t height)
{
	s_RendererContext.As<DX11Context>()->SetViewportSize(width, height);

	//	D3D11_VIEWPORT vp = {};
	//	vp.Width = (FLOAT)width;
	//	vp.Height = (FLOAT)height;
	//	vp.MinDepth = 0.0f;
	//	vp.MaxDepth = 1.0f;
	//	
	//	DX11Context::Get()->GetDX11DeviceContext()->RSSetViewports(1, &vp);
}

void DX11RendererBasic::SetDefaultFramebuffer(unsigned int width, unsigned int height)
{
	Log::GetLogger()->warn("DX11RendererBasic::SetDefaultFramebuffer: Method not yet supported!");
}

void DX11RendererBasic::RenderPassMain(Scene* scene, glm::mat4 projectionMatrix, Window* mainWindow)
{
	// throw std::logic_error("The method or operation is not implemented.");
	Log::GetLogger()->warn("DX11RendererBasic::RenderPassMain: Method not yet supported!");
}

void DX11RendererBasic::Clear()
{
	// Log::GetLogger()->warn("DX11RendererBasic::Clear(): Method not yet supported!");
}

void DX11RendererBasic::Clear(float r, float g, float b, float a)
{
	// Log::GetLogger()->warn("DX11RendererBasic::Clear(r, g, b, a): Method not yet supported!");
}

// Obsolete method in vulkan branch 237c6703 (OpenGL-specific)
void DX11RendererBasic::DrawIndexed(uint32_t count, H2M::PrimitiveTypeH2M type, bool depthTest)
{
	// throw std::logic_error("The method or operation is not implemented.");
	Log::GetLogger()->warn("DX11RendererBasic::DrawIndexed: Method not yet supported!");
}

void DX11RendererBasic::SetLineThickness(float thickness)
{
	Log::GetLogger()->warn("DX11RendererBasic::SetLineThickness: Method not yet supported!");
}

void DX11RendererBasic::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, void* indicesPtr)
{
	DX11Context::Get()->GetDX11DeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	DX11Context::Get()->GetDX11DeviceContext()->DrawIndexed((UINT)indexCount, (UINT)startIndexLocation, (UINT)baseVertexLocation);
}

void DX11RendererBasic::DrawLines(H2M::RefH2M<H2M::VertexArrayH2M> vertexArray, uint32_t vertexCount)
{
	Log::GetLogger()->warn("DX11RendererBasic::DrawLines: Method not yet supported!");
}

void DX11RendererBasic::SetLineWidth(float width)
{
	Log::GetLogger()->warn("DX11RendererBasic::SetLineWidth: Method not yet supported!");
}

void DX11RendererBasic::SetPolygonMode(PolygonMode polygonMode)
{
	Log::GetLogger()->warn("DX11RendererBasic::SetPolygonMode: Method not yet supported!");
}

void DX11RendererBasic::UpdateProjectionMatrix(glm::mat4* projectionMatrix, Scene* scene)
{
	float aspectRatio = scene->GetCameraController()->GetAspectRatio();
	*projectionMatrix = glm::perspective(glm::radians(scene->GetFOV()), aspectRatio, scene->GetSettings().nearPlane, scene->GetSettings().farPlane);
	s_ProjectionMatrix = *projectionMatrix;
}
