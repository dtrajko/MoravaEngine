#include "H2M/Core/AssertH2M.h"

#include "Core/Log.h"
#include "Core/Util.h"
#include "Framebuffer/Attachment.h"

#include <GL/glew.h>

#include <algorithm>
#include <locale>
#include <cstdint>
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

	while (error != GL_NO_ERROR)
	{
		const char* errorString = "Unknown OpenGL Error";

		switch (error)
		{
		case GL_INVALID_ENUM:      errorString = "GL_INVALID_ENUM"; break;
		case GL_INVALID_VALUE:     errorString = "GL_INVALID_VALUE"; break;
		case GL_INVALID_OPERATION: errorString = "GL_INVALID_OPERATION"; break;
		case GL_OUT_OF_MEMORY:     errorString = "GL_OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: errorString = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
		}

		Log::GetLogger()->error(
			"[{}] OpenGL Error code: {}, Message: '{}'",
			label, error, errorString
		);

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

// Text between UTF-8 (std::string, what the engine works with) and wide characters (std::wstring: UTF-16 where wchar_t
// has 16 bits, as on Windows, UTF-32 where it has 32, as on Linux and macOS). Written out here, so the same code runs on
// every platform. A byte or unit that isn't valid text becomes U+FFFD, the replacement character.
static const uint32_t s_ReplacementCharacter = 0xFFFD;

static void AppendWide(std::wstring& out, uint32_t code)
{
	if (sizeof(wchar_t) == 2 && code > 0xFFFF)
	{
		// Above the 16-bit range: a pair of surrogates
		code -= 0x10000;
		out.push_back((wchar_t)(0xD800 + (code >> 10)));
		out.push_back((wchar_t)(0xDC00 + (code & 0x3FF)));
	}
	else
	{
		out.push_back((wchar_t)code);
	}
}

static void AppendUtf8(std::string& out, uint32_t code)
{
	if (code < 0x80)
	{
		out.push_back((char)code);
	}
	else if (code < 0x800)
	{
		out.push_back((char)(0xC0 | (code >> 6)));
		out.push_back((char)(0x80 | (code & 0x3F)));
	}
	else if (code < 0x10000)
	{
		out.push_back((char)(0xE0 | (code >> 12)));
		out.push_back((char)(0x80 | ((code >> 6) & 0x3F)));
		out.push_back((char)(0x80 | (code & 0x3F)));
	}
	else
	{
		out.push_back((char)(0xF0 | (code >> 18)));
		out.push_back((char)(0x80 | ((code >> 12) & 0x3F)));
		out.push_back((char)(0x80 | ((code >> 6) & 0x3F)));
		out.push_back((char)(0x80 | (code & 0x3F)));
	}
}

std::wstring Util::to_wstr(const std::string& str)
{
	std::wstring result;
	result.reserve(str.size());
	size_t i = 0;
	while (i < str.size())
	{
		const unsigned char lead = (unsigned char)str[i];
		// The bytes that follow the first one, and the smallest character that needs this many (a shorter one written
		// with more bytes isn't valid)
		size_t following = 0;
		uint32_t code = lead;
		uint32_t smallest = 0;
		if (lead >= 0xF0 && lead <= 0xF4)      { following = 3; code = lead & 0x07; smallest = 0x10000; }
		else if (lead >= 0xE0 && lead < 0xF0)  { following = 2; code = lead & 0x0F; smallest = 0x800; }
		else if (lead >= 0xC0 && lead < 0xE0)  { following = 1; code = lead & 0x1F; smallest = 0x80; }
		else if (lead >= 0x80)                 { code = s_ReplacementCharacter; }

		bool valid = i + following < str.size() || following == 0;
		for (size_t k = 1; valid && k <= following; k++)
		{
			const unsigned char next = (unsigned char)str[i + k];
			valid = (next & 0xC0) == 0x80;
			code = (code << 6) | (next & 0x3F);
		}
		if (!valid || (following > 0 && (code < smallest || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF))))
		{
			AppendWide(result, s_ReplacementCharacter);
			i++; // go on with the next byte
			continue;
		}
		AppendWide(result, code);
		i += 1 + following;
	}
	return result;
}

// convert from const wchar_t* to const char* (UTF-8)
std::string Util::to_str(const wchar_t* wcstr)
{
	return wcstr ? ToUtf8(std::wstring(wcstr)) : std::string();
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
		Log::GetLogger()->error("AttachmentFormat '{0}' undefined", static_cast<int>(attachmentFormat));
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

std::string Util::ToUtf8(const std::wstring& wstr)
{
	std::string result;
	result.reserve(wstr.size());
	for (size_t i = 0; i < wstr.size(); i++)
	{
		uint32_t code = (uint32_t)wstr[i];
		if (sizeof(wchar_t) == 2)
		{
			code &= 0xFFFF;
			if (code >= 0xD800 && code <= 0xDBFF && i + 1 < wstr.size() &&
				((uint32_t)wstr[i + 1] & 0xFFFF) >= 0xDC00 && ((uint32_t)wstr[i + 1] & 0xFFFF) <= 0xDFFF)
			{
				// A pair of surrogates: one character above the 16-bit range
				code = 0x10000 + ((code - 0xD800) << 10) + (((uint32_t)wstr[i + 1] & 0xFFFF) - 0xDC00);
				i++;
			}
		}
		if ((code >= 0xD800 && code <= 0xDFFF) || code > 0x10FFFF)
		{
			code = s_ReplacementCharacter; // half a pair, or beyond the last character
		}
		AppendUtf8(result, code);
	}
	return result;
}

bool Util::IsValidMinFilter(GLint filter)
{
	switch (filter)
	{
	case GL_NEAREST:
	case GL_LINEAR:
	case GL_NEAREST_MIPMAP_NEAREST:
	case GL_LINEAR_MIPMAP_NEAREST:
	case GL_NEAREST_MIPMAP_LINEAR:
	case GL_LINEAR_MIPMAP_LINEAR:
		return true;

	default:
		return false;
	}
}

bool Util::IsValidMagFilter(GLint filter)
{
	switch (filter)
	{
	case GL_NEAREST:
	case GL_LINEAR:
		return true;

	default:
		return false;
	}
}
