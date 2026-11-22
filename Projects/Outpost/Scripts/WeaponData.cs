using System;
using System.Collections.Generic;
using System.Numerics;
using System.Text.Json;
using Morava;

namespace Outpost
{
    /// <summary>What a weapon does with Fire: shoots (a gun) or strikes (a knife).</summary>
    public enum WeaponKind { Gun, Melee }

    /// <summary>
    /// A weapon's numbers (docs/games/Shooter_Plan.html, Phase 2.2: weapons as data, the same on every machine): read from
    /// its file in the project, Assets/Weapons/&lt;Name&gt;.weapon.json. Meters, seconds and degrees. Phase 2.4 adds the
    /// knife (Kind Melee), the slots and the switching times, the animator states each weapon is held with, where the
    /// first person's view model is put for it, and aiming down the sights. Phase 2.5 adds the sniper rifle: its scope
    /// (Scope: zoom levels, the scoped spread, the sway and steadying it), its bolt worked after each shot (Bolt), the
    /// reload's sounds as a list of cues, and a weapon that slows its carrier (MoveSpeedScale).
    /// </summary>
    public sealed class WeaponData
    {
        public string Name = "";
        public string Model = "";
        public WeaponKind Kind = WeaponKind.Gun;

        /// <summary>The number key that takes it (1 the rifle, 2 the sniper rifle, 3 the pistol, 4 the knife).</summary>
        public int Slot = 1;

        /// <summary>Seconds to take it out (it can't fire before) and to put it away (before the next one comes out).</summary>
        public float DrawSeconds = 0.4f, HolsterSeconds = 0.25f;

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

        /// <summary>Seconds into the reload its sounds come: the magazine out, a new one in (the reload clip's moments).</summary>
        public float MagazineOutAt = 0.35f, MagazineInAt = 1.55f;

        /// <summary>
        /// The reload's sounds at their moments (seconds into it, the clip, the volume), in time order: from ReloadSounds'
        /// MagazineOut and MagazineIn, or the file's ReloadCues (the sniper's: the bolt open, a round in twice, the bolt shut).
        /// </summary>
        public readonly List<(float At, string Sound, float Volume)> ReloadCues = new();

        /// <summary>The moving speeds times this while it is out (the sniper rifle is slow to run with; 1: as they are).</summary>
        public float MoveSpeedScale = 1.0f;

        /// <summary>
        /// A bolt worked after each shot (Bolt in the file; the sniper rifle): it starts BoltAfter seconds after the shot and
        /// takes BoltSeconds (the clip's), the scope staying up; its sounds at their moments into it.
        /// </summary>
        public bool HasBolt;
        public float BoltAfter = 0.4f, BoltSeconds = 1.1f;
        public readonly List<(float At, string Sound, float Volume)> BoltCues = new();

        /// <summary>
        /// The left hand holds the gun while the right one works the bolt (LeftHand in the file; the sniper rifle): the
        /// seconds into the bolt's clip and the reload's when it does (each a window, from, to: both hands are on the gun at
        /// either end), and where the gun is in the left hand then (meters and a rotation in hand_l's space, as
        /// Tools/CharacterPoses/make_rifle_poses.py measures it: "the gun in hand_l"). The game shows the weapon's model on
        /// the left hand then, the right hand's otherwise.
        /// </summary>
        public sealed class LeftHold
        {
            public readonly List<(float From, float To)> Windows = new();
            public Vector3 Translation;
            public Quaternion Rotation = Quaternion.Identity;

            public bool Holds(float seconds)
            {
                foreach (var (from, to) in Windows)
                    if (seconds >= from && seconds <= to)
                        return true;
                return false;
            }
        }
        public LeftHold? LeftInBolt, LeftInReload;

        /// <summary>
        /// The scope (Scope in the file; the sniper rifle: docs/games/Shooter_Plan.html, "The sniper scope"). Aim steps
        /// through its Zooms (magnifications) and back to none; scoped, the wheel zooms between MinZoom and MaxZoom
        /// (WheelStep times a notch). It comes up in ScopeSeconds. Scoped the spread is its own: ScopeStill standing, up to
        /// ScopeMoving at ScopeMovingSpeed (scoped it walks slower: AimSpeedScale), ScopeAir in the air (times
        /// SpreadCrouched crouched). The view sways with breathing (SwayDegrees at most, a slow figure of eight SwaySeconds long); held
        /// breath (Sprint or Walk held) steadies it to SteadyScale for SteadySeconds, then it sways TiredScale as much until
        /// the breath is back (SteadyRecoverSeconds for all of it).
        /// </summary>
        public bool HasScope;
        public float[] ScopeZooms = { 4.0f, 10.0f };
        public float ScopeMinZoom = 2.0f, ScopeMaxZoom = 12.0f, ScopeWheelStep = 1.25f, ScopeSeconds = 0.25f;
        public float ScopeStill = 0.02f, ScopeMoving = 1.5f, ScopeMovingSpeed = 1.2f, ScopeAir = 6.0f;
        public float SwayDegrees = 0.05f, SwaySeconds = 4.5f;
        public float SteadySeconds = 4.0f, SteadyScale = 0.15f, TiredScale = 1.8f, SteadyRecoverSeconds = 6.0f;

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

