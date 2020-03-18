#include "RendererJoey.h"

#include "ShaderPBR.h"
#include "ShaderCubemap.h"
#include "ShaderSkyboxJoey.h"
#include "learnopengl/shaderJoey.h"
#include "SceneJoey.h"
#include "ShaderLearnOpenGL.h"



RendererJoey::RendererJoey()
{
}

void RendererJoey::Init()
{
	SetUniforms();
	SetShaders();
	SetFramebuffers();
	LoadHDREnvironmentMap();
	SetupCubemap();
	SetupMatrices();
	ConvertHDREquirectangularToCubemap();
	CreateIrradianceCubemap();
	SolveDiffuseIntegralByConvolution();
	CreatePreFilterCubemap();
	RunQuasiMonteCarloSimulation();
	Generate2DLUTFromBRDF();
}

void RendererJoey::SetUniforms()
{
	uniforms.insert(std::make_pair("model", 0));
	uniforms.insert(std::make_pair("view", 0));
	uniforms.insert(std::make_pair("projection", 0));
}

void RendererJoey::SetShaders()
{
	ShaderLearnOpenGL* pbrShader = new ShaderLearnOpenGL("Shaders/learnopengl/2.2.2.pbr.vs", "Shaders/learnopengl/2.2.2.pbr.fs");
	shaders.insert(std::make_pair("pbrShader", pbrShader));
	printf("RendererJoey: pbrShader compiled [programID=%d]\n", pbrShader->GetProgramID());

	ShaderLearnOpenGL* equirectangularToCubemapShader = new ShaderLearnOpenGL("Shaders/learnopengl/2.2.2.cubemap.vs", "Shaders/learnopengl/2.2.2.equirectangular_to_cubemap.fs");
	shaders.insert(std::make_pair("equirectangularToCubemapShader", equirectangularToCubemapShader));
	printf("RendererJoey: equirectangularToCubemapShader compiled [programID=%d]\n", equirectangularToCubemapShader->GetProgramID());

	ShaderLearnOpenGL* irradianceShader = new ShaderLearnOpenGL("Shaders/learnopengl/2.2.2.cubemap.vs", "Shaders/learnopengl/2.2.2.irradiance_convolution.fs");
	shaders.insert(std::make_pair("irradianceShader", irradianceShader));
	printf("RendererJoey: irradianceShader compiled [programID=%d]\n", irradianceShader->GetProgramID());

	ShaderLearnOpenGL* prefilterShader = new ShaderLearnOpenGL("Shaders/learnopengl/2.2.2.cubemap.vs", "Shaders/learnopengl/2.2.2.prefilter.fs");
	shaders.insert(std::make_pair("prefilterShader", prefilterShader));
	printf("RendererJoey: prefilterShader compiled [programID=%d]\n", prefilterShader->GetProgramID());

	ShaderLearnOpenGL* brdfShader = new ShaderLearnOpenGL("Shaders/learnopengl/2.2.2.brdf.vs", "Shaders/learnopengl/2.2.2.brdf.fs");
	shaders.insert(std::make_pair("brdfShader", brdfShader));
	printf("RendererJoey: brdfShader compiled [programID=%d]\n", brdfShader->GetProgramID());

	ShaderLearnOpenGL* backgroundShader = new ShaderLearnOpenGL("Shaders/learnopengl/2.2.2.background.vs", "Shaders/learnopengl/2.2.2.background.fs");
	shaders.insert(std::make_pair("backgroundShader", backgroundShader));
	printf("RendererJoey: backgroundShader compiled [programID=%d]\n", backgroundShader->GetProgramID());

	shaders["pbrShader"]->Bind();
	shaders["pbrShader"]->setInt("irradianceMap", 0);
	shaders["pbrShader"]->setInt("prefilterMap", 1);
	shaders["pbrShader"]->setInt("brdfLUT", 2);
	shaders["pbrShader"]->setInt("albedoMap", 3);
	shaders["pbrShader"]->setInt("normalMap", 4);
	shaders["pbrShader"]->setInt("metallicMap", 5);
	shaders["pbrShader"]->setInt("roughnessMap", 6);
	shaders["pbrShader"]->setInt("aoMap", 7);

	shaders["backgroundShader"]->Bind();
	shaders["backgroundShader"]->setInt("environmentMap", 0);
}

void RendererJoey::SetFramebuffers()
{
	// PBR: setup framebuffer
	glGenFramebuffers(1, &m_CaptureFBO);
	glGenRenderbuffers(1, &m_CaptureRBO);

	glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_CaptureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_CaptureRBO);
}

