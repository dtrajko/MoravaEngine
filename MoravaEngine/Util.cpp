#include "Util.h"

#include <algorithm>


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
