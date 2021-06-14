#pragma once
#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS

#include <GL/glew.h>

#include <iostream>
#include <ctime>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>


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

	static std::wstring StringToWideString(const std::string& sourceString);
	static const wchar_t* ConvertStdStringToWideChar(const std::string& sourceString);

};
