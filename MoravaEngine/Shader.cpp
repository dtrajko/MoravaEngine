#include "Shader.h"
#include "Log.h"
#include "Util.h"

#include "Hazel/Core/Assert.h"


Shader::Shader()
{
	shaderID = 0;

	m_Name = "Untitled";

	m_UniformLocations = std::map<std::string, int>();
}

Shader::Shader(const char* vertexLocation, const char* fragmentLocation)
	: Shader()
{
	m_ShaderFilepath_Vertex = vertexLocation;
	m_ShaderFilepath_Fragment = fragmentLocation;

	m_Name = Util::StripExtensionFromFileName(Util::GetFileNameFromFullPath(vertexLocation));

	CreateFromFiles(vertexLocation, fragmentLocation);
}

Shader::Shader(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation)
	: Shader()
{
	m_ShaderFilepath_Vertex = vertexLocation;
	m_ShaderFilepath_Geometry = geometryLocation;
	m_ShaderFilepath_Fragment = fragmentLocation;

	m_Name = Util::StripExtensionFromFileName(Util::GetFileNameFromFullPath(vertexLocation));

	CreateFromFiles(vertexLocation, geometryLocation, fragmentLocation);
}

Shader::Shader(const char* computeLocation)
	: Shader()
{
	m_ShaderFilepath_Compute = computeLocation;

	m_Name = Util::StripExtensionFromFileName(Util::GetFileNameFromFullPath(computeLocation));

	CreateFromFileCompute(computeLocation);

	CompileProgram();
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

void Shader::CreateFromFileVertex(const char* vertexLocation)
{
	std::string vertexString = ReadFile(vertexLocation);
	const char* vertexCode = vertexString.c_str();
	AddShaderVertex(vertexCode);
}

void Shader::CreateFromFileFragment(const char* fragmentLocation)
{
	std::string fragmentString = ReadFile(fragmentLocation);
	const char* fragmentCode = fragmentString.c_str();
	AddShaderFragment(fragmentCode);
}

void Shader::CreateFromFileGeometry(const char* geometryLocation)
{
	std::string geometryString = ReadFile(geometryLocation);
	const char* geometryCode = geometryString.c_str();
	AddShaderGeometry(geometryCode);
}

void Shader::CreateFromFileCompute(const char* computeLocation)
{
	std::string computeString = ReadFile(computeLocation);
	const char* computeCode = computeString.c_str();
	AddShaderCompute(computeCode);
}

std::string Shader::ReadFile(const char* fileLocation)
{
	std::string content;
	std::ifstream fileStream(fileLocation, std::ios::in);

	if (!fileStream.is_open())
	{
		LOG_ERROR("Failed to read '{0}'! File doesn't exist.", fileLocation);
		return "";
	}

	std::string line = "";
	while (!fileStream.eof())
	{
		std::getline(fileStream, line);
		content.append(line + "\n");
	}

	fileStream.close();

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
		LOG_ERROR("Shader program [ID={0}] validation error: '{1}'", programID, eLog);
		return;
	}

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
		return it->second;
	}
	else
	{
		int uniformLocation = glGetUniformLocation(programID, name.c_str());
		m_UniformLocations.insert(std::make_pair(name, uniformLocation));
		return uniformLocation;
	}
}

void Shader::SetIntArray(const std::string& name, int* values, uint32_t size)
{
	glUniform1iv(GetUniformLocation(name), size, values);
}

const std::string& Shader::GetName() const
{
	return std::string();
}

const std::unordered_map<std::string, Hazel::ShaderBuffer>& Shader::GetShaderBuffers() const
{
	return std::unordered_map<std::string, Hazel::ShaderBuffer>();
}

const std::unordered_map<std::string, Hazel::ShaderResourceDeclaration>& Shader::GetResources() const
{
	return std::unordered_map<std::string, Hazel::ShaderResourceDeclaration>();
}

void Shader::AddShaderReloadedCallback(const ShaderReloadedCallback& callback)
{
}

uint32_t Shader::GetRendererID() const
{
	return programID;
}

void Shader::SetUniformBuffer(const std::string& name, const void* data, uint32_t size)
{
}

void Shader::SetUniform(const std::string& fullname, float value)
{
	HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
	GLint location = m_UniformLocations.at(fullname);
	glUniform1f(location, value);
}

void Shader::SetUniform(const std::string& fullname, int value)
{
	HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
	GLint location = m_UniformLocations.at(fullname);
	glUniform1i(location, value);
}

void Shader::SetUniform(const std::string& fullname, const glm::vec2& value)
{
	HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
	GLint location = m_UniformLocations.at(fullname);
	glUniform2fv(location, 1, glm::value_ptr(value));
}

void Shader::SetUniform(const std::string& fullname, const glm::vec3& value)
{
	HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
	GLint location = m_UniformLocations.at(fullname);
	glUniform3fv(location, 1, glm::value_ptr(value));
}

void Shader::SetUniform(const std::string& fullname, const glm::vec4& value)
{
	HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
	GLint location = m_UniformLocations.at(fullname);
	glUniform4fv(location, 1, glm::value_ptr(value));
}

