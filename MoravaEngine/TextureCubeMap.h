#pragma once

#include <vector>
#include <string>


class TextureCubeMap
{
public:
	TextureCubeMap();
	TextureCubeMap(std::vector<std::string> faces);
	~TextureCubeMap();

	inline unsigned int GetID() const { return m_ID; };
	void Bind(unsigned int slot);

private:
	unsigned int m_ID;

};
