#pragma once

#include "Mesh/Mesh.h"
#include "Shader/MoravaShader.h"
#include "Texture/MoravaTexture.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <vector>
#include <map>


#define INVALID_MATERIAL 0xFFFFFFFF
#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a) / sizeof(a[0]))
#define ZERO_MEM(a) memset(a, 0, sizeof(a))
#define ZERO_MEM_VAR(var) memset(&var, 0, sizeof(var))
#define GLCheckError() (glGetError() == GL_NO_ERROR)
#define SAFE_DELETE(p) if (p) { delete p; p = NULL; }

#define POSITION_LOCATION    0
#define TEX_COORD_LOCATION   1
#define NORMAL_LOCATION      2
#define BONE_ID_LOCATION     3
#define BONE_WEIGHT_LOCATION 4


enum VB_TYPES {
    INDEX_BUFFER,
    POS_VB,
    NORMAL_VB,
    TEXCOORD_VB,
    BONE_VB,
    NUM_VBs
};

struct BoneInfo
{
    glm::mat4 BoneOffset;
    glm::mat4 FinalTransformation;

    BoneInfo()
    {
        BoneOffset = glm::mat4(0.0f);
        FinalTransformation = glm::mat4(0.0f);
    }
};

class SkinnedMesh : public Mesh
{
public:
	SkinnedMesh();
    SkinnedMesh(const std::string& Filename, const std::string& TexturesDir);
	bool LoadMesh(const std::string& Filename, const std::string& TexturesDir);
    virtual void Render() override;
    void BoneTransform(float TimeInSeconds, std::vector<glm::mat4>& Transforms);
    void BindTextures();
    inline void SetTimeMultiplier(float timeMultiplier) { m_TimeMultiplier = timeMultiplier; }
	~SkinnedMesh();

private:
    #define NUM_BONES_PER_VEREX 4

    struct VertexBoneData
    {
        unsigned int IDs[NUM_BONES_PER_VEREX];
        float Weights[NUM_BONES_PER_VEREX];

        VertexBoneData()
        {
            Reset();
        };

        void Reset()
        {
            ZERO_MEM(IDs);
            ZERO_MEM(Weights);
        }

        void AddBoneData(unsigned int BoneID, float Weight);
    };

    void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    unsigned int FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
    unsigned int FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
    unsigned int FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
    const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const std::string NodeName);
    void ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const glm::mat4& ParentTransform);
    bool InitFromScene(const aiScene* pScene, const std::string& Filename, const std::string& TexturesDir);
    void InitMesh(unsigned int MeshIndex, const aiMesh* paiMesh,
        std::vector<glm::vec3>& Positions, std::vector<glm::vec3>& Normals, std::vector<glm::vec2>& TexCoords,
        std::vector<VertexBoneData>& Bones, std::vector<unsigned int>& Indices);
    void LoadBones(unsigned int MeshIndex, const aiMesh* paiMesh, std::vector<VertexBoneData>& Bones);
    bool InitMaterials(const aiScene* pScene, const std::string& Filename, const std::string& TexturesDir);
    void Clear();

    unsigned int m_Buffers[NUM_VBs];

    struct MeshEntry {
        MeshEntry()
        {
            NumIndices = 0;
            BaseVertex = 0;
            BaseIndex = 0;
            MaterialIndex = INVALID_MATERIAL;
        }

        unsigned int NumIndices;
        unsigned int BaseVertex;
        unsigned int BaseIndex;
        unsigned int MaterialIndex;
    };

    std::vector<MeshEntry> m_Entries;
    std::vector<H2M::RefH2M<MoravaTexture>> m_Textures;

    std::map<std::string, unsigned int> m_BoneMapping; // maps a bone name to its index
    unsigned int m_NumBones = 0;
    std::vector<BoneInfo> m_BoneInfo;
    glm::mat4 m_GlobalInverseTransform;

    const aiScene* m_pScene;
    Assimp::Importer m_Importer;

    float m_TimeMultiplier = 1.0f;

};
