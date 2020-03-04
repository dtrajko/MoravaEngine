#include "Shader.h"


Shader::Shader()
{
	shaderID = 0;
	uniformModel = 0;
	uniformProjection = 0;
	pointLightCount = 0;
	spotLightCount = 0;
}

void Shader::CreateFromString(const char* vertexCode, const char* fragmentCode)
{
	CompileShader(vertexCode, fragmentCode);
}

void Shader::CreateFromFiles(const char* vertexLocation, const char* fragmentLocation)
{
	std::string vertexString = ReadFile(vertexLocation);
	std::string fragmentString = ReadFile(fragmentLocation);
	const char* vertexCode = vertexString.c_str();
	const char* fragmentCode = fragmentString.c_str();
	CompileShader(vertexCode, fragmentCode);
}

void Shader::CreateFromFiles(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation)
{
	std::string vertexString = ReadFile(vertexLocation);
	std::string geometryString = ReadFile(geometryLocation);
	std::string fragmentString = ReadFile(fragmentLocation);
	const char* vertexCode = vertexString.c_str();
	const char* geometryCode = geometryString.c_str();
	const char* fragmentCode = fragmentString.c_str();
	CompileShader(vertexCode, geometryCode, fragmentCode);
}

std::string Shader::ReadFile(const char* fileLocation)
{
	std::string content;
	std::ifstream fileStream(fileLocation, std::ios::in);

	if (!fileStream.is_open())
	{
		printf("Failed to read %s! File doesn't exist.", fileLocation);
		return "";
	}

	std::string line = "";
	while (!fileStream.eof())
	{
		std::getline(fileStream, line);
		content.append(line + "\n");
	}

	fileStream.close();

	printf("Content loaded from file '%s'\n", fileLocation);

	return content;
}

GLuint Shader::GetProgramID()
{
	return programID;
}

GLint Shader::GetModelLocation()
{
	return uniformModel;
}

GLint Shader::GetViewLocation()
{
	return uniformView;
}

GLint Shader::GetProjectionLocation()
{
	return uniformProjection;
}

GLint Shader::GetUniformLocationEyePosition()
{
	return uniformEyePosition;
}

GLint Shader::GetUniformLocationAmbientColor()
{
	return uniformDirectionalLight.uniformColor;
}

GLint Shader::GetUniformLocationAmbientIntensity()
{
	return uniformDirectionalLight.uniformAmbientIntensity;
}

GLint Shader::GetUniformLocationDiffuseIntensity()
{
	return uniformDirectionalLight.uniformDiffuseIntensity;
}

GLint Shader::GetUniformLocationDirectionalLightDirection()
{
	return uniformDirectionalLight.uniformDirection;
}

GLuint Shader::GetUniformLocationOmniLightPos()
{
	return uniformOmniLightPos;
}

GLuint Shader::GetUniformLocationFarPlane()
{
	return uniformFarPlane;
}

GLuint Shader::GetUniformLocationNearPlane()
{
	return uniformNearPlane;
}

GLint Shader::GetUniformLocationSpecularIntensity()
{
	return uniformSpecularIntensity;
}

GLint Shader::GetUniformLocationShininess()
{
	return uniformShininess;
}

GLint Shader::GetUniformLocationReflectionTexture()
{
	return uniformReflectionTexture;
}

GLint Shader::GetUniformLocationRefractionTexture()
{
	return uniformRefractionTexture;
}

GLint Shader::GetUniformLocationDuDvMap()
{
	return uniformDuDvMap;
}

GLint Shader::GetUniformLocationNormalMap()
{
	return uniformNormalMap;
}

GLint Shader::GetUniformLocationDepthMap()
{
	return uniformDepthMap;
}

GLint Shader::GetUniformLocationWaterMoveFactor()
{
	return uniformWaterMoveFactor;
}

GLint Shader::GetUniformLocationCameraPosition()
{
	return uniformCameraPosition;
}

GLint Shader::GetUniformLocationLightColor()
{
	return uniformLightColor;
}

GLint Shader::GetUniformLocationLightDirection()
{
	return uniformLightDirection;
}

void Shader::SetDirectionalLight(DirectionalLight* directionalLight)
{
	directionalLight->UseLight(
		uniformDirectionalLight.uniformColor,
		uniformDirectionalLight.uniformAmbientIntensity,
		uniformDirectionalLight.uniformDiffuseIntensity,
		uniformDirectionalLight.uniformDirection);
}

