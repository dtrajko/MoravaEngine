#pragma once
#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS

#include <GL/glew.h>

#include <iostream>
#include <ctime>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <string>
#include <clocale>


enum class AttachmentFormat;

class Util
{
public:
	static char* printTime();
	static void printVec3(glm::vec3 vec);
	static void printVec4(glm::vec4 vec);
	static void printMatrix(glm::mat4 mat);

	// filesystem utility methods
	static std::string GetFileNameFromFullPath(std::string fullPath);
	static std::string StripExtensionFromFileName(std::string fileName);
	static std::string randomString(size_t length);

	static glm::mat4 CalculateLightTransform(glm::mat4 lightProjectionMatrix, glm::vec3 lightDirection);

	static void CheckOpenGLErrors(const std::string& label = "Undefined");
	static void OpenGLLogMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

	static std::string SpaceToUnderscore(std::string text);

	// convert from const char* to const wchar_t*
	static std::wstring to_wstr(const char* mbstr);
	// convert from const wchar_t* to const char*
	static std::string to_str(const wchar_t* wcstr);

	static glm::vec3 Lerp(const glm::vec3& start, const glm::vec3& end, float delta);

	static char DirectorySeparator();

	template <typename T, typename... Rest>
	static void HashCombine(std::size_t& seed, const T& v, const Rest&... rest);

	static const char* AttachmentFormatToString(AttachmentFormat attachmentFormat);
	static const char* FormatToString(GLenum format);

};
