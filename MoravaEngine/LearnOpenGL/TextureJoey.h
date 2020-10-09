#pragma once

#include <string>


class TextureJoey
{

public:

    TextureJoey(const char* path, const std::string& directory, bool gamma);
    inline unsigned int GetID() const { return m_ID; };

private:

    unsigned int m_ID;

};
