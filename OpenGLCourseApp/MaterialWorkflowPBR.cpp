#include "MaterialWorkflowPBR.h"

#include <GL/glew.h>
#include "stb_image.h"

#include <iostream>


MaterialWorkflowPBR::MaterialWorkflowPBR()
{
}

void MaterialWorkflowPBR::Init(std::string envMapHDR)
{
	SetupShaders();                       // Line 26
	SetupGeometry();                      // Line 32
	SetupFramebuffers();                  // Line 37
	LoadHDREnvironmentMap(envMapHDR);     // Line 49
	SetupCubemap();                       // Line 74
	SetupMatrices();                      // Line 90
	ConvertHDREquirectangularToCubemap(); // Line 102
	CreateIrradianceCubemap();            // Line 128
	SolveDiffuseIntegralByConvolution();  // Line 148
	CreatePreFilterCubemap();             // Line 177
	RunQuasiMonteCarloSimulation();       // Line 195
	Generate2DLUTFromBRDF();              // Line 230
}

void MaterialWorkflowPBR::SetupShaders()
{
	m_ShaderEquirectangularToCubemap = new Shader("Shaders/learnopengl/2.2.2.cubemap.vs", "Shaders/learnopengl/2.2.2.equirectangular_to_cubemap.fs");
	printf("MaterialWorkflowPBR: m_ShaderEquirectangularToCubemap compiled [programID=%d]\n", m_ShaderEquirectangularToCubemap->GetProgramID());

	m_ShaderIrradiance = new Shader("Shaders/learnopengl/2.2.2.cubemap.vs", "Shaders/learnopengl/2.2.2.irradiance_convolution.fs");
	printf("MaterialWorkflowPBR: m_IrradianceShader compiled [programID=%d]\n", m_ShaderIrradiance->GetProgramID());

	m_ShaderPrefilter = new Shader("Shaders/learnopengl/2.2.2.cubemap.vs", "Shaders/learnopengl/2.2.2.prefilter.fs");
	printf("MaterialWorkflowPBR: m_PrefilterShader compiled [programID=%d]\n", m_ShaderPrefilter->GetProgramID());

	m_ShaderBRDF = new Shader("Shaders/learnopengl/2.2.2.brdf.vs", "Shaders/learnopengl/2.2.2.brdf.fs");
	printf("MaterialWorkflowPBR: m_ShaderBRDF compiled [programID=%d]\n", m_ShaderBRDF->GetProgramID());
}

void MaterialWorkflowPBR::SetupGeometry()
{
	m_SkyboxCube = new Cube();
	m_Quad = new Quad();
}

void MaterialWorkflowPBR::SetupFramebuffers()
{
	// PBR: setup framebuffer
	glGenFramebuffers(1, &m_CaptureFBO);
	glGenRenderbuffers(1, &m_CaptureRBO);

	glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_CaptureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_CaptureRBO);
}

void MaterialWorkflowPBR::LoadHDREnvironmentMap(std::string envMapHDR)
{
	// PBR: load the HDR environment map
	stbi_set_flip_vertically_on_load(true);
	int width, height, nrComponents;
	float* data = stbi_loadf(envMapHDR.c_str(), &width, &height, &nrComponents, 0); // newport_loft.hdr
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

void MaterialWorkflowPBR::SetupCubemap()
{
	// pbr: setup cubemap to render to and attach to framebuffer
	glGenTextures(1, &m_EnvironmentCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_EnvironmentCubemap);
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

void MaterialWorkflowPBR::SetupMatrices()
{
	// pbr: set up projection and view matrices for capturing data onto the 6 cubemap face directions
	m_CaptureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	m_CaptureViews[0] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	m_CaptureViews[1] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	m_CaptureViews[2] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	m_CaptureViews[3] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
	m_CaptureViews[4] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	m_CaptureViews[5] = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
}

void MaterialWorkflowPBR::ConvertHDREquirectangularToCubemap()
{
	// pbr: convert HDR equirectangular environment map to cubemap equivalent
	// ----------------------------------------------------------------------
	m_ShaderEquirectangularToCubemap->Bind();
	m_ShaderEquirectangularToCubemap->setInt("equirectangularMap", 0);
	m_ShaderEquirectangularToCubemap->setMat4("projection", m_CaptureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_HDRTexture);

	glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFBO);
	for (unsigned int i = 0; i < 6; ++i)
	{
		m_ShaderEquirectangularToCubemap->setMat4("view", m_CaptureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_EnvironmentCubemap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_SkyboxCube->Render();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// then let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_EnvironmentCubemap);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

void MaterialWorkflowPBR::CreateIrradianceCubemap()
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

void MaterialWorkflowPBR::SolveDiffuseIntegralByConvolution()
{
	// pbr: solve diffuse integral by convolution to create an irradiance (cube)map.	
	m_ShaderIrradiance->Bind();
	m_ShaderIrradiance->setInt("environmentMap", 0);
	m_ShaderIrradiance->setMat4("projection", m_CaptureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_EnvironmentCubemap);

	glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
	glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFBO);
	for (unsigned int i = 0; i < 6; ++i)
	{
		m_ShaderIrradiance->setMat4("view", m_CaptureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_IrradianceMap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_SkyboxCube->Render();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MaterialWorkflowPBR::CreatePreFilterCubemap()
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

void MaterialWorkflowPBR::RunQuasiMonteCarloSimulation()
{
	// pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
	m_ShaderPrefilter->Bind();
	m_ShaderPrefilter->setInt("environmentMap", 0);
	m_ShaderPrefilter->setMat4("projection", m_CaptureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_EnvironmentCubemap);

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
		m_ShaderPrefilter->setFloat("roughness", roughness);
		for (unsigned int i = 0; i < 6; ++i)
		{
			m_ShaderPrefilter->setMat4("view", m_CaptureViews[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_PrefilterMap, mip);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			m_SkyboxCube->Render();
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MaterialWorkflowPBR::Generate2DLUTFromBRDF()
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

	m_ShaderBRDF->Bind();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_Quad->Render();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

MaterialWorkflowPBR::~MaterialWorkflowPBR()
{
}
