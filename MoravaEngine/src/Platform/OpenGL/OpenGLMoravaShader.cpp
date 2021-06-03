#include "OpenGLMoravaShader.h"

#include "Hazel/Core/Assert.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "Core/Log.h"
#include "Core/Util.h"


OpenGLMoravaShader::OpenGLMoravaShader()
{
	shaderID = 0;

	m_Name = "Untitled";

	m_UniformLocations = std::map<std::string, int>();
}

OpenGLMoravaShader::OpenGLMoravaShader(const char* vertexLocation, const char* fragmentLocation, bool forceCompile)
	: OpenGLMoravaShader()
{
	m_ShaderFilepath_Vertex = vertexLocation;
	m_ShaderFilepath_Fragment = fragmentLocation;

	m_Name = Util::StripExtensionFromFileName(Util::GetFileNameFromFullPath(vertexLocation));

	CreateFromFiles(vertexLocation, fragmentLocation);
}

OpenGLMoravaShader::OpenGLMoravaShader(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation, bool forceCompile)
	: OpenGLMoravaShader()
{
	m_ShaderFilepath_Vertex = vertexLocation;
	m_ShaderFilepath_Geometry = geometryLocation;
	m_ShaderFilepath_Fragment = fragmentLocation;

	m_Name = Util::StripExtensionFromFileName(Util::GetFileNameFromFullPath(vertexLocation));

	CreateFromFiles(vertexLocation, geometryLocation, fragmentLocation);
}

OpenGLMoravaShader::OpenGLMoravaShader(const char* computeLocation, bool forceCompile)
	: OpenGLMoravaShader()
{
	m_ShaderFilepath_Compute = computeLocation;

	m_Name = Util::StripExtensionFromFileName(Util::GetFileNameFromFullPath(computeLocation));

	CreateFromFileCompute(computeLocation);

	CompileProgram();
}

Hazel::Ref<OpenGLMoravaShader> OpenGLMoravaShader::Create(const char* vertexLocation, const char* fragmentLocation, bool forceCompile)
{
	Hazel::Ref<OpenGLMoravaShader> result = Hazel::Ref<OpenGLMoravaShader>();

	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return Hazel::Ref<OpenGLMoravaShader>();
	case Hazel::RendererAPIType::OpenGL:
		result = Hazel::Ref<OpenGLMoravaShader>::Create(vertexLocation, fragmentLocation, forceCompile);
		break;
	case Hazel::RendererAPIType::Vulkan:
		Log::GetLogger()->error("Not implemented for Vulkan API!");
		break;
	}
	s_AllShaders.push_back(result);
	return result;
}

Hazel::Ref<OpenGLMoravaShader> OpenGLMoravaShader::Create(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation, bool forceCompile)
{
	Hazel::Ref<OpenGLMoravaShader> result = Hazel::Ref<OpenGLMoravaShader>();

	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return Hazel::Ref<OpenGLMoravaShader>();
	case Hazel::RendererAPIType::OpenGL:
		result = Hazel::Ref<OpenGLMoravaShader>::Create(vertexLocation, geometryLocation, fragmentLocation, forceCompile);
		break;
	case Hazel::RendererAPIType::Vulkan:
		Log::GetLogger()->error("Not implemented for Vulkan API!");
		break;
	}
	s_AllShaders.push_back(result);
	return result;
}

Hazel::Ref<OpenGLMoravaShader> OpenGLMoravaShader::Create(const char* computeLocation, bool forceCompile)
{
	Hazel::Ref<OpenGLMoravaShader> result = Hazel::Ref<OpenGLMoravaShader>();

	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return Hazel::Ref<OpenGLMoravaShader>();
	case Hazel::RendererAPIType::OpenGL:
		result = Hazel::Ref<OpenGLMoravaShader>::Create(computeLocation, forceCompile);
		break;
	case Hazel::RendererAPIType::Vulkan:
		Log::GetLogger()->error("Not implemented for Vulkan API!");
		break;
	}
	s_AllShaders.push_back(result);
	return result;
}

