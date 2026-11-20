using System;
using System.Collections.Generic;
using System.Numerics;
using System.Text.Json;
using Morava;

namespace Outpost
{
    /// <summary>
    /// A weapon's numbers (docs/games/Shooter_Plan.html, Phase 2.2: weapons as data, the same on every machine): read from
    /// its file in the project, Assets/Weapons/&lt;Name&gt;.weapon.json. Meters, seconds and degrees.
    /// </summary>
    public sealed class WeaponData
    {
        public string Name = "";
        public string Model = "";

        /// <summary>The muzzle in the gun model's own space (its file's units): where the muzzle ray starts.</summary>
        public Vector3 Muzzle;

        /// <summary>"Auto", "Semi": FireMode steps through them, the first is the start.</summary>
        public string[] FireModes = { "Semi" };

        public float RoundsPerMinute = 600.0f;
        public float Damage = 25.0f;
        public float HeadMultiplier = 4.0f;
        public float Range = 300.0f;
        public int Magazine = 30;
        public int Reserve = 120;
        public float ReloadSeconds = 2.2f;

        /// <summary>
        /// The shot's spread, a cone's half angle in degrees: Still standing, up to Moving more at MovingSpeed (meters per
        /// second), times Crouched crouched, Air in the air; each shot adds PerShot (up to Max), Recovery degrees a second
        /// back once RecoveryDelay seconds passed without one (so automatic fire spreads).
        /// </summary>
        public float SpreadStill = 0.25f, SpreadMoving = 1.6f, SpreadMovingSpeed = 3.0f, SpreadCrouched = 0.7f, SpreadAir = 4.0f;
        public float SpreadPerShot = 0.35f, SpreadMax = 3.5f, SpreadRecovery = 5.0f, SpreadRecoveryDelay = 0.15f;

        /// <summary>
        /// The kick each shot gives the look (degrees up, and at most this much to either side), taken back at Recovery
        /// degrees a second once RecoveryDelay seconds have passed without a shot (only what the kick added).
        /// </summary>
        public float RecoilPitch = 0.55f, RecoilYaw = 0.25f, RecoilRecovery = 6.0f, RecoilRecoveryDelay = 0.12f;

        /// <summary>The sounds: the shot ({0}: 1 to ShotVariants), the empty click, the magazine out and in, the fire mode's switch.</summary>
        public string ShotSound = "", EmptySound = "", MagazineOutSound = "", MagazineInSound = "", FireModeSound = "";
        public int ShotVariants = 1;

        /// <summary>Seconds between two shots.</summary>
        public float ShotInterval => 60.0f / MathF.Max(1.0f, RoundsPerMinute);

        static readonly Dictionary<string, WeaponData> s_Loaded = new();

        /// <summary>A weapon by its name (Assets/Weapons/&lt;name&gt;.weapon.json), read once. Null when it can't be read (the log says why).</summary>
        public static WeaponData? Load(string name)
        {
            if (s_Loaded.TryGetValue(name, out WeaponData? known))
                return known;
            string path = $"Assets/Weapons/{name}.weapon.json";
            string? text = Assets.ReadText(path);
            if (text == null)
            {
                Log.Error($"WeaponData: no {path}");
                return null;
            }
            try
            {
                using JsonDocument document = JsonDocument.Parse(text);
                WeaponData data = Parse(document.RootElement);
                s_Loaded[name] = data;
                return data;
            }
            catch (Exception e) when (e is JsonException || e is InvalidOperationException || e is KeyNotFoundException || e is FormatException)
            {
                Log.Error($"WeaponData: {path}: {e.Message}");
                return null;
            }
        }

        static WeaponData Parse(JsonElement root)
        {
            WeaponData d = new WeaponData();
            d.Name = root.GetProperty("Name").GetString() ?? "";
            d.Model = Text(root, "Model", "");
            if (root.TryGetProperty("Muzzle", out JsonElement muzzle))
                d.Muzzle = new Vector3(muzzle[0].GetSingle(), muzzle[1].GetSingle(), muzzle[2].GetSingle());
            if (root.TryGetProperty("FireModes", out JsonElement modes))
            {
                List<string> list = new();
                foreach (JsonElement mode in modes.EnumerateArray())
                    list.Add(mode.GetString() ?? "Semi");
                if (list.Count > 0)
                    d.FireModes = list.ToArray();
            }
            d.RoundsPerMinute = Number(root, "RoundsPerMinute", d.RoundsPerMinute);
            d.Damage = Number(root, "Damage", d.Damage);
            d.HeadMultiplier = Number(root, "HeadMultiplier", d.HeadMultiplier);
            d.Range = Number(root, "Range", d.Range);
            d.Magazine = (int)Number(root, "Magazine", d.Magazine);
            d.Reserve = (int)Number(root, "Reserve", d.Reserve);
            d.ReloadSeconds = Number(root, "ReloadSeconds", d.ReloadSeconds);
            if (root.TryGetProperty("Spread", out JsonElement spread))
            {
                d.SpreadStill = Number(spread, "Still", d.SpreadStill);
                d.SpreadMoving = Number(spread, "Moving", d.SpreadMoving);
                d.SpreadMovingSpeed = Number(spread, "MovingSpeed", d.SpreadMovingSpeed);
                d.SpreadCrouched = Number(spread, "Crouched", d.SpreadCrouched);
                d.SpreadAir = Number(spread, "Air", d.SpreadAir);
                d.SpreadPerShot = Number(spread, "PerShot", d.SpreadPerShot);
                d.SpreadMax = Number(spread, "Max", d.SpreadMax);
                d.SpreadRecovery = Number(spread, "Recovery", d.SpreadRecovery);
                d.SpreadRecoveryDelay = Number(spread, "RecoveryDelay", d.SpreadRecoveryDelay);
            }
            if (root.TryGetProperty("Recoil", out JsonElement recoil))
            {
                d.RecoilPitch = Number(recoil, "Pitch", d.RecoilPitch);
                d.RecoilYaw = Number(recoil, "Yaw", d.RecoilYaw);
                d.RecoilRecovery = Number(recoil, "Recovery", d.RecoilRecovery);
                d.RecoilRecoveryDelay = Number(recoil, "RecoveryDelay", d.RecoilRecoveryDelay);
            }
            if (root.TryGetProperty("Sounds", out JsonElement sounds))
            {
                d.ShotSound = Text(sounds, "Shot", "");
                d.ShotVariants = (int)Number(sounds, "ShotVariants", 1);
                d.EmptySound = Text(sounds, "Empty", "");
                d.MagazineOutSound = Text(sounds, "MagazineOut", "");
                d.MagazineInSound = Text(sounds, "MagazineIn", "");
                d.FireModeSound = Text(sounds, "FireMode", "");
            }
            return d;
        }

        static float Number(JsonElement e, string name, float fallback) =>
            e.TryGetProperty(name, out JsonElement v) ? v.GetSingle() : fallback;

        static string Text(JsonElement e, string name, string fallback) =>
            e.TryGetProperty(name, out JsonElement v) ? v.GetString() ?? fallback : fallback;
    }
}
