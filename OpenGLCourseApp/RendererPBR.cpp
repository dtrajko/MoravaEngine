#include "RendererPBR.h"

#include "ShaderMain.h"
#include "ShaderPBR.h"


RendererPBR::RendererPBR()
{
}

void RendererPBR::Init()
{
	SetUniforms();
	SetShaders();
}

void RendererPBR::SetUniforms()
{
	// common
	uniforms.insert(std::make_pair("model", 0));
	uniforms.insert(std::make_pair("view", 0));
	uniforms.insert(std::make_pair("projection", 0));
	uniforms.insert(std::make_pair("nearPlane", 0));
	uniforms.insert(std::make_pair("farPlane", 0));
	uniforms.insert(std::make_pair("directionalLightTransform", 0));
	uniforms.insert(std::make_pair("normalMap", 0));
	uniforms.insert(std::make_pair("lightPosition", 0));

	// main
	uniforms.insert(std::make_pair("eyePosition", 0));

	// water
	uniforms.insert(std::make_pair("reflectionTexture", 0));
	uniforms.insert(std::make_pair("refractionTexture", 0));
	uniforms.insert(std::make_pair("dudvMap", 0));
	uniforms.insert(std::make_pair("depthMap", 0));
	uniforms.insert(std::make_pair("moveFactor", 0));
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

void RendererPBR::SetShaders()
{
	static const char* vertShader = "Shaders/shader.vert";
	static const char* fragShader = "Shaders/shader.frag";
	ShaderMain* shaderMain = new ShaderMain();
	shaderMain->CreateFromFiles(vertShader, fragShader);
	shaders.insert(std::make_pair("main", shaderMain));
	printf("Renderer: Main shader compiled [programID=%d]\n", shaderMain->GetProgramID());

	static const char* vertShaderDirShadowMap = "Shaders/directional_shadow_map.vert";
	static const char* fragShaderDirShadowMap = "Shaders/directional_shadow_map.frag";
	Shader* shaderDirectionalShadow = new Shader();
	shaderDirectionalShadow->CreateFromFiles(vertShaderDirShadowMap, fragShaderDirShadowMap);
	shaders.insert(std::make_pair("directionalShadow", shaderDirectionalShadow));
	printf("Renderer: Shadow shader compiled [programID=%d]\n", shaderDirectionalShadow->GetProgramID());

	static const char* vertShaderOmniShadowMap = "Shaders/omni_shadow_map.vert";
	static const char* geomShaderOmniShadowMap = "Shaders/omni_shadow_map.geom";
	static const char* fragShaderOmniShadowMap = "Shaders/omni_shadow_map.frag";
	Shader* shaderOmniShadow = new Shader();
	shaderOmniShadow->CreateFromFiles(vertShaderOmniShadowMap, geomShaderOmniShadowMap, fragShaderOmniShadowMap);
	shaders.insert(std::make_pair("omniShadow", shaderOmniShadow));
	printf("Renderer: OmniShadow shader compiled [programID=%d]\n", shaderOmniShadow->GetProgramID());

	static const char* vertWaterShader = "Shaders/water.vert";
	static const char* fragWaterShader = "Shaders/water.frag";
	Shader* shaderWater = new Shader(vertWaterShader, fragWaterShader);
	shaders.insert(std::make_pair("water", shaderWater));
	printf("Renderer: Water shader compiled [programID=%d]\n", shaderWater->GetProgramID());

	static const char* vertPBRShader = "Shaders/PBR.vert";
	static const char* fragPBRShader = "Shaders/PBR.frag";
	ShaderPBR* shaderPBR = new ShaderPBR();
	shaderPBR->CreateFromFiles(vertPBRShader, fragPBRShader);
	shaders.insert(std::make_pair("pbr", shaderPBR));
	printf("Renderer: PBR shader compiled [programID=%d]\n", shaderPBR->GetProgramID());
}

void RendererPBR::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RenderPass(mainWindow, scene, projectionMatrix);
}

