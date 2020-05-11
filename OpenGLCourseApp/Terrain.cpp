#include "Terrain.h"
#include "VertexTBN.h"
#include "Mesh.h"


Terrain::Terrain(const char* heightMapPath, float tilingFactor, const char* colorMapPath)
{
	m_HeightMapPath = heightMapPath;
	m_TilingFactor = tilingFactor;

	m_TxHeightMap = new Texture(heightMapPath);
	m_TxHeightMap->Load();

	if (colorMapPath != nullptr)
	{
		m_TxColorMap = new Texture(colorMapPath);
		m_TxColorMap->Load();
		printf("Color map texture width=%d height=%d\n", m_TxColorMap->GetWidth(), m_TxColorMap->GetHeight());
	}

	GenerateTerrain();
}

Terrain::~Terrain()
{
	delete m_TxHeightMap;
	delete m_TxColorMap;
}

void Terrain::GenerateTerrain()
{
	int hiMapWidth = m_TxHeightMap->GetWidth();
	int hiMapHeight = m_TxHeightMap->GetHeight();
	unsigned int pixelCount = hiMapWidth * hiMapHeight;
	unsigned int vertexStride = (unsigned int)(sizeof(VertexTBN) / sizeof(float));

	vertexBufferSize = sizeof(VertexTBN) * pixelCount;
	indexCount = 6 * (hiMapWidth - 1) * (hiMapHeight - 1);

	printf("Generate terrain hiMapWidth=%d hiMapHeight=%d vertexStride=%d vertexBufferSize=%d indexCount=%d\n",
		hiMapWidth, hiMapHeight, vertexStride, vertexBufferSize, indexCount);

	vertices = new float[vertexBufferSize];
	indices = new unsigned int[indexCount];

	printf("Generate terrain vertices...\n");
	printf("Number of vertices: %d Number of indices: %d\n", pixelCount, indexCount);

	// position   tex coords   normal       tangent      bitangent
	// X  Y  Z    U  V         NX  NY  NZ   TX  TY  TZ   BX  BY  BZ
	int vertexPointer = 0;
	for (int z = -(hiMapHeight / 2); z < (hiMapHeight / 2); z++)
	{
		for (int x = -(hiMapWidth / 2); x < (hiMapWidth / 2); x++)
		{
			// vertex
			vertices[vertexPointer + 0] = (float)x;
			vertices[vertexPointer + 1] = GetHeight(x, z);
			vertices[vertexPointer + 2] = (float)z;

			// texture coords
			if (m_TxColorMap != nullptr)
			{
				// use texture coords for color map
				vertices[vertexPointer + 3] = 1.0f - GetHeight(x, z) * (1.0f / (float)m_TxColorMap->GetHeight());
				vertices[vertexPointer + 4] = 1.0f - GetHeight(x, z) * (1.0f / (float)m_TxColorMap->GetHeight());
			}
			else
			{
				// use texture coords for a regular diffuse texture
				vertices[vertexPointer + 3] = ((float)x * m_TextureTileRatio) / (float)(hiMapWidth - 1);
				vertices[vertexPointer + 4] = ((float)z * m_TextureTileRatio) / (float)(hiMapHeight - 1);
			}

			// normals
			vertices[vertexPointer + 5] = 0.0f;
			vertices[vertexPointer + 6] = 0.0f;
			vertices[vertexPointer + 7] = 0.0f;

			// tangents
			vertices[vertexPointer + 8] = 0.0f;
			vertices[vertexPointer + 9] = 0.0f;
			vertices[vertexPointer + 10] = 0.0f;

			// bitangents
			vertices[vertexPointer + 11] = 0.0f;
			vertices[vertexPointer + 12] = 0.0f;
			vertices[vertexPointer + 13] = 0.0f;

			vertexPointer += vertexStride;
		}
	}

	// Generate terrain indices
	printf("Generate terrain indices...\n");

	unsigned int indexPointer = 0;
	for (int z = 0; z < hiMapHeight - 1; z++)
	{
		for (int x = 0; x < hiMapWidth - 1; x++)
		{
			int topLeft = z * hiMapWidth + x;
			int topRight = topLeft + 1;
			int bottomLeft  = (z + 1) * hiMapWidth + x;
			int bottomRight = bottomLeft + 1;

			indices[indexPointer + 0] = topLeft;
			indices[indexPointer + 1] = bottomLeft;
			indices[indexPointer + 2] = topRight;
			indices[indexPointer + 3] = topRight;
			indices[indexPointer + 4] = bottomLeft;
			indices[indexPointer + 5] = bottomRight;

			indexPointer += 6;
		}
	}

	printf("Final value of indexPointer: %d\n", indexPointer);

	Mesh::CalcAverageNormals(vertices, vertexBufferSize, indices, indexCount);
	Mesh::CalcTangentSpace(vertices, vertexBufferSize, indices, indexCount);

	for (unsigned int i = 0; i < pixelCount; i++)
	{
		if (false && vertices[i * vertexStride + 3] > 0.9f)
		{
			printf("Index=%d Vertex X=%.2f Y=%.2f Z=%.2f TexCoords U=%.2f V=%.2f Normals NX=%.2f NY=%.2f NZ=%.2f\n", i, 
				vertices[i * vertexStride + 0], vertices[i * vertexStride + 1], vertices[i * vertexStride + 2],
				vertices[i * vertexStride + 3], vertices[i * vertexStride + 4],
				vertices[i * vertexStride + 5], vertices[i * vertexStride + 6], vertices[i * vertexStride + 7]);
		}
	}

	printf("GenerateTerrain pixelCount=%d vertexStride=%d\n", pixelCount, vertexStride);
}

float Terrain::GetHeight(int x, int z)
{
	x += (int)m_TxHeightMap->GetWidth() / 2;
	z += (int)m_TxHeightMap->GetHeight() / 2;

	if (x < 0 || x >= (int)m_TxHeightMap->GetWidth() || z < 0 || z >= (int)m_TxHeightMap->GetHeight())
	{
		return 0.0f;
	}

	float heightRatio = ((float)(m_TxHeightMap->GetWidth() + m_TxHeightMap->GetHeight()) / 2.0f) / (float)m_MaxPixelColor;
	heightRatio /= 4.0f;

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