void Shader::SetPointLights(PointLight* pointLights, unsigned int lightCount, unsigned int textureUnit, unsigned int offset)
{
	if (lightCount > MAX_POINT_LIGHTS) lightCount = MAX_POINT_LIGHTS;

	glUniform1i(uniformPointLightCount, lightCount);

	for (unsigned int i = 0; i < lightCount; i++)
	{
		pointLights[i].UseLight(
			uniformPointLight[i].uniformColor,
			uniformPointLight[i].uniformAmbientIntensity,
			uniformPointLight[i].uniformDiffuseIntensity,
			uniformPointLight[i].uniformPosition,
			uniformPointLight[i].uniformConstant,
			uniformPointLight[i].uniformLinear,
			uniformPointLight[i].uniformExponent);

		pointLights[i].GetShadowMap()->Read(textureUnit + offset + i);
		glUniform1i(uniformOmniShadowMap[offset + i].shadowMap, textureUnit + offset + i);
		glUniform1f(uniformOmniShadowMap[offset + i].farPlane, pointLights[i].GetFarPlane());
	}
}

void Shader::SetSpotLights(SpotLight* spotLights, unsigned int lightCount, unsigned int textureUnit, unsigned int offset)
{
	if (lightCount > MAX_SPOT_LIGHTS) lightCount = MAX_SPOT_LIGHTS;

	glUniform1i(uniformSpotLightCount, lightCount);

	for (unsigned int i = 0; i < lightCount; i++)
	{
		spotLights[i].UseLight(
			uniformSpotLight[i].uniformColor,
			uniformSpotLight[i].uniformAmbientIntensity,
			uniformSpotLight[i].uniformDiffuseIntensity,
			uniformSpotLight[i].uniformPosition,
			uniformSpotLight[i].uniformDirection,
			uniformSpotLight[i].uniformConstant,
			uniformSpotLight[i].uniformLinear,
			uniformSpotLight[i].uniformExponent,
			uniformSpotLight[i].uniformEdge);

		spotLights[i].GetShadowMap()->Read(textureUnit + offset + i);
		glUniform1i(uniformOmniShadowMap[offset + i].shadowMap, textureUnit + offset + i);
		glUniform1f(uniformOmniShadowMap[offset + i].farPlane, spotLights[i].GetFarPlane());
	}
}

void Shader::SetClipPlane(glm::vec4 clipPlane)
{
	glUniform4f(uniformPlane, clipPlane.x, clipPlane.y, clipPlane.z, clipPlane.w);
}

void Shader::SetNearPlane(float nearPlane)
{
	glUniform1f(uniformNearPlane, nearPlane);
	// printf("Shader::SetNearPlane %.2f Uniform: %d\n", nearPlane, uniformNearPlane);
}

void Shader::SetFarPlane(float farPlane)
{
	glUniform1f(uniformFarPlane, farPlane);
	// printf("Shader::SetFarPlane %.2f Uniform: %d\n", farPlane, uniformFarPlane);
}

void Shader::SetWaterMoveFactor(float waterMoveFactor)
{
	glUniform1f(uniformWaterMoveFactor, waterMoveFactor);
}

void Shader::SetCameraPosition(glm::vec3 cameraPosition)
{
	glUniform3f(uniformCameraPosition, cameraPosition.x, cameraPosition.y, cameraPosition.z);
}

void Shader::SetLightColor(glm::vec3 lightColor)
{
	glUniform3f(uniformLightColor, lightColor.r, lightColor.g, lightColor.b);
}

void Shader::SetLightDirection(glm::vec3 lightDirection)
{
	glUniform3f(uniformLightDirection, lightDirection.x, lightDirection.y, lightDirection.z);
}

void Shader::SetWater(unsigned int txUnitReflection, unsigned int txUnitRefraction,
	unsigned int txUnitDuDv, unsigned int txUnitDepth)
{
	glUniform1i(uniformReflectionTexture, txUnitReflection);
	glUniform1i(uniformRefractionTexture, txUnitRefraction);
	glUniform1i(uniformDuDvMap, txUnitDuDv);
	glUniform1i(uniformDepthMap, txUnitDepth);
}

void Shader::SetTexture(GLuint textureUnit)
{
	glUniform1i(uniformTexture, textureUnit);
}

void Shader::SetNormalMap(GLuint textureUnit)
{
	glUniform1i(uniformNormalMap, textureUnit);
}

void Shader::SetDepthMap(GLuint textureUnit)
{
	glUniform1i(uniformDepthMap, textureUnit);
}

void Shader::SetDirectionalShadowMap(GLuint textureUnit)
{
	glUniform1i(uniformDirectionalShadowMap, textureUnit);
}

void Shader::SetDirectionalLightTransform(glm::mat4* transform)
{
	glUniformMatrix4fv(uniformDirectionalLightTransform, 1, GL_FALSE, glm::value_ptr(*transform));
}

