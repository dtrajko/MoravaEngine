#include "Shader.h"

Shader::Shader()
{
	shaderID = 0;
	uniformModel = 0;
	uniformProjection = 0;
	pointLightCount = 0;
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

	printf("Content loaded from file '%s'.\n", fileLocation);

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

GLint Shader::GetUniformLocationLightDirection()
{
	return uniformDirectionalLight.uniformDirection;
}

GLint Shader::GetUniformLocationSpecularIntensity()
{
	return uniformSpecularIntensity;
}

GLint Shader::GetUniformLocationShininess()
{
	return uniformShininess;
}

void Shader::SetDirectionalLight(DirectionalLight* directionalLight)
{
	directionalLight->UseLight(
		uniformDirectionalLight.uniformColor,
		uniformDirectionalLight.uniformAmbientIntensity,
		uniformDirectionalLight.uniformDiffuseIntensity,
		uniformDirectionalLight.uniformDirection);
}

void Shader::SetPointLights(PointLight* pointLights, unsigned int lightCount)
{
	if (lightCount > MAX_POINT_LIGHTS) lightCount = MAX_POINT_LIGHTS;

	glUniform1i(uniformPointLightCount, lightCount);

	for (size_t i = 0; i < lightCount; i++)
	{
		pointLights[i].UseLight(
			uniformPointLight[i].uniformColor,
			uniformPointLight[i].uniformAmbientIntensity,
			uniformPointLight[i].uniformDiffuseIntensity,
			uniformPointLight[i].uniformPosition,
			uniformPointLight[i].uniformConstant,
			uniformPointLight[i].uniformLinear,
			uniformPointLight[i].uniformExponent);
	}
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

	glValidateProgram(programID);
	glGetProgramiv(programID, GL_VALIDATE_STATUS, &result);
	if (!result)
	{
		glGetProgramInfoLog(programID, sizeof(eLog), NULL, eLog);
		printf("Shader program validation error: '%s'\n", eLog);
		return;
	}

	uniformModel             = glGetUniformLocation(programID, "model");
	uniformView              = glGetUniformLocation(programID, "view");
	uniformProjection        = glGetUniformLocation(programID, "projection");
	uniformEyePosition       = glGetUniformLocation(programID, "eyePosition");

	uniformDirectionalLight.uniformColor = glGetUniformLocation(programID, "directionalLight.base.color");
	uniformDirectionalLight.uniformAmbientIntensity = glGetUniformLocation(programID, "directionalLight.base.ambientIntensity");
	uniformDirectionalLight.uniformDiffuseIntensity = glGetUniformLocation(programID, "directionalLight.base.diffuseIntensity");
	uniformDirectionalLight.uniformDirection = glGetUniformLocation(programID, "directionalLight.direction");

	uniformSpecularIntensity = glGetUniformLocation(programID, "material.specularIntensity");
	uniformShininess         = glGetUniformLocation(programID, "material.shininess");

	uniformPointLightCount = glGetUniformLocation(programID, "pointLightCount");

	for (size_t i = 0; i < MAX_POINT_LIGHTS; i++)
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

	printf("Shader program validation complete.\n");
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
	codeLength[0] = strlen(shaderCode);

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
