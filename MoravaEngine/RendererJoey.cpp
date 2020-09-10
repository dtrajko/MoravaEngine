#include "RendererJoey.h"

#include "ShaderPBR.h"
#include "SceneJoey.h"
#include "Material.h"


RendererJoey::RendererJoey()
{
}

void RendererJoey::Init(Scene* scene)
{
	SetUniforms();
	SetShaders();
}

void RendererJoey::SetUniforms()
{
	s_Uniforms.insert(std::make_pair("projection", 0));
	s_Uniforms.insert(std::make_pair("model", 0));
	s_Uniforms.insert(std::make_pair("view", 0));
}

void RendererJoey::SetShaders()
{
	Shader* pbrShader = new Shader("Shaders/LearnOpenGL/2.2.2.pbr.vs", "Shaders/LearnOpenGL/2.2.2.pbr.fs");
	s_Shaders.insert(std::make_pair("pbrShader", pbrShader));
	printf("RendererJoey: pbrShader compiled [programID=%d]\n", pbrShader->GetProgramID());

	Shader* pbrShaderMRE = new Shader("Shaders/LearnOpenGL/2.2.2.pbr.vs", "Shaders/LearnOpenGL/2.2.3.pbr.fs");
	s_Shaders.insert(std::make_pair("pbrShaderMRE", pbrShaderMRE));
	printf("RendererJoey: pbrShaderMRE compiled [programID=%d]\n", pbrShaderMRE->GetProgramID());

	Shader* backgroundShader = new Shader("Shaders/LearnOpenGL/2.2.2.background.vs", "Shaders/LearnOpenGL/2.2.2.background.fs");
	s_Shaders.insert(std::make_pair("backgroundShader", backgroundShader));
	printf("RendererJoey: backgroundShader compiled [programID=%d]\n", backgroundShader->GetProgramID());

	s_Shaders["pbrShader"]->Bind();
	s_Shaders["pbrShader"]->setInt("irradianceMap", 0);
	s_Shaders["pbrShader"]->setInt("prefilterMap",  1);
	s_Shaders["pbrShader"]->setInt("brdfLUT",       2);
	s_Shaders["pbrShader"]->setInt("albedoMap",     3);
	s_Shaders["pbrShader"]->setInt("normalMap",     4);
	s_Shaders["pbrShader"]->setInt("metallicMap",   5);
	s_Shaders["pbrShader"]->setInt("roughnessMap",  6);
	s_Shaders["pbrShader"]->setInt("aoMap",         7);

	s_Shaders["pbrShaderMRE"]->Bind();
	s_Shaders["pbrShaderMRE"]->setInt("irradianceMap", 0);
	s_Shaders["pbrShaderMRE"]->setInt("prefilterMap",  1);
	s_Shaders["pbrShaderMRE"]->setInt("brdfLUT",       2);
	s_Shaders["pbrShaderMRE"]->setInt("albedoMap",     3);
	s_Shaders["pbrShaderMRE"]->setInt("normalMap",     4);
	s_Shaders["pbrShaderMRE"]->setInt("metalRoughMap", 5);
	s_Shaders["pbrShaderMRE"]->setInt("emissiveMap",   6);
	s_Shaders["pbrShaderMRE"]->setInt("aoMap",         7);

	s_Shaders["backgroundShader"]->Bind();
	s_Shaders["backgroundShader"]->setInt("environmentMap", 0);
}

void RendererJoey::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RendererBasic::UpdateProjectionMatrix(&projectionMatrix, scene);

	// Clear the window
	glClearColor(s_BgColor.r, s_BgColor.g, s_BgColor.b, s_BgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// configure global opengl state
	glEnable(GL_DEPTH_TEST);
	// set depth function to less than AND equal for skybox depth trick.
	glDepthFunc(GL_LEQUAL);
	// enable seamless cubemap sampling for lower mip levels in the pre-filter map.
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	// then before rendering, configure the viewport to the original framebuffer's screen dimensions
	SetDefaultFramebuffer((unsigned int)mainWindow.GetBufferWidth(), (unsigned int)mainWindow.GetBufferHeight());

	std::string passType = "main";
	scene->Render(mainWindow, projectionMatrix, passType, s_Shaders, s_Uniforms);
}

void RendererJoey::RenderPass()
{
}

RendererJoey::~RendererJoey()
{
}
