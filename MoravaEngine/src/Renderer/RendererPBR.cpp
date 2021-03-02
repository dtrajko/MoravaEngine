#include "Renderer/RendererPBR.h"

#include "Core/Application.h"
#include "Shader/ShaderMain.h"
#include "Shader/ShaderPBR.h"


RendererPBR::RendererPBR()
{
}

void RendererPBR::Init(Scene* scene)
{
	SetUniforms();
	SetShaders();
}

void RendererPBR::SetUniforms()
{
	// common
	s_Uniforms.insert(std::make_pair("model", 0));
	s_Uniforms.insert(std::make_pair("view", 0));
	s_Uniforms.insert(std::make_pair("projection", 0));
	s_Uniforms.insert(std::make_pair("nearPlane", 0));
	s_Uniforms.insert(std::make_pair("farPlane", 0));
	s_Uniforms.insert(std::make_pair("dirLightTransform", 0));
	s_Uniforms.insert(std::make_pair("normalMap", 0));
	s_Uniforms.insert(std::make_pair("lightPosition", 0));

	// main
	s_Uniforms.insert(std::make_pair("eyePosition", 0));

	// water
	s_Uniforms.insert(std::make_pair("reflectionTexture", 0));
	s_Uniforms.insert(std::make_pair("refractionTexture", 0));
	s_Uniforms.insert(std::make_pair("dudvMap", 0));
	s_Uniforms.insert(std::make_pair("depthMap", 0));
	s_Uniforms.insert(std::make_pair("moveFactor", 0));
	s_Uniforms.insert(std::make_pair("cameraPosition", 0));
	s_Uniforms.insert(std::make_pair("lightColor", 0));

	// PBR - physically based rendering
	s_Uniforms.insert(std::make_pair("albedo", 0));
	s_Uniforms.insert(std::make_pair("metallic", 0));
	s_Uniforms.insert(std::make_pair("roughness", 0));
	s_Uniforms.insert(std::make_pair("ao", 0));
	s_Uniforms.insert(std::make_pair("albedoMap", 0));
	s_Uniforms.insert(std::make_pair("normalMap", 0));
	s_Uniforms.insert(std::make_pair("metallicMap", 0));
	s_Uniforms.insert(std::make_pair("roughnessMap", 0));
	s_Uniforms.insert(std::make_pair("aoMap", 0));
	s_Uniforms.insert(std::make_pair("camPos", 0));
	s_Uniforms.insert(std::make_pair("ambientIntensity", 0));

	// cubemap shader
	s_Uniforms.insert(std::make_pair("equirectangularMap", 0));

	// skybox Joey shader
	s_Uniforms.insert(std::make_pair("environmentMap", 0));
}

void RendererPBR::SetShaders()
{
	ShaderMain* shaderMain = new ShaderMain();
	shaderMain->CreateFromFiles("Shaders/shader.vert", "Shaders/shader.frag");
	s_Shaders.insert(std::make_pair("main", shaderMain));
	printf("Renderer: Main shader compiled [programID=%d]\n", shaderMain->GetProgramID());

	Shader* shaderDirectionalShadow = new Shader("Shaders/directional_shadow_map.vert", "Shaders/directional_shadow_map.frag");
	s_Shaders.insert(std::make_pair("directionalShadow", shaderDirectionalShadow));
	printf("Renderer: Shadow shader compiled [programID=%d]\n", shaderDirectionalShadow->GetProgramID());

	static const char* vertShaderOmniShadowMap = "Shaders/omni_shadow_map.vert";
	static const char* geomShaderOmniShadowMap = "Shaders/omni_shadow_map.geom";
	static const char* fragShaderOmniShadowMap = "Shaders/omni_shadow_map.frag";
	Shader* shaderOmniShadow = new Shader();
	shaderOmniShadow->CreateFromFiles(vertShaderOmniShadowMap, geomShaderOmniShadowMap, fragShaderOmniShadowMap);
	s_Shaders.insert(std::make_pair("omniShadow", shaderOmniShadow));
	printf("Renderer: OmniShadow shader compiled [programID=%d]\n", shaderOmniShadow->GetProgramID());

	Shader* shaderWater = new Shader("Shaders/water.vert", "Shaders/water.frag");
	s_Shaders.insert(std::make_pair("water", shaderWater));
	printf("Renderer: Water shader compiled [programID=%d]\n", shaderWater->GetProgramID());

	ShaderPBR* shaderPBR = new ShaderPBR();
	shaderPBR->CreateFromFiles("Shaders/PBR.vert", "Shaders/PBR.frag");
	s_Shaders.insert(std::make_pair("pbr", shaderPBR));
	printf("Renderer: PBR shader compiled [programID=%d]\n", shaderPBR->GetProgramID());
}

