#include "BlurEffect.h"
#include "Log.h"
#include "Application.h"


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
}

void BlurEffect::Init(int width, int height, int textureID)
{
	m_Width = width;
	m_Height = height;
	m_TextureID = textureID;

	SetupShaders();
	SetupGeometry();

	HorizontalBlurSetup(m_Width, m_Height);
	VerticalBlurSetup(m_Width, m_Height);

	Generate(m_Width, m_Height);
}

void BlurEffect::SetupShaders()
{
	if (!m_ShaderHorizontalBlur) {
		m_ShaderHorizontalBlur = new Shader("Shaders/ThinMatrix/blur_horizontal.vs", "Shaders/ThinMatrix/blur.fs");
		Log::GetLogger()->info("BlurEffect: m_ShaderHorizontalBlur compiled [programID={0}]", m_ShaderHorizontalBlur->GetProgramID());
	}

	if (!m_ShaderVerticalBlur) {
		m_ShaderVerticalBlur = new Shader("Shaders/ThinMatrix/blur_vertical.vs", "Shaders/ThinMatrix/blur.fs");
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

	m_HorizontalFBO = new Framebuffer(m_Width, m_Height);
	m_HorizontalFBO->AddAttachmentSpecification(m_Width, m_Height, AttachmentType::Texture, AttachmentFormat::Color);
	m_HorizontalFBO->Generate(m_Width, m_Height);

	if (!m_HorizontalFBO->CheckStatus())
		Log::GetLogger()->error("ERROR: BlurEffect Horizontal blur FBO is not complete!");

	Log::GetLogger()->info("Horizontal Blur FBO created successfully.");

	m_ShaderHorizontalBlur->Bind();
	m_ShaderHorizontalBlur->setFloat("targetWidth", (float)m_Width);
	m_ShaderHorizontalBlur->Unbind();

	// -- Unbind the framebuffer / back to default framebuffer
	m_HorizontalFBO->Unbind(Application::Get()->GetWindow()->GetBufferWidth(), Application::Get()->GetWindow()->GetBufferHeight());
}

void BlurEffect::VerticalBlurSetup(int width, int height)
{
	m_Width = width;
	m_Height = height;

	m_VerticalFBO = new Framebuffer(m_Width, m_Height);
	m_VerticalFBO->AddAttachmentSpecification(m_Width, m_Height, AttachmentType::Texture, AttachmentFormat::Color);
	m_VerticalFBO->Generate(m_Width, m_Height);

	if (!m_VerticalFBO->CheckStatus())
		Log::GetLogger()->error("ERROR: BlurEffect Vertical blur FBO is not complete!");

	Log::GetLogger()->info("Vertical Blur FBO created successfully.");

	m_ShaderVerticalBlur->Bind();
	m_ShaderVerticalBlur->setFloat("targetHeight", (float)m_Height);
	m_ShaderVerticalBlur->Unbind();

	// -- Unbind the framebuffer / back to default framebuffer
	m_VerticalFBO->Unbind(Application::Get()->GetWindow()->GetBufferWidth(), Application::Get()->GetWindow()->GetBufferHeight());
}

void BlurEffect::RenderHorizontal(int textureHorizontal)
{
	if (m_HorizontalFBO)
		m_HorizontalFBO->Bind();

	m_ShaderHorizontalBlur->Bind();
	glBindTexture(GL_TEXTURE_2D, textureHorizontal);
	m_ShaderHorizontalBlur->setInt("originalTexture", textureHorizontal);
	RenderQuadHorizontal();
	m_ShaderHorizontalBlur->Unbind();
}

void BlurEffect::RenderVertical(int textureVertical)
{
	if (m_VerticalFBO)
		m_VerticalFBO->Bind();

	m_ShaderVerticalBlur->Bind();
	glBindTexture(GL_TEXTURE_2D, textureVertical);
	m_ShaderVerticalBlur->setInt("originalTexture", textureVertical);
	RenderQuadVertical();
	m_ShaderVerticalBlur->Unbind();
}

void BlurEffect::RenderQuadHorizontal()
{
	m_ShaderHorizontalBlur->Bind();
	m_HorizontalFBO->GetTextureAttachmentColor()->Bind(0);

	glBindVertexArray(m_QuadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void BlurEffect::RenderQuadVertical()
{
	m_ShaderVerticalBlur->Bind();
	m_VerticalFBO->GetTextureAttachmentColor()->Bind(0);

	glBindVertexArray(m_QuadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

BlurEffect::~BlurEffect()
{
	Release();

	if (m_HorizontalFBO) delete m_HorizontalFBO;
	if (m_VerticalFBO) delete m_VerticalFBO;

	if (m_ShaderHorizontalBlur) delete m_ShaderHorizontalBlur;
	if (m_ShaderVerticalBlur) delete m_ShaderVerticalBlur;

	if (m_QuadVAO) glDeleteVertexArrays(1, &m_QuadVAO);
	if (m_QuadVBO) glDeleteBuffers(1, &m_QuadVBO);
}

void BlurEffect::Release()
{
}

void BlurEffect::Render(int inputTexture)
{
	RenderHorizontal(inputTexture);
	RenderVertical((int)GetHorizontalOutputTexture()->GetID());
}

FramebufferTexture* BlurEffect::GetHorizontalOutputTexture()
{
	return m_HorizontalFBO->GetTextureAttachmentColor();
}

FramebufferTexture* BlurEffect::GetVerticalOutputTexture()
{
	return m_VerticalFBO->GetTextureAttachmentColor();
}

void BlurEffect::Unbind(int width, int height)
{
	m_HorizontalFBO->Unbind(width, height);
	m_VerticalFBO->Unbind(width, height);
}