void Shader::SetLightMatrices(std::vector<glm::mat4> lightMatrices)
{
	for (GLuint i = 0; i < 6; i++)
	{
		glUniformMatrix4fv(uniformLightMatrices[i], 1, GL_FALSE, glm::value_ptr(lightMatrices[i]));
	}
}

void Shader::SetViewMatrix(glm::mat4* viewMatrix)
{
	glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(*viewMatrix));
}

void Shader::SetProjectionMatrix(glm::mat4* projectionMatrix)
{
	glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(*projectionMatrix));
}

void Shader::Bind()
{
	glUseProgram(programID);
}

void Shader::Unbind()
{
	glUseProgram(0);
}

void Shader::ClearShader()
{
	if (programID != 0)
	{
		glDeleteProgram(programID);
		programID = 0;
	}

	uniformModel = 0;
	uniformView = 0;
	uniformProjection = 0;
}

Shader::~Shader()
{
	ClearShader();
}

void Shader::CompileShader(const char* vertexCode, const char* fragmentCode)
{
	programID = glCreateProgram();

	if (!programID)
	{
		printf("Error creating shader program!");
		return;
	}

	AddShader(programID, vertexCode, GL_VERTEX_SHADER);
	AddShader(programID, fragmentCode, GL_FRAGMENT_SHADER);

	CompileProgram();
}

void Shader::CompileShader(const char* vertexCode, const char* geometryCode, const char* fragmentCode)
{
	programID = glCreateProgram();

	if (!programID)
	{
		printf("Error creating shader program!");
		return;
	}

	AddShader(programID, vertexCode, GL_VERTEX_SHADER);
	AddShader(programID, geometryCode, GL_GEOMETRY_SHADER);
	AddShader(programID, fragmentCode, GL_FRAGMENT_SHADER);

	CompileProgram();
}

const char* Shader::GetShaderTypeNameFromEnum(const GLenum shaderType)
{
	const char* shaderTypeName = "Unknown";
	if (shaderType == GL_VERTEX_SHADER)               shaderTypeName = "Vertex";
	else if (shaderType == GL_FRAGMENT_SHADER)        shaderTypeName = "Fragment";
	else if (shaderType == GL_TESS_CONTROL_SHADER)    shaderTypeName = "Tessellation Control";
	else if (shaderType == GL_TESS_EVALUATION_SHADER) shaderTypeName = "Tessellation Evaluation";
	else if (shaderType == GL_GEOMETRY_SHADER)        shaderTypeName = "Geometry";
	else if (shaderType == GL_COMPUTE_SHADER)         shaderTypeName = "Compute";
	return shaderTypeName;
}

void Shader::AddShader(GLuint programID, const char* shaderCode, GLenum shaderType)
{
	const char* shaderTypeName = GetShaderTypeNameFromEnum(shaderType);

	GLuint shaderID = glCreateShader(shaderType);

	const GLchar* theCode[1];
	theCode[0] = shaderCode;

	GLint codeLength[1];
	codeLength[0] = (GLint)strlen(shaderCode);

	glShaderSource(shaderID, 1, theCode, codeLength);
	glCompileShader(shaderID);

	GLint result = 0;
	GLchar eLog[1024] = { 0 };

	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(shaderID, sizeof(eLog), NULL, eLog);
		printf("%s shader compilation error: '%s'\n", shaderTypeName, eLog);
		return;
	}

	printf("%s shader compiled.\n", shaderTypeName);

	glAttachShader(programID, shaderID);
	return;
}

