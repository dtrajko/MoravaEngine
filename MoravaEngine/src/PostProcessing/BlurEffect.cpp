#include "PostProcessing/BlurEffect.h"

#include "Core/Application.h"
#include "Core/Log.h"


BlurEffect::BlurEffect()
{
	m_Width = 0;
	m_Height = 0;
	m_TextureID = 0;

	m_QuadVAO = 0;
	m_QuadVBO = 0;

	m_HorizontalFBO = 0;
	m_VerticalFBO = 0;

	m_ShaderHorizontalBlur = nullptr;
	m_ShaderVerticalBlur = nullptr;

	m_OriginalTextureSlot = 1;
}

void BlurEffect::Init(int width, int height, int textureID)
{
	Log::GetLogger()->info("-- BEGIN BlurEffect::Init [width={0}, height={1}, textureID={2}]", width, height, textureID);

	m_Width = width;
	m_Height = height;
	m_TextureID = textureID;

	SetupShaders();
	SetupGeometry();

	HorizontalBlurSetup(m_Width, m_Height);
	VerticalBlurSetup(m_Width, m_Height);

	Generate(m_Width, m_Height);

	Log::GetLogger()->info("-- END BlurEffect::Init");
}

void BlurEffect::SetupShaders()
{
	if (!m_ShaderHorizontalBlur) {
		m_ShaderHorizontalBlur = MoravaShader::Create("Shaders/ThinMatrix/blur_horizontal.vs", "Shaders/ThinMatrix/blur.fs");
		Log::GetLogger()->info("BlurEffect: m_ShaderHorizontalBlur compiled [programID={0}]", m_ShaderHorizontalBlur->GetProgramID());
	}

	if (!m_ShaderVerticalBlur) {
		m_ShaderVerticalBlur = MoravaShader::Create("Shaders/ThinMatrix/blur_vertical.vs", "Shaders/ThinMatrix/blur.fs");
		Log::GetLogger()->info("BlurEffect: m_ShaderVerticalBlur compiled [programID={0}]", m_ShaderVerticalBlur->GetProgramID());
	}
}

