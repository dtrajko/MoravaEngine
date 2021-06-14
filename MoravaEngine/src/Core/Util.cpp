#include "Core/Log.h"
#include "Core/Util.h"

#include "Hazel/Core/Assert.h"

#include <GL/glew.h>

#include <algorithm>


void Util::OpenGLLogMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:
		Log::GetLogger()->error("[OpenGL Debug HIGH] {0}", message);
		// HZ_CORE_ASSERT(false, "GL_DEBUG_SEVERITY_HIGH");
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		Log::GetLogger()->warn("[OpenGL Debug MEDIUM] {0}", message);
		break;
	case GL_DEBUG_SEVERITY_LOW:
		Log::GetLogger()->info("[OpenGL Debug LOW] {0}", message);
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		Log::GetLogger()->trace("[OpenGL Debug NOTIFICATION] {0}", message);
		break;
	}
}

char* Util::printTime()
{
	// Declaring argument for time() 
	time_t tt;

	// Declaring variable to store return value of 
	// localtime() 
	struct tm* ti;

	// Applying time()
	time(&tt);

	// Using localtime() 
	ti = localtime(&tt);

	return asctime(ti);
}

void Util::printVec3(glm::vec3 vec)
{
	std::cout << "[x]: " << vec.x << " [y]: " << vec.y << " [z]: " << vec.z << std::endl;
}

void Util::printVec4(glm::vec4 vec)
{
	std::cout << "[x]: " << vec.x << " [y]: " << vec.y << " [z]: " << vec.z << " [w]: " << vec.w << std::endl;
}

void Util::printMatrix(glm::mat4 mat)
{
	std::cout << "[00]: " << mat[0][0] << " [10]: " << mat[1][0] << " [20]: " << mat[2][0] << " [30]: " << mat[3][0] << std::endl;
	std::cout << "[01]: " << mat[0][1] << " [12]: " << mat[1][1] << " [21]: " << mat[2][1] << " [31]: " << mat[3][1] << std::endl;
	std::cout << "[02]: " << mat[0][2] << " [13]: " << mat[1][2] << " [22]: " << mat[2][2] << " [32]: " << mat[3][2] << std::endl;
	std::cout << "[03]: " << mat[0][3] << " [14]: " << mat[1][3] << " [23]: " << mat[2][3] << " [33]: " << mat[3][3] << std::endl;
}

std::string Util::GetFileNameFromFullPath(std::string fullPath)
{
	std::string fileName = "";
	if (fullPath != "")
	{
		size_t lastSlashPosition = fullPath.find_last_of("/\\");
		fileName = lastSlashPosition != std::string::npos ? fullPath.substr(lastSlashPosition + 1) : fullPath;
	}
	return fileName;
}

std::string Util::StripExtensionFromFileName(std::string fileName)
{
	std::string fileNameNoExt = "";
	if (fileName != "")
	{
		size_t lastSlashPosition = fileName.find_last_of("/\\");
		size_t lastDotPosition = fileName.find_last_of(".");
		fileNameNoExt = lastDotPosition != std::string::npos ? fileName.substr(lastSlashPosition + 1, lastDotPosition) : fileName;
	}
	return fileNameNoExt;
}

// https://stackoverflow.com/a/12468109
std::string Util::randomString(size_t length)
{
	auto randchar = []() -> char
	{
		const char charset[] =
			"0123456789"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz";
		const size_t max_index = (sizeof(charset) - 1);
		return charset[rand() % max_index];
	};
	std::string str(length, 0);
	std::generate_n(str.begin(), length, randchar);
	return str;
}

glm::mat4 Util::CalculateLightTransform(glm::mat4 lightProjectionMatrix, glm::vec3 lightDirection)
{
	return lightProjectionMatrix * glm::lookAt(-lightDirection, glm::vec3(0.0f, -lightDirection.y * 0.75f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

void Util::CheckOpenGLErrors(const std::string& label)
{
	GLenum error = glGetError();
	const GLubyte* errorString = glewGetErrorString(error);

	while (error != GL_NO_ERROR)
	{
		Log::GetLogger()->error("[{0}] OpenGL Error code: {1}, Message: '{2}'", label, error, errorString);
		error = glGetError();
	}
}

std::string Util::SpaceToUnderscore(std::string text)
{
	for (std::string::iterator it = text.begin(); it != text.end(); ++it) {
		if (*it == ' ') {
			*it = '_';
		}
	}
	return text;
}

std::wstring Util::StringToWideString(const std::string& sourceString)
{
	int len;
	int slength = (int)sourceString.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, sourceString.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, sourceString.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}

const wchar_t* Util::ConvertStdStringToWideChar(const std::string& sourceString)
{
	std::wstring destWideString = StringToWideString(sourceString);
	const wchar_t* destWideChar = destWideString.c_str();
	return destWideChar;
}