void Shader::CompileProgram()
{
	GLint result = 0;
	GLchar eLog[1024] = { 0 };

	glLinkProgram(programID);
	glGetProgramiv(programID, GL_LINK_STATUS, &result);
	if (!result)
	{
		glGetProgramInfoLog(programID, sizeof(eLog), NULL, eLog);
		printf("Shader program linking error: '%s'\n", eLog);
		return;
	}

	printf("Shader program linking complete.\n");

	uniformModel = glGetUniformLocation(programID, "model");
	uniformView = glGetUniformLocation(programID, "view");
	uniformProjection = glGetUniformLocation(programID, "projection");
	uniformEyePosition = glGetUniformLocation(programID, "eyePosition");

	uniformDirectionalLight.uniformColor = glGetUniformLocation(programID, "directionalLight.base.color");
	uniformDirectionalLight.uniformAmbientIntensity = glGetUniformLocation(programID, "directionalLight.base.ambientIntensity");
	uniformDirectionalLight.uniformDiffuseIntensity = glGetUniformLocation(programID, "directionalLight.base.diffuseIntensity");
	uniformDirectionalLight.uniformDirection = glGetUniformLocation(programID, "directionalLight.direction");

	uniformSpecularIntensity = glGetUniformLocation(programID, "material.specularIntensity");
	uniformShininess = glGetUniformLocation(programID, "material.shininess");

	uniformPointLightCount = glGetUniformLocation(programID, "pointLightCount");

	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		char locBuff[100] = { '\0' };

		snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.color", i);
		uniformPointLight[i].uniformColor = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.ambientIntensity", i);
		uniformPointLight[i].uniformAmbientIntensity = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "pointLights[%d].base.diffuseIntensity", i);
		uniformPointLight[i].uniformDiffuseIntensity = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "pointLights[%d].position", i);
		uniformPointLight[i].uniformPosition = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "pointLights[%d].constant", i);
		uniformPointLight[i].uniformConstant = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "pointLights[%d].linear", i);
		uniformPointLight[i].uniformLinear = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "pointLights[%d].exponent", i);
		uniformPointLight[i].uniformExponent = glGetUniformLocation(programID, locBuff);
	}

	uniformSpotLightCount = glGetUniformLocation(programID, "spotLightCount");

	for (int i = 0; i < MAX_SPOT_LIGHTS; i++)
	{
		char locBuff[100] = { '\0' };

		snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.base.color", i);
		uniformSpotLight[i].uniformColor = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.base.ambientIntensity", i);
		uniformSpotLight[i].uniformAmbientIntensity = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.base.diffuseIntensity", i);
		uniformSpotLight[i].uniformDiffuseIntensity = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.position", i);
		uniformSpotLight[i].uniformPosition = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "spotLights[%d].direction", i);
		uniformSpotLight[i].uniformDirection = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.constant", i);
		uniformSpotLight[i].uniformConstant = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.linear", i);
		uniformSpotLight[i].uniformLinear = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "spotLights[%d].base.exponent", i);
		uniformSpotLight[i].uniformExponent = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "spotLights[%d].edge", i);
		uniformSpotLight[i].uniformEdge = glGetUniformLocation(programID, locBuff);
	}

	// Directional shadow map
	uniformTexture = glGetUniformLocation(programID, "theTexture");
	uniformNormalMap = glGetUniformLocation(programID, "normalMap");
	uniformDirectionalShadowMap = glGetUniformLocation(programID, "directionalShadowMap");
	uniformDirectionalLightTransform = glGetUniformLocation(programID, "directionalLightTransform");

	// Omni shadow map
	uniformOmniLightPos = glGetUniformLocation(programID, "lightPos");
	uniformFarPlane = glGetUniformLocation(programID, "farPlane");

	for (unsigned int i = 0; i < 6; i++)
	{
		char locBuff[100] = { '\0' };

		snprintf(locBuff, sizeof(locBuff), "lightMatrices[%d]", i);
		uniformLightMatrices[i] = glGetUniformLocation(programID, locBuff);
	}

	for (unsigned int i = 0; i < MAX_POINT_LIGHTS + MAX_SPOT_LIGHTS; i++)
	{
		char locBuff[100] = { '\0' };

		snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].shadowMap", i);
		uniformOmniShadowMap[i].shadowMap = glGetUniformLocation(programID, locBuff);

		snprintf(locBuff, sizeof(locBuff), "omniShadowMaps[%d].farPlane", i);
		uniformOmniShadowMap[i].farPlane = glGetUniformLocation(programID, locBuff);
	}

	// Water shader sampler2D uniforms
	uniformReflectionTexture = glGetUniformLocation(programID, "reflectionTexture");
	uniformRefractionTexture = glGetUniformLocation(programID, "refractionTexture");
	uniformDuDvMap = glGetUniformLocation(programID, "dudvMap");
	uniformDepthMap = glGetUniformLocation(programID, "depthMap");
	uniformPlane = glGetUniformLocation(programID, "plane");
	uniformWaterMoveFactor = glGetUniformLocation(programID, "waterMoveFactor");
	uniformCameraPosition = glGetUniformLocation(programID, "cameraPosition");
	uniformLightColor = glGetUniformLocation(programID, "lightColor");
	uniformLightDirection = glGetUniformLocation(programID, "lightDirection");
	uniformFarPlane = glGetUniformLocation(programID, "farPlane");
	uniformNearPlane = glGetUniformLocation(programID, "nearPlane");
}

void Shader::Validate()
{
	if (m_Validated) return;

	GLint result = 0;
	GLchar eLog[1024] = { 0 };

	glValidateProgram(programID);
	glGetProgramiv(programID, GL_VALIDATE_STATUS, &result);
	if (!result)
	{
		glGetProgramInfoLog(programID, sizeof(eLog), NULL, eLog);
		printf("Shader program [ID=%d] validation error: '%s'\n", programID, eLog);
		return;
	}

	printf("Shader program [ID=%d] validation complete.\n", programID);

	m_Validated = true;
}
