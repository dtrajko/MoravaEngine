#pragma once

#include <vector>
#include <string>


class TextureCubemapFaces
{
public:
	TextureCubemapFaces();
	TextureCubemapFaces(std::vector<std::string> faces);
	~TextureCubemapFaces();

	inline unsigned int GetID() const { return m_ID; };
	void Bind(unsigned int slot);

private:
	unsigned int m_ID;

};
