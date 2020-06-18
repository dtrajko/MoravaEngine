#include "Shader.h"
#include "Log.h"


Shader::Shader()
{
	shaderID = 0;
}

Shader::Shader(const char* vertexLocation, const char* fragmentLocation)
	: Shader()
{
	CreateFromFiles(vertexLocation, fragmentLocation);
}

Shader::Shader(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation)
	: Shader()
{
	CreateFromFiles(vertexLocation, geometryLocation, fragmentLocation);
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
		printf("Failed to read '{0}'! File doesn't exist.\n", fileLocation);
		return "";
	}

	std::string line = "";
	while (!fileStream.eof())
	{
		std::getline(fileStream, line);
		content.append(line + "\n");
	}

	fileStream.close();

	// printf("Content loaded from file '%s'\n", fileLocation);
	LOG_INFO("Content loaded from file '{0}'", fileLocation);

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
		// printf("Shader program [ID=%d] validation error: '%s'\n", programID, eLog);
		LOG_ERROR("Shader program [ID={0}] validation error: '{1}'", programID, eLog);
		return;
	}

	// printf("Shader program [ID=%d] validation complete.\n", programID);
	LOG_INFO("Shader program [ID={0}] validation complete.", programID);

	m_Validated = true;
}

GLuint Shader::GetProgramID()
{
	return programID;
}

// utility uniform functions
void Shader::setBool(const std::string& name, bool value)
{
	glUniform1i(GetUniformLocation(name), (int)value);
}

void Shader::setInt(const std::string& name, int value)
{
	glUniform1i(GetUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value)
{
	glUniform1f(GetUniformLocation(name), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value)
{
	glUniform2fv(GetUniformLocation(name), 1, &value[0]);
}

void Shader::setVec2(const std::string& name, float x, float y)
{
	glUniform2f(GetUniformLocation(name), x, y);
}
void Shader::setVec3(const std::string& name, const glm::vec3& value)
{
	glUniform3fv(GetUniformLocation(name), 1, &value[0]);
}

void Shader::setVec3(const std::string& name, float x, float y, float z)
{
	glUniform3f(GetUniformLocation(name), x, y, z);
}

void Shader::setVec4(const std::string& name, const glm::vec4& value)
{
	glUniform4fv(GetUniformLocation(name), 1, &value[0]);
}

void Shader::setVec4(const std::string& name, float x, float y, float z, float w)
{
	glUniform4f(GetUniformLocation(name), x, y, z, w);
}

void Shader::setMat2(const std::string& name, const glm::mat2& mat)
{
	glUniformMatrix2fv(GetUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

void Shader::setMat3(const std::string& name, const glm::mat3& mat)
{
	glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat)
{
	glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

GLint Shader::GetUniformLocation(const std::string& name)
{
	std::map<std::string, int>::const_iterator it;
	it = m_UniformLocations.find(name.c_str());

	if (it != m_UniformLocations.end())
	{
		// printf("GetUniformLocation CACHE HIT [name: %s, value: %d]\n", name.c_str(), it->second);
		return it->second;
	}
	else
	{
		int uniformLocation = glGetUniformLocation(programID, name.c_str());
		m_UniformLocations.insert(std::make_pair(name, uniformLocation));
		// printf("GetUniformLocation CACHE MISS [name: %s, value: %d]\n", name.c_str(), uniformLocation);
		return uniformLocation;
	}
}

void Shader::SetLightMatrices(std::vector<glm::mat4> lightMatrices)
{
	for (GLuint i = 0; i < 6; i++)
	{
		glUniformMatrix4fv(uniformLightMatrices[i], 1, GL_FALSE, glm::value_ptr(lightMatrices[i]));
	}
}

void Shader::setLightMat4(std::vector<glm::mat4> lightMatrices)
{
	for (GLuint i = 0; i < 6; i++)
	{
		std::string name = "lightMatrices[" + std::to_string(i) + "]";
		setMat4(name, lightMatrices[i]);
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

	m_UniformLocations.clear();
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
		// printf("Error creating shader program!\n");
		LOG_ERROR("Error creating shader program!");
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
		// printf("Error creating shader program!\n");
		LOG_ERROR("Error creating shader program!");
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
		// printf("%s shader compilation error: '%s'\n", shaderTypeName, eLog);
		LOG_ERROR("%s shader compilation error: '{0}'", shaderTypeName, eLog);
		return;
	}

	// printf("%s shader compiled.\n", shaderTypeName);
	LOG_INFO("{0} shader compiled.", shaderTypeName);

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
		// printf("Shader program linking error: '%s'\n", eLog);
		LOG_ERROR("Shader program linking error: '{0}'", eLog);
		
		return;
	}

	// printf("Shader program linking complete.\n");
	LOG_INFO("Shader program linking complete.");

	GetUniformLocations();
}

void Shader::GetUniformLocations()
{
	printf("Shader::GetUniformLocations\n");

	for (unsigned int i = 0; i < 6; i++)
	{
		char locBuff[100] = { '\0' };

		snprintf(locBuff, sizeof(locBuff), "lightMatrices[%d]", i);
		uniformLightMatrices[i] = glGetUniformLocation(programID, locBuff);
	}
}
