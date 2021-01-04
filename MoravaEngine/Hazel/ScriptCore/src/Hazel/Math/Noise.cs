using System.Runtime.CompilerServices;

namespace Hazel
{
    public static class Noise
    {
        public static float PerlinNoise(float x, float y)
        {
            return PerlinNoise_Native(x, y);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern float PerlinNoise_Native(float x, float y);

    }
}
