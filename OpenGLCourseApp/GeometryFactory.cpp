#include "GeometryFactory.h"

#include <GL/glew.h>


// set up vertex data (and buffer(s)) and configure vertex attributes
std::vector<float> GeometryFactory::cubeVerticesTexCoords =
{
	// positions           // texture Coords
   -0.5f, -0.5f, -0.5f,    0.0f, 0.0f,
	0.5f, -0.5f, -0.5f,    1.0f, 0.0f,
	0.5f,  0.5f, -0.5f,    1.0f, 1.0f,
	0.5f,  0.5f, -0.5f,    1.0f, 1.0f,
   -0.5f,  0.5f, -0.5f,    0.0f, 1.0f,
   -0.5f, -0.5f, -0.5f,    0.0f, 0.0f,

   -0.5f, -0.5f,  0.5f,    0.0f, 0.0f,
	0.5f, -0.5f,  0.5f,    1.0f, 0.0f,
	0.5f,  0.5f,  0.5f,    1.0f, 1.0f,
	0.5f,  0.5f,  0.5f,    1.0f, 1.0f,
   -0.5f,  0.5f,  0.5f,    0.0f, 1.0f,
   -0.5f, -0.5f,  0.5f,    0.0f, 0.0f,

   -0.5f,  0.5f,  0.5f,    1.0f, 0.0f,
   -0.5f,  0.5f, -0.5f,    1.0f, 1.0f,
   -0.5f, -0.5f, -0.5f,    0.0f, 1.0f,
   -0.5f, -0.5f, -0.5f,    0.0f, 1.0f,
   -0.5f, -0.5f,  0.5f,    0.0f, 0.0f,
   -0.5f,  0.5f,  0.5f,    1.0f, 0.0f,

	0.5f,  0.5f,  0.5f,    1.0f, 0.0f,
	0.5f,  0.5f, -0.5f,    1.0f, 1.0f,
	0.5f, -0.5f, -0.5f,    0.0f, 1.0f,
	0.5f, -0.5f, -0.5f,    0.0f, 1.0f,
	0.5f, -0.5f,  0.5f,    0.0f, 0.0f,
	0.5f,  0.5f,  0.5f,    1.0f, 0.0f,

   -0.5f, -0.5f, -0.5f,    0.0f, 1.0f,
	0.5f, -0.5f, -0.5f,    1.0f, 1.0f,
	0.5f, -0.5f,  0.5f,    1.0f, 0.0f,
	0.5f, -0.5f,  0.5f,    1.0f, 0.0f,
   -0.5f, -0.5f,  0.5f,    0.0f, 0.0f,
   -0.5f, -0.5f, -0.5f,    0.0f, 1.0f,

   -0.5f,  0.5f, -0.5f,    0.0f, 1.0f,
	0.5f,  0.5f, -0.5f,    1.0f, 1.0f,
	0.5f,  0.5f,  0.5f,    1.0f, 0.0f,
	0.5f,  0.5f,  0.5f,    1.0f, 0.0f,
   -0.5f,  0.5f,  0.5f,    0.0f, 0.0f,
   -0.5f,  0.5f, -0.5f,    0.0f, 1.0f
};

std::vector<float> GeometryFactory::planeVertices =
{
	// positions           // texture Coords 
	5.0f, -0.5f,  5.0f,    2.0f, 0.0f,
   -5.0f, -0.5f,  5.0f,    0.0f, 0.0f,
   -5.0f, -0.5f, -5.0f,    0.0f, 2.0f,

	5.0f, -0.5f,  5.0f,    2.0f, 0.0f,
   -5.0f, -0.5f, -5.0f,    0.0f, 2.0f,
	5.0f, -0.5f, -5.0f,    2.0f, 2.0f
};

// vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates
std::vector<float> GeometryFactory::quadVertices =
{
	// positions   // texCoords
	-1.0f,  1.0f,  0.0f, 1.0f,
	-1.0f, -1.0f,  0.0f, 0.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,

	-1.0f,  1.0f,  0.0f, 1.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,
	 1.0f,  1.0f,  1.0f, 1.0f,
};

