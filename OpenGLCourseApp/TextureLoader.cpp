#include "TextureLoader.h"



TextureLoader* TextureLoader::s_Instance = nullptr;

TextureLoader* TextureLoader::Get()
{
	if (s_Instance == nullptr)
	{
		s_Instance = new TextureLoader();
	}

	return s_Instance;
}

TextureLoader::TextureLoader()
{
}

Texture* TextureLoader::GetTexture(const char* fileLoc, bool flipVert)
{
	return GetTexture(fileLoc, flipVert, GL_LINEAR);
}

Texture* TextureLoader::GetTexture(const char* fileLoc, bool flipVert, GLenum filter)
{
	std::map<std::string, Texture*>::iterator it;
	it = m_Textures.find(fileLoc);

	if (it != m_Textures.end())
	{
		printf("TextureLoader HIT - Texture '%s' already loaded [ m_Textures.size = %zu ].\n", fileLoc, m_Textures.size());
		return m_Textures[fileLoc];
	}

	Texture* texture = new Texture(fileLoc);
	m_Textures.insert(std::make_pair(fileLoc, texture));
	printf("TextureLoader MISS - New texture '%s' loaded [ m_Textures.size = %zu ].\n", fileLoc, m_Textures.size());
	return texture;
}

void TextureLoader::Print()
{
	std::map<std::string, Texture*>::reverse_iterator rit;

	size_t id = 0;
	for (rit = m_Textures.rbegin(); rit != m_Textures.rend(); ++rit)
	{
		printf("TextureLoader ID %zu\t[ TextureID: %i Path: '%s' ]\n", id, (int)rit->second->GetID(), rit->first.c_str());
		id++;

	}
}

TextureLoader::~TextureLoader()
{
	for (auto& texture : m_Textures)
	{
		delete texture.second;
	}
}
