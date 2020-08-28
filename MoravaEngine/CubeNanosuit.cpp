#include "CubeNanosuit.h"

#include "GL/glew.h"

#include "Shader.h"
#include "LearnOpenGL/TextureJoey.h"


CubeNanosuit::CubeNanosuit()
{
}

CubeNanosuit::CubeNanosuit(std::string const& textureDirectory)
    : CubeNanosuit()
{
    m_TextureDirectory = textureDirectory;

    //                       X      Y      Z          NX     NY     NZ        U     V        TX     TY     TZ        BX     BY     BZ
    m_Vertices.push_back({ -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    1.0f, 1.0f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f });
    m_Vertices.push_back({ -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    1.0f, 0.0f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f });
    m_Vertices.push_back({  0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,    0.0f, 0.0f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f });
    m_Vertices.push_back({  0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,    0.0f, 1.0f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f });

    m_Vertices.push_back({ -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    0.0f, 1.0f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f });
    m_Vertices.push_back({ -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    0.0f, 0.0f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f });
    m_Vertices.push_back({  0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,    1.0f, 0.0f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f });
    m_Vertices.push_back({  0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,    1.0f, 1.0f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f });

    m_Vertices.push_back({  0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,    1.0f, 1.0f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f });
    m_Vertices.push_back({  0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,    1.0f, 0.0f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f });
    m_Vertices.push_back({  0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,    0.0f, 0.0f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f });
    m_Vertices.push_back({  0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,    0.0f, 1.0f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f });

    m_Vertices.push_back({ -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    0.0f, 1.0f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f });
    m_Vertices.push_back({ -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    0.0f, 0.0f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f });
    m_Vertices.push_back({ -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    1.0f, 0.0f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f });
    m_Vertices.push_back({ -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    1.0f, 1.0f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f });

    m_Vertices.push_back({ -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    1.0f, 1.0f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f });
    m_Vertices.push_back({ -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    1.0f, 0.0f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f });
    m_Vertices.push_back({  0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,    0.0f, 0.0f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f });
    m_Vertices.push_back({  0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,    0.0f, 1.0f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f });

    m_Vertices.push_back({ -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    0.0f, 1.0f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f });
    m_Vertices.push_back({ -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    0.0f, 0.0f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f });
    m_Vertices.push_back({  0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,    1.0f, 0.0f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f });
    m_Vertices.push_back({  0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,    1.0f, 1.0f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f });

    m_Indices =
    {
        0, 3, 1,
        3, 2, 1,
        4, 5, 7,
        7, 5, 6,
        8, 11, 9,
        11, 10, 9,
        12, 13, 15,
        15, 13, 14,
        16, 19, 17,
        19, 18, 17,
        20, 21, 23,
        23, 21, 22,
    };

    m_Mesh = processMesh();
}

CubeNanosuit::~CubeNanosuit()
{
}

// draws the model, and thus all its meshes
void CubeNanosuit::Draw(Shader* shader)
{
    m_Mesh->Draw(shader);
}

MeshJoey* CubeNanosuit::processMesh()
{
    // data to fill
    std::vector<VertexTangents> vertices;
    std::vector<TextureData> textures;

    // Walk through each of the mesh's vertices
    for (unsigned int i = 0; i < (unsigned int)m_Vertices.size(); i++)
    {
        VertexTangents vertex;
        glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class 
        // that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.

        // positions
        vector.x = m_Vertices[i][0];
        vector.y = m_Vertices[i][1];
        vector.z = m_Vertices[i][2];
        vertex.base.Position = vector;

        // normals
        vector.x = m_Vertices[i][3];
        vector.y = m_Vertices[i][4];
        vector.z = m_Vertices[i][5];
        vertex.base.Normal = vector;

        // texture coordinates
        if (m_Vertices[i][3]) // does the mesh contain texture coordinates?
        {
            glm::vec2 vec;
            // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
            // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
            vec.x = m_Vertices[i][6];
            vec.y = m_Vertices[i][7];
            vertex.base.TexCoords = vec;
        }
        else
            vertex.base.TexCoords = glm::vec2(0.0f, 0.0f);

        // tangent
        vector.x = m_Vertices[i][8];
        vector.y = m_Vertices[i][9];
        vector.z = m_Vertices[i][10];
        vertex.Tangent = vector;

        // bitangent
        vector.x = m_Vertices[i][11];
        vector.y = m_Vertices[i][12];
        vector.z = m_Vertices[i][13];
        vertex.Bitangent = vector;

        vertices.push_back(vertex);
    }

    // 1. diffuse maps
    std::vector<TextureData> diffuseMaps = loadMaterialTextures("container2.png", "texture_diffuse");
    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

    // 2. specular maps
    std::vector<TextureData> specularMaps = loadMaterialTextures("container2_specular.png", "texture_specular");
    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

    // 3. normal maps
    std::vector<TextureData> normalMaps = loadMaterialTextures("container2_normal.png", "texture_normal");
    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

    // return a mesh object created from the extracted mesh data
    return new MeshJoey(vertices, m_Indices, textures);
}

std::vector<TextureData> CubeNanosuit::loadMaterialTextures(std::string fileName, std::string typeName)
{
    std::vector<TextureData> textures;

    TextureJoey texture(fileName.c_str(), m_TextureDirectory, false);
    TextureData textureData;
    textureData.id = texture.GetTextureID();
    textureData.type = typeName;
    textureData.path = m_TextureDirectory + "/" + fileName;
    textures.push_back(textureData);

    return textures;
}
