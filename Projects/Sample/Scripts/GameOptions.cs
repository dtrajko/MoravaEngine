using Morava;

namespace Sample
{
    /// <summary>
    /// The player's choices in the Settings menu (SettingsMenu), kept between runs with Settings: the music's and the
    /// effects' volumes, the mouse's sensitivity (CameraFollow multiplies its own by it) and fullscreen. ApplyAtStart puts
    /// them in place once per run, from whichever scene the run starts in (the main menu, or a level played directly in the
    /// editor); a value never chosen stays as the project has it.
    /// </summary>
    public static class GameOptions
    {
        public const string MusicKey = "MusicVolume";
        public const string EffectsKey = "EffectsVolume";
        public const string MouseKey = "MouseSensitivity";
        public const string FullscreenKey = "Fullscreen";

        /// <summary>Times each camera's own mouse sensitivity (1: as the scene has it).</summary>
        public static float MouseSensitivity { get; private set; } = 1.0f;

        static bool applied;

        /// <summary>The saved choices into the game, once per run (later calls do nothing).</summary>
        public static void ApplyAtStart()
        {
            if (applied)
                return;
            applied = true;
            if (Settings.Has(MusicKey))
                Audio.SetBusVolume(AudioBus.Music, Settings.GetFloat(MusicKey, 1.0f));
            if (Settings.Has(EffectsKey))
                Audio.SetBusVolume(AudioBus.Effects, Settings.GetFloat(EffectsKey, 1.0f));
            MouseSensitivity = Settings.GetFloat(MouseKey, 1.0f);
            if (Settings.Has(FullscreenKey) && Settings.GetBool(FullscreenKey) != Application.Fullscreen)
                Application.Fullscreen = Settings.GetBool(FullscreenKey);
            Log.Info($"Options: music {Audio.GetBusVolume(AudioBus.Music):0.00}, effects {Audio.GetBusVolume(AudioBus.Effects):0.00}, " +
                $"mouse x{MouseSensitivity:0.00}" + (Settings.Has(FullscreenKey) ? $", fullscreen {Settings.GetBool(FullscreenKey)}" : ""));
        }

        public static void SetMusic(float volume)
        {
            Audio.SetBusVolume(AudioBus.Music, volume);
            Settings.SetFloat(MusicKey, volume);
        }

        public static void SetEffects(float volume)
        {
            Audio.SetBusVolume(AudioBus.Effects, volume);
            Settings.SetFloat(EffectsKey, volume);
        }

        public static void SetMouseSensitivity(float times)
        {
            MouseSensitivity = times;
            Settings.SetFloat(MouseKey, times);
        }

        public static void SetFullscreen(bool fullscreen)
        {
            Application.Fullscreen = fullscreen;
            Settings.SetBool(FullscreenKey, fullscreen);
        }
    }
}
