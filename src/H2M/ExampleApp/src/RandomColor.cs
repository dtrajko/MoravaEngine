using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Hazel;

namespace Example
{
    class RandomColor : Entity
    {
        void OnCreate()
        {
            Random random = new Random();

            MeshComponent meshComponent = GetComponent<MeshComponent>();
            MaterialInstance material = meshComponent.Mesh.GetMaterial(0);
            float r = (float)random.NextDouble();
            float g = (float)random.NextDouble();
            float b = (float)random.NextDouble();
            material.Set("u_MaterialUniforms.AlbedoColor", new Vector3(r, g, b));
        }

    }
}
