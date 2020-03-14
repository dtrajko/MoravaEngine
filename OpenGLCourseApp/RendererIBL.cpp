#include "RendererIBL.h"

#include "ShaderPBR.h"
#include "ShaderCubemap.h"
#include "ShaderSkyboxJoey.h"


RadianceHDR* RendererIBL::m_RadianceHDR;
Cubemap* RendererIBL::m_EnvironmentCubemap;
Cube* RendererIBL::m_Cube1x1;

void RendererIBL::Init()
{
	SetUniforms();
	SetShaders();

	m_RadianceHDR = new RadianceHDR("Textures/HDR/newport_loft.hdr");
	m_RadianceHDR->Load();
	printf("Renderer m_RadianceHDR Width=%d, Height=%d\n", m_RadianceHDR->GetWidth(), m_RadianceHDR->GetHeight());
	m_EnvironmentCubemap = new Cubemap(512, 512);
	m_Cube1x1 = new Cube();
}

void RendererIBL::SetUniforms()
{
	// common
	uniforms.insert(std::make_pair("model", 0));
	uniforms.insert(std::make_pair("view", 0));
	uniforms.insert(std::make_pair("projection", 0));
	uniforms.insert(std::make_pair("nearPlane", 0));
	uniforms.insert(std::make_pair("farPlane", 0));
	uniforms.insert(std::make_pair("normalMap", 0));
	uniforms.insert(std::make_pair("lightPosition", 0));
	uniforms.insert(std::make_pair("cameraPosition", 0));
	uniforms.insert(std::make_pair("lightColor", 0));

	// PBR - physically based rendering
	uniforms.insert(std::make_pair("albedo", 0));
	uniforms.insert(std::make_pair("metallic", 0));
	uniforms.insert(std::make_pair("roughness", 0));
	uniforms.insert(std::make_pair("ao", 0));
	uniforms.insert(std::make_pair("albedoMap", 0));
	uniforms.insert(std::make_pair("normalMap", 0));
	uniforms.insert(std::make_pair("metallicMap", 0));
	uniforms.insert(std::make_pair("roughnessMap", 0));
	uniforms.insert(std::make_pair("aoMap", 0));
	uniforms.insert(std::make_pair("camPos", 0));
	uniforms.insert(std::make_pair("ambientIntensity", 0));

	// cubemap shader
	uniforms.insert(std::make_pair("equirectangularMap", 0));

	// skybox Joey shader
	uniforms.insert(std::make_pair("environmentMap", 0));
}

void RendererIBL::SetShaders()
{
	static const char* vertPBRShader = "Shaders/PBR.vert";
	static const char* fragPBRShader = "Shaders/PBR.frag";
	ShaderPBR* shaderPBR = new ShaderPBR();
	shaderPBR->CreateFromFiles(vertPBRShader, fragPBRShader);
	shaders.insert(std::make_pair("pbr", shaderPBR));
	printf("Renderer: PBR shader compiled [programID=%d]\n", shaderPBR->GetProgramID());

	static const char* vertCubemapShader = "Shaders/cubemap.vert";
	static const char* fragCubemapShader = "Shaders/cubemap.frag";
	ShaderCubemap* shaderCubemap = new ShaderCubemap();
	shaderCubemap->CreateFromFiles(vertCubemapShader, fragCubemapShader);
	shaders.insert(std::make_pair("cubemap", shaderCubemap));
	printf("Renderer: Cubemap shader compiled [programID=%d]\n", shaderCubemap->GetProgramID());

	static const char* vertSkyboxJoey = "Shaders/skybox_joey.vert";
	static const char* fragSkyboxJoey = "Shaders/skybox_joey.frag";
	ShaderSkyboxJoey* skyboxJoeyShader = new ShaderSkyboxJoey();
	skyboxJoeyShader->CreateFromFiles(vertSkyboxJoey, fragSkyboxJoey);
	shaders.insert(std::make_pair("skybox_joey", skyboxJoeyShader));
	printf("Renderer: SkyboxJoey shader compiled [programID=%d]\n", skyboxJoeyShader->GetProgramID());
}

void RendererIBL::RenderPass(glm::mat4 projectionMatrix, Window& mainWindow, Scene* scene, Camera* camera, WaterManager* waterManager)
{
	glDisable(GL_CLIP_DISTANCE0);

	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());

	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	RenderSimpleSkyboxJoey(camera->CalculateViewMatrix(), projectionMatrix, scene);
}

void RendererIBL::RenderEnvironmentCubemap(Window& mainWindow, Scene* scene)
{
	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] =
	{
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	};

	// convert HDR equirectangular environment map to cubemap equivalent
	ShaderCubemap* shaderCubemap = (ShaderCubemap*)shaders["cubemap"];

	shaderCubemap->Bind();

	uniforms["equirectangularMap"] = shaderCubemap->GetUniformLocationEquirectangularMap();

	shaderCubemap->SetEquirectangularMap(scene->GetTextureSlots()["equirectangularMap"]);
	shaderCubemap->SetProjectionMatrix(&captureProjection);

	// printf("Renderer: bind m_RadianceHDR ID=%d to slot=%d\n", m_RadianceHDR->GetID(), scene->GetTextureSlots()["equirectangularMap"]);
	m_RadianceHDR->Bind(scene->GetTextureSlots()["equirectangularMap"]);

	m_EnvironmentCubemap->GetTextureCubemap()->Bind(0); // scene->GetTextureSlots()["environmentMap"]

	glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
	m_EnvironmentCubemap->GetCaptureFBO()->Bind();
	glBindFramebuffer(GL_FRAMEBUFFER, m_EnvironmentCubemap->GetCaptureFBO()->GetID());

	// printf("m_EnvironmentCubemap GetCaptureFBO.GetID=%d\n", m_EnvironmentCubemap->GetCaptureFBO()->GetID());

	for (unsigned int i = 0; i < 6; ++i)
	{
		shaderCubemap->SetViewMatrix(&captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			m_EnvironmentCubemap->GetTextureCubemap()->GetID(), 0);
		// printf("Renderer m_EnvironmentCubemap GetID=%d\n", m_EnvironmentCubemap->GetTextureCubemap()->GetID());
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_Cube1x1->Render();
	}

	m_EnvironmentCubemap->GetCaptureFBO()->Unbind();
	shaderCubemap->Unbind();

	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());
}

void RendererIBL::RenderSimpleSkyboxJoey(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, Scene* scene)
{
	glDepthFunc(GL_LEQUAL);

	ShaderSkyboxJoey* shaderSkyboxJoey = static_cast<ShaderSkyboxJoey*>(shaders["skybox_joey"]);

	shaderSkyboxJoey->Bind();

	uniforms["environmentMap"] = shaderSkyboxJoey->GetUniformLocationEnvironmentMap();

	shaderSkyboxJoey->SetEnvironmentMap(scene->GetTextureSlots()["environmentMap"]);
	shaderSkyboxJoey->SetProjectionMatrix(&projectionMatrix);
	shaderSkyboxJoey->SetViewMatrix(&viewMatrix);
	m_EnvironmentCubemap->GetTextureCubemap()->Bind(scene->GetTextureSlots()["environmentMap"]);
	m_Cube1x1->Render();
	shaderSkyboxJoey->Unbind();
}
