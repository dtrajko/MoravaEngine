using System;

using Hazel;

namespace Example
{
    public class Script : Entity
    {
        public float VerticalSpeed = 5.0f;
        public float Speed = 5.0f;
        public float Rotation = 0.0f;

        public Vector3 Velocity;
        public float SinkRate;

        public void OnCreate()
        {
            Console.WriteLine("Script.OnCreate");
        }

        public void OnUpdate(float ts)
        {
            Rotation += ts;

            Matrix4 transform = GetTransform();
            Vector3 translation = transform.Translation;

            float speed = Speed * ts;
            
            translation.X += Velocity.X * ts;
            translation.Y += Velocity.Y * ts;
            translation.Z += Velocity.Z * ts;
            
            translation.Y -= SinkRate * ts;

            transform.Translation = translation;
            SetTransform(transform);
        }

    }
}
