using System.Numerics;

namespace Morava
{
    /// <summary>The groups sounds are mixed in, each with its volume (the Audio panel's mixer). The engine's numbers (AudioBus).</summary>
    public enum AudioBus
    {
        /// <summary>Music: usually streamed, the same everywhere.</summary>
        Music = 0,
        /// <summary>The game's sounds: steps, hits, pickups.</summary>
        Effects = 1,
        /// <summary>Menus and buttons.</summary>
        UI = 2,
    }

    /// <summary>Sounds of the running game that belong to no entity, and its volumes.</summary>
    public static unsafe class Audio
    {
        /// <summary>
        /// Plays a sound file once, the same everywhere: clip is relative to the project ("Assets/Audio/Win.wav"). False when
        /// it can't be played (the log says why, once).
        /// </summary>
        public static bool PlayOneShot(string clip, float volume = 1.0f, AudioBus bus = AudioBus.Effects, float pitch = 1.0f)
        {
            fixed (char* chars = clip)
                return Interop.Game->Audio_PlayOneShot(chars, clip.Length, null, (int)bus, volume, pitch) != 0;
        }

        /// <summary>Plays a sound file once from a place in the world: louder near the listener, from its side.</summary>
        public static bool PlayOneShot(string clip, Vector3 position, float volume = 1.0f, AudioBus bus = AudioBus.Effects, float pitch = 1.0f)
        {
            fixed (char* chars = clip)
                return Interop.Game->Audio_PlayOneShot(chars, clip.Length, &position, (int)bus, volume, pitch) != 0;
        }

        /// <summary>The volume everything goes through, 0 to 1. For this game: the next one starts with the project's.</summary>
        public static float MasterVolume
        {
            get => GetVolumes()[0];
            set { float[] volumes = GetVolumes(); volumes[0] = value; SetVolumes(volumes); }
        }

        /// <summary>A bus's volume, 0 to 1.</summary>
        public static float GetBusVolume(AudioBus bus) => GetVolumes()[1 + (int)bus];

        /// <summary>Sets a bus's volume, 0 to 1, for this game: music softer in a menu, effects off.</summary>
        public static void SetBusVolume(AudioBus bus, float volume)
        {
            float[] volumes = GetVolumes();
            volumes[1 + (int)bus] = volume;
            SetVolumes(volumes);
        }

        // Master, then each bus's
        const int VolumeCount = 4;

        static float[] GetVolumes()
        {
            float[] volumes = new float[VolumeCount];
            fixed (float* values = volumes)
                Interop.Game->Audio_GetVolumes(values);
            return volumes;
        }

        static void SetVolumes(float[] volumes)
        {
            fixed (float* values = volumes)
                Interop.Game->Audio_SetVolumes(values);
        }
    }
}
