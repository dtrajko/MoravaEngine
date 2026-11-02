#include "Texture/TextureLoader.h"

#include "Core/Log.h"


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

H2M::RefH2M<MoravaTexture> TextureLoader::GetTexture(const char* fileLoc, bool flipVert, bool force)
{
	return GetTexture(fileLoc, flipVert, GL_LINEAR, force);
}

H2M::RefH2M<MoravaTexture> TextureLoader::GetTexture(const char* fileLoc, bool flipVert, GLenum filter, bool force)
{
	if (!force) {
		std::map<std::string, H2M::RefH2M<MoravaTexture>>::iterator it;
		it = m_Textures.find(fileLoc);
	
		if (it != m_Textures.end())
		{
			Log::GetLogger()->info("TextureLoader HIT - Texture '{0}' already loaded [m_Textures.size={1}]", fileLoc, m_Textures.size());
			return m_Textures[fileLoc];
		}
	}

	H2M::RefH2M<MoravaTexture> texture = MoravaTexture::Create(fileLoc, false, true, filter);

	m_Textures.insert(std::make_pair(fileLoc, texture));
	Log::GetLogger()->info("TextureLoader MISS - New texture '{0}' loaded [m_Textures.size={1}]", fileLoc, m_Textures.size());
	return texture;
}

void TextureLoader::Print()
{
	std::map<std::string, H2M::RefH2M<MoravaTexture>>::reverse_iterator rit;

	size_t id = 0;
	for (rit = m_Textures.rbegin(); rit != m_Textures.rend(); ++rit)
	{
		Log::GetLogger()->info("TextureLoader ID {0}\t[TextureID: {1} Path: '{2}']", id, (int)rit->second->GetID(), rit->first.c_str());
		id++;
	}
}

void TextureLoader::Clean()
{
	for (auto& texture : m_Textures)
	{
		// delete texture.second;
	}
}

TextureLoader::~TextureLoader()
{
}