        /// <summary>
        /// Aiming down the sights (Aim held, a gun; AimDownSights in the file, else none): the view's field of view and the
        /// view model's (degrees), the seconds the sights take to come up, the spread times SpreadScale, the moving speed
        /// times SpeedScale (and no sprint), the third person's camera this far behind the shoulder (meters).
        /// </summary>
        public bool CanAimDownSights;
        public float AimFieldOfView = 50.0f, AimViewModelFieldOfView = 45.0f, AimSeconds = 0.2f;
        public float AimSpreadScale = 0.4f, AimSpeedScale = 0.6f, AimDistance = 1.3f;

        /// <summary>
        /// The knife (Kind Melee): damage a strike does, and from behind (within BehindAngle degrees of the target's back: a
        /// takedown); meters it reaches from the eyes, the strike's thickness (a sphere's radius), seconds a strike takes
        /// and seconds into it the blade arrives (the hit test).
        /// </summary>
        public float MeleeDamage = 50.0f, MeleeBehindDamage = 1000.0f, MeleeBehindAngle = 60.0f;
        public float MeleeRange = 1.5f, MeleeRadius = 0.2f, MeleeSeconds = 0.55f, MeleeHitAt = 0.2f;

        /// <summary>
        /// The animator states it is held with (Player.manim's UpperBody layer: aiming, lowered while sprinting, reloading,
        /// a strike, the bolt worked; ViewModel.manim's: aiming, a shot, reloading, a strike, the bolt worked).
        /// </summary>
        public string BodyAim = "Aim", BodyReady = "Ready", BodyReload = "Reload", BodyAttack = "", BodyBolt = "";
        public string ViewAim = "Aim", ViewFire = "Shoot", ViewReload = "Reload", ViewAttack = "", ViewBolt = "";

        /// <summary>
        /// The first person's view model for it: the eye in the arms' model (the aim pose's, by the sight line) and how far
        /// the model is moved from the camera (meters: right, up, forward), so the weapon shows low and to the right; aiming
        /// down the sights the eye is AimEye (on the sight line) and the model isn't moved.
        /// </summary>
        public Vector3 ViewModelEye = new Vector3(-0.1128f, 1.5315f, 0.2128f);
        public Vector3 ViewModelOffset = new Vector3(0.11f, -0.09f, 0.0f);
        public Vector3 AimEye = new Vector3(-0.0912f, 1.4897f, 0.1031f);

        /// <summary>The sounds: the shot ({0}: 1 to ShotVariants), the empty click, the magazine out and in, the fire mode's switch, taking it out.</summary>
        public string ShotSound = "", EmptySound = "", MagazineOutSound = "", MagazineInSound = "", FireModeSound = "", DrawSound = "";
        public int ShotVariants = 1;

