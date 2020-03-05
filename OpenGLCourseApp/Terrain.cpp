#include "Terrain.h"
#include "Vertex.h"
#include "Mesh.h"


Terrain::Terrain(const char* heightMapPath, bool invertHeight)
{
	m_HeightMapPath = heightMapPath;
	m_InvertHeight = invertHeight;
	LoadHeightMap(m_HeightMapPath);
	GenerateTerrain();
}

Terrain::~Terrain()
{
	delete m_TxHeightMap;
}

void Terrain::LoadHeightMap(const char* heightMapPath)
{
	m_TxHeightMap = new Texture(heightMapPath);
	m_TxHeightMap->LoadTexture();
}

void Terrain::GenerateTerrain()
{
	unsigned int hiMapWidth = m_TxHeightMap->m_Width;
	unsigned int hiMapHeight = m_TxHeightMap->m_Height;
	unsigned int pixelCount = hiMapWidth * hiMapHeight;
	unsigned int vertexStride = (unsigned int)(sizeof(Vertex) / sizeof(float));

	vertexCount = sizeof(Vertex) * pixelCount;
	indexCount = 6 * (hiMapWidth - 1) * (hiMapHeight - 1);

	printf("Generate terrain vertexCount=%d indexCount=%d\n", vertexCount, indexCount);

	vertices = new float[vertexCount];
	indices = new unsigned int[indexCount];

	printf("Generate terrain vertices...\n");
	printf("Number of vertices: %d Number of indices: %d\n", pixelCount, indexCount);

	// position   tex coords   normal       tangent      bitangent 
	// X  Y  Z    U  V         NX  NY  NZ   TX  TY  TZ   BX  BY  BZ
	int vertexPointer = 0;
	for (unsigned int z = 0; z < hiMapHeight; z++)
	{
		for (unsigned int x = 0; x < hiMapWidth; x++)
		{
			// vertex
			vertices[vertexPointer + 0] = (float)x;
			vertices[vertexPointer + 1] = GetHeight(x, z);
			vertices[vertexPointer + 2] = (float)z;
			vertexPointer += vertexStride;

			// texture coords
			vertices[vertexPointer + 3] = ((float)x * m_TextureTileRatio) / (float)(hiMapWidth - 1);
			vertices[vertexPointer + 4] = ((float)z * m_TextureTileRatio) / (float)(hiMapHeight - 1);

			// normals
			vertices[vertexPointer + 5] = 0.0f;
			vertices[vertexPointer + 6] = 0.0f;
			vertices[vertexPointer + 7] = 0.0f;
		}
	}

	// Generate terrain indices
	printf("Generate terrain indices...\n");

	unsigned int indexPointer = 0;
	for (unsigned int z = 0; z < hiMapHeight - 1; z++)
	{
		for (unsigned int x = 0; x < hiMapWidth - 1; x++)
		{
			unsigned int topLeft = z * hiMapWidth + x;
			unsigned int topRight = topLeft + 1;
			unsigned int bottomLeft  = (z + 1) * hiMapWidth + x;
			unsigned int bottomRight = bottomLeft + 1;

			indices[indexPointer + 0] = topLeft;
			indices[indexPointer + 1] = bottomLeft;
			indices[indexPointer + 2] = topRight;
			indices[indexPointer + 3] = topRight;
			indices[indexPointer + 4] = bottomLeft;
			indices[indexPointer + 5] = bottomRight;

			// printf("Indices indexPointer=%d %d %d %d %d %d %d\n", indexPointer,
			// 	indices[indexPointer + 0], indices[indexPointer + 1], indices[indexPointer + 2],
			// 	indices[indexPointer + 3], indices[indexPointer + 4], indices[indexPointer + 5]);

			indexPointer += 6;
		}
	}

	printf("Final value of indexPointer: %d\n", indexPointer);

	Mesh::CalcAverageNormals(indices, indexCount, vertices, vertexCount);
	Mesh::CalcTangentSpace(indices, indexCount, vertices, vertexCount);

	for (unsigned int i = 0; i < pixelCount; i++)
	{
		/**
		printf("Index=%d Vertex X=%.2f Y=%.2f Z=%.2f TexCoords U=%.2f V=%.2f Normals NX=%.2f NY=%.2f NZ=%.2f\n", i, 
			vertices[i * vertexStride + 0], vertices[i * vertexStride + 1], vertices[i * vertexStride + 2],
			vertices[i * vertexStride + 3], vertices[i * vertexStride + 4],
			vertices[i * vertexStride + 5], vertices[i * vertexStride + 6], vertices[i * vertexStride + 7]);
		*/
	}

	printf("GenerateTerrain pixelCount=%d vertexStride=%d\n", pixelCount, vertexStride);
}

float Terrain::GetHeight(int x, int z)
{
	if (x < 0 || x >= m_TxHeightMap->m_Width || z < 0 || z >= m_TxHeightMap->m_Height)
	{
		return 0.0f;
	}

	float heightRatio = ((float)(m_TxHeightMap->m_Width + m_TxHeightMap->m_Height) / 2.0f) / (float)m_MaxPixelColor;

	int red   = m_TxHeightMap->getRed(x, z);
	int green = m_TxHeightMap->getGreen(x, z);
	int blue  = m_TxHeightMap->getBlue(x, z);
	int alpha = m_TxHeightMap->getAlpha(x, z);

	// printf("HeightMap color R=%d G=%d B=%d A=%d\n", red, green, blue, alpha);

	float height = ((float)red + (float)green + (float)blue) / 3;
	if (m_InvertHeight)
		height = m_MaxPixelColor - height;
	height *= heightRatio;
	return height;
}
