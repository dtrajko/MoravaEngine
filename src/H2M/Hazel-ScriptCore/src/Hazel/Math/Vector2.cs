using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace Hazel
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector2
    {
        public float X;
        public float Y;

        public Vector2(float scalar)
        {
            X = Y = scalar;
        }

        public Vector2(float x, float y)
        {
            X = x;
            Y = y;
        }

        public Vector2(Vector3 vector)
        {
            X = vector.X;
            Y = vector.Y;
        }

        public void Clamp(Vector2 min, Vector2 max)
        {
            if (X < min.X)
                X = min.X;
            if (X > max.X)
                X = max.X;

            if (Y < min.Y)
                Y = min.Y;
            if (Y > max.Y)
                Y = max.Y;
        }

        public static Vector2 operator -(Vector2 vector)
        {
            return new Vector2(-vector.X, -vector.Y);
        }

    }
}