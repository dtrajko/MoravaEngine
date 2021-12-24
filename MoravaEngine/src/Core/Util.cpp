#include "H2M/Core/AssertH2M.h"

#include "Core/Log.h"
#include "Core/Util.h"
#include "Framebuffer/Attachment.h"

#include <GL/glew.h>

#include <algorithm>
#include <locale>
#include <codecvt>
#include <string>
#include <cstring>


void Util::OpenGLLogMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:
		Log::GetLogger()->error("[Util OpenGL Debug HIGH] {0}", message);
		// H2M_CORE_ASSERT(false, "GL_DEBUG_SEVERITY_HIGH");
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		Log::GetLogger()->warn("[Util OpenGL Debug MEDIUM] {0}", message);
		break;
	case GL_DEBUG_SEVERITY_LOW:
		Log::GetLogger()->info("[Util OpenGL Debug LOW] {0}", message);
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		// Log::GetLogger()->trace("[Util OpenGL Debug NOTIFICATION] {0}", message);
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

// convert from const char* to const wchar_t*
// https://www.youtube.com/watch?v=DZyzPSwe5l4
std::wstring Util::to_wstr(const char* mbstr)
{
	// not required, but nice to have
	// use underscored "en_US" instead of 
	// hyphened "en-US" for compatibility with GNU g++
#ifdef _MSC_VER
	std::setlocale(LC_ALL, "en-US"); // Microsoft Visual Studio
#else
	std::setlocale(LC_ALL, "en_US"); // GNU g++
#endif

	if (mbstr == NULL || strlen(mbstr) == 0)
	{
		std::cerr << "Invalid input parameter!" << std::endl;
		return L"";
	}

	std::mbstate_t state{}; // conversion state

	const char* p = mbstr;

	// get the number of characters
	// when successfully converted
	// mbsrtowcs - [multi byte string] to [wide char string]
	// https://en.cppreference.com/w/cpp/string/multibyte/mbsrtowcs
	size_t clen = mbsrtowcs(NULL, &p, 0 /* ignore */, &state);
		// + 1 is redundant because std::wstring manages the terminating null character

	// failed to calculate
	// the character length of the converted string 
	if (clen == 0 || clen == static_cast<size_t>(-1))
	{
		std::cerr << "Failed to compute clen!" << std::endl;
		return L""; // empty wstring
	}

	// reserve clen characters
	// wstring reserves +1 character (termination char)
	std::wstring rlt(clen, L'\0');

	size_t converted = mbsrtowcs(&rlt[0], &mbstr, rlt.size(), &state);

	// conversion failed
	if (converted == static_cast<std::size_t>(-1))
	{
		std::cerr << "The mbsrtowcs() conversion failed!" << std::endl;
		return L"";
	}
	else
	{
		return rlt;
	}
}

// convert from const wchar_t* to const char*
// https://www.youtube.com/watch?v=DZyzPSwe5l4
std::string Util::to_str(const wchar_t* wcstr)
{
	// not required, but nice to have
	// use underscored "en_US" instead of 
	// hyphened "en-US" for compatibility with GNU g++
#ifdef _MSC_VER
	std::setlocale(LC_ALL, "en-US"); // Microsoft Visual Studio
#else
	std::setlocale(LC_ALL, "en_US"); // GNU g++
#endif

	if (wcstr == NULL || wcslen(wcstr) == 0)
	{
		return ""; // empty string
	}

	std::mbstate_t state{};

	const wchar_t* p = wcstr;

	// wcsrtombs - [wide char string] to [multi byte string]
	// https://en.cppreference.com/w/cpp/string/multibyte/wcsrtombs
	size_t clen = wcsrtombs(NULL, &p, 0 /* ignore */, &state);
		// + 1 is redundant, because std::string manages the terminating null character

	// cannot determine or convert to const char*
	if (clen == 0 || clen == static_cast<std::size_t>(-1))
	{
		return ""; // empty string
	}

	std::string rlt(clen, '\0');

	size_t converted = wcsrtombs(&rlt[0], &wcstr, rlt.size(), &state);

	if (converted == static_cast<size_t>(-1))
	{
		return ""; // return empty string
	}
	else
	{
		return rlt;
	}
}

glm::vec3 Util::Lerp(const glm::vec3& start, const glm::vec3& end, float delta)
{
	glm::vec3 out;

	out.x = start.x * (1.0f - delta) + end.x * delta;
	out.y = start.y * (1.0f - delta) + end.y * delta;
	out.z = start.z * (1.0f - delta) + end.z * delta;

	return out;
}

char Util::DirectorySeparator()
{
#ifdef _WIN32
	return '\\';
#else
	return '/';
#endif
}

// from: https://stackoverflow.com/a/57595105
template <typename T, typename... Rest>
void Util::HashCombine(std::size_t& seed, const T& v, const Rest&... rest)
{
	seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	(hashCombine(seed, rest), ...);
}

const char* Util::AttachmentFormatToString(AttachmentFormat attachmentFormat)
{
	switch (attachmentFormat)
	{
	case AttachmentFormat::None:               return "None";
	case AttachmentFormat::Color:              return "Color";
	case AttachmentFormat::Depth:              return "Depth";
	case AttachmentFormat::DEPTH32F:           return "DEPTH32F";
	case AttachmentFormat::Depth_24:           return "Depth_24";
	case AttachmentFormat::DepthStencil:       return "DepthStencil";
	case AttachmentFormat::Depth_24_Stencil_8: return "Depth_24_Stencil_8";
	case AttachmentFormat::RGBA:               return "RGBA";
	case AttachmentFormat::RGBA16F:            return "RGBA16F";
	case AttachmentFormat::RGBA8:              return "RGBA8";
	case AttachmentFormat::RGB:                return "RGB";
	case AttachmentFormat::RGBA32F:            return "RGBA32F";
	case AttachmentFormat::RG32F:              return "RG32F";
	case AttachmentFormat::RED_INTEGER:        return "RED_INTEGER";
	case AttachmentFormat::SRGB:               return "SRGB";
	case AttachmentFormat::Stencil:            return "Stencil";
	default:
		Log::GetLogger()->error("AttachmentFormat '{0}' undefined", attachmentFormat);
		return "None";
	}
}

const char* Util::FormatToString(GLenum format)
{
	switch (format)
	{
		case GL_RGBA8:                    return "GL_RGBA8";
		case GL_RGBA:                     return "GL_RGBA";
		case GL_R8I:                      return "GL_R8I";
		case GL_R16I:                     return "GL_R16I";
		case GL_R32I:                     return "GL_R32I";
		case GL_RED_INTEGER:              return "GL_RED_INTEGER";
		case GL_DEPTH24_STENCIL8:         return "GL_DEPTH24_STENCIL8";
		case GL_INT:                      return "GL_INT";
		case GL_DEPTH_STENCIL_ATTACHMENT: return "GL_DEPTH_STENCIL_ATTACHMENT";
	default:
		Log::GetLogger()->error("AttachmentFormat '{0}' undefined", format);
		return "UNDEFINED";
	}

}
