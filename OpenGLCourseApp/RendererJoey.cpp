#include "RendererJoey.h"

#include "ShaderPBR.h"
#include "SceneJoey.h"


RendererJoey::RendererJoey()
{
}

void RendererJoey::Init(Scene* scene)
{
	SetGeometry();
	SetUniforms();
	SetShaders();

	m_MaterialWorkflowPBR = new MaterialWorkflowPBR();
	m_MaterialWorkflowPBR->Init("Textures/HDR/Ice_Lake_Ref.hdr");
}

void RendererJoey::SetGeometry()
{
	m_SphereJoey = new SphereJoey();
}

void RendererJoey::SetUniforms()
{
	uniforms.insert(std::make_pair("model", 0));
	uniforms.insert(std::make_pair("view", 0));
	uniforms.insert(std::make_pair("projection", 0));
}

void RendererJoey::SetShaders()
{
	Shader* pbrShader = new Shader("Shaders/learnopengl/2.2.2.pbr.vs", "Shaders/learnopengl/2.2.2.pbr.fs");
	shaders.insert(std::make_pair("pbrShader", pbrShader));
	printf("RendererJoey: pbrShader compiled [programID=%d]\n", pbrShader->GetProgramID());

	Shader* pbrShaderMRE = new Shader("Shaders/learnopengl/2.2.2.pbr.vs", "Shaders/learnopengl/2.2.3.pbr.fs");
	shaders.insert(std::make_pair("pbrShaderMRE", pbrShaderMRE));
	printf("RendererJoey: pbrShaderMRE compiled [programID=%d]\n", pbrShaderMRE->GetProgramID());

	Shader* backgroundShader = new Shader("Shaders/learnopengl/2.2.2.background.vs", "Shaders/learnopengl/2.2.2.background.fs");
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

	shaders["pbrShaderMRE"]->Bind();
	shaders["pbrShaderMRE"]->setInt("irradianceMap", 0);
	shaders["pbrShaderMRE"]->setInt("prefilterMap", 1);
	shaders["pbrShaderMRE"]->setInt("brdfLUT", 2);
	shaders["pbrShaderMRE"]->setInt("albedoMap", 3);
	shaders["pbrShaderMRE"]->setInt("normalMap", 4);
	shaders["pbrShaderMRE"]->setInt("metalRoughMap", 5);
	shaders["pbrShaderMRE"]->setInt("emissiveMap", 6);
	shaders["pbrShaderMRE"]->setInt("aoMap", 7);

	shaders["backgroundShader"]->Bind();
	shaders["backgroundShader"]->setInt("environmentMap", 0);
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
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_MaterialWorkflowPBR->GetIrradianceMap());
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_MaterialWorkflowPBR->GetPrefilterMap());
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_MaterialWorkflowPBR->GetBRDF_LUT_Texture());

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
	m_SphereJoey->Render();

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

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-3.0, 0.0, 2.0));
	shaders["pbrShader"]->setMat4("model", model);
	m_SphereJoey->Render();

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
	m_SphereJoey->Render();

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
	m_SphereJoey->Render();

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
	m_SphereJoey->Render();

	// render light source (simply re-render sphere at light positions)
	// this looks a bit off as we use the same shader, but it'll make their positions obvious and 
	// keeps the codeprint small.

	// silver (light source)
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, textures["silverAlbedoMap"]->GetID());
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, textures["silverNormalMap"]->GetID());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, textures["silverMetallicMap"]->GetID());
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, textures["silverRoughnessMap"]->GetID());
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, textures["silverAOMap"]->GetID());

	for (unsigned int i = 0; i < sizeof(sceneJoey->m_LightPositions) / sizeof(sceneJoey->m_LightPositions[0]); ++i)
	{
		glm::vec3 newPos = sceneJoey->m_LightPositions[i] + glm::vec3(sin(glfwGetTime() * 5.0) * 5.0, 0.0, 0.0);
		newPos = sceneJoey->m_LightPositions[i];

		shaders["pbrShaderMRE"]->Bind();
		shaders["pbrShaderMRE"]->setVec3("lightPositions[" + std::to_string(i) + "]", newPos);
		shaders["pbrShaderMRE"]->setVec3("lightColors[" + std::to_string(i) + "]", sceneJoey->m_LightColors[i]);

		shaders["pbrShader"]->Bind();
		shaders["pbrShader"]->setVec3("lightPositions[" + std::to_string(i) + "]", newPos);
		shaders["pbrShader"]->setVec3("lightColors[" + std::to_string(i) + "]", sceneJoey->m_LightColors[i]);
		model = glm::mat4(1.0f);
		model = glm::translate(model, newPos);
		model = glm::scale(model, glm::vec3(0.5f));
		shaders["pbrShader"]->setMat4("model", model);
		m_SphereJoey->Render();

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
	model = glm::translate(model, glm::vec3(0.0f, -10.0f, 25.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(0.1f));
	shaders["pbrShader"]->Bind();
	shaders["pbrShader"]->setMat4("model", model);
	sceneJoey->GetModels()["cerberus"]->RenderModelPBR();

	/* Khronos DamagedHelmet model */
	m_Timestep = sceneJoey->m_IsRotating ? m_Timestep - 0.1f * sceneJoey->m_RotationFactor : 0.0f;

	shaders["pbrShaderMRE"]->Bind();
	shaders["pbrShaderMRE"]->setMat4("projection", projectionMatrix);
	shaders["pbrShaderMRE"]->setMat4("view", view);
	shaders["pbrShaderMRE"]->setVec3("camPos", scene->GetCamera()->GetPosition());
	shaders["pbrShaderMRE"]->setFloat("emissiveFactor", sceneJoey->m_EmissiveFactor);
	shaders["pbrShaderMRE"]->setFloat("metalnessFactor", sceneJoey->m_MetalnessFactor);
	shaders["pbrShaderMRE"]->setFloat("roughnessFactor", sceneJoey->m_RoughnessFactor);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, textures["damagedHelmetAlbedoMap"]->GetID());
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, textures["damagedHelmetNormalMap"]->GetID());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, textures["damagedHelmetMetalRoughMap"]->GetID());
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, textures["damagedHelmetEmissiveMap"]->GetID());
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, textures["damagedHelmetAmbOcclusionMap"]->GetID());

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-10.0f, 15.0f, 0.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(0.0f + m_Timestep), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(5.0f));
	shaders["pbrShaderMRE"]->setMat4("model", model);
	sceneJoey->GetModels()["damagedHelmet"]->RenderModelPBR();

	/* Khronos SciFiHelmet model */
	shaders["pbrShaderMRE"]->Bind();
	shaders["pbrShaderMRE"]->setMat4("projection", projectionMatrix);
	shaders["pbrShaderMRE"]->setMat4("view", view);
	shaders["pbrShaderMRE"]->setVec3("camPos", scene->GetCamera()->GetPosition());
	shaders["pbrShaderMRE"]->setFloat("emissiveFactor", sceneJoey->m_EmissiveFactor);
	shaders["pbrShaderMRE"]->setFloat("metalnessFactor", sceneJoey->m_MetalnessFactor);
	shaders["pbrShaderMRE"]->setFloat("roughnessFactor", sceneJoey->m_RoughnessFactor);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, textures["sfHelmetAlbedoMap"]->GetID());
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, textures["sfHelmetNormalMap"]->GetID());
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, textures["sfHelmetMetalRoughMap"]->GetID());
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, textures["sfHelmetEmissiveMap"]->GetID());
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, textures["sfHelmetAmbOcclusionMap"]->GetID());

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(10.0f, 15.0f, 0.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(0.0f - m_Timestep), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(5.0f));
	shaders["pbrShaderMRE"]->setMat4("model", model);
	sceneJoey->GetModels()["sfHelmet"]->RenderModelPBR();

	// render skybox (render as last to prevent overdraw)
	shaders["backgroundShader"]->Bind();
	shaders["backgroundShader"]->setMat4("view", view);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_MaterialWorkflowPBR->GetEnvironmentCubemap());
	// glBindTexture(GL_TEXTURE_CUBE_MAP, m_MaterialWorkflowPBR->GetIrradianceMap()); // display irradiance map
	// glBindTexture(GL_TEXTURE_CUBE_MAP, m_MaterialWorkflowPBR->GetPrefilterMap()); // display prefilter map
	m_MaterialWorkflowPBR->GetSkyboxCube()->Render();
}

void RendererJoey::RenderPass()
{
}

RendererJoey::~RendererJoey()
{
}
