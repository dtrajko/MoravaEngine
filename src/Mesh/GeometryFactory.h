#pragma once

#include <vector>


/****
 * A common class for building geometry shapes for LearnOpenGL examples
 */
namespace GeometryFactory
{
    class Cube
    {
    public:
        static void Create();
        static void Destroy();
        static inline unsigned int GetVAO() { return s_VAO; };

    private:
        static std::vector<float> s_Vertices;
        static unsigned int s_VAO;
        static unsigned int s_VBO;
    };

    class CubeTexCoords
    {
    public:
        static void Create();
        static void Destroy();
        static inline unsigned int GetVAO() { return s_VAO; };

    private:
        static std::vector<float> s_Vertices;
        static unsigned int s_VAO;
        static unsigned int s_VBO;
    };

    class Plane
    {
    public:
        static void Create();
        static void Destroy();
        static inline unsigned int GetVAO() { return s_VAO; };

    private:
        static std::vector<float> s_Vertices;
        static unsigned int s_VAO;
        static unsigned int s_VBO;
    };

    class Quad
    {
    public:
        static void Create();
        static void Destroy();
        static inline unsigned int GetVAO() { return s_VAO; };

    private:
        static std::vector<float> s_Vertices;
        static unsigned int s_VAO;
        static unsigned int s_VBO;
    };

    class CubeNormals
    {
    public:
        static void Create();
        static void Destroy();
        static inline unsigned int GetVAO() { return s_VAO; };

    private:
        static std::vector<float> s_Vertices;
        static unsigned int s_VAO;
        static unsigned int s_VBO;
    };

    class Skybox
    {
    public:
        static void Create();
        static void Destroy();
        static inline unsigned int GetVAO() { return s_VAO; };

    private:
        static std::vector<float> s_Vertices;
        static unsigned int s_VAO;
        static unsigned int s_VBO;
    };

}