void OpenGLMoravaShader::CreateFromString(const char* vertexCode, const char* fragmentCode)
{
	CompileShader(vertexCode, fragmentCode);
}

void OpenGLMoravaShader::CreateFromFiles(const char* vertexLocation, const char* fragmentLocation)
{
	std::string vertexString = ReadFile(vertexLocation);
	std::string fragmentString = ReadFile(fragmentLocation);
	const char* vertexCode = vertexString.c_str();
	const char* fragmentCode = fragmentString.c_str();
	CompileShader(vertexCode, fragmentCode);
}

void OpenGLMoravaShader::CreateFromFiles(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation)
{
	std::string vertexString = ReadFile(vertexLocation);
	std::string geometryString = ReadFile(geometryLocation);
	std::string fragmentString = ReadFile(fragmentLocation);
	const char* vertexCode = vertexString.c_str();
	const char* geometryCode = geometryString.c_str();
	const char* fragmentCode = fragmentString.c_str();
	CompileShader(vertexCode, geometryCode, fragmentCode);
}

void OpenGLMoravaShader::CreateFromFileVertex(const char* vertexLocation)
{
	std::string vertexString = ReadFile(vertexLocation);
	const char* vertexCode = vertexString.c_str();
	AddShaderVertex(vertexCode);
}

void OpenGLMoravaShader::CreateFromFileFragment(const char* fragmentLocation)
{
	std::string fragmentString = ReadFile(fragmentLocation);
	const char* fragmentCode = fragmentString.c_str();
	AddShaderFragment(fragmentCode);
}

void OpenGLMoravaShader::CreateFromFileGeometry(const char* geometryLocation)
{
	std::string geometryString = ReadFile(geometryLocation);
	const char* geometryCode = geometryString.c_str();
	AddShaderGeometry(geometryCode);
}

void OpenGLMoravaShader::CreateFromFileCompute(const char* computeLocation)
{
	std::string computeString = ReadFile(computeLocation);
	const char* computeCode = computeString.c_str();
	AddShaderCompute(computeCode);
}

std::string OpenGLMoravaShader::ReadFile(const char* fileLocation)
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

void OpenGLMoravaShader::Validate()
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

GLuint OpenGLMoravaShader::GetProgramID()
{
	return programID;
}

void OpenGLMoravaShader::setVec2(const std::string& name, const glm::vec2& value)
{
	glUniform2fv(GetUniformLocation(name), 1, &value[0]);
}

void OpenGLMoravaShader::setVec2(const std::string& name, float x, float y)
{
	glUniform2f(GetUniformLocation(name), x, y);
}