void BlurEffect::SetupGeometry()
{
	// vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates
	std::vector<float> vertices =
	{
		// positions vec2
		-1.0f,  1.0f,
		-1.0f, -1.0f,
		 1.0f, -1.0f,

		-1.0f,  1.0f,
		 1.0f, -1.0f,
		 1.0f,  1.0f,
	};

	// screen quad VAO
	glGenVertexArrays(1, &m_QuadVAO);
	glGenBuffers(1, &m_QuadVBO);
	glBindVertexArray(m_QuadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
}

void BlurEffect::Generate(int width, int height)
{
	Release();

	m_HorizontalFBO->Generate(m_Width, m_Height);
	m_VerticalFBO->Generate(m_Width, m_Height);
}

void BlurEffect::HorizontalBlurSetup(int width, int height)
{
	m_Width = width;
	m_Height = height;

	m_HorizontalFBO = MoravaFramebuffer::Create(m_Width, m_Height);
	m_HorizontalFBO->AddColorAttachmentSpecification(m_Width, m_Height, AttachmentType::Texture, AttachmentFormat::Color);
	m_HorizontalFBO->Generate(m_Width, m_Height);

	if (!m_HorizontalFBO->CheckStatus()) {
		Log::GetLogger()->error("ERROR: BlurEffect Horizontal blur FBO is not complete!");
	}

	Log::GetLogger()->info("Horizontal Blur FBO created successfully.");

	// -- Unbind the framebuffer / back to default framebuffer
	m_HorizontalFBO->Unbind(Application::Get()->GetWindow()->GetWidth(), Application::Get()->GetWindow()->GetHeight());
}

void BlurEffect::VerticalBlurSetup(int width, int height)
{
	m_Width = width;
	m_Height = height;

	m_VerticalFBO = MoravaFramebuffer::Create(m_Width, m_Height);
	m_VerticalFBO->AddColorAttachmentSpecification(m_Width, m_Height, AttachmentType::Texture, AttachmentFormat::Color);
	m_VerticalFBO->Generate(m_Width, m_Height);

	if (!m_VerticalFBO->CheckStatus()) {
		Log::GetLogger()->error("ERROR: BlurEffect Vertical blur FBO is not complete!");
	}

	Log::GetLogger()->info("Vertical Blur FBO created successfully.");

	// -- Unbind the framebuffer / back to default framebuffer
	m_VerticalFBO->Unbind(Application::Get()->GetWindow()->GetWidth(), Application::Get()->GetWindow()->GetHeight());
}

void BlurEffect::Render()
{
	// Log::GetLogger()->info("-- BEGIN BlurEffect::Render --");

	RenderHorizontal(m_TextureID);
	RenderVertical((int)GetHorizontalOutputTexture()->GetID());

	// Log::GetLogger()->info("-- END BlurEffect::Render --");
}

void BlurEffect::RenderHorizontal(int textureHorizontal)
{
	m_HorizontalFBO->Bind(m_Width, m_Height);
	m_ShaderHorizontalBlur->Bind();

	glViewport(0, 0, m_Width, m_Height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0 + m_OriginalTextureSlot);
	glBindTexture(GL_TEXTURE_2D, textureHorizontal);
	m_ShaderHorizontalBlur->SetInt("originalTexture", m_OriginalTextureSlot);
	m_ShaderHorizontalBlur->SetFloat("targetWidth", (float)m_Width);
	// Log::GetLogger()->debug("-- BlurEffect::RenderHorizontal originalTexture: {0} targetHeight: {1}", textureHorizontal, (float)m_Height);

	RenderQuadHorizontal();

	m_ShaderHorizontalBlur->Unbind();
	m_HorizontalFBO->Unbind(Application::Get()->GetWindow()->GetWidth(), Application::Get()->GetWindow()->GetHeight());
}

void BlurEffect::RenderVertical(int textureVertical)
{
	m_VerticalFBO->Bind(m_Width, m_Height);
	m_ShaderVerticalBlur->Bind();

	glViewport(0, 0, m_Width, m_Height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0 + m_OriginalTextureSlot);
	glBindTexture(GL_TEXTURE_2D, textureVertical);
	m_ShaderVerticalBlur->SetInt("originalTexture", m_OriginalTextureSlot);
	m_ShaderVerticalBlur->SetFloat("targetHeight", (float)m_Height);
	// Log::GetLogger()->debug("-- BlurEffect::RenderVertical originalTexture: {0} targetWidth: {1}", textureVertical, (float)m_Width);

	RenderQuadVertical();

	m_ShaderVerticalBlur->Unbind();
	m_VerticalFBO->Unbind(Application::Get()->GetWindow()->GetWidth(), Application::Get()->GetWindow()->GetHeight());
}

void BlurEffect::RenderQuadHorizontal()
{
	m_ShaderHorizontalBlur->Bind();
	// m_HorizontalFBO->GetTextureAttachmentColor()->Bind(0);

	glBindVertexArray(m_QuadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void BlurEffect::RenderQuadVertical()
{
	m_ShaderVerticalBlur->Bind();
	// m_VerticalFBO->GetTextureAttachmentColor()->Bind(0);

	glBindVertexArray(m_QuadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

BlurEffect::~BlurEffect()
{
	Release();

	// if (m_HorizontalFBO) delete m_HorizontalFBO;
	// if (m_VerticalFBO) delete m_VerticalFBO;

	// if (m_ShaderHorizontalBlur) delete m_ShaderHorizontalBlur;
	// if (m_ShaderVerticalBlur) delete m_ShaderVerticalBlur;

	if (m_QuadVAO) glDeleteVertexArrays(1, &m_QuadVAO);
	if (m_QuadVBO) glDeleteBuffers(1, &m_QuadVBO);
}

void BlurEffect::Release()
{
}

H2M::Ref<FramebufferTexture> BlurEffect::GetHorizontalOutputTexture()
{
	return m_HorizontalFBO->GetTextureAttachmentColor();
}

H2M::Ref<FramebufferTexture> BlurEffect::GetVerticalOutputTexture()
{
	return m_VerticalFBO->GetTextureAttachmentColor();
}

void BlurEffect::Unbind(int width, int height)
{
	m_HorizontalFBO->Unbind(width, height);
	m_VerticalFBO->Unbind(width, height);
}
