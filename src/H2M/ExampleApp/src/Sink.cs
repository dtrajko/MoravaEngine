using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Hazel;

namespace Example
{
    class Sink : Entity
    {

        public float SinkSpeed;

        void OnCreate()
        {

        }

        void OnUpdate(float ts)
        {
            Matrix4 transform = GetTransform();
            Vector3 translation = transform.Translation;

            translation.Y -= SinkSpeed * ts;

            transform.Translation = translation;
            SetTransform(transform);
        }

    }
}
