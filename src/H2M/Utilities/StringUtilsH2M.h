/**
 *
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include <string>
#include <vector>


namespace H2M::Utils
{

	std::string GetFilename(const std::string& filepath);
	std::string GetExtension(const std::string& filename);
	std::string RemoveExtension(const std::string& filename);
	bool StartsWith(const std::string& string, const std::string& start);
	std::vector<std::string> SplitString(const std::string& string, const std::string& delimiters);
	std::vector<std::string> SplitString(const std::string& string, const char delimiter);

}