        /// <summary>The knife's sounds: the swing ({0}: 1 to SwingVariants), a strike into a body, into something hard.</summary>
        public string SwingSound = "", HitFleshSound = "", HitHardSound = "";
        public int SwingVariants = 1;

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
            d.Kind = Text(root, "Kind", "Gun") == "Melee" ? WeaponKind.Melee : WeaponKind.Gun;
            d.Slot = (int)Number(root, "Slot", d.Slot);
            d.DrawSeconds = Number(root, "DrawSeconds", d.DrawSeconds);
            d.HolsterSeconds = Number(root, "HolsterSeconds", d.HolsterSeconds);
            if (root.TryGetProperty("Muzzle", out JsonElement muzzle))
                d.Muzzle = Vector(muzzle);
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
            if (root.TryGetProperty("ReloadSounds", out JsonElement reloadSounds))
            {
                d.MagazineOutAt = Number(reloadSounds, "MagazineOut", d.MagazineOutAt);
                d.MagazineInAt = Number(reloadSounds, "MagazineIn", d.MagazineInAt);
            }
            d.MoveSpeedScale = Number(root, "MoveSpeedScale", d.MoveSpeedScale);
            if (root.TryGetProperty("ReloadCues", out JsonElement reloadCues))
                Cues(reloadCues, d.ReloadCues);
            if (root.TryGetProperty("Bolt", out JsonElement bolt))
            {
                d.HasBolt = true;
                d.BoltAfter = Number(bolt, "After", d.BoltAfter);
                d.BoltSeconds = Number(bolt, "Seconds", d.BoltSeconds);
                if (bolt.TryGetProperty("Cues", out JsonElement boltCues))
                    Cues(boltCues, d.BoltCues);
            }
            if (root.TryGetProperty("LeftHand", out JsonElement left))
            {
                d.LeftInBolt = left.TryGetProperty("Bolt", out JsonElement inBolt) ? Hold(inBolt) : null;
                d.LeftInReload = left.TryGetProperty("Reload", out JsonElement inReload) ? Hold(inReload) : null;
            }
            if (root.TryGetProperty("Scope", out JsonElement scope))
            {
                d.HasScope = true;
                if (scope.TryGetProperty("Zooms", out JsonElement zooms))
                {
                    List<float> list = new();
                    foreach (JsonElement zoom in zooms.EnumerateArray())
                        list.Add(zoom.GetSingle());
                    if (list.Count > 0)
                        d.ScopeZooms = list.ToArray();
                }
                d.ScopeMinZoom = Number(scope, "MinZoom", d.ScopeMinZoom);
                d.ScopeMaxZoom = Number(scope, "MaxZoom", d.ScopeMaxZoom);
                d.ScopeWheelStep = Number(scope, "WheelStep", d.ScopeWheelStep);
                d.ScopeSeconds = Number(scope, "Seconds", d.ScopeSeconds);
                d.AimSpeedScale = Number(scope, "SpeedScale", d.AimSpeedScale);
                if (scope.TryGetProperty("Spread", out JsonElement scoped))
                {
                    d.ScopeStill = Number(scoped, "Still", d.ScopeStill);
                    d.ScopeMoving = Number(scoped, "Moving", d.ScopeMoving);
                    d.ScopeMovingSpeed = Number(scoped, "MovingSpeed", d.ScopeMovingSpeed);
                    d.ScopeAir = Number(scoped, "Air", d.ScopeAir);
                }
                if (scope.TryGetProperty("Sway", out JsonElement sway))
                {
                    d.SwayDegrees = Number(sway, "Degrees", d.SwayDegrees);
                    d.SwaySeconds = Number(sway, "Seconds", d.SwaySeconds);
                    d.SteadySeconds = Number(sway, "SteadySeconds", d.SteadySeconds);
                    d.SteadyScale = Number(sway, "SteadyScale", d.SteadyScale);
                    d.TiredScale = Number(sway, "TiredScale", d.TiredScale);
                    d.SteadyRecoverSeconds = Number(sway, "RecoverSeconds", d.SteadyRecoverSeconds);
                }
            }
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
            if (root.TryGetProperty("AimDownSights", out JsonElement ads))
            {
                d.CanAimDownSights = true;
                d.AimFieldOfView = Number(ads, "FieldOfView", d.AimFieldOfView);
                d.AimViewModelFieldOfView = Number(ads, "ViewModelFieldOfView", d.AimViewModelFieldOfView);
                d.AimSeconds = Number(ads, "Seconds", d.AimSeconds);
                d.AimSpreadScale = Number(ads, "SpreadScale", d.AimSpreadScale);
                d.AimSpeedScale = Number(ads, "SpeedScale", d.AimSpeedScale);
                d.AimDistance = Number(ads, "Distance", d.AimDistance);
            }
            if (root.TryGetProperty("Melee", out JsonElement melee))
            {
                d.MeleeDamage = Number(melee, "Damage", d.MeleeDamage);
                d.MeleeBehindDamage = Number(melee, "BehindDamage", d.MeleeBehindDamage);
                d.MeleeBehindAngle = Number(melee, "BehindAngle", d.MeleeBehindAngle);
                d.MeleeRange = Number(melee, "Range", d.MeleeRange);
                d.MeleeRadius = Number(melee, "Radius", d.MeleeRadius);
                d.MeleeSeconds = Number(melee, "Seconds", d.MeleeSeconds);
                d.MeleeHitAt = Number(melee, "HitAt", d.MeleeHitAt);
            }
            if (root.TryGetProperty("States", out JsonElement states))
            {
                d.BodyAim = Text(states, "BodyAim", d.BodyAim);
                d.BodyReady = Text(states, "BodyReady", d.BodyReady);
                d.BodyReload = Text(states, "BodyReload", d.BodyReload);
                d.BodyAttack = Text(states, "BodyAttack", d.BodyAttack);
                d.ViewAim = Text(states, "ViewAim", d.ViewAim);
                d.ViewFire = Text(states, "ViewFire", d.ViewFire);
                d.ViewReload = Text(states, "ViewReload", d.ViewReload);
                d.ViewAttack = Text(states, "ViewAttack", d.ViewAttack);
                d.BodyBolt = Text(states, "BodyBolt", d.BodyBolt);
                d.ViewBolt = Text(states, "ViewBolt", d.ViewBolt);
            }
            if (root.TryGetProperty("ViewModel", out JsonElement view))
            {
                if (view.TryGetProperty("Eye", out JsonElement eye))
                    d.ViewModelEye = Vector(eye);
                if (view.TryGetProperty("Offset", out JsonElement offset))
                    d.ViewModelOffset = Vector(offset);
                if (view.TryGetProperty("AimEye", out JsonElement aimEye))
                    d.AimEye = Vector(aimEye);
            }
            if (root.TryGetProperty("Sounds", out JsonElement sounds))
            {
                d.ShotSound = Text(sounds, "Shot", "");
                d.ShotVariants = (int)Number(sounds, "ShotVariants", 1);
                d.EmptySound = Text(sounds, "Empty", "");
                d.MagazineOutSound = Text(sounds, "MagazineOut", "");
                d.MagazineInSound = Text(sounds, "MagazineIn", "");
                d.FireModeSound = Text(sounds, "FireMode", "");
                d.DrawSound = Text(sounds, "Draw", "");
                d.SwingSound = Text(sounds, "Swing", "");
                d.SwingVariants = (int)Number(sounds, "SwingVariants", 1);
                d.HitFleshSound = Text(sounds, "HitFlesh", "");
                d.HitHardSound = Text(sounds, "HitHard", "");
            }
            // A gun's reload without a list of cues: the magazine out, then in (Phase 2.2's)
            if (d.ReloadCues.Count == 0 && d.Kind == WeaponKind.Gun)
            {
                d.ReloadCues.Add((d.MagazineOutAt, d.MagazineOutSound, 0.6f));
                d.ReloadCues.Add((d.MagazineInAt, d.MagazineInSound, 0.7f));
            }
            d.ReloadCues.Sort((a, b) => a.At.CompareTo(b.At));
            d.BoltCues.Sort((a, b) => a.At.CompareTo(b.At));
            return d;
        }

