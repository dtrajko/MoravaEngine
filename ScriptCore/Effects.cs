using System;
using System.Numerics;

namespace Morava
{
    /// <summary>
    /// Short lights a running game adds without entities (Shooter E8): a muzzle flash, an explosion. The scene's own lights
    /// are components (<see cref="PointLight"/>, <see cref="SpotLight"/>).
    /// </summary>
    public static unsafe class Lights
    {
        /// <summary>
        /// A point light at position that fades out over seconds (its intensity falls with the square of the time left), without
        /// shadows. Up to 8 at once (a new one takes the place of the one nearest its end); they take the point lights' places
        /// the scene leaves (16 in all).
        /// </summary>
        public static void Flash(Vector3 position, Vector3 color, float intensity, float range, float seconds)
        {
            Interop.Game->Lights_Flash(&position, &color, intensity, range, seconds);
        }
    }

    /// <summary>
    /// Decals a running game spawns without entities (Shooter E6): bullet holes, scorch marks. Kept in a pool of 512: past
    /// that the oldest fade out. The scene's placed ones are components (<see cref="Decal"/>).
    /// </summary>
    public static unsafe class Decals
    {
        /// <summary>
        /// A decal on the surface at position whose normal (out of the surface, length 1) is given: texture (a project path;
        /// empty: a soft round spot) projected along -normal onto what is within depth / 2 of the point, size meters across,
        /// turned by angle degrees about the normal. color: times the texture's, w how much it covers. lifetime: seconds it
        /// stays, fading at its end (0: until the pool needs its place).
        /// </summary>
        public static void Spawn(string texture, Vector3 position, Vector3 normal, float size, float depth, float angle, Vector4 color,
            float lifetime = 0.0f)
        {
            // The box: its up the normal, its x and z across the surface, turned by the angle
            Vector3 up = normal.LengthSquared() > 1e-8f ? Vector3.Normalize(normal) : Vector3.UnitY;
            Vector3 helper = MathF.Abs(up.Y) < 0.99f ? Vector3.UnitY : Vector3.UnitX;
            Vector3 x = Vector3.Normalize(Vector3.Cross(helper, up));
            Vector3 z = Vector3.Cross(x, up);
            float a = angle * MathF.PI / 180.0f;
            (x, z) = (x * MathF.Cos(a) + z * MathF.Sin(a), z * MathF.Cos(a) - x * MathF.Sin(a));
            Spawn(texture, new Matrix4x4(
                x.X * size, x.Y * size, x.Z * size, 0.0f,
                up.X * depth, up.Y * depth, up.Z * depth, 0.0f,
                z.X * size, z.Y * size, z.Z * size, 0.0f,
                position.X, position.Y, position.Z, 1.0f), color, lifetime);
        }

        /// <summary>A decal whose box is the transform (a unit cube from -0.5 to 0.5 placed by it; projected along its -Y).</summary>
        public static void Spawn(string texture, Matrix4x4 transform, Vector4 color, float lifetime = 0.0f)
        {
            texture ??= "";
            fixed (char* chars = texture)
            {
                Interop.Game->Decals_Spawn(chars, texture.Length, &transform, &color, lifetime);
            }
        }

        /// <summary>The spawned decals go (a restart).</summary>
        public static void Clear() => Interop.Game->Decals_Clear();
    }
}
