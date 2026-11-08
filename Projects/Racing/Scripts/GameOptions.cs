using Morava;

namespace Racing
{
    /// <summary>
    /// The player's choices the game keeps itself (Settings): the mouse's sensitivity (VehicleCamera multiplies its own by
    /// it). The engine keeps the others and puts them in place when the game starts: the volumes (Audio.SetBusVolume),
    /// fullscreen, the resolution, VSync, the quality and the bindings. ApplyAtStart reads the sensitivity once per run, from
    /// whichever scene the run starts in (the main menu, or a level played directly in the editor).
    /// </summary>
    public static class GameOptions
    {
        public const string MouseKey = "MouseSensitivity";

        /// <summary>Times each camera's own mouse sensitivity (1: as the scene has it).</summary>
        public static float MouseSensitivity { get; private set; } = 1.0f;

        static bool applied;

        /// <summary>The saved choices into the game, once per run (later calls do nothing).</summary>
        public static void ApplyAtStart()
        {
            if (applied)
                return;
            applied = true;
            MouseSensitivity = Settings.GetFloat(MouseKey, 1.0f);
            Log.Info($"Options: music {Audio.GetBusVolume(AudioBus.Music):0.00}, effects {Audio.GetBusVolume(AudioBus.Effects):0.00}, " +
                $"mouse x{MouseSensitivity:0.00}, quality {Quality.Level}{(Quality.IsAutomatic ? " (automatic)" : "")}, " +
                $"{(Application.Fullscreen ? "fullscreen" : "a window")}, VSync {(Application.VSync ? "on" : "off")}");
        }

        public static void SetMouseSensitivity(float times)
        {
            MouseSensitivity = times;
            Settings.SetFloat(MouseKey, times);
        }
    }
}
