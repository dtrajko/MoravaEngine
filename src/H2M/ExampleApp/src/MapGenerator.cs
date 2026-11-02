using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Hazel;

namespace Example
{
    public class MapGenerator : Entity
    {
        // [EditorSlider("MapWidth Custom Name", 2, 0, 1024)]
        public int MapWidth = 128;
        public int MapHeight = 128;
        public int Octaves = 4;
        public float Persistance = 0.74f;
        public int Seed = 21;
        public float Lacunarity = 3.0f;
        public Vector2 Offset = new Vector2(13.4f, 6.26f);
        public float NoiseScale = 0.5f;

        public float Speed = 0.0f;

        public void GenerateMap()
        {
            //float[,] noiseMap = Noise.GenerateNoiseMap(mapWidth, mapHeight, noiseScale);
            float[,] noiseMap = Noise.GenerateNoiseMap(MapWidth, MapHeight, Seed, NoiseScale, Octaves, Persistance, Lacunarity, Offset);

            uint width = (uint)noiseMap.GetLength(0);
            uint height = (uint)noiseMap.GetLength(1);

            Texture2D texture = new Texture2D(width, height);
            Vector4[] colorMap = new Vector4[width * height];
            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    colorMap[x + y * width] = Vector4.Lerp(Color.Black, Color.White, noiseMap[x, y]);
                }
            }

            texture.SetData(colorMap);

            Console.WriteLine("HasComponent - TransformComponent = {0}", HasComponent<TransformComponent>());
            Console.WriteLine("HasComponent - ScriptComponent = {0}", HasComponent<ScriptComponent>());
            Console.WriteLine("HasComponent - MeshComponent = {0}", HasComponent<MeshComponent>());

            MeshComponent meshComponent = GetComponent<MeshComponent>();
            if (meshComponent == null)
            {
                Console.WriteLine("MeshComponent is null!");
                meshComponent = CreateComponent<MeshComponent>();
            }
            meshComponent.Mesh = MeshFactory.CreatePlane(1.0f, 1.0f);

            Console.WriteLine("Mesh has {0} materials!", meshComponent.Mesh.GetMaterialCount());

            MaterialInstance material = meshComponent.Mesh.GetMaterial(1);
            material.Set("u_MaterialUniforms.AlbedoTexToggle", 1.0f);
            material.Set("u_AlbedoTexture", texture);
        }

        void OnCreate()
        {
            GenerateMap();
        }

        void OnUpdate(float ts)
        {
            Matrix4 transform = GetTransform();
            Vector3 translation = transform.Translation;
            translation.Y += ts * Speed;
            if (Input.IsKeyPressed(KeyCode.Space))
            {
                translation.Y -= 10.0f;
            }
            transform.Translation = translation;
            SetTransform(transform);
        }


    }
}