void Shader::SetUniform(const std::string& fullname, const glm::mat3& value)
{
	HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
	GLint location = m_UniformLocations.at(fullname);
	glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::SetUniform(const std::string& fullname, const glm::mat4& value)
{
	HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
	GLint location = m_UniformLocations.at(fullname);
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::SetFloat(const std::string& name, float value)
{
	glUseProgram(programID);
	auto location = glGetUniformLocation(programID, name.c_str());
	if (location != -1)
		glUniform1f(location, value);
	else
		Log::GetLogger()->error("Uniform '{0}' not found!", name);
}

void Shader::SetInt(const std::string& name, int value)
{
	int32_t location = GetUniformLocation(name);
	glUniform1i(location, value);
}

void Shader::SetBool(const std::string& name, bool value)
{
	int32_t location = GetUniformLocation(name);
	glUniform1i(location, value);
}

void Shader::SetFloat3(const std::string& name, const glm::vec3& value)
{
	glUseProgram(programID);
	auto location = glGetUniformLocation(programID, name.c_str());
	if (location != -1)
		glUniform3f(location, value.x, value.y, value.z);
	else
		Log::GetLogger()->error("Uniform '{0}' not found!", name);
}

void Shader::SetMat4(const std::string& name, const glm::mat4& value)
{
	glUseProgram(programID);
	auto location = glGetUniformLocation(programID, name.c_str());
	if (location != -1)
		glUniformMatrix4fv(location, 1, GL_FALSE, (const float*)&value);
	else
		Log::GetLogger()->error("Uniform '{0}' not found!", name);
}

void Shader::SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind)
{
	if (bind)
	{
		UploadUniformMat4(name, value);
	}
	else
	{
		int location = glGetUniformLocation(programID, name.c_str());
		if (location != -1)
			UploadUniformMat4(location, value);
	}
}

void Shader::UploadUniformMat4(const std::string& name, const glm::mat4& values)
{
	glUseProgram(programID);
	auto location = glGetUniformLocation(programID, name.c_str());
	if (location != -1)
		glUniformMatrix4fv(location, 1, GL_FALSE, (const float*)&values);
	else
		Log::GetLogger()->error("Uniform '{0}' not found!", name);
}

void Shader::UploadUniformMat4(uint32_t location, const glm::mat4& value)
{
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
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

void Shader::Reload()
{
	ClearShader();

	programID = glCreateProgram();

	if (!m_ShaderFilepath_Vertex.empty()) {
		CreateFromFileVertex(m_ShaderFilepath_Vertex.c_str());
	}

	if (!m_ShaderFilepath_Fragment.empty()) {
		CreateFromFileFragment(m_ShaderFilepath_Fragment.c_str());
	}

	if (!m_ShaderFilepath_Geometry.empty()) {
		CreateFromFileGeometry(m_ShaderFilepath_Geometry.c_str());
	}

	if (!m_ShaderFilepath_Compute.empty()) {
		CreateFromFileCompute(m_ShaderFilepath_Compute.c_str());
	}

	CompileProgram();
}

void Shader::Unbind()
{
	glUseProgram(0);
}

void Shader::ClearShader()
{
	if (programID > 0)
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
		LOG_ERROR("Error creating shader program!");
		return;
	}

	AddShader(programID, vertexCode,   GL_VERTEX_SHADER);
	AddShader(programID, geometryCode, GL_GEOMETRY_SHADER);
	AddShader(programID, fragmentCode, GL_FRAGMENT_SHADER);

	CompileProgram();
}

void Shader::AddShaderVertex(const char* vertexCode)
{
	if (programID == -1) {
		programID = glCreateProgram();
	}

	if (!programID)
	{
		LOG_ERROR("Program ID not valid (VERTEX shader)!");
		return;
	}

	AddShader(programID, vertexCode, GL_VERTEX_SHADER);
}

void Shader::AddShaderFragment(const char* fragmentCode)
{
	if (programID == -1) {
		programID = glCreateProgram();
	}

	if (!programID)
	{
		LOG_ERROR("Program ID not valid (FRAGMENT shader)!");
		return;
	}

	AddShader(programID, fragmentCode, GL_FRAGMENT_SHADER);
}

void Shader::AddShaderGeometry(const char* geometryCode)
{
	if (programID == -1) {
		programID = glCreateProgram();
	}

	if (!programID)
	{
		LOG_ERROR("Program ID not valid (GEOMETRY shader)!");
		return;
	}

	AddShader(programID, geometryCode, GL_GEOMETRY_SHADER);
}

void Shader::AddShaderCompute(const char* computeCode)
{
	if (programID == -1) {
		programID = glCreateProgram();
	}

	if (!programID)
	{
		LOG_ERROR("Program ID not valid (COMPUTE shader)!");
		return;
	}

	AddShader(programID, computeCode, GL_COMPUTE_SHADER);
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

	// Log::GetLogger()->info("-- BEGIN shader code --");
	// Log::GetLogger()->info("{0}", shaderCode);
	// Log::GetLogger()->info("-- END shader code --");

	GLint result = 0;
	GLchar eLog[1024] = { 0 };

	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(shaderID, sizeof(eLog), NULL, eLog);
		LOG_ERROR("{0} shader compilation error: '{1}'", shaderTypeName, eLog);
		return;
	}

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
		LOG_ERROR("Shader program linking error: '{0}'", eLog);
		
		return;
	}

	LOG_INFO("Shader program linking complete.");

	GetUniformLocations();
}

void Shader::GetUniformLocations()
{
	for (unsigned int i = 0; i < 6; i++)
	{
		char locBuff[100] = { '\0' };

		snprintf(locBuff, sizeof(locBuff), "lightMatrices[%d]", i);
		uniformLightMatrices[i] = glGetUniformLocation(programID, locBuff);
	}
}
