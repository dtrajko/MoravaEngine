#include "TextureCubeMap.h"

#include <GL/glew.h>
#include "stb_image.h"

#include <stdexcept>


TextureCubeMap::TextureCubeMap()
{
    m_ID = 0;
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
TextureCubeMap::TextureCubeMap(std::vector<std::string> faces)
    : TextureCubeMap()
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

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    printf("Texture Cube Map succesfully created! [m_ID=%i]\n", m_ID);
}

TextureCubeMap::~TextureCubeMap()
{
    glDeleteTextures(1, &m_ID);
}
