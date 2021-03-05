using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Hazel;

namespace Example
{
    public static class Noise
    {
        /* public static float[,] GenerateNoiseMap(int mapWidth, int mapHeight, float scale)
		 {
			 float[,] noiseMap = new float[mapWidth, mapHeight];

			 if (scale <= 0)
				 scale = 0.0001f;

			 for (int y = 0; y < mapHeight; y++)
			 {
				 for (int x = 0; x < mapWidth; x++)
				 {
					 float sampleX = x / scale;
					 float sampleY = y / scale;

					 float perlinValue = Hazel.Noise.PerlinNoise(sampleX, sampleY);
					 noiseMap[x, y] = perlinValue;
				 }
			 }

			 return noiseMap;
		 }*/

        public static float InverseLerp(float min, float max, float value)
        {
            if (Math.Abs(max - min) < 0.000001f) return min;
            return (value - min) / (max - min);
        }

        public static float[,] GenerateNoiseMap(int mapWidth, int mapHeight, int seed, float scale, int octaves, float persistance, float lacunarity, Vector2 offset)
        {
            float[,] noiseMap = new float[mapWidth, mapHeight];

            System.Random prng = new System.Random(seed);
            Vector2[] octaveOffsets = new Vector2[octaves];
            for (int i = 0; i < octaves; i++)
            {
                float offsetX = prng.Next(-100000, 100000) + offset.X;
                float offsetY = prng.Next(-100000, 100000) + offset.Y;
                octaveOffsets[i] = new Vector2(offsetX, offsetY);
            }

            if (scale <= 0)
            {
                scale = 0.0001f;
            }

            float maxNoiseHeight = float.MinValue;
            float minNoiseHeight = float.MaxValue;

            float halfWidth = mapWidth / 2f;
            float halfHeight = mapHeight / 2f;


            for (int y = 0; y < mapHeight; y++)
            {
                for (int x = 0; x < mapWidth; x++)
                {

                    float amplitude = 1;
                    float frequency = 1;
                    float noiseHeight = 0;

                    for (int i = 0; i < octaves; i++)
                    {
                        float sampleX = (x - halfWidth) / scale * frequency + octaveOffsets[i].X;
                        float sampleY = (y - halfHeight) / scale * frequency + octaveOffsets[i].Y;

                        float perlinValue = Hazel.Noise.PerlinNoise(sampleX, sampleY);// * 2 - 1; // 0->1  // -1 -> 1      
                        noiseHeight += perlinValue * amplitude;

                        amplitude *= persistance;
                        frequency *= lacunarity;
                    }

                    if (noiseHeight > maxNoiseHeight)
                    {
                        maxNoiseHeight = noiseHeight;
                    }
                    else if (noiseHeight < minNoiseHeight)
                    {
                        minNoiseHeight = noiseHeight;
                    }
                    noiseMap[x, y] = noiseHeight;
                }
            }

            for (int y = 0; y < mapHeight; y++)
            {
                for (int x = 0; x < mapWidth; x++)
                {
                    noiseMap[x, y] = InverseLerp(minNoiseHeight, maxNoiseHeight, noiseMap[x, y]);
                }
            }

            return noiseMap;
        }

    }
}
