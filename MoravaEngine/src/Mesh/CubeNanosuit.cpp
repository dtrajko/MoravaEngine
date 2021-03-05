#include "Mesh/CubeNanosuit.h"

#include "LearnOpenGL/TextureJoey.h"
#include "Shader/Shader.h"

#include "GL/glew.h"


CubeNanosuit::CubeNanosuit()
{
}

CubeNanosuit::CubeNanosuit(std::string const& textureDirectory)
    : CubeNanosuit()
{
    m_TextureDirectory = textureDirectory;

    float sizeX = 0.5f;
    float sizeY = 0.5f;
    float sizeZ = 0.5f;

    float txCoX = 1.0f;
    float txCoY = 1.0f;
    float txCoZ = 1.0f;

    //                            X      Y      Z          U     V        NX     NY     NZ        TX     TY     TZ        BX     BY     BZ
    m_Vertices.push_back({ -sizeX,  sizeY, -sizeZ,    txCoX,  0.0f,     -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f, });
    m_Vertices.push_back({ -sizeX, -sizeY, -sizeZ,    txCoX, txCoY,     -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f, });
    m_Vertices.push_back({  sizeX, -sizeY, -sizeZ,     0.0f, txCoY,      0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f, });
    m_Vertices.push_back({  sizeX,  sizeY, -sizeZ,     0.0f,  0.0f,      0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f, });

    m_Vertices.push_back({ -sizeX,  sizeY,  sizeZ,     0.0f,  0.0f,     -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f, });
    m_Vertices.push_back({ -sizeX, -sizeY,  sizeZ,     0.0f, txCoY,     -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f, });
    m_Vertices.push_back({  sizeX, -sizeY,  sizeZ,    txCoX, txCoY,      0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f, });
    m_Vertices.push_back({  sizeX,  sizeY,  sizeZ,    txCoX,  0.0f,      0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f, });

    m_Vertices.push_back({  sizeX,  sizeY, -sizeZ,    txCoZ,  0.0f,      0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f, });
    m_Vertices.push_back({  sizeX, -sizeY, -sizeZ,    txCoZ, txCoY,      0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f, });
    m_Vertices.push_back({  sizeX, -sizeY,  sizeZ,     0.0f, txCoY,      0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f, });
    m_Vertices.push_back({  sizeX,  sizeY,  sizeZ,     0.0f,  0.0f,      0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f, });

    m_Vertices.push_back({ -sizeX,  sizeY, -sizeZ,     0.0f,  0.0f,     -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f, });
    m_Vertices.push_back({ -sizeX, -sizeY, -sizeZ,     0.0f, txCoY,     -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f, });
    m_Vertices.push_back({ -sizeX, -sizeY,  sizeZ,    txCoZ, txCoY,     -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f, });
    m_Vertices.push_back({ -sizeX,  sizeY,  sizeZ,    txCoZ,  0.0f,     -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f, });

    m_Vertices.push_back({ -sizeX,  sizeY,  sizeZ,     0.0f, txCoZ,      0.0f,  1.0f,  0.0f,     0.0f,  1.0f,  0.0f,     0.0f,  1.0f,  0.0f, });
    m_Vertices.push_back({ -sizeX,  sizeY, -sizeZ,     0.0f,  0.0f,      0.0f,  1.0f,  0.0f,     0.0f,  1.0f,  0.0f,     0.0f,  1.0f,  0.0f, });
    m_Vertices.push_back({  sizeX,  sizeY, -sizeZ,    txCoX,  0.0f,      0.0f,  1.0f,  0.0f,     0.0f,  1.0f,  0.0f,     0.0f,  1.0f,  0.0f, });
    m_Vertices.push_back({  sizeX,  sizeY,  sizeZ,    txCoX, txCoZ,      0.0f,  1.0f,  0.0f,     0.0f,  1.0f,  0.0f,     0.0f,  1.0f,  0.0f, });

    m_Vertices.push_back({ -sizeX, -sizeY,  sizeZ,     0.0f,  0.0f,      0.0f, -1.0f,  0.0f,     0.0f, -1.0f,  0.0f,     0.0f, -1.0f,  0.0f, });
    m_Vertices.push_back({ -sizeX, -sizeY, -sizeZ,     0.0f, txCoZ,      0.0f, -1.0f,  0.0f,     0.0f, -1.0f,  0.0f,     0.0f, -1.0f,  0.0f, });
    m_Vertices.push_back({  sizeX, -sizeY, -sizeZ,    txCoX, txCoZ,      0.0f, -1.0f,  0.0f,     0.0f, -1.0f,  0.0f,     0.0f, -1.0f,  0.0f, });
    m_Vertices.push_back({  sizeX, -sizeY,  sizeZ,    txCoX,  0.0f,      0.0f, -1.0f,  0.0f,     0.0f, -1.0f,  0.0f,     0.0f, -1.0f,  0.0f, });

    m_Indices =
    {
         0,  3,  1,
         3,  2,  1,
         4,  5,  7,
         7,  5,  6,
         8, 11,  9,
        11, 10,  9,
        12, 13, 15,
        15, 13, 14,
        16, 19, 17, // top
        19, 18, 17, // top
        21, 22, 23, // bottom
        20, 21, 23, // bottom
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
        vertex.Position = vector;

        // tex coords
        //     a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
        //     use models where a vertex can have multiple texture coordinates so we always take the first set (0).
        vector.x = m_Vertices[i][3];
        vector.y = m_Vertices[i][4];
        vertex.TexCoord = vector;

        // normals
        vector.x = m_Vertices[i][5];
        vector.y = m_Vertices[i][6];
        vector.z = m_Vertices[i][7];
        vertex.Normal = vector;

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
    // std::vector<TextureData> diffuseMaps = loadMaterialTextures("../texture_checker.png", "texture_diffuse");
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
    textureData.id = texture.GetID();
    textureData.type = typeName;
    textureData.path = m_TextureDirectory + "/" + fileName;
    textures.push_back(textureData);

    return textures;
}
