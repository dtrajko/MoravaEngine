#pragma once

#include <vector>
#include <string>


class TextureCubeMap
{
public:
	TextureCubeMap();
	TextureCubeMap(std::vector<std::string> faces);
	inline unsigned int GetID() const { return m_ID; };
	~TextureCubeMap();

private:
	unsigned int m_ID;
};
