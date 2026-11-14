using Morava;

namespace Racing
{
    /// <summary>
    /// The player's choices the game keeps itself (Settings): the mouse's sensitivity (VehicleCamera multiplies its own by
    /// it), the gearbox (automatic, or manual: the player shifts; CarController), the engine's power (Buffed or Realistic;
    /// CarController) and the ghost car (RaceGhost, off by default). The engine keeps the others and puts them in place when the game starts: the volumes (Audio.SetBusVolume),
    /// fullscreen, the resolution, VSync, the quality and the bindings. ApplyAtStart reads the sensitivity once per run, from
    /// whichever scene the run starts in (the main menu, or a level played directly in the editor).
    /// </summary>
    public static class GameOptions
    {
        public const string MouseKey = "MouseSensitivity";
        public const string GearboxKey = "ManualGearbox";
        public const string GhostKey = "GhostCar";
        public const string PowerKey = "RealisticPower";

        /// <summary>
        /// Realistic power: the Inspector's Engine Torque (900 Nm, ~710 hp at 7000 rpm) times this, a GT40 Mk II's: 630 Nm,
        /// ~495 hp.
        /// </summary>
        public const float RealisticTorque = 0.7f;

        /// <summary>Times each camera's own mouse sensitivity (1: as the scene has it).</summary>
        public static float MouseSensitivity { get; private set; } = 1.0f;

        /// <summary>The player shifts (ShiftUp, ShiftDown); false (the default): the gearbox is automatic.</summary>
        public static bool ManualGearbox { get; private set; }

        /// <summary>The engine has a GT40 Mk II's power (RealisticTorque); false (the default): Buffed, the car's own.</summary>
        public static bool RealisticPower { get; private set; }

        /// <summary>The engine's torque times the car's (Vehicle.SetEngineTorque): 1 Buffed, RealisticTorque Realistic.</summary>
        public static float EngineTorque => RealisticPower ? RealisticTorque : 1.0f;

        /// <summary>The ghost of the best lap drives beside the car (RaceGhost); false (the default): no ghost.</summary>
        public static bool Ghost { get; private set; }

        static bool applied;

        /// <summary>The saved choices into the game, once per run (later calls do nothing).</summary>
        public static void ApplyAtStart()
        {
            if (applied)
                return;
            applied = true;
            MouseSensitivity = Settings.GetFloat(MouseKey, 1.0f);
            ManualGearbox = Settings.GetBool(GearboxKey, false);
            Ghost = Settings.GetBool(GhostKey, false);
            RealisticPower = Settings.GetBool(PowerKey, false);
            Log.Info($"Options: music {Audio.GetBusVolume(AudioBus.Music):0.00}, effects {Audio.GetBusVolume(AudioBus.Effects):0.00}, " +
                $"mouse x{MouseSensitivity:0.00}, gearbox {(ManualGearbox ? "manual" : "automatic")}, power {(RealisticPower ? "realistic" : "buffed")}, ghost {(Ghost ? "on" : "off")}, quality {Quality.Level}{(Quality.IsAutomatic ? " (automatic)" : "")}, " +
                $"{(Application.Fullscreen ? "fullscreen" : "a window")}, VSync {(Application.VSync ? "on" : "off")}, " +
                $"frame rate limit {(Application.FrameRateLimit > 0 ? Application.FrameRateLimit.ToString() : "off")}, on battery {(Application.BatteryFrameRateLimit > 0 ? Application.BatteryFrameRateLimit.ToString() : "as plugged in")}{(Application.OnBattery ? " (on battery now)" : "")}");
        }

        public static void SetMouseSensitivity(float times)
        {
            MouseSensitivity = times;
            Settings.SetFloat(MouseKey, times);
        }

        public static void SetManualGearbox(bool manual)
        {
            ManualGearbox = manual;
            Settings.SetBool(GearboxKey, manual);
        }

        public static void SetRealisticPower(bool realistic)
        {
            RealisticPower = realistic;
            Settings.SetBool(PowerKey, realistic);
        }

        public static void SetGhost(bool shown)
        {
            Ghost = shown;
            Settings.SetBool(GhostKey, shown);
        }
    }
}
