#include "Shader.h"


Shader::Shader()
{
	shaderID = 0;
	uniformModel = 0;
	uniformProjection = 0;
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

GLuint Shader::GetUniformLocationNearPlane()
{
	return uniformNearPlane;
}

GLuint Shader::GetUniformLocationFarPlane()
{
	return uniformFarPlane;
}

GLint Shader::GetUniformLocationNormalMap()
{
	return uniformNormalMap;
}

GLint Shader::GetUniformLocationDepthMap()
{
	return uniformDepthMap;
}

GLint Shader::GetUniformLocationLightPosition()
{
	return uniformLightPosition;
}

void Shader::SetNearPlane(float nearPlane)
{
	glUniform1f(uniformNearPlane, nearPlane);
}

void Shader::SetFarPlane(float farPlane)
{
	glUniform1f(uniformFarPlane, farPlane);
}

void Shader::SetLightPosition(glm::vec3 lightPosition)
{
	glUniform3f(uniformLightPosition, lightPosition.x, lightPosition.y, lightPosition.z);
}

void Shader::SetViewMatrix(glm::mat4* viewMatrix)
{
	glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(*viewMatrix));
}

void Shader::SetProjectionMatrix(glm::mat4* projectionMatrix)
{
	glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(*projectionMatrix));
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

void Shader::SetDirectionalLightTransform(glm::mat4* transform)
{
	glUniformMatrix4fv(uniformDirectionalLightTransform, 1, GL_FALSE, glm::value_ptr(*transform));
}

void Shader::SetDirectionalShadowMap(GLuint textureUnit)
{
	glUniform1i(uniformDirectionalShadowMap, textureUnit);
}

void Shader::SetLightMatrices(std::vector<glm::mat4> lightMatrices)
{
	for (GLuint i = 0; i < 6; i++)
	{
		glUniformMatrix4fv(uniformLightMatrices[i], 1, GL_FALSE, glm::value_ptr(lightMatrices[i]));
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

	GetUniformLocations();
}

void Shader::GetUniformLocations()
{
	printf("Shader::GetUniformLocations\n");

	uniformModel = glGetUniformLocation(programID, "model");
	uniformView = glGetUniformLocation(programID, "view");
	uniformProjection = glGetUniformLocation(programID, "projection");

	uniformNearPlane = glGetUniformLocation(programID, "nearPlane");
	uniformFarPlane = glGetUniformLocation(programID, "farPlane");
	uniformLightPosition = glGetUniformLocation(programID, "lightPosition");

	uniformNormalMap = glGetUniformLocation(programID, "normalMap");
	uniformDirectionalLightTransform = glGetUniformLocation(programID, "directionalLightTransform");

	for (unsigned int i = 0; i < 6; i++)
	{
		char locBuff[100] = { '\0' };

		snprintf(locBuff, sizeof(locBuff), "lightMatrices[%d]", i);
		uniformLightMatrices[i] = glGetUniformLocation(programID, locBuff);
	}
}
