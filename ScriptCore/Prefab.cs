using System;

namespace Morava
{
    /// <summary>
    /// A prefab file (.mprefab): entities saved to be placed again, in the editor or by a script with
    /// <see cref="Scene.Instantiate(Prefab, System.Numerics.Vector3)"/>. A script's public Prefab field shows in the
    /// Inspector: drag a prefab onto it from the Content Browser.
    /// </summary>
    public sealed class Prefab : IEquatable<Prefab>
    {
        /// <summary>The prefab of a file, relative to the project ("Assets/Prefabs/Sparkle.mprefab").</summary>
        public Prefab(string path)
        {
            if (string.IsNullOrEmpty(path)) throw new ArgumentException("A prefab needs its file", nameof(path));
            Path = path;
        }

        /// <summary>Its file, relative to the project.</summary>
        public string Path { get; }

        /// <summary>The file's name without its folder and extension: "Sparkle".</summary>
        public string Name => System.IO.Path.GetFileNameWithoutExtension(Path);

        /// <summary>Two prefabs are equal when they are the same file.</summary>
        public bool Equals(Prefab? other) => other is not null && string.Equals(Path, other.Path, StringComparison.OrdinalIgnoreCase);
        /// <inheritdoc/>
        public override bool Equals(object? obj) => obj is Prefab other && Equals(other);
        /// <inheritdoc/>
        public override int GetHashCode() => StringComparer.OrdinalIgnoreCase.GetHashCode(Path);
        /// <summary>Its file.</summary>
        public override string ToString() => Path;
    }
}
