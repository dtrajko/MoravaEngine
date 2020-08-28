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
	uniforms.insert(std::make_pair("projection", 0));
	uniforms.insert(std::make_pair("model", 0));
	uniforms.insert(std::make_pair("view", 0));
}

void RendererJoey::SetShaders()
{
	Shader* pbrShader = new Shader("Shaders/LearnOpenGL/2.2.2.pbr.vs", "Shaders/LearnOpenGL/2.2.2.pbr.fs");
	shaders.insert(std::make_pair("pbrShader", pbrShader));
	printf("RendererJoey: pbrShader compiled [programID=%d]\n", pbrShader->GetProgramID());

	Shader* pbrShaderMRE = new Shader("Shaders/LearnOpenGL/2.2.2.pbr.vs", "Shaders/LearnOpenGL/2.2.3.pbr.fs");
	shaders.insert(std::make_pair("pbrShaderMRE", pbrShaderMRE));
	printf("RendererJoey: pbrShaderMRE compiled [programID=%d]\n", pbrShaderMRE->GetProgramID());

	Shader* backgroundShader = new Shader("Shaders/LearnOpenGL/2.2.2.background.vs", "Shaders/LearnOpenGL/2.2.2.background.fs");
	shaders.insert(std::make_pair("backgroundShader", backgroundShader));
	printf("RendererJoey: backgroundShader compiled [programID=%d]\n", backgroundShader->GetProgramID());

	shaders["pbrShader"]->Bind();
	shaders["pbrShader"]->setInt("irradianceMap", 0);
	shaders["pbrShader"]->setInt("prefilterMap",  1);
	shaders["pbrShader"]->setInt("brdfLUT",       2);
	shaders["pbrShader"]->setInt("albedoMap",     3);
	shaders["pbrShader"]->setInt("normalMap",     4);
	shaders["pbrShader"]->setInt("metallicMap",   5);
	shaders["pbrShader"]->setInt("roughnessMap",  6);
	shaders["pbrShader"]->setInt("aoMap",         7);

	shaders["pbrShaderMRE"]->Bind();
	shaders["pbrShaderMRE"]->setInt("irradianceMap", 0);
	shaders["pbrShaderMRE"]->setInt("prefilterMap",  1);
	shaders["pbrShaderMRE"]->setInt("brdfLUT",       2);
	shaders["pbrShaderMRE"]->setInt("albedoMap",     3);
	shaders["pbrShaderMRE"]->setInt("normalMap",     4);
	shaders["pbrShaderMRE"]->setInt("metalRoughMap", 5);
	shaders["pbrShaderMRE"]->setInt("emissiveMap",   6);
	shaders["pbrShaderMRE"]->setInt("aoMap",         7);

	shaders["backgroundShader"]->Bind();
	shaders["backgroundShader"]->setInt("environmentMap", 0);
}

void RendererJoey::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RendererBasic::UpdateProjectionMatrix(&projectionMatrix, scene);

	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
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
	scene->Render(mainWindow, projectionMatrix, passType, shaders, uniforms);
}

void RendererJoey::RenderPass()
{
}

RendererJoey::~RendererJoey()
{
}