void RendererPBR::RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glDisable(GL_CLIP_DISTANCE0);

	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());

	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (scene->GetSettings().enableSkybox)
	{
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		float angleRadians = glm::radians((GLfloat)glfwGetTime());
		modelMatrix = glm::rotate(modelMatrix, angleRadians, glm::vec3(0.0f, 1.0f, 0.0f));
		scene->GetSkybox()->Draw(modelMatrix, scene->GetCamera()->CalculateViewMatrix(), projectionMatrix);
	}

	ShaderMain* shaderMain = (ShaderMain*)shaders["main"];
	shaderMain->Bind();

	uniforms["model"]       = shaderMain->GetUniformLocation("model");
	uniforms["projection"]  = shaderMain->GetUniformLocation("projection");
	uniforms["view"]        = shaderMain->GetUniformLocation("view");
	uniforms["eyePosition"] = shaderMain->GetUniformLocation("eyePosition");
	uniforms["specularIntensity"] = shaderMain->GetUniformLocationMaterialSpecularIntensity();
	uniforms["shininess"] = shaderMain->GetUniformLocationMaterialShininess();

	shaderMain->setMat4("view", scene->GetCamera()->CalculateViewMatrix());
	shaderMain->setMat4("projection", projectionMatrix);
	shaderMain->setVec3("eyePosition", glm::vec3(scene->GetCamera()->GetPosition().x, scene->GetCamera()->GetPosition().y, scene->GetCamera()->GetPosition().z));
	shaderMain->SetDirectionalLight(&LightManager::directionalLight);
	shaderMain->SetPointLights(LightManager::pointLights, LightManager::pointLightCount, scene->GetTextureSlots()["omniShadow"], 0);
	shaderMain->SetSpotLights(LightManager::spotLights, LightManager::spotLightCount, scene->GetTextureSlots()["omniShadow"], LightManager::pointLightCount);
	shaderMain->setMat4("directionalLightTransform", LightManager::directionalLight.CalculateLightTransform());

	LightManager::directionalLight.GetShadowMap()->Bind(scene->GetTextureSlots()["shadow"]);
	shaderMain->setInt("theTexture", scene->GetTextureSlots()["diffuse"]);
	shaderMain->setInt("normalMap", scene->GetTextureSlots()["normal"]);
	shaderMain->setInt("directionalShadowMap", scene->GetTextureSlots()["shadow"]);
	shaderMain->setVec4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, -10000));
	shaderMain->Validate();

	glm::vec3 lowerLight = scene->GetCamera()->GetPosition();
	lowerLight.y -= 0.2f;
	LightManager::spotLights[2].SetFlash(lowerLight, scene->GetCamera()->GetDirection());

	std::string passType = "main";

	EnableCulling();
	scene->Render(projectionMatrix, passType, shaders, uniforms);

	shaderMain->Unbind();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Shader* shaderWater = shaders["water"];
	shaderWater->Bind();
	uniforms["model"]             = shaderWater->GetUniformLocation("model");
	uniforms["projection"]        = shaderWater->GetUniformLocation("projection");
	uniforms["view"]              = shaderWater->GetUniformLocation("view");
	uniforms["reflectionTexture"] = shaderWater->GetUniformLocation("reflectionTexture");
	uniforms["refractionTexture"] = shaderWater->GetUniformLocation("refractionTexture");
	uniforms["dudvMap"]           = shaderWater->GetUniformLocation("dudvMap");
	uniforms["normalMap"]         = shaderWater->GetUniformLocation("normalMap");
	uniforms["depthMap"]          = shaderWater->GetUniformLocation("depthMap");
	uniforms["moveFactor"]        = shaderWater->GetUniformLocation("moveFactor");
	uniforms["cameraPosition"]    = shaderWater->GetUniformLocation("cameraPosition");
	uniforms["lightColor"]        = shaderWater->GetUniformLocation("lightColor");
	uniforms["lightPosition"]     = shaderWater->GetUniformLocation("lightPosition");
	uniforms["nearPlane"]         = shaderWater->GetUniformLocation("nearPlane");
	uniforms["farPlane"]          = shaderWater->GetUniformLocation("farPlane");

	shaderWater->setMat4("model",      glm::mat4(1.0f));
	shaderWater->setMat4("view",       scene->GetCamera()->CalculateViewMatrix());
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
	scene->RenderWater(projectionMatrix, passType, shaders, uniforms);
	shaderWater->Unbind();

	ShaderPBR* shaderPBR = static_cast<ShaderPBR*>(shaders["pbr"]);

	shaderPBR->Bind();
	uniforms["model"]      = shaderPBR->GetUniformLocation("model");
	uniforms["projection"] = shaderPBR->GetUniformLocation("projection");
	uniforms["view"]       = shaderPBR->GetUniformLocation("view");
	shaderPBR->SetCameraPosition(scene->GetCamera()->GetPosition());

	shaderPBR->setMat4("model", glm::mat4(1.0f));
	shaderPBR->setMat4("view", scene->GetCamera()->CalculateViewMatrix());
	shaderPBR->setMat4("projection", projectionMatrix);

	DisableCulling();
	scene->Render(projectionMatrix, passType, shaders, uniforms);
}

RendererPBR::~RendererPBR()
{
}
