#pragma once
#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <ctime>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


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

};
