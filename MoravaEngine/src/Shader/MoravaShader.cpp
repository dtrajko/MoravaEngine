#include "Shader/MoravaShader.h"

#include "Hazel/Core/Assert.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "Core/Log.h"
#include "Core/Util.h"
#include "Platform/OpenGL/OpenGLMoravaShader.h"
#include "Platform/Vulkan/VulkanMoravaShader.h"
#include "Platform/DX11/DX11Shader.h"


std::vector<Hazel::Ref<MoravaShader>> MoravaShader::s_AllShaders;
MoravaShaderSpecification MoravaShader::s_Specification = MoravaShaderSpecification{};


// the ultimate Create method that can create both MoravaShader and HazelShader shader types
Hazel::Ref<MoravaShader> MoravaShader::Create(MoravaShaderSpecification moravaShaderSpecification)
{
	s_Specification = moravaShaderSpecification;

	Hazel::Ref<MoravaShader> moravaShader;

	if (moravaShaderSpecification.ShaderType == MoravaShaderSpecification::ShaderType::MoravaShader)
	{
		moravaShader = OpenGLMoravaShader::Create(moravaShaderSpecification.VertexShaderPath.c_str(), moravaShaderSpecification.FragmentShaderPath.c_str(), moravaShaderSpecification.ForceCompile);
	}
	else if (moravaShaderSpecification.ShaderType == MoravaShaderSpecification::ShaderType::HazelShader)
	{
		Hazel::Ref<Hazel::HazelShader> hazelShader = HazelShader::Create(moravaShaderSpecification.HazelShaderPath, moravaShaderSpecification.ForceCompile);
		moravaShader = Hazel::Ref<MoravaShader>(hazelShader);
	}
	else if (moravaShaderSpecification.ShaderType == MoravaShaderSpecification::ShaderType::DX11Shader)
	{
		Hazel::Ref<DX11Shader> dx11Shader = Hazel::Ref<DX11Shader>::Create(
			Util::to_wstr(moravaShaderSpecification.VertexShaderPath.c_str()).c_str(),
			Util::to_wstr(moravaShaderSpecification.PixelShaderPath.c_str()).c_str());
		moravaShader = Hazel::Ref<MoravaShader>(dx11Shader);
	}

	return moravaShader;
}

MoravaShader::MoravaShader()
{
	shaderID = 0;

	m_Name = "Untitled";

	m_UniformLocations = std::map<std::string, int>();
}

MoravaShader::MoravaShader(const char* vertexLocation, const char* fragmentLocation, bool forceCompile)
	: MoravaShader()
{
	m_ShaderFilepath_Vertex = vertexLocation;
	m_ShaderFilepath_Fragment = fragmentLocation;

	m_Name = Util::StripExtensionFromFileName(Util::GetFileNameFromFullPath(vertexLocation));

	CreateFromFiles(vertexLocation, fragmentLocation);
}

MoravaShader::MoravaShader(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation, bool forceCompile)
	: MoravaShader()
{
	m_ShaderFilepath_Vertex = vertexLocation;
	m_ShaderFilepath_Geometry = geometryLocation;
	m_ShaderFilepath_Fragment = fragmentLocation;

	m_Name = Util::StripExtensionFromFileName(Util::GetFileNameFromFullPath(vertexLocation));

	CreateFromFiles(vertexLocation, geometryLocation, fragmentLocation);
}

MoravaShader::MoravaShader(const char* computeLocation, bool forceCompile)
	: MoravaShader()
{
	m_ShaderFilepath_Compute = computeLocation;

	m_Name = Util::StripExtensionFromFileName(Util::GetFileNameFromFullPath(computeLocation));

	CreateFromFileCompute(computeLocation);

	CompileProgram();
}

