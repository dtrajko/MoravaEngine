#pragma once

#include <vector>


/****
 * A common class for building geometry shapes for LearnOpenGL examples
 */
class GeometryFactory
{
public:
    // SceneFramebuffers Methods
    static void CreateCubeTexCoords();
    static void CreatePlane();
    static void CreateQuad();

    static void DestroyCubeTexCoords();
    static void DestroyPlane();
    static void DestroyQuad();

    static inline unsigned int GetCubeTexCoordsVAO() { return s_CubeTexCoordsVAO; };
    static inline unsigned int GetPlaneVAO() { return s_PlaneVAO; };
    static inline unsigned int GetQuadVAO() { return s_QuadVAO; };

    // SceneFramebuffers Data
    static std::vector<float> cubeVerticesTexCoords;
    static std::vector<float> planeVertices;
    static std::vector<float> quadVertices;

    static unsigned int s_CubeTexCoordsVAO;
    static unsigned int s_CubeTexCoordsVBO;
    static unsigned int s_PlaneVAO;
    static unsigned int s_PlaneVBO;
    static unsigned int s_QuadVAO;
    static unsigned int s_QuadVBO;


    // SceneCubemaps Methods
    static void CreateCubeNormals();
    static void CreateSkybox();

    static void DestroyCubeNormals();
    static void DestroySkybox();

    static inline unsigned int GetCubeNormalsVAO() { return s_CubeNormalsVAO; };
    static inline unsigned int GetSkyboxVAO() { return s_SkyboxVAO; };

    // SceneCubemaps Data
    static std::vector<float> cubeVerticesNormals;
    static std::vector<float> skyboxVertices;

    static unsigned int s_CubeNormalsVAO;
    static unsigned int s_CubeNormalsVBO;
    static unsigned int s_SkyboxVAO;
    static unsigned int s_SkyboxVBO;

};