void RendererPBR::RenderPass(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glDisable(GL_CLIP_DISTANCE0);

	glViewport(0, 0, (GLsizei)mainWindow->GetWidth(), (GLsizei)mainWindow->GetHeight());

	// Clear the window
	glClearColor(s_BgColor.r, s_BgColor.g, s_BgColor.b, s_BgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (scene->GetSettings().enableSkybox)
	{
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		float angleRadians = glm::radians((GLfloat)glfwGetTime());
		modelMatrix = glm::rotate(modelMatrix, angleRadians, glm::vec3(0.0f, 1.0f, 0.0f));
		scene->GetSkybox()->Draw(modelMatrix, scene->GetCamera()->GetViewMatrix(), projectionMatrix);
	}

	ShaderMain* shaderMain = (ShaderMain*)s_Shaders["main"];
	shaderMain->Bind();

	s_Uniforms["model"]       = shaderMain->GetUniformLocation("model");
	s_Uniforms["projection"]  = shaderMain->GetUniformLocation("projection");
	s_Uniforms["view"]        = shaderMain->GetUniformLocation("view");
	s_Uniforms["eyePosition"] = shaderMain->GetUniformLocation("eyePosition");
	s_Uniforms["specularIntensity"] = shaderMain->GetUniformLocationMaterialSpecularIntensity();
	s_Uniforms["shininess"]         = shaderMain->GetUniformLocationMaterialShininess();

	shaderMain->setMat4("view", scene->GetCamera()->GetViewMatrix());
	shaderMain->setMat4("projection", projectionMatrix);
	shaderMain->setVec3("eyePosition", glm::vec3(scene->GetCamera()->GetPosition().x, scene->GetCamera()->GetPosition().y, scene->GetCamera()->GetPosition().z));
	shaderMain->SetDirectionalLight(&LightManager::directionalLight);
	shaderMain->SetPointLights(LightManager::pointLights, LightManager::pointLightCount, scene->GetTextureSlots()["omniShadow"], 0);
	shaderMain->SetSpotLights(LightManager::spotLights, LightManager::spotLightCount, scene->GetTextureSlots()["omniShadow"], LightManager::pointLightCount);
	shaderMain->setMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());

	LightManager::directionalLight.GetShadowMap()->Read(scene->GetTextureSlots()["shadow"]);
	shaderMain->setInt("albedoMap", scene->GetTextureSlots()["diffuse"]);
	shaderMain->setInt("normalMap", scene->GetTextureSlots()["normal"]);
	shaderMain->setInt("shadowMap", scene->GetTextureSlots()["shadow"]);
	shaderMain->setVec4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, -10000));
	shaderMain->Validate();

	glm::vec3 lowerLight = scene->GetCamera()->GetPosition();
	lowerLight.y -= 0.2f;
	LightManager::spotLights[2].SetFlash(lowerLight, scene->GetCamera()->GetDirection());

	std::string passType = "main";

	EnableCulling();
	scene->Render(mainWindow, projectionMatrix, passType, s_Shaders, s_Uniforms);

	shaderMain->Unbind();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Shader* shaderWater = s_Shaders["water"];
	shaderWater->Bind();
	s_Uniforms["model"]             = shaderWater->GetUniformLocation("model");
	s_Uniforms["projection"]        = shaderWater->GetUniformLocation("projection");
	s_Uniforms["view"]              = shaderWater->GetUniformLocation("view");
	s_Uniforms["reflectionTexture"] = shaderWater->GetUniformLocation("reflectionTexture");
	s_Uniforms["refractionTexture"] = shaderWater->GetUniformLocation("refractionTexture");
	s_Uniforms["dudvMap"]           = shaderWater->GetUniformLocation("dudvMap");
	s_Uniforms["normalMap"]         = shaderWater->GetUniformLocation("normalMap");
	s_Uniforms["depthMap"]          = shaderWater->GetUniformLocation("depthMap");
	s_Uniforms["moveFactor"]        = shaderWater->GetUniformLocation("moveFactor");
	s_Uniforms["cameraPosition"]    = shaderWater->GetUniformLocation("cameraPosition");
	s_Uniforms["lightColor"]        = shaderWater->GetUniformLocation("lightColor");
	s_Uniforms["lightPosition"]     = shaderWater->GetUniformLocation("lightPosition");
	s_Uniforms["nearPlane"]         = shaderWater->GetUniformLocation("nearPlane");
	s_Uniforms["farPlane"]          = shaderWater->GetUniformLocation("farPlane");

	shaderWater->setMat4("model",      glm::mat4(1.0f));
	shaderWater->setMat4("view",       scene->GetCamera()->GetViewMatrix());
	shaderWater->setMat4("projection", projectionMatrix);

	scene->GetWaterManager()->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(scene->GetTextureSlots()["depth"]);
	scene->GetTextures()["waterDuDv"]->Bind(scene->GetTextureSlots()["DuDv"]);

	shaderWater->setFloat("nearPlane", scene->GetSettings().nearPlane);
	shaderWater->setFloat("farPlane", scene->GetSettings().farPlane);
	shaderWater->setInt("reflectionTexture", scene->GetTextureSlots()["reflection"]);
	shaderWater->setInt("refractionTexture", scene->GetTextureSlots()["refraction"]);
	shaderWater->setInt("normalMap", scene->GetTextureSlots()["normal"]);
	shaderWater->setInt("depthMap", scene->GetTextureSlots()["depth"]);
	shaderWater->setInt("dudvMap", scene->GetTextureSlots()["DuDv"]);
	shaderWater->setFloat("moveFactor", scene->GetWaterManager()->GetWaterMoveFactor());
	shaderWater->setVec3("cameraPosition", scene->GetCamera()->GetPosition());
	shaderWater->setVec3("lightColor", LightManager::directionalLight.GetColor());
	shaderWater->setVec3("lightPosition", LightManager::directionalLight.GetPosition());

	EnableCulling();
	passType = "main";
	scene->RenderWater(projectionMatrix, passType, s_Shaders, s_Uniforms);
	shaderWater->Unbind();

	ShaderPBR* shaderPBR = static_cast<ShaderPBR*>(s_Shaders["pbr"]);

	shaderPBR->Bind();
	s_Uniforms["model"]      = shaderPBR->GetUniformLocation("model");
	s_Uniforms["projection"] = shaderPBR->GetUniformLocation("projection");
	s_Uniforms["view"]       = shaderPBR->GetUniformLocation("view");
	s_Uniforms["camPos"]     = shaderPBR->GetUniformLocation("camPos");
	shaderPBR->setMat4("model", glm::mat4(1.0f));
	shaderPBR->setMat4("projection", projectionMatrix);
	shaderPBR->setMat4("view", scene->GetCamera()->GetViewMatrix());
	shaderPBR->setVec3("camPos", scene->GetCamera()->GetPosition());

	DisableCulling();
	scene->Render(mainWindow, projectionMatrix, passType, s_Shaders, s_Uniforms);
}

void RendererPBR::Render(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RendererBasic::UpdateProjectionMatrix(&projectionMatrix, scene);

	RenderPass(mainWindow, scene, projectionMatrix);
}

RendererPBR::~RendererPBR()
{
}