Hazel::Ref<MoravaShader> MoravaShader::Create(const char* vertexLocation, const char* fragmentLocation, bool forceCompile)
{
	Hazel::Ref<MoravaShader> result = Hazel::Ref<MoravaShader>();

	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return Hazel::Ref<MoravaShader>();
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

Hazel::Ref<MoravaShader> MoravaShader::Create(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation, bool forceCompile)
{
	Hazel::Ref<MoravaShader> result = Hazel::Ref<MoravaShader>();

	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return Hazel::Ref<MoravaShader>();
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

Hazel::Ref<MoravaShader> MoravaShader::Create(const char* computeLocation, bool forceCompile)
{
	Hazel::Ref<MoravaShader> result = Hazel::Ref<MoravaShader>();

	switch (Hazel::RendererAPI::Current())
	{
	case Hazel::RendererAPIType::None: return Hazel::Ref<MoravaShader>();
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

void MoravaShader::CreateFromString(const char* vertexCode, const char* fragmentCode)
{
	CompileShader(vertexCode, fragmentCode);
}

void MoravaShader::CreateFromFiles(const char* vertexLocation, const char* fragmentLocation)
{
	std::string vertexString = ReadFile(vertexLocation);
	std::string fragmentString = ReadFile(fragmentLocation);
	const char* vertexCode = vertexString.c_str();
	const char* fragmentCode = fragmentString.c_str();
	CompileShader(vertexCode, fragmentCode);
}

void MoravaShader::CreateFromFiles(const char* vertexLocation, const char* geometryLocation, const char* fragmentLocation)
{
	std::string vertexString = ReadFile(vertexLocation);
	std::string geometryString = ReadFile(geometryLocation);
	std::string fragmentString = ReadFile(fragmentLocation);
	const char* vertexCode = vertexString.c_str();
	const char* geometryCode = geometryString.c_str();
	const char* fragmentCode = fragmentString.c_str();
	CompileShader(vertexCode, geometryCode, fragmentCode);
}

void MoravaShader::CreateFromFileVertex(const char* vertexLocation)
{
	std::string vertexString = ReadFile(vertexLocation);
	const char* vertexCode = vertexString.c_str();
	AddShaderVertex(vertexCode);
}

void MoravaShader::CreateFromFileFragment(const char* fragmentLocation)
{
	std::string fragmentString = ReadFile(fragmentLocation);
	const char* fragmentCode = fragmentString.c_str();
	AddShaderFragment(fragmentCode);
}

void MoravaShader::CreateFromFileGeometry(const char* geometryLocation)
{
	std::string geometryString = ReadFile(geometryLocation);
	const char* geometryCode = geometryString.c_str();
	AddShaderGeometry(geometryCode);
}

void MoravaShader::CreateFromFileCompute(const char* computeLocation)
{
	std::string computeString = ReadFile(computeLocation);
	const char* computeCode = computeString.c_str();
	AddShaderCompute(computeCode);
}

std::string MoravaShader::ReadFile(const char* fileLocation)
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

void MoravaShader::Validate()
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

GLuint MoravaShader::GetProgramID()
{
	return programID;
}

GLint MoravaShader::GetUniformLocation(const std::string& name)
{
	std::map<std::string, int>::const_iterator it;
	it = m_UniformLocations.find(name);

	if (it != m_UniformLocations.end()) {
		return it->second;
	} else {
		glUseProgram(programID);
		int uniformLocation = glGetUniformLocation(programID, name.c_str());
		if (uniformLocation != -1) {
			m_UniformLocations.insert(std::make_pair(name, uniformLocation));
		}
		return uniformLocation;
	}
}

const std::unordered_map<std::string, Hazel::ShaderBuffer>& MoravaShader::GetShaderBuffers() const
{
	// OpenGLMaterial::FindUniformDeclaration requires at least 2 shader buffers
	// std::unordered_map<std::string, Hazel::ShaderBuffer> shaderBuffers = ;
	// shaderBuffers.insert(std::make_pair("One", Hazel::ShaderBuffer()));
	// shaderBuffers.insert(std::make_pair("Two", Hazel::ShaderBuffer()));
	return std::unordered_map<std::string, Hazel::ShaderBuffer>();
}

const std::unordered_map<std::string, Hazel::ShaderResourceDeclaration>& MoravaShader::GetResources() const
{
	return std::unordered_map<std::string, Hazel::ShaderResourceDeclaration>();
}

void MoravaShader::AddShaderReloadedCallback(const ShaderReloadedCallback& callback)
{
}

uint32_t MoravaShader::GetRendererID() const
{
	return programID;
}

size_t MoravaShader::GetHash() const
{
	return std::hash<std::string>{}(m_ShaderFilepath_Vertex + m_ShaderFilepath_Compute + m_ShaderFilepath_Fragment);
}

void MoravaShader::Bind()
{
	Log::GetLogger()->error("MoravaShader::Bind() shouldn't be called directly!");
}

void MoravaShader::Reload(bool forceCompile)
{
	Log::GetLogger()->error("MoravaShader::Reload() shouldn't be called directly!");
}

void MoravaShader::SetIntArray(const std::string& name, int* values, uint32_t size)
{
	Log::GetLogger()->error("MoravaShader::SetIntArray() shouldn't be called directly!");
}

void MoravaShader::SetUniformBuffer(const std::string& name, const void* data, uint32_t size)
{
	Log::GetLogger()->error("MoravaShader::SetUniformBuffer() shouldn't be called directly!");
}

void MoravaShader::SetUniform(const std::string& fullname, float value)
{
	Log::GetLogger()->error("MoravaShader::SetUniform() shouldn't be called directly!");
}

void MoravaShader::SetUniform(const std::string& fullname, uint32_t value)
{
	Log::GetLogger()->error("MoravaShader::SetUniform() shouldn't be called directly!");
}

void MoravaShader::SetUniform(const std::string& fullname, int value)
{
	Log::GetLogger()->error("MoravaShader::SetUniform() shouldn't be called directly!");
}

void MoravaShader::SetUniform(const std::string& fullname, const glm::vec2& value)
{
	Log::GetLogger()->error("MoravaShader::SetUniform() shouldn't be called directly!");
}

void MoravaShader::SetUniform(const std::string& fullname, const glm::vec3& value)
{
	Log::GetLogger()->error("MoravaShader::SetUniform() shouldn't be called directly!");
}

void MoravaShader::SetUniform(const std::string& fullname, const glm::vec4& value)
{
	Log::GetLogger()->error("MoravaShader::SetUniform() shouldn't be called directly!");
}

void MoravaShader::SetUniform(const std::string& fullname, const glm::mat3& value)
{
	Log::GetLogger()->error("MoravaShader::SetUniform() shouldn't be called directly!");
}

void MoravaShader::SetUniform(const std::string& fullname, const glm::mat4& value)
{
	Log::GetLogger()->error("MoravaShader::SetUniform() shouldn't be called directly!");
}

void MoravaShader::SetFloat(const std::string& name, float value)
{
	Log::GetLogger()->error("MoravaShader::SetFloat() shouldn't be called directly!");
}

void MoravaShader::SetUInt(const std::string& name, uint32_t value)
{
	Log::GetLogger()->error("MoravaShader::SetUInt() shouldn't be called directly!");
}

void MoravaShader::SetInt(const std::string& name, int value)
{
	Log::GetLogger()->error("MoravaShader::SetInt() shouldn't be called directly!");
}

void MoravaShader::SetBool(const std::string& name, bool value)
{
	Log::GetLogger()->error("MoravaShader::SetBool() shouldn't be called directly!");
}

void MoravaShader::SetFloat2(const std::string& name, const glm::vec2& value)
{
	Log::GetLogger()->error("MoravaShader::SetFloat2() shouldn't be called directly!");
}

void MoravaShader::SetFloat3(const std::string& name, const glm::vec3& value)
{
	Log::GetLogger()->error("MoravaShader::SetFloat3() shouldn't be called directly!");
}

void MoravaShader::SetFloat4(const std::string& name, const glm::vec4& value)
{
	Log::GetLogger()->error("MoravaShader::SetFloat4() shouldn't be called directly!");
}

void MoravaShader::SetMat4(const std::string& name, const glm::mat4& value)
{
	Log::GetLogger()->error("MoravaShader::SetMat4() shouldn't be called directly!");
}

void MoravaShader::SetMat4FromRenderThread(const std::string& name, const glm::mat4& value, bool bind)
{
	Log::GetLogger()->error("MoravaShader::SetMat4FromRenderThread() shouldn't be called directly!");
}

void MoravaShader::UploadUniformMat4(const std::string& name, const glm::mat4& values)
{
	Log::GetLogger()->error("MoravaShader::UploadUniformMat4() shouldn't be called directly!");
}

void MoravaShader::UploadUniformMat4(uint32_t location, const glm::mat4& values)
{
	Log::GetLogger()->error("MoravaShader::UploadUniformMat4() shouldn't be called directly!");
}

void MoravaShader::setLightMat4(std::vector<glm::mat4> lightMatrices)
{
	Log::GetLogger()->error("MoravaShader::setLightMat4() shouldn't be called directly!");
}

void MoravaShader::Unbind()
{
	Log::GetLogger()->error("MoravaShader::Unbind() shouldn't be called directly!");
}

void MoravaShader::SetLightMatrices(std::vector<glm::mat4> lightMatrices)
{
	Log::GetLogger()->error("MoravaShader::SetLightMatrices() shouldn't be called directly!");
}

void MoravaShader::SetVec2(const std::string& name, const glm::vec2& value)
{
	Log::GetLogger()->error("MoravaShader::SetVec2() shouldn't be called directly!");
}

void MoravaShader::SetVec2(const std::string& name, float x, float y)
{
	Log::GetLogger()->error("MoravaShader::SetVec2() shouldn't be called directly!");
}

void MoravaShader::SetMat2(const std::string& name, const glm::mat2& mat)
{
	Log::GetLogger()->error("MoravaShader::SetMat2() shouldn't be called directly!");
}

void MoravaShader::SetMat3(const std::string& name, const glm::mat3& mat)
{
	Log::GetLogger()->error("MoravaShader::SetMat3() shouldn't be called directly!");
}

MoravaShader::~MoravaShader()
{
	ClearShader();
}

void MoravaShader::ClearShader()
{
}

void MoravaShader::CompileShader(const char* vertexCode, const char* fragmentCode)
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

void MoravaShader::CompileShader(const char* vertexCode, const char* geometryCode, const char* fragmentCode)
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

void MoravaShader::AddShaderVertex(const char* vertexCode)
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

void MoravaShader::AddShaderFragment(const char* fragmentCode)
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

void MoravaShader::AddShaderGeometry(const char* geometryCode)
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

void MoravaShader::AddShaderCompute(const char* computeCode)
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

const char* MoravaShader::GetShaderTypeNameFromEnum(const GLenum shaderType)
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

void MoravaShader::AddShader(GLuint programID, const char* shaderCode, GLenum shaderType)
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

void MoravaShader::CompileProgram()
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

void MoravaShader::GetUniformLocations()
{
	for (unsigned int i = 0; i < 6; i++)
	{
		char locBuff[100] = { '\0' };

		snprintf(locBuff, sizeof(locBuff), "lightMatrices[%d]", i);
		uniformLightMatrices[i] = glGetUniformLocation(programID, locBuff);
	}
}