void OpenGLMoravaShader::setMat2(const std::string& name, const glm::mat2& mat)
{
	glUniformMatrix2fv(GetUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

void OpenGLMoravaShader::setMat3(const std::string& name, const glm::mat3& mat)
{
	glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, &mat[0][0]);
}

GLint OpenGLMoravaShader::GetUniformLocation(const std::string& name)
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

void OpenGLMoravaShader::SetIntArray(const std::string& name, int* values, uint32_t size)
{
	glUniform1iv(GetUniformLocation(name), size, values);
}

const std::string& OpenGLMoravaShader::GetName() const
{
	return std::string();
}

const std::unordered_map<std::string, Hazel::ShaderBuffer>& OpenGLMoravaShader::GetShaderBuffers() const
{
	// OpenGLMaterial::FindUniformDeclaration requires at least 2 shader buffers
	std::unordered_map<std::string, Hazel::ShaderBuffer> shaderBuffers = std::unordered_map<std::string, Hazel::ShaderBuffer>();
	shaderBuffers.insert(std::make_pair("One", Hazel::ShaderBuffer()));
	shaderBuffers.insert(std::make_pair("Two", Hazel::ShaderBuffer()));
	return shaderBuffers;
}

const std::unordered_map<std::string, Hazel::ShaderResourceDeclaration>& OpenGLMoravaShader::GetResources() const
{
	return std::unordered_map<std::string, Hazel::ShaderResourceDeclaration>();
}

void OpenGLMoravaShader::AddShaderReloadedCallback(const ShaderReloadedCallback& callback)
{
}

uint32_t OpenGLMoravaShader::GetRendererID() const
{
	return programID;
}

size_t OpenGLMoravaShader::GetHash() const
{
	return std::hash<std::string>{}(m_ShaderFilepath_Vertex + m_ShaderFilepath_Compute + m_ShaderFilepath_Fragment);
}

void OpenGLMoravaShader::SetUniformBuffer(const std::string& name, const void* data, uint32_t size)
{
}

void OpenGLMoravaShader::SetUniform(const std::string& fullname, float value)
{
	HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
	GLint location = m_UniformLocations.at(fullname);
	glUniform1f(location, value);
}

void OpenGLMoravaShader::SetUniform(const std::string& fullname, uint32_t value)
{
	HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
	GLint location = m_UniformLocations.at(fullname);
	glUniform1ui(location, value);
}

void OpenGLMoravaShader::SetUniform(const std::string& fullname, int value)
{
	HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
	GLint location = m_UniformLocations.at(fullname);
	glUniform1i(location, value);
}

void OpenGLMoravaShader::SetUniform(const std::string& fullname, const glm::vec2& value)
{
	HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
	GLint location = m_UniformLocations.at(fullname);
	glUniform2fv(location, 1, glm::value_ptr(value));
}

void OpenGLMoravaShader::SetUniform(const std::string& fullname, const glm::vec3& value)
{
	HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
	GLint location = m_UniformLocations.at(fullname);
	glUniform3fv(location, 1, glm::value_ptr(value));
}

void OpenGLMoravaShader::SetUniform(const std::string& fullname, const glm::vec4& value)
{
	HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
	GLint location = m_UniformLocations.at(fullname);
	glUniform4fv(location, 1, glm::value_ptr(value));
}

void OpenGLMoravaShader::SetUniform(const std::string& fullname, const glm::mat3& value)
{
	HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
	GLint location = m_UniformLocations.at(fullname);
	glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void OpenGLMoravaShader::SetUniform(const std::string& fullname, const glm::mat4& value)
{
	HZ_CORE_ASSERT(m_UniformLocations.find(fullname) != m_UniformLocations.end());
	GLint location = m_UniformLocations.at(fullname);
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void OpenGLMoravaShader::SetFloat(const std::string& name, float value)
{
	glUseProgram(programID);
	auto location = glGetUniformLocation(programID, name.c_str());
	if (location != -1)
		glUniform1f(location, value);
	else
		Log::GetLogger()->error("Uniform '{0}' not found!", name);
}

void OpenGLMoravaShader::SetUInt(const std::string& name, uint32_t value)
{
	int32_t location = GetUniformLocation(name);
	glUniform1ui(location, value);
}

void OpenGLMoravaShader::SetInt(const std::string& name, int value)
{
	int32_t location = GetUniformLocation(name);
	glUniform1i(location, value);
}

void OpenGLMoravaShader::SetBool(const std::string& name, bool value)
{
	int32_t location = GetUniformLocation(name);
	glUniform1i(location, value);
}

void OpenGLMoravaShader::SetFloat2(const std::string& name, const glm::vec2& value)
{
	glUseProgram(programID);
	auto location = glGetUniformLocation(programID, name.c_str());
	if (location != -1) {
		glUniform2f(location, value.x, value.y);
	}
	else {
		Log::GetLogger()->error("Uniform '{0}' not found!", name);
	}
}

void OpenGLMoravaShader::SetFloat3(const std::string& name, const glm::vec3& value)
{
	glUseProgram(programID);
	auto location = glGetUniformLocation(programID, name.c_str());
	if (location != -1) {
		glUniform3f(location, value.x, value.y, value.z);
	}
	else {
		Log::GetLogger()->error("Uniform '{0}' not found!", name);
	}
}

void OpenGLMoravaShader::SetFloat4(const std::string& name, const glm::vec4& value)
{
	glUseProgram(programID);
	auto location = glGetUniformLocation(programID, name.c_str());
	if (location != -1) {
		glUniform4f(location, value.x, value.y, value.z, value.w);
	}
	else {
		Log::GetLogger()->error("Uniform '{0}' not found!", name);
	}
}

void OpenGLMoravaShader::SetMat4(const std::string& name, const glm::mat4& value)
{
	glUseProgram(programID);
	auto location = glGetUniformLocation(programID, name.c_str());
	if (location != -1)
		glUniformMatrix4fv(location, 1, GL_FALSE, (const float*)&value);
	else
		Log::GetLogger()->error("Uniform '{0}' not found!", name);
}

void OpenGLMoravaShader::SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind)
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

void OpenGLMoravaShader::UploadUniformMat4(const std::string& name, const glm::mat4& values)
{
	glUseProgram(programID);
	auto location = glGetUniformLocation(programID, name.c_str());
	if (location != -1)
		glUniformMatrix4fv(location, 1, GL_FALSE, (const float*)&values);
	else
		Log::GetLogger()->error("Uniform '{0}' not found!", name);
}

void OpenGLMoravaShader::UploadUniformMat4(uint32_t location, const glm::mat4& value)
{
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void OpenGLMoravaShader::SetLightMatrices(std::vector<glm::mat4> lightMatrices)
{
	for (GLuint i = 0; i < 6; i++)
	{
		glUniformMatrix4fv(uniformLightMatrices[i], 1, GL_FALSE, glm::value_ptr(lightMatrices[i]));
	}
}

void OpenGLMoravaShader::setLightMat4(std::vector<glm::mat4> lightMatrices)
{
	for (GLuint i = 0; i < 6; i++)
	{
		std::string name = "lightMatrices[" + std::to_string(i) + "]";
		SetMat4(name, lightMatrices[i]);
	}
}

void OpenGLMoravaShader::Bind()
{
	glUseProgram(programID);
}

void OpenGLMoravaShader::Reload(bool forceCompile)
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

void OpenGLMoravaShader::Unbind()
{
	glUseProgram(0);
}

void OpenGLMoravaShader::ClearShader()
{
	if (programID > 0)
	{
		glDeleteProgram(programID);
		programID = 0;
	}

	m_UniformLocations.clear();
}

OpenGLMoravaShader::~OpenGLMoravaShader()
{
	ClearShader();
}

void OpenGLMoravaShader::CompileShader(const char* vertexCode, const char* fragmentCode)
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

void OpenGLMoravaShader::CompileShader(const char* vertexCode, const char* geometryCode, const char* fragmentCode)
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

void OpenGLMoravaShader::AddShaderVertex(const char* vertexCode)
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

void OpenGLMoravaShader::AddShaderFragment(const char* fragmentCode)
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

void OpenGLMoravaShader::AddShaderGeometry(const char* geometryCode)
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

void OpenGLMoravaShader::AddShaderCompute(const char* computeCode)
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

const char* OpenGLMoravaShader::GetShaderTypeNameFromEnum(const GLenum shaderType)
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

void OpenGLMoravaShader::AddShader(GLuint programID, const char* shaderCode, GLenum shaderType)
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

void OpenGLMoravaShader::CompileProgram()
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

void OpenGLMoravaShader::GetUniformLocations()
{
	for (unsigned int i = 0; i < 6; i++)
	{
		char locBuff[100] = { '\0' };

		snprintf(locBuff, sizeof(locBuff), "lightMatrices[%d]", i);
		uniformLightMatrices[i] = glGetUniformLocation(programID, locBuff);
	}
}