std::vector<float> GeometryFactory::cubeVerticesNormals =
{
    // positions          // normals
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
};

std::vector<float> GeometryFactory::skyboxVertices =
{
    // positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

unsigned int GeometryFactory::s_CubeTexCoordsVAO;
unsigned int GeometryFactory::s_CubeTexCoordsVBO;
unsigned int GeometryFactory::s_PlaneVAO;
unsigned int GeometryFactory::s_PlaneVBO;
unsigned int GeometryFactory::s_QuadVAO;
unsigned int GeometryFactory::s_QuadVBO;

unsigned int GeometryFactory::s_CubeNormalsVAO;
unsigned int GeometryFactory::s_CubeNormalsVBO;
unsigned int GeometryFactory::s_SkyboxVAO;
unsigned int GeometryFactory::s_SkyboxVBO;


void GeometryFactory::CreateCubeTexCoords()
{
    // cube VAO
    glGenVertexArrays(1, &s_CubeTexCoordsVAO);
    glGenBuffers(1, &s_CubeTexCoordsVBO);
    glBindVertexArray(s_CubeTexCoordsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_CubeTexCoordsVBO);
    glBufferData(GL_ARRAY_BUFFER, GeometryFactory::cubeVerticesTexCoords.size() * sizeof(float), GeometryFactory::cubeVerticesTexCoords.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}

void GeometryFactory::CreatePlane()
{
    // plane VAO
    glGenVertexArrays(1, &s_PlaneVAO);
    glGenBuffers(1, &s_PlaneVBO);
    glBindVertexArray(s_PlaneVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_PlaneVBO);
    glBufferData(GL_ARRAY_BUFFER, GeometryFactory::planeVertices.size() * sizeof(float), GeometryFactory::planeVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}

void GeometryFactory::CreateQuad()
{
    // screen quad VAO
    glGenVertexArrays(1, &s_QuadVAO);
    glGenBuffers(1, &s_QuadVBO);
    glBindVertexArray(s_QuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_QuadVBO);
    glBufferData(GL_ARRAY_BUFFER, GeometryFactory::quadVertices.size() * sizeof(float), GeometryFactory::quadVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void GeometryFactory::DestroyCubeTexCoords()
{
    glDeleteVertexArrays(1, &s_CubeTexCoordsVAO);
    glDeleteBuffers(1, &s_CubeTexCoordsVBO);
}

void GeometryFactory::DestroyPlane()
{
    glDeleteVertexArrays(1, &s_PlaneVAO);
    glDeleteBuffers(1, &s_PlaneVBO);
}

void GeometryFactory::DestroyQuad()
{
    glDeleteVertexArrays(1, &s_QuadVAO);
    glDeleteBuffers(1, &s_QuadVBO);
}

void GeometryFactory::CreateCubeNormals()
{
    // cube VAO
    glGenVertexArrays(1, &s_CubeNormalsVAO);
    glGenBuffers(1, &s_CubeNormalsVBO);
    glBindVertexArray(s_CubeNormalsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_CubeNormalsVBO);
    glBufferData(GL_ARRAY_BUFFER, GeometryFactory::cubeVerticesNormals.size() * sizeof(float), GeometryFactory::cubeVerticesNormals.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
}

void GeometryFactory::CreateSkybox()
{
    // skybox VAO
    glGenVertexArrays(1, &s_SkyboxVAO);
    glGenBuffers(1, &s_SkyboxVBO);
    glBindVertexArray(s_SkyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_SkyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, GeometryFactory::skyboxVertices.size() * sizeof(float), GeometryFactory::skyboxVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

void GeometryFactory::DestroyCubeNormals()
{
    glDeleteVertexArrays(1, &s_CubeNormalsVAO);
    glDeleteBuffers(1, &s_CubeNormalsVBO);
}

void GeometryFactory::DestroySkybox()
{
    glDeleteVertexArrays(1, &s_SkyboxVAO);
    glDeleteBuffers(1, &s_SkyboxVBO);
}