void RendererJoey::LoadHDREnvironmentMap()
{
	// pbr: load the HDR environment map
	stbi_set_flip_vertically_on_load(true);
	int width, height, nrComponents;
	float* data = stbi_loadf("Textures/HDR/newport_loft.hdr", &width, &height, &nrComponents, 0);
	if (data)
	{
		glGenTextures(1, &m_HDRTexture);
		glBindTexture(GL_TEXTURE_2D, m_HDRTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); // note how we specify the texture's data value to be float

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Failed to load HDR image." << std::endl;
	}
}

void RendererJoey::SetupCubemap()
{
	// pbr: setup cubemap to render to and attach to framebuffer
	glGenTextures(1, &m_EnvCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_EnvCubemap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // enable pre-filter mipmap sampling (combatting visible dots artifact)
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void RendererJoey::SetupMatrices()
{
	// pbr: set up projection and view matrices for capturing data onto the 6 cubemap face directions
	m_CaptureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	m_CaptureViews[0] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f));
	m_CaptureViews[1] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f));
	m_CaptureViews[2] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f));
	m_CaptureViews[3] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f));
	m_CaptureViews[4] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f));
	m_CaptureViews[5] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f));
}

void RendererJoey::ConvertHDREquirectangularToCubemap()
{
	// pbr: convert HDR equirectangular environment map to cubemap equivalent
	// ----------------------------------------------------------------------
	shaders["equirectangularToCubemapShader"]->Bind();
	shaders["equirectangularToCubemapShader"]->setInt("equirectangularMap", 0);
	shaders["equirectangularToCubemapShader"]->setMat4("projection", m_CaptureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_HDRTexture);

	glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFBO);
	for (unsigned int i = 0; i < 6; ++i)
	{
		shaders["equirectangularToCubemapShader"]->setMat4("view", m_CaptureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_EnvCubemap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		RenderCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// then let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_EnvCubemap);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

void RendererJoey::CreateIrradianceCubemap()
{
	// pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
	glGenTextures(1, &m_IrradianceMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_IrradianceMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_CaptureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);
}

void RendererJoey::SolveDiffuseIntegralByConvolution()
{
	// pbr: solve diffuse integral by convolution to create an irradiance (cube)map.	
	shaders["irradianceShader"]->Bind();
	shaders["irradianceShader"]->setInt("environmentMap", 0);
	shaders["irradianceShader"]->setMat4("projection", m_CaptureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_EnvCubemap);

	glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFBO);
	for (unsigned int i = 0; i < 6; ++i)
	{
		shaders["irradianceShader"]->setMat4("view", m_CaptureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_IrradianceMap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		RenderCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RendererJoey::CreatePreFilterCubemap()
{
	// PBR: create a pre-filter cubemap, and re-scale capture FBO to pre-filter scale.
	glGenTextures(1, &m_PrefilterMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_PrefilterMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minifcation filter to mip_linear 
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

void RendererJoey::RunQuasiMonteCarloSimulation()
{
	// pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
	shaders["prefilterShader"]->Bind();
	shaders["prefilterShader"]->setInt("environmentMap", 0);
	shaders["prefilterShader"]->setMat4("projection", m_CaptureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_EnvCubemap);

	glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFBO);
	unsigned int maxMipLevels = 5;
	for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
	{
		// resize framebuffer according to mip-level size.
		unsigned int mipWidth = (unsigned int)(128 * std::pow(0.5, mip));
		unsigned int mipHeight = (unsigned int)(128 * std::pow(0.5, mip));
		glBindRenderbuffer(GL_RENDERBUFFER, m_CaptureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
		glViewport(0, 0, mipWidth, mipHeight);

		float roughness = (float)mip / (float)(maxMipLevels - 1);
		shaders["prefilterShader"]->setFloat("roughness", roughness);
		for (unsigned int i = 0; i < 6; ++i)
		{
			shaders["prefilterShader"]->setMat4("view", m_CaptureViews[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_PrefilterMap, mip);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			RenderCube();
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RendererJoey::Generate2DLUTFromBRDF()
{
	// pbr: generate a 2D LUT from the BRDF equations used
	glGenTextures(1, &m_BRDF_LUT_Texture);

	// pre-allocate enough memory for the LUT texture.
	glBindTexture(GL_TEXTURE_2D, m_BRDF_LUT_Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
	// be sure to set wrapping mode to GL_CLAMP_TO_EDGE
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
	glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_CaptureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_BRDF_LUT_Texture, 0);

	glViewport(0, 0, 512, 512);
	
	shaders["brdfShader"]->Bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	RenderQuad();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RendererJoey::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
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

	SceneJoey* sceneJoey = static_cast<SceneJoey*>(scene);
	std::map<std::string, Texture*> textures = sceneJoey->GetTextures();

	// initialize static shader uniforms before rendering
	shaders["pbrShader"]->Bind();
	shaders["pbrShader"]->setMat4("projection", projectionMatrix);
	shaders["backgroundShader"]->Bind();
	shaders["backgroundShader"]->setMat4("projection", projectionMatrix);

	// render scene, supplying the convoluted irradiance map to the final shader.
	shaders["pbrShader"]->Bind();
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = scene->GetCamera()->CalculateViewMatrix();
	shaders["pbrShader"]->setMat4("view", view);
	shaders["pbrShader"]->setVec3("camPos", scene->GetCamera()->GetPosition());

	// bind pre-computed IBL data
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_IrradianceMap);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_PrefilterMap);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_BRDF_LUT_Texture);

	// printf("irradianceMap=%d prefilterMap=%d brdfLUTTexture=%d\n", m_IrradianceMap,m_PrefilterMap, m_BRDF_LUT_Texture);

	// rusted iron
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, textures["ironAlbedoMap"]->GetID());
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, textures["ironNormalMap"]->GetID());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, textures["ironMetallicMap"]->GetID());
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, textures["ironRoughnessMap"]->GetID());
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, textures["ironAOMap"]->GetID());

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-5.0, 0.0, 2.0));
	shaders["pbrShader"]->setMat4("model", model);
	RenderSphere();

	// gold
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, textures["goldAlbedoMap"]->GetID());
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, textures["goldNormalMap"]->GetID());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, textures["goldMetallicMap"]->GetID());
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, textures["goldRoughnessMap"]->GetID());
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, textures["goldAOMap"]->GetID());

	// printf("goldAlbedoMap=%d goldNormalMap=%d goldMetallicMap=%d goldRoughnessMap=%d goldAOMap=%d\n",
	// 	textures["goldAlbedoMap"]->GetID(), textures["goldNormalMap"]->GetID(), textures["goldMetallicMap"]->GetID(),
	// 	textures["goldRoughnessMap"]->GetID(), textures["goldAOMap"]->GetID());

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-3.0, 0.0, 2.0));
	shaders["pbrShader"]->setMat4("model", model);
	RenderSphere();

	// grass
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, textures["grassAlbedoMap"]->GetID());
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, textures["grassNormalMap"]->GetID());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, textures["grassMetallicMap"]->GetID());
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, textures["grassRoughnessMap"]->GetID());
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, textures["grassAOMap"]->GetID());

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-1.0, 0.0, 2.0));
	shaders["pbrShader"]->setMat4("model", model);
	RenderSphere();

	// plastic
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, textures["plasticAlbedoMap"]->GetID());
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, textures["plasticNormalMap"]->GetID());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, textures["plasticMetallicMap"]->GetID());
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, textures["plasticRoughnessMap"]->GetID());
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, textures["plasticAOMap"]->GetID());

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(1.0, 0.0, 2.0));
	shaders["pbrShader"]->setMat4("model", model);
	RenderSphere();

	// wall
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, textures["wallAlbedoMap"]->GetID());
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, textures["wallNormalMap"]->GetID());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, textures["wallMetallicMap"]->GetID());
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, textures["wallRoughnessMap"]->GetID());
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, textures["wallAOMap"]->GetID());

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(3.0, 0.0, 2.0));
	shaders["pbrShader"]->setMat4("model", model);
	RenderSphere();

	// render light source (simply re-render sphere at light positions)
	// this looks a bit off as we use the same shader, but it'll make their positions obvious and 
	// keeps the codeprint small.
	for (unsigned int i = 0; i < sizeof(sceneJoey->m_LightPositions) / sizeof(sceneJoey->m_LightPositions[0]); ++i)
	{
		glm::vec3 newPos = sceneJoey->m_LightPositions[i] + glm::vec3(sin(glfwGetTime() * 5.0) * 5.0, 0.0, 0.0);
		newPos = sceneJoey->m_LightPositions[i];
		shaders["pbrShader"]->setVec3("lightPositions[" + std::to_string(i) + "]", newPos);
		shaders["pbrShader"]->setVec3("lightColors[" + std::to_string(i) + "]", sceneJoey->m_LightColors[i]);

		model = glm::mat4(1.0f);
		model = glm::translate(model, newPos);
		model = glm::scale(model, glm::vec3(0.5f));
		shaders["pbrShader"]->setMat4("model", model);
		RenderSphere();
	}

	std::string passType = "main";
	scene->Render(projectionMatrix, passType, shaders, uniforms);

	/* Cerberus model */
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, textures["cerberusAlbedoMap"]->GetID());
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, textures["cerberusNormalMap"]->GetID());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, textures["cerberusMetallicMap"]->GetID());
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, textures["cerberusRoughnessMap"]->GetID());
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, textures["goldAOMap"]->GetID());

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 3.0f, -5.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(0.1f));
	shaders["pbrShader"]->setMat4("model", model);
	sceneJoey->GetModels()["cerberus"]->RenderModelPBR();

	// render skybox (render as last to prevent overdraw)
	shaders["backgroundShader"]->Bind();
	shaders["backgroundShader"]->setMat4("view", view);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_EnvCubemap);
	// glBindTexture(GL_TEXTURE_CUBE_MAP, m_IrradianceMap); // display irradiance map
	// glBindTexture(GL_TEXTURE_CUBE_MAP, m_PrefilterMap); // display prefilter map
	RenderCube();
}

