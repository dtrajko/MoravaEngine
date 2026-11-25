using System;
using Morava;

namespace Outpost
{
    /// <summary>
    /// The player's choices the game keeps itself (Settings, Phase 3.3): the mouse's sensitivity (times the input map's
    /// Look and the Player's own Sensitivity), the scope's (times the slower turn a zoom gives) and the field of view (the
    /// view's height in degrees; the sights and the scope's zoom narrow it from there). The engine keeps the others and puts
    /// them in place when the game starts: the volumes, fullscreen, the resolution, VSync, the quality, the frame rate
    /// limits and the bindings. ApplyAtStart reads them once per run, from whichever scene the run starts in (the main menu,
    /// or the level played directly in the editor). The Player reads them each frame, so a change in the pause menu's
    /// Settings counts at once.
    /// </summary>
    public static class GameOptions
    {
        public const string MouseKey = "MouseSensitivity";
        public const string ScopeKey = "ScopeSensitivity";
        public const string FieldOfViewKey = "FieldOfView";

        public const float MinSensitivity = 0.2f, MaxSensitivity = 3.0f;
        public const float MinScopeSensitivity = 0.25f, MaxScopeSensitivity = 2.0f;
        public const float MinFieldOfView = 55.0f, MaxFieldOfView = 90.0f, DefaultFieldOfView = 70.0f;

        /// <summary>Times the Look action (1: as the input map has it).</summary>
        public static float MouseSensitivity { get; private set; } = 1.0f;

        /// <summary>Scoped, times the turn slowed in proportion to the zoom (1: exactly in proportion).</summary>
        public static float ScopeSensitivity { get; private set; } = 1.0f;

        /// <summary>Degrees from the view's bottom edge to its top edge, not aiming (70: about 103 across at 16:9).</summary>
        public static float FieldOfView { get; private set; } = DefaultFieldOfView;

        static bool applied;

        /// <summary>The saved choices into the game, once per run (later calls do nothing).</summary>
        public static void ApplyAtStart()
        {
            if (applied)
                return;
            applied = true;
            MouseSensitivity = Math.Clamp(Settings.GetFloat(MouseKey, 1.0f), MinSensitivity, MaxSensitivity);
            ScopeSensitivity = Math.Clamp(Settings.GetFloat(ScopeKey, 1.0f), MinScopeSensitivity, MaxScopeSensitivity);
            FieldOfView = Math.Clamp(Settings.GetFloat(FieldOfViewKey, DefaultFieldOfView), MinFieldOfView, MaxFieldOfView);
            Log.Info($"Options: music {Audio.GetBusVolume(AudioBus.Music):0.00}, effects {Audio.GetBusVolume(AudioBus.Effects):0.00}, " +
                $"mouse x{MouseSensitivity:0.00}, scope x{ScopeSensitivity:0.00}, field of view {FieldOfView:0} degrees, " +
                $"quality {Quality.Level}{(Quality.IsAutomatic ? " (automatic)" : "")}, " +
                $"{(Application.Fullscreen ? "fullscreen" : "a window")}, VSync {(Application.VSync ? "on" : "off")}, " +
                $"frame rate limit {(Application.FrameRateLimit > 0 ? Application.FrameRateLimit.ToString() : "off")}, " +
                $"on battery {(Application.BatteryFrameRateLimit > 0 ? Application.BatteryFrameRateLimit.ToString() : "as plugged in")}");
        }

        public static void SetMouseSensitivity(float times)
        {
            MouseSensitivity = Math.Clamp(times, MinSensitivity, MaxSensitivity);
            Settings.SetFloat(MouseKey, MouseSensitivity);
        }

        public static void SetScopeSensitivity(float times)
        {
            ScopeSensitivity = Math.Clamp(times, MinScopeSensitivity, MaxScopeSensitivity);
            Settings.SetFloat(ScopeKey, ScopeSensitivity);
        }

        public static void SetFieldOfView(float degrees)
        {
            FieldOfView = Math.Clamp(degrees, MinFieldOfView, MaxFieldOfView);
            Settings.SetFloat(FieldOfViewKey, FieldOfView);
        }
    }
}
