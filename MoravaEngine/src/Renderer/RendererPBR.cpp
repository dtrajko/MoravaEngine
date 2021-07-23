#include "Renderer/RendererPBR.h"

#include "Core/Application.h"
#include "Shader/ShaderMain.h"
#include "Shader/ShaderPBR.h"


RendererPBR::RendererPBR()
{
}

RendererPBR::~RendererPBR()
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
	RendererBasic::GetUniforms().insert(std::make_pair("model", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("view", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("projection", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("nearPlane", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("farPlane", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("dirLightTransform", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("normalMap", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("lightPosition", 0));

	// main
	RendererBasic::GetUniforms().insert(std::make_pair("eyePosition", 0));

	// water
	RendererBasic::GetUniforms().insert(std::make_pair("reflectionTexture", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("refractionTexture", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("dudvMap", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("depthMap", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("moveFactor", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("cameraPosition", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("lightColor", 0));

	// PBR - physically based rendering
	RendererBasic::GetUniforms().insert(std::make_pair("albedo", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("metallic", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("roughness", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("ao", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("albedoMap", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("normalMap", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("metallicMap", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("roughnessMap", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("aoMap", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("camPos", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("ambientIntensity", 0));

	// cubemap shader
	RendererBasic::GetUniforms().insert(std::make_pair("equirectangularMap", 0));

	// skybox Joey shader
	RendererBasic::GetUniforms().insert(std::make_pair("environmentMap", 0));
}

void RendererPBR::SetShaders()
{
	Hazel::Ref<MoravaShader> shaderMain = MoravaShader::Create("Shaders/shader.vert", "Shaders/shader.frag");
	RendererBasic::GetShaders().insert(std::make_pair("main", shaderMain));
	printf("Renderer: Main shader compiled [programID=%d]\n", shaderMain->GetProgramID());

	Hazel::Ref<MoravaShader> shaderDirectionalShadow = MoravaShader::Create("Shaders/directional_shadow_map.vert", "Shaders/directional_shadow_map.frag");
	RendererBasic::GetShaders().insert(std::make_pair("directionalShadow", shaderDirectionalShadow));
	printf("Renderer: Shadow shader compiled [programID=%d]\n", shaderDirectionalShadow->GetProgramID());

	static const char* vertShaderOmniShadowMap = "Shaders/omni_shadow_map.vert";
	static const char* geomShaderOmniShadowMap = "Shaders/omni_shadow_map.geom";
	static const char* fragShaderOmniShadowMap = "Shaders/omni_shadow_map.frag";
	MoravaShader* shaderOmniShadow = new MoravaShader();
	shaderOmniShadow->CreateFromFiles(vertShaderOmniShadowMap, geomShaderOmniShadowMap, fragShaderOmniShadowMap);
	RendererBasic::GetShaders().insert(std::make_pair("omniShadow", shaderOmniShadow));
	printf("Renderer: OmniShadow shader compiled [programID=%d]\n", shaderOmniShadow->GetProgramID());

	MoravaShader* shaderWater = new MoravaShader("Shaders/water.vert", "Shaders/water.frag");
	RendererBasic::GetShaders().insert(std::make_pair("water", shaderWater));
	printf("Renderer: Water shader compiled [programID=%d]\n", shaderWater->GetProgramID());

	ShaderPBR* shaderPBR = new ShaderPBR();
	shaderPBR->CreateFromFiles("Shaders/PBR.vert", "Shaders/PBR.frag");
	RendererBasic::GetShaders().insert(std::make_pair("pbr", shaderPBR));
	printf("Renderer: PBR shader compiled [programID=%d]\n", shaderPBR->GetProgramID());
}

void RendererPBR::RenderPassMain(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glDisable(GL_CLIP_DISTANCE0);

	glViewport(0, 0, (GLsizei)mainWindow->GetWidth(), (GLsizei)mainWindow->GetHeight());

	// Clear the window
	RendererBasic::Clear();

	if (scene->GetSettings().enableSkybox)
	{
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		float angleRadians = glm::radians((GLfloat)glfwGetTime());
		modelMatrix = glm::rotate(modelMatrix, angleRadians, glm::vec3(0.0f, 1.0f, 0.0f));
		scene->GetSkybox()->Draw(modelMatrix, scene->GetCamera()->GetViewMatrix(), projectionMatrix);
	}

	Hazel::Ref<ShaderMain> shaderMain = RendererBasic::GetShaders()["main"];
	shaderMain->Bind();

	RendererBasic::GetUniforms()["model"]       = shaderMain->GetUniformLocation("model");
	RendererBasic::GetUniforms()["projection"]  = shaderMain->GetUniformLocation("projection");
	RendererBasic::GetUniforms()["view"]        = shaderMain->GetUniformLocation("view");
	RendererBasic::GetUniforms()["eyePosition"] = shaderMain->GetUniformLocation("eyePosition");
	RendererBasic::GetUniforms()["specularIntensity"] = shaderMain->GetUniformLocationMaterialSpecularIntensity();
	RendererBasic::GetUniforms()["shininess"]         = shaderMain->GetUniformLocationMaterialShininess();

	shaderMain->SetMat4("view", scene->GetCamera()->GetViewMatrix());
	shaderMain->SetMat4("projection", projectionMatrix);
	shaderMain->SetFloat3("eyePosition", glm::vec3(scene->GetCamera()->GetPosition().x, scene->GetCamera()->GetPosition().y, scene->GetCamera()->GetPosition().z));
	shaderMain->SetDirectionalLight(&LightManager::directionalLight);
	shaderMain->SetPointLights(LightManager::pointLights, LightManager::pointLightCount, scene->GetTextureSlots()["omniShadow"], 0);
	shaderMain->SetSpotLights(LightManager::spotLights, LightManager::spotLightCount, scene->GetTextureSlots()["omniShadow"], LightManager::pointLightCount);
	shaderMain->SetMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());

	LightManager::directionalLight.GetShadowMap()->ReadTexture(scene->GetTextureSlots()["shadow"]);
	shaderMain->SetInt("albedoMap", scene->GetTextureSlots()["diffuse"]);
	shaderMain->SetInt("normalMap", scene->GetTextureSlots()["normal"]);
	shaderMain->SetInt("shadowMap", scene->GetTextureSlots()["shadow"]);
	shaderMain->SetFloat4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, -10000));
	shaderMain->Validate();

	glm::vec3 lowerLight = scene->GetCamera()->GetPosition();
	lowerLight.y -= 0.2f;
	LightManager::spotLights[2].SetFlash(lowerLight, scene->GetCamera()->GetDirection());

	std::string passType = "main";

	EnableCulling();
	scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());

	shaderMain->Unbind();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Hazel::Ref<MoravaShader> shaderWater = RendererBasic::GetShaders()["water"];
	shaderWater->Bind();
	RendererBasic::GetUniforms()["model"]             = shaderWater->GetUniformLocation("model");
	RendererBasic::GetUniforms()["projection"]        = shaderWater->GetUniformLocation("projection");
	RendererBasic::GetUniforms()["view"]              = shaderWater->GetUniformLocation("view");
	RendererBasic::GetUniforms()["reflectionTexture"] = shaderWater->GetUniformLocation("reflectionTexture");
	RendererBasic::GetUniforms()["refractionTexture"] = shaderWater->GetUniformLocation("refractionTexture");
	RendererBasic::GetUniforms()["dudvMap"]           = shaderWater->GetUniformLocation("dudvMap");
	RendererBasic::GetUniforms()["normalMap"]         = shaderWater->GetUniformLocation("normalMap");
	RendererBasic::GetUniforms()["depthMap"]          = shaderWater->GetUniformLocation("depthMap");
	RendererBasic::GetUniforms()["moveFactor"]        = shaderWater->GetUniformLocation("moveFactor");
	RendererBasic::GetUniforms()["cameraPosition"]    = shaderWater->GetUniformLocation("cameraPosition");
	RendererBasic::GetUniforms()["lightColor"]        = shaderWater->GetUniformLocation("lightColor");
	RendererBasic::GetUniforms()["lightPosition"]     = shaderWater->GetUniformLocation("lightPosition");
	RendererBasic::GetUniforms()["nearPlane"]         = shaderWater->GetUniformLocation("nearPlane");
	RendererBasic::GetUniforms()["farPlane"]          = shaderWater->GetUniformLocation("farPlane");

	shaderWater->SetMat4("model",      glm::mat4(1.0f));
	shaderWater->SetMat4("view",       scene->GetCamera()->GetViewMatrix());
	shaderWater->SetMat4("projection", projectionMatrix);

	scene->GetWaterManager()->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(scene->GetTextureSlots()["depth"]);
	scene->GetTextures()["waterDuDv"]->Bind(scene->GetTextureSlots()["DuDv"]);

	shaderWater->SetFloat("nearPlane", scene->GetSettings().nearPlane);
	shaderWater->SetFloat("farPlane", scene->GetSettings().farPlane);
	shaderWater->SetInt("reflectionTexture", scene->GetTextureSlots()["reflection"]);
	shaderWater->SetInt("refractionTexture", scene->GetTextureSlots()["refraction"]);
	shaderWater->SetInt("normalMap", scene->GetTextureSlots()["normal"]);
	shaderWater->SetInt("depthMap", scene->GetTextureSlots()["depth"]);
	shaderWater->SetInt("dudvMap", scene->GetTextureSlots()["DuDv"]);
	shaderWater->SetFloat("moveFactor", scene->GetWaterManager()->GetWaterMoveFactor());
	shaderWater->SetFloat3("cameraPosition", scene->GetCamera()->GetPosition());
	shaderWater->SetFloat3("lightColor", LightManager::directionalLight.GetColor());
	shaderWater->SetFloat3("lightPosition", LightManager::directionalLight.GetPosition());

	EnableCulling();
	passType = "main";
	scene->RenderWater(projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());
	shaderWater->Unbind();

	Hazel::Ref<MoravaShader> shaderPBR = RendererBasic::GetShaders()["pbr"];

	shaderPBR->Bind();
	RendererBasic::GetUniforms()["model"]      = shaderPBR->GetUniformLocation("model");
	RendererBasic::GetUniforms()["projection"] = shaderPBR->GetUniformLocation("projection");
	RendererBasic::GetUniforms()["view"]       = shaderPBR->GetUniformLocation("view");
	RendererBasic::GetUniforms()["camPos"]     = shaderPBR->GetUniformLocation("camPos");
	shaderPBR->SetMat4("model", glm::mat4(1.0f));
	shaderPBR->SetMat4("projection", projectionMatrix);
	shaderPBR->SetMat4("view", scene->GetCamera()->GetViewMatrix());
	shaderPBR->SetFloat3("camPos", scene->GetCamera()->GetPosition());

	DisableCulling();
	scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());
}

void RendererPBR::BeginFrame()
{
}

void RendererPBR::WaitAndRender(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RendererBasic::UpdateProjectionMatrix(&projectionMatrix, scene);

	RenderPassMain(mainWindow, scene, projectionMatrix);
}