void RendererJoey::RenderPass()
{
}

void RendererJoey::RenderCube()
{
	// renderCube() renders a 1x1 3D cube in NDC.
	// initialize (if necessary)
	if (m_CubeVAO == 0)
	{
		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			// bottom face
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			// top face
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			 1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};

		glGenVertexArrays(1, &m_CubeVAO);
		glGenBuffers(1, &m_CubeVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, m_CubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(m_CubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(m_CubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

void RendererJoey::RenderQuad()
{
	// renderQuad() renders a 1x1 XY quad in NDC
	if (m_QuadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &m_QuadVAO);
		glGenBuffers(1, &m_QuadVBO);
		glBindVertexArray(m_QuadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(m_QuadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void RendererJoey::RenderSphere()
{
	// renders (and builds at first invocation) a sphere
	if (m_SphereVAO == 0)
	{
		glGenVertexArrays(1, &m_SphereVAO);

		unsigned int vbo, ebo;
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);

		std::vector<glm::vec3> positions;
		std::vector<glm::vec2> uv;
		std::vector<glm::vec3> normals;
		std::vector<unsigned int> indices;

		const unsigned int X_SEGMENTS = 64;
		const unsigned int Y_SEGMENTS = 64;
		const float PI = 3.14159265359f;
		for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
		{
			for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
			{
				float xSegment = (float)x / (float)X_SEGMENTS;
				float ySegment = (float)y / (float)Y_SEGMENTS;
				float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
				float yPos = std::cos(ySegment * PI);
				float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

				positions.push_back(glm::vec3(xPos, yPos, zPos));
				uv.push_back(glm::vec2(xSegment, ySegment));
				normals.push_back(glm::vec3(xPos, yPos, zPos));
			}
		}

		bool oddRow = false;
		for (int y = 0; y < Y_SEGMENTS; ++y)
		{
			if (!oddRow) // even rows: y == 0, y == 2; and so on
			{
				for (int x = 0; x <= X_SEGMENTS; ++x)
				{
					indices.push_back(y * (X_SEGMENTS + 1) + x);
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				}
			}
			else
			{
				for (int x = X_SEGMENTS; x >= 0; --x)
				{
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
					indices.push_back(y * (X_SEGMENTS + 1) + x);
				}
			}
			oddRow = !oddRow;
		}
		m_SphereIndexCount = (unsigned int)indices.size();

		std::vector<float> data;
		for (int i = 0; i < positions.size(); ++i)
		{
			data.push_back(positions[i].x);
			data.push_back(positions[i].y);
			data.push_back(positions[i].z);
			if (uv.size() > 0)
			{
				data.push_back(uv[i].x);
				data.push_back(uv[i].y);
			}
			if (normals.size() > 0)
			{
				data.push_back(normals[i].x);
				data.push_back(normals[i].y);
				data.push_back(normals[i].z);
			}
		}
		glBindVertexArray(m_SphereVAO);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
		float stride = (3 + 2 + 3) * sizeof(float);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (GLsizei)stride, (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (GLsizei)stride, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, (GLsizei)stride, (void*)(5 * sizeof(float)));
	}

	glBindVertexArray(m_SphereVAO);
	glDrawElements(GL_TRIANGLE_STRIP, m_SphereIndexCount, GL_UNSIGNED_INT, 0);
}

RendererJoey::~RendererJoey()
{
}
