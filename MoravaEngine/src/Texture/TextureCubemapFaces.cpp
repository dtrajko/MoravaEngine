#include "Texture/TextureCubemapFaces.h"

#include <GL/glew.h>
#include "stb_image.h"

#include <stdexcept>


TextureCubemapFaces::TextureCubemapFaces()
{
    m_ID = 0;

    m_Spec.Texture_Wrap_S = GL_CLAMP_TO_EDGE;
    m_Spec.Texture_Wrap_T = GL_CLAMP_TO_EDGE;
    m_Spec.Texture_Wrap_R = GL_CLAMP_TO_EDGE;
    m_Spec.Texture_Min_Filter = GL_LINEAR;
    m_Spec.Texture_Mag_Filter = GL_LINEAR;
}

/** Loads a cubemap texture from 6 individual texture faces
    order:
    +X (right)
    -X (left)
    +Y (top)
    -Y (bottom)
    +Z (front)
    -Z (back)
*/
TextureCubemapFaces::TextureCubemapFaces(std::vector<std::string> faces)
    : TextureCubemapFaces()
{
    glGenTextures(1, &m_ID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_ID);

    int width, height, nrComponents;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            throw std::runtime_error("ERROR: Cubemap texture failed to load at path: " + faces[i]);
            stbi_image_free(data);
        }

        printf("Cubemap texture '%s' succesfully loaded.\n", faces[i].c_str());
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, m_Spec.Texture_Wrap_S);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, m_Spec.Texture_Wrap_T);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, m_Spec.Texture_Wrap_R);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, m_Spec.Texture_Min_Filter);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, m_Spec.Texture_Mag_Filter);

    printf("Texture Cube Map succesfully created! [m_ID=%i]\n", m_ID);
}

std::pair<uint32_t, uint32_t> TextureCubemapFaces::GetMipSize(uint32_t mip) const
{
    Log::GetLogger()->error("TextureCubemapFaces::GetMipSize({0}) - method not implemented!", mip);
    return std::pair<uint32_t, uint32_t>();
}

void TextureCubemapFaces::Bind(uint32_t textureSlot) const
{
    glActiveTexture(GL_TEXTURE0 + textureSlot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_ID);
}

TextureCubemapFaces::~TextureCubemapFaces()
{
    glDeleteTextures(1, &m_ID);
}