        // {"Windows": [[from, to], ...], "Translation": [x, y, z], "Rotation": [w, x, y, z]} (the bake's print: w first)
        static LeftHold Hold(JsonElement e)
        {
            LeftHold hold = new LeftHold();
            if (e.TryGetProperty("Windows", out JsonElement windows))
                foreach (JsonElement w in windows.EnumerateArray())
                    hold.Windows.Add((w[0].GetSingle(), w[1].GetSingle()));
            if (e.TryGetProperty("Translation", out JsonElement t))
                hold.Translation = Vector(t);
            if (e.TryGetProperty("Rotation", out JsonElement r))
                hold.Rotation = Quaternion.Normalize(new Quaternion(r[1].GetSingle(), r[2].GetSingle(), r[3].GetSingle(), r[0].GetSingle()));
            return hold;
        }

        // [[seconds, "sound"], [seconds, "sound", volume], ...]
        static void Cues(JsonElement list, List<(float At, string Sound, float Volume)> into)
        {
            foreach (JsonElement cue in list.EnumerateArray())
                into.Add((cue[0].GetSingle(), cue[1].GetString() ?? "", cue.GetArrayLength() > 2 ? cue[2].GetSingle() : 0.7f));
        }

        static Vector3 Vector(JsonElement e) => new Vector3(e[0].GetSingle(), e[1].GetSingle(), e[2].GetSingle());

        static float Number(JsonElement e, string name, float fallback) =>
            e.TryGetProperty(name, out JsonElement v) ? v.GetSingle() : fallback;

        static string Text(JsonElement e, string name, string fallback) =>
            e.TryGetProperty(name, out JsonElement v) ? v.GetString() ?? fallback : fallback;
    }
}
