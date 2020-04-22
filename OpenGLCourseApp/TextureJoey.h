#pragma once

#include <string>


class TextureJoey
{

public:

    TextureJoey(const char* path, const std::string& directory, bool gamma);
    inline unsigned int GetTextureID() const { return textureID; };

private:

    unsigned int textureID;


};
