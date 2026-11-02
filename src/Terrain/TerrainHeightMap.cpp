#include "Terrain/TerrainHeightMap.h"

#include "Mesh/Mesh.h"
#include "Mesh/VertexTBN.h"
#include "Texture/TextureSampler.h"


TerrainHeightMap::TerrainHeightMap(const char* heightMapPath, float tilingFactor, const char* colorMapPath)
{
	m_VertexCount = 0;
	m_IndexCount = 0;

	m_Scale = glm::vec3(1.0f);
	m_ScalePrev = glm::vec3(0.0f);

	m_HeightMapPath = heightMapPath;
	m_TilingFactor = tilingFactor;

	m_TxHeightMap = TextureSampler::Create(heightMapPath, false, true);

	if (colorMapPath != nullptr)
	{
		m_TxColorMap = TextureSampler::Create(colorMapPath, false, true);
		printf("Color map texture width=%d height=%d\n", m_TxColorMap->GetWidth(), m_TxColorMap->GetHeight());
	}

	Generate(m_Scale);

	printf("Terrain constructor m_VertexCount = %i, m_IndexCount = %i\n", m_VertexCount, m_IndexCount);

	Create();
}

void TerrainHeightMap::Generate(glm::vec3 scale)
{
	if (m_VertexCount > 0 || m_IndexCount > 0) return;

	m_Scale = scale;
	m_ScalePrev = m_Scale;

	int hiMapWidth = m_TxHeightMap->GetWidth();
	int hiMapHeight = m_TxHeightMap->GetHeight();
	unsigned int pixelCount = hiMapWidth * hiMapHeight;
	unsigned int vertexStride = (unsigned int)(sizeof(VertexTBN) / sizeof(float));

	m_VertexCount = sizeof(VertexTBN) * pixelCount;
	m_IndexCount = 6 * (hiMapWidth - 1) * (hiMapHeight - 1);

	printf("Generate terrain hiMapWidth=%d hiMapHeight=%d vertexStride=%d m_VertexCount=%d indexCount=%d\n",
		hiMapWidth, hiMapHeight, vertexStride, m_VertexCount, m_IndexCount);

	delete[] m_Vertices;
	delete[] m_Indices;

	m_Vertices = new float[m_VertexCount];
	m_Indices = new unsigned int[m_IndexCount];

	printf("Generate terrain vertices...\n");
	printf("Number of vertices: %d Number of indices: %d\n", pixelCount, m_IndexCount);

	// position   tex coords   normal       tangent      bitangent
	// X  Y  Z    U  V         NX  NY  NZ   TX  TY  TZ   BX  BY  BZ
	int vertexPointer = 0;
	for (int z = -(hiMapHeight / 2); z < (hiMapHeight / 2); z++)
	{
		for (int x = -(hiMapWidth / 2); x < (hiMapWidth / 2); x++)
		{
			// vertex
			m_Vertices[vertexPointer + 0] = (float)x;
			m_Vertices[vertexPointer + 1] = GetMaxY(x, z);
			m_Vertices[vertexPointer + 2] = (float)z;
			// Log::GetLogger()->debug("GetMaxY({0}, {1}) = {2}", x, z, GetMaxY(x, z));

			// texture coords
			if (m_TxColorMap)
			{
				// use texture coords for color map
				m_Vertices[vertexPointer + 3] = 1.0f - GetMaxY(x, z) * (1.0f / (float)m_TxColorMap->GetHeight());
				m_Vertices[vertexPointer + 4] = 1.0f - GetMaxY(x, z) * (1.0f / (float)m_TxColorMap->GetHeight());
			}
			else
			{
				// use texture coords for a regular diffuse texture
				m_Vertices[vertexPointer + 3] = ((float)x * m_TextureTileRatio) / (float)(hiMapWidth - 1);
				m_Vertices[vertexPointer + 4] = ((float)z * m_TextureTileRatio) / (float)(hiMapHeight - 1);
			}

			// normals
			m_Vertices[vertexPointer + 5] = 0.0f;
			m_Vertices[vertexPointer + 6] = 0.0f;
			m_Vertices[vertexPointer + 7] = 0.0f;

			// tangents
			m_Vertices[vertexPointer + 8] = 0.0f;
			m_Vertices[vertexPointer + 9] = 0.0f;
			m_Vertices[vertexPointer + 10] = 0.0f;

			// bitangents
			m_Vertices[vertexPointer + 11] = 0.0f;
			m_Vertices[vertexPointer + 12] = 0.0f;
			m_Vertices[vertexPointer + 13] = 0.0f;

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

			m_Indices[indexPointer + 0] = topLeft;
			m_Indices[indexPointer + 1] = bottomLeft;
			m_Indices[indexPointer + 2] = topRight;
			m_Indices[indexPointer + 3] = topRight;
			m_Indices[indexPointer + 4] = bottomLeft;
			m_Indices[indexPointer + 5] = bottomRight;

			indexPointer += 6;
		}
	}

	printf("Final value of indexPointer: %d\n", indexPointer);

	m_VertexCount = vertexPointer;
	m_IndexCount = indexPointer;

	RecalculateNormals();
	RecalculateTangentSpace();

	for (unsigned int i = 0; i < pixelCount; i++)
	{
		if (false && m_Vertices[i * vertexStride + 3] > 0.9f)
		{
			printf("Index=%d Vertex X=%.2f Y=%.2f Z=%.2f TexCoords U=%.2f V=%.2f Normals NX=%.2f NY=%.2f NZ=%.2f\n", i, 
				m_Vertices[i * vertexStride + 0], m_Vertices[i * vertexStride + 1], m_Vertices[i * vertexStride + 2],
				m_Vertices[i * vertexStride + 3], m_Vertices[i * vertexStride + 4],
				m_Vertices[i * vertexStride + 5], m_Vertices[i * vertexStride + 6], m_Vertices[i * vertexStride + 7]);
		}
	}

	Create();

	printf("GenerateTerrain pixelCount=%d vertexStride=%d\n", pixelCount, vertexStride);
}

float TerrainHeightMap::GetMaxY(int x, int z)
{
	x += (int)m_TxHeightMap->GetWidth() / 2;
	z += (int)m_TxHeightMap->GetHeight() / 2;

	if (x < 0 || x >= (int)m_TxHeightMap->GetWidth() || z < 0 || z >= (int)m_TxHeightMap->GetHeight())
	{
		return 0.0f;
	}

	float heightRatio = ((float)(m_TxHeightMap->GetWidth() + m_TxHeightMap->GetHeight()) / 2.0f) / (float)m_MaxPixelColor;
	heightRatio /= 4.0f;

	int red   = m_TxHeightMap->GetRed(x, z);
	int green = m_TxHeightMap->GetGreen(x, z);
	int blue  = m_TxHeightMap->GetBlue(x, z);
	int alpha = m_TxHeightMap->GetAlpha(x, z);

	// printf("HeightMap color R=%d G=%d B=%d A=%d\n", red, green, blue, alpha);

	float height = ((float)red + (float)green + (float)blue) / 3;
	if (m_InvertHeight)
	{
		height = m_MaxPixelColor - height;
	}
	height *= heightRatio;

	return height;
}

TerrainHeightMap::~TerrainHeightMap()
{
	// delete m_TxHeightMap;
	// delete m_TxColorMap;
}
