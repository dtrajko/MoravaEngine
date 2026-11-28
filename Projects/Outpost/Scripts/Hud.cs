using System;
using System.Collections.Generic;
using System.Numerics;
using Morava;

namespace Outpost
{
    /// <summary>
    /// The HUD (docs/games/Shooter_Plan.html: Phase 2.2 starts it with what shooting needs, Phase 3 adds the rest), on the
    /// canvas entity: the crosshair, four bars round the middle of the screen that open with the weapon's spread (the gap is
    /// where the spread's cone meets the screen), and the ammo (the magazine and the reserve) with the fire mode. The text
    /// uses the engine's font until the HUD gets its own (Phase 3). Phase 2.3: the hit marker, four short diagonal bars
    /// round the crosshair while a shot that hit fades (red when it downed its target). Phase 2.4: the weapon's name before
    /// its fire mode; the knife has no ammo and no spread (no bars); down the sights the bars stay (since 2026-11-22, the
    /// user: they went, and the sights weren't quite where the shots go; now the front sight is on the middle). Phase 2.5: the sniper's scope, on a canvas of its own matched to the screen's height (so the
    /// lens stays round in any aspect ratio, black bars beside it): shown while the player's view is the scope's, with the
    /// darkening of its coming up; the crosshair's bars go meanwhile, and the zoom shows after the fire mode.
    ///
    /// Phase 3.1: health and armor at the bottom left (the numbers and a bar each; health red and pulsing when low), a
    /// message above them (a pickup's "+25 HEALTH", "CHECKPOINT") fading out, the screen's edges red after a hit (the
    /// DamageVignette texture, Tools/Textures/GenerateHudTextures.py), and dead: the screen darkened, the title, and once
    /// Fire starts again the hint, slowly pulsing; the crosshair, the ammo and the hit marker go while dead.
    ///
    /// Phase 3.2: the HUD's own font (Barlow Condensed, SIL OFL: Licenses.md), set on the texts in the scene; red arcs round
    /// the crosshair toward where hits came from (Player.HitDirections; the DamageArc texture), turning as the player
    /// turns; the weapons' slots, a row of four at the bottom right with the weapons' silhouettes
    /// (Tools/Textures/RenderWeaponIcons.py), shown for a moment when the weapon chosen changes, that one lit; the
    /// objectives: the ones not done at the top right, all of them in a panel while Objectives (Tab) is held, and a banner
    /// with a radio's tones when one is done. Their parts are found by name: "Damage Arc 1".., "Slot 1 Frame", "Slot 1
    /// Icon", "Slot 1 Number".., "Objective Line 1".., "Objectives Panel Line 1".. and "Objectives Panel Status 1"..
    /// </summary>
    public class Hud : Entity
    {
        public Entity? Player;
        public Entity? CrosshairTop, CrosshairBottom, CrosshairLeft, CrosshairRight;
        public Entity? Ammo, FireMode;
        public Entity? HitTopLeft, HitTopRight, HitBottomLeft, HitBottomRight;

        /// <summary>The scope's overlay: its root (the lens and the bars under it), and the darkening over it.</summary>
        public Entity? Scope, ScopeDark;

        /// <summary>
        /// Phase 3.1: health and armor (the numbers, the bars' fills: their full widths are the scene's, and their frames
        /// and labels, shown with them), the message, the red at the edges after a hit, and the death screen (the
        /// darkening, the title, the hint).
        /// </summary>
        public Entity? HealthValue, HealthBar, HealthFrame, HealthLabel, ArmorValue, ArmorBar, ArmorFrame, ArmorLabel;
        public Entity? Message, DamageFlash, DeathDark, DeathTitle, DeathHint;

        /// <summary>Phase 4.1: under the crosshair, the key and what Use does to what is looked at ("E  OPEN", "LOCKED").</summary>
        public Entity? UsePrompt;

        /// <summary>
        /// Phase 3.2: the roots of the weapons' slots, the objectives' list at the top right, the panel shown while
        /// Objectives is held, and the banner when one is done (its title and its line under it).
        /// </summary>
        public Entity? Slots, ObjectiveList, ObjectivesPanel, ObjectiveBanner, ObjectiveBannerTitle, ObjectiveBannerText;

        /// <summary>The arcs' color (their alpha the most, at a strong hit) and how far their texture's middle is from the
        /// crosshair (Tools/Textures/GenerateHudTextures.py prints it: the ring's middle below the picture's).</summary>
        public Vector4 DamageArcColor = new Vector4(0.95f, 0.12f, 0.05f, 0.9f);
        public float DamageArcDistance = 204.0f;

        /// <summary>Seconds the slots show after a change (fading over the last half second), the weapons' silhouettes
        /// ({0}: the weapon's name), and the slots' colors: chosen, the others, and a gun with no rounds left.</summary>
        public float SlotsSeconds = 2.0f;
        public string WeaponIcon = "Assets/Textures/UI/Weapon_{0}.png";
        public Vector4 SlotChosenColor = new Vector4(0.95f, 0.78f, 0.35f, 0.35f), SlotColor = new Vector4(0.0f, 0.0f, 0.0f, 0.45f);
        public Vector4 IconChosenColor = new Vector4(1.0f, 1.0f, 1.0f, 1.0f), IconColor = new Vector4(1.0f, 1.0f, 1.0f, 0.45f);
        public Vector4 IconEmptyColor = new Vector4(1.0f, 0.3f, 0.25f, 0.55f);

        /// <summary>Seconds the banner shows when an objective is done, its sound and how loud.</summary>
        public float BannerSeconds = 3.5f;
        public string ObjectiveSound = "Assets/Audio/Objective_Complete.wav";
        public float ObjectiveVolume = 0.6f;

        /// <summary>The objectives' panel shown as while Objectives is held (a test; Phase 3.3's pause menu).</summary>
        public bool ShowObjectives { get; set; }

        /// <summary>The objectives' colors: to do, optional, done.</summary>
        public Vector4 ObjectiveColor = new Vector4(1.0f, 1.0f, 1.0f, 0.9f), OptionalColor = new Vector4(0.8f, 0.85f, 0.9f, 0.6f);
        public Vector4 DoneColor = new Vector4(0.55f, 0.9f, 0.5f, 0.85f);

        /// <summary>Health at or below this shows red, pulsing; the colors of the health, the low health and the armor.</summary>
        public float LowHealth = 25.0f;
        public Vector4 HealthColor = new Vector4(1.0f, 1.0f, 1.0f, 0.9f), LowHealthColor = new Vector4(1.0f, 0.25f, 0.2f, 1.0f);
        public Vector4 ArmorColor = new Vector4(0.6f, 0.78f, 1.0f, 0.9f);

        /// <summary>The hit's red at the screen's edges (its alpha the most, at a big hit).</summary>
        public Vector4 DamageColor = new Vector4(0.75f, 0.02f, 0.0f, 0.85f);

        /// <summary>The hit marker's bars: pixels from the middle to their middles, and their colors (a hit, a kill).</summary>
        public float HitMarkerGap = 13.0f;
        public Vector4 HitColor = new Vector4(1.0f, 1.0f, 1.0f, 0.95f), KillColor = new Vector4(1.0f, 0.25f, 0.2f, 1.0f);

        /// <summary>Pixels (of the canvas's 1080 high) the crosshair's bars keep from the middle at the least, and their length.</summary>
        public float MinGap = 5.0f, BarLength = 14.0f;

        Player? player;
        UIRect? top, bottom, left, right;
        UIText? ammo, fireMode;
        UIRect?[] hitRects = Array.Empty<UIRect?>();
        UIImage?[] hitImages = Array.Empty<UIImage?>();
        Morava.Camera? camera;
        UIRect? scopeRect;
        UIRect? scopeDarkRect;
        UIImage? scopeDarkImage;
        UIText? healthValue, armorValue, message, deathTitle, deathHint, usePrompt;
        UIRect? healthBar, armorBar;
        UIImage? healthBarImage, armorBarImage, damageImage, deathDarkImage;
        float healthBarWidth, armorBarWidth;
        float pulse;

        // Phase 3.2
        readonly List<(UIRect? Rect, UIImage? Image)> arcs = new();
        readonly List<(UIImage? Frame, UIImage? Icon, UIText? Number, Entity? IconEntity)> slots = new();
        readonly List<UIText?> objectiveLines = new();
        readonly List<(UIText? Line, UIText? Status)> panelLines = new();
        UIText? bannerTitle, bannerText;
        PlayerWeapon? lastChosen;
        float slotsLeft, bannerLeft;
        int completionsSeen, respawnsSeen;

        protected override void OnCreate()
        {
            player = Player as Player;
            if (player == null)
                Log.Warn($"{Name}: no Player, the HUD shows nothing");
            top = CrosshairTop?.GetComponent<UIRect>();
            bottom = CrosshairBottom?.GetComponent<UIRect>();
            left = CrosshairLeft?.GetComponent<UIRect>();
            right = CrosshairRight?.GetComponent<UIRect>();
            ammo = Ammo?.GetComponent<UIText>();
            fireMode = FireMode?.GetComponent<UIText>();
            camera = player?.Camera?.GetComponent<Morava.Camera>();
            Entity?[] hits = { HitTopLeft, HitTopRight, HitBottomLeft, HitBottomRight };
            hitRects = Array.ConvertAll(hits, e => e?.GetComponent<UIRect>());
            hitImages = Array.ConvertAll(hits, e => e?.GetComponent<UIImage>());
            scopeRect = Scope?.GetComponent<UIRect>();
            scopeDarkRect = ScopeDark?.GetComponent<UIRect>();
            scopeDarkImage = ScopeDark?.GetComponent<UIImage>();
            healthValue = HealthValue?.GetComponent<UIText>();
            armorValue = ArmorValue?.GetComponent<UIText>();
            message = Message?.GetComponent<UIText>();
            usePrompt = UsePrompt?.GetComponent<UIText>();
            deathTitle = DeathTitle?.GetComponent<UIText>();
            deathHint = DeathHint?.GetComponent<UIText>();
            healthBar = HealthBar?.GetComponent<UIRect>();
            armorBar = ArmorBar?.GetComponent<UIRect>();
            healthBarImage = HealthBar?.GetComponent<UIImage>();
            armorBarImage = ArmorBar?.GetComponent<UIImage>();
            damageImage = DamageFlash?.GetComponent<UIImage>();
            deathDarkImage = DeathDark?.GetComponent<UIImage>();
            healthBarWidth = healthBar?.Size.X ?? 0.0f;
            armorBarWidth = armorBar?.Size.X ?? 0.0f;
            FindParts();
        }

        // Phase 3.2's parts, by their names (as many as the scene has)
        void FindParts()
        {
            for (int i = 1; Scene.FindEntityByName($"Damage Arc {i}") is Entity arc; i++)
                arcs.Add((arc.GetComponent<UIRect>(), arc.GetComponent<UIImage>()));
            for (int i = 1; Scene.FindEntityByName($"Slot {i} Frame") is Entity frame; i++)
            {
                Entity? icon = Scene.FindEntityByName($"Slot {i} Icon");
                slots.Add((frame.GetComponent<UIImage>(), icon?.GetComponent<UIImage>(),
                    Scene.FindEntityByName($"Slot {i} Number")?.GetComponent<UIText>(), icon));
            }
            for (int i = 1; Scene.FindEntityByName($"Objective Line {i}") is Entity line; i++)
                objectiveLines.Add(line.GetComponent<UIText>());
            for (int i = 1; Scene.FindEntityByName($"Objectives Panel Line {i}") is Entity line; i++)
                panelLines.Add((line.GetComponent<UIText>(), Scene.FindEntityByName($"Objectives Panel Status {i}")?.GetComponent<UIText>()));
            bannerTitle = ObjectiveBannerTitle?.GetComponent<UIText>();
            bannerText = ObjectiveBannerText?.GetComponent<UIText>();
            completionsSeen = Objectives.Completions;
            lastChosen = player?.Arsenal.Selected;
            respawnsSeen = player?.Respawns ?? 0;

            // The slots' silhouettes: the weapon in each slot (none: the slot's icon hidden)
            for (int i = 0; i < slots.Count; i++)
            {
                PlayerWeapon? weapon = WeaponInSlot(i + 1);
                if (slots[i].Icon is UIImage image && weapon != null)
                    image.Texture = string.Format(WeaponIcon, weapon.Data.Name);
                Show(slots[i].IconEntity, weapon != null);
            }
            Show(Slots, false);
            Show(ObjectivesPanel, false);
            Show(ObjectiveBanner, false);
            if (arcs.Count == 0 || slots.Count == 0 || objectiveLines.Count == 0 || panelLines.Count == 0)
                Log.Warn($"{Name}: parts missing ({arcs.Count} damage arcs, {slots.Count} slots, {objectiveLines.Count} objective lines, {panelLines.Count} panel lines)");
        }

        PlayerWeapon? WeaponInSlot(int slot)
        {
            if (player == null)
                return null;
            int index = player.Arsenal.IndexOfSlot(slot);
            return index >= 0 ? player.Arsenal.Weapons[index] : null;
        }

        protected override void OnUpdate(float dt)
        {
            PlayerWeapon? weapon = player?.Weapon;
            UpdateVitals(dt);
            UpdateDamageArcs();
            UpdateSlots(dt);
            UpdateObjectives(dt);
            bool shown = weapon != null && player != null && !player.IsFlying && !player.IsDead;
            SetVisible(shown);
            UpdateScope(shown && player != null && player.ScopeShown);
            if (!shown || weapon == null || player == null)
                return;
            // The bars: not for the knife, nor through the scope (its reticle is the aim). Down the sights they stay (the
            // user, 2026-11-22): they frame the front sight's tip, which the view model puts on the middle (SightDirection)
            bool bars = !weapon.IsMelee && !player.ScopeShown;
            foreach (UIRect? rect in new[] { top, bottom, left, right })
                if (rect != null && rect.Visible != bars)
                    rect.Visible = bars;

            // The spread's cone where it meets the screen: tan(spread) over tan(half the field of view), of half the height
            float fov = camera?.FieldOfView ?? 70.0f;
            float gap = MathF.Tan(weapon.Spread * MathF.PI / 180.0f) / MathF.Tan(fov * 0.5f * MathF.PI / 180.0f) * 540.0f;
            gap = MathF.Max(MinGap, gap);
            float middle = gap + BarLength * 0.5f;
            Place(top, new Vector2(0.0f, -middle)); // the canvas's y runs down
            Place(bottom, new Vector2(0.0f, middle));
            Place(left, new Vector2(-middle, 0.0f));
            Place(right, new Vector2(middle, 0.0f));

            if (ammo != null)
            {
                string text = weapon.IsMelee ? "" : weapon.IsReloading ? $"RELOADING  {weapon.Reserve}" : $"{weapon.Ammo}  /  {weapon.Reserve}";
                if (ammo.Text != text)
                    ammo.Text = text;
            }
            if (fireMode != null)
            {
                string name = weapon.Data.Name.ToUpperInvariant();
                string text = weapon.IsMelee ? name : $"{name}   {weapon.FireMode.ToUpperInvariant()}";
                if (player.ScopeShown)
                    text += $"   {player.ScopeZoom:0.0}x";
                if (fireMode.Text != text)
                    fireMode.Text = text;
            }
            UpdateHitMarker();
        }

        // The four bars on the diagonals (the canvas's y down: top left is -x, -y), turned 45 degrees along them, faded out
        void UpdateHitMarker()
        {
            float shown = player != null ? Math.Clamp(player.HitMarker, 0.0f, 1.0f) : 0.0f;
            Vector4 color = player != null && player.KillMarker ? KillColor : HitColor;
            float d = HitMarkerGap * 0.7071f * (1.0f + 0.25f * (1.0f - shown)); // opening a little as it fades
            Vector2[] places = { new(-d, -d), new(d, -d), new(-d, d), new(d, d) };
            float[] turns = { -45.0f, 45.0f, 45.0f, -45.0f }; // a tall bar turned clockwise: leaning right
            for (int i = 0; i < hitRects.Length; i++)
            {
                UIRect? rect = hitRects[i];
                if (rect == null)
                    continue;
                bool visible = shown > 0.0f;
                if (rect.Visible != visible)
                    rect.Visible = visible;
                if (!visible)
                    continue;
                Place(rect, places[i]);
                if (rect.Rotation != turns[i])
                    rect.Rotation = turns[i];
                if (hitImages[i] is UIImage image)
                    image.Color = new Vector4(color.X, color.Y, color.Z, color.W * MathF.Min(1.0f, shown * 1.5f));
            }
        }

        // Phase 3.1: health and armor, the message, the hit's red, the death screen (all hidden while the fly camera has the view)
        void UpdateVitals(float dt)
        {
            bool shown = player != null && !player.IsFlying;
            pulse += dt;
            foreach (Entity? e in new[] { HealthValue, HealthFrame, HealthLabel, ArmorValue, ArmorFrame, ArmorLabel })
                Show(e, shown);
            if (player == null || !shown)
            {
                foreach (Entity? e in new[] { HealthBar, ArmorBar, Message, DamageFlash, DeathDark, DeathTitle, DeathHint, UsePrompt })
                    Show(e, false);
                return;
            }
            float health = player.Health, armor = player.Armor;
            bool low = health <= LowHealth;
            Vector4 healthColor = HealthColor;
            if (low)
            {
                // Low: red, pulsing faster the lower it is (dead: still)
                float rate = player.IsDead ? 0.0f : 1.0f + 1.5f * (1.0f - health / MathF.Max(1.0f, LowHealth));
                float beat = 0.75f + 0.25f * MathF.Cos(pulse * rate * MathF.PI * 2.0f);
                healthColor = new Vector4(LowHealthColor.X, LowHealthColor.Y, LowHealthColor.Z, LowHealthColor.W * beat);
            }
            SetText(healthValue, $"{MathF.Ceiling(health):0}");
            SetText(armorValue, $"{MathF.Ceiling(armor):0}");
            if (healthValue != null && healthValue.Color != healthColor)
                healthValue.Color = healthColor;
            SetBar(healthBar, healthBarImage, healthBarWidth, health / MathF.Max(1.0f, player.Vitals.MaxHealth), healthColor);
            SetBar(armorBar, armorBarImage, armorBarWidth, armor / MathF.Max(1.0f, player.Vitals.MaxArmor), ArmorColor);
            if (armorValue != null)
            {
                Vector4 color = armor > 0.0f ? ArmorColor : new Vector4(ArmorColor.X, ArmorColor.Y, ArmorColor.Z, ArmorColor.W * 0.45f);
                if (armorValue.Color != color)
                    armorValue.Color = color;
            }

            // The message, fading out at its end (not while dead)
            float messageShown = player.IsDead ? 0.0f : player.MessageShown;
            Show(Message, messageShown > 0.0f);
            if (message != null && messageShown > 0.0f)
            {
                SetText(message, player.Message);
                Vector4 c = message.Color;
                message.Color = new Vector4(c.X, c.Y, c.Z, 0.95f * messageShown);
            }

            // What Use does to what is looked at (not while dead or scoped)
            IUsable? usable = player.IsDead || player.ScopeShown ? null : player.UseTarget;
            Show(UsePrompt, usable != null);
            if (usable != null)
                SetText(usePrompt, usable.CanUse ? $"{HudMap.Binding("Use")}  {usable.UseText}" : usable.UseText);

            // The hit's red at the edges; low on health a little stays, dead more
            float flash = MathF.Max(player.HurtFlash, player.IsDead ? 0.5f : low ? 0.18f : 0.0f);
            Show(DamageFlash, flash > 0.001f);
            if (damageImage != null && flash > 0.001f)
                damageImage.Color = new Vector4(DamageColor.X, DamageColor.Y, DamageColor.Z, DamageColor.W * flash);

            // Dead: darkening over the first second, the title, then the hint once Fire starts again
            float dark = player.IsDead ? Math.Clamp(player.DeadSeconds / 1.0f, 0.0f, 1.0f) : 0.0f;
            Show(DeathDark, dark > 0.0f);
            Show(DeathTitle, dark > 0.0f);
            if (deathDarkImage != null && dark > 0.0f)
                deathDarkImage.Color = new Vector4(0.0f, 0.0f, 0.0f, 0.45f * dark);
            if (deathTitle != null && dark > 0.0f)
            {
                Vector4 c = deathTitle.Color;
                deathTitle.Color = new Vector4(c.X, c.Y, c.Z, dark);
            }
            bool hint = player.CanRespawn;
            Show(DeathHint, hint);
            if (deathHint != null && hint)
            {
                Vector4 c = deathHint.Color;
                deathHint.Color = new Vector4(c.X, c.Y, c.Z, 0.65f + 0.35f * MathF.Cos(pulse * MathF.PI));
            }
        }

        // Phase 3.2: an arc toward each recent hit's source, round the crosshair; the angle from the look's forward,
        // clockwise (to the right) positive, as the canvas turns its rectangles (its y down: up is -y)
        void UpdateDamageArcs()
        {
            bool shown = player != null && !player.IsFlying && !player.IsDead;
            IReadOnlyList<HitDirection> hits = player != null ? player.HitDirections : Array.Empty<HitDirection>();
            float yaw = (player?.LookYaw ?? 0.0f) * MathF.PI / 180.0f;
            Vector3 forward = new Vector3(-MathF.Sin(yaw), 0.0f, -MathF.Cos(yaw));
            Vector3 right = new Vector3(MathF.Cos(yaw), 0.0f, -MathF.Sin(yaw));
            Vector3 eyes = player?.CameraPosition ?? Vector3.Zero;
            for (int i = 0; i < arcs.Count; i++)
            {
                (UIRect? rect, UIImage? image) = arcs[i];
                bool visible = shown && i < hits.Count;
                if (rect != null && rect.Visible != visible)
                    rect.Visible = visible;
                if (!visible || rect == null)
                    continue;
                HitDirection hit = hits[i];
                Vector3 to = hit.From - eyes;
                float angle = MathF.Atan2(Vector3.Dot(to, right), Vector3.Dot(to, forward));
                Place(rect, new Vector2(MathF.Sin(angle), -MathF.Cos(angle)) * DamageArcDistance);
                float degrees = angle * 180.0f / MathF.PI;
                if (rect.Rotation != degrees)
                    rect.Rotation = degrees;
                if (image != null)
                {
                    float fade = MathF.Min(1.0f, hit.Left * 2.5f); // full, then fading over its last 40 %
                    image.Color = new Vector4(DamageArcColor.X, DamageArcColor.Y, DamageArcColor.Z, DamageArcColor.W * hit.Strength * fade);
                }
            }
        }

        // Phase 3.2: the slots, for SlotsSeconds after the weapon chosen changes (the number keys, the wheel, Q), the chosen
        // one lit; a gun with nothing left to fire red
        void UpdateSlots(float dt)
        {
            PlayerWeapon? chosen = player?.Arsenal.Selected;
            if (chosen != lastChosen)
            {
                // (not for starting again: the first weapon is out by itself)
                bool respawned = player != null && player.Respawns != respawnsSeen;
                if (lastChosen != null && chosen != null && player != null && !player.IsDead && !respawned)
                    slotsLeft = SlotsSeconds;
                lastChosen = chosen;
            }
            respawnsSeen = player?.Respawns ?? 0;
            if (player != null && player.IsDead)
                slotsLeft = 0.0f;
            slotsLeft = MathF.Max(0.0f, slotsLeft - dt);
            bool shown = slotsLeft > 0.0f && player != null && !player.IsFlying && !player.IsDead;
            Show(Slots, shown);
            if (!shown)
                return;
            float fade = MathF.Min(1.0f, slotsLeft / 0.5f);
            for (int i = 0; i < slots.Count; i++)
            {
                PlayerWeapon? weapon = WeaponInSlot(i + 1);
                bool isChosen = weapon != null && weapon == chosen;
                bool empty = weapon != null && !weapon.IsMelee && weapon.Ammo + weapon.Reserve <= 0;
                (UIImage? frame, UIImage? icon, UIText? number, _) = slots[i];
                if (frame != null)
                    frame.Color = Faded(isChosen ? SlotChosenColor : SlotColor, fade);
                if (icon != null)
                    icon.Color = Faded(empty ? IconEmptyColor : isChosen ? IconChosenColor : IconColor, fade);
                if (number != null)
                    number.Color = Faded(isChosen ? IconChosenColor : IconColor, fade);
            }
        }

        // Phase 3.2: the objectives not done at the top right (not while the panel shows them all), the panel while
        // Objectives is held, the banner and its sound when one is done
        void UpdateObjectives(float dt)
        {
            bool alive = player != null && !player.IsFlying;
            IReadOnlyList<Objective> all = Objectives.All;

            if (Objectives.Completions != completionsSeen)
            {
                completionsSeen = Objectives.Completions;
                Objective? done = Objectives.LastCompleted;
                SetText(bannerTitle, Objectives.AllRequiredComplete ? "ALL OBJECTIVES COMPLETE" : "OBJECTIVE COMPLETE");
                SetText(bannerText, done?.Title ?? "");
                bannerLeft = BannerSeconds;
                if (!string.IsNullOrEmpty(ObjectiveSound))
                    Audio.PlayOneShot(ObjectiveSound, ObjectiveVolume);
            }
            bannerLeft = MathF.Max(0.0f, bannerLeft - dt);
            bool banner = alive && bannerLeft > 0.0f;
            Show(ObjectiveBanner, banner);
            if (banner)
            {
                // In quickly, out over the last 0.8 s
                float fade = MathF.Min(1.0f, bannerLeft / 0.8f) * MathF.Min(1.0f, (BannerSeconds - bannerLeft) / 0.15f);
                if (bannerTitle != null)
                    bannerTitle.Color = Faded(bannerTitle.Color with { W = 1.0f }, fade);
                if (bannerText != null)
                    bannerText.Color = Faded(bannerText.Color with { W = 0.9f }, fade);
            }

            bool panel = alive && (ShowObjectives || Input.IsHeld("Objectives"));
            Show(ObjectivesPanel, panel);
            if (panel)
            {
                for (int i = 0; i < panelLines.Count; i++)
                {
                    (UIText? line, UIText? status) = panelLines[i];
                    Objective? o = i < all.Count ? all[i] : null;
                    SetText(line, o != null ? $"{i + 1}   {o.Title}" : "");
                    SetText(status, o == null ? "" : o.IsComplete ? "COMPLETE" : o.Optional ? "OPTIONAL" : "");
                    Vector4 color = o == null ? ObjectiveColor : o.IsComplete ? DoneColor : o.Optional ? OptionalColor : ObjectiveColor;
                    if (line != null && line.Color != color)
                        line.Color = color;
                    if (status != null && status.Color != color)
                        status.Color = color;
                }
            }

            // The list: those not done, the ones that must be done first (each group in its order)
            bool list = alive && !panel && player != null && !player.IsDead;
            int shownLines = 0;
            if (list)
                foreach (bool optional in new[] { false, true })
                    foreach (Objective o in all)
                    {
                        if (o.IsComplete || o.Optional != optional || shownLines >= objectiveLines.Count)
                            continue;
                        UIText? line = objectiveLines[shownLines++];
                        SetText(line, optional ? $"{o.Title}  (optional)" : o.Title);
                        Vector4 color = optional ? OptionalColor : ObjectiveColor;
                        if (line != null && line.Color != color)
                            line.Color = color;
                    }
            Show(ObjectiveList, list && shownLines > 0);
            for (int i = shownLines; i < objectiveLines.Count; i++)
                SetText(objectiveLines[i], "");
        }

        static Vector4 Faded(Vector4 color, float fade) => new Vector4(color.X, color.Y, color.Z, color.W * fade);

        static void SetBar(UIRect? bar, UIImage? image, float width, float fraction, Vector4 color)
        {
            if (bar == null)
                return;
            Vector2 size = new Vector2(width * Math.Clamp(fraction, 0.0f, 1.0f), bar.Size.Y);
            if (bar.Size != size)
                bar.Size = size;
            bool visible = size.X > 0.5f;
            if (bar.Visible != visible)
                bar.Visible = visible;
            if (image != null && image.Color != color)
                image.Color = color;
        }

        static void SetText(UIText? text, string value)
        {
            if (text != null && text.Text != value)
                text.Text = value;
        }

        static void Show(Entity? entity, bool shown)
        {
            if (entity?.GetComponent<UIRect>() is UIRect rect && rect.Visible != shown)
                rect.Visible = shown;
        }

        // The scope's overlay while the view is the scope's, the darkening fading as it comes up
        void UpdateScope(bool scoped)
        {
            if (scopeRect != null && scopeRect.Visible != scoped)
                scopeRect.Visible = scoped;
            float dark = scoped && player != null ? player.ScopeDarkness : 0.0f;
            bool darkShown = dark > 0.001f;
            if (scopeDarkRect != null && scopeDarkRect.Visible != darkShown)
                scopeDarkRect.Visible = darkShown;
            if (darkShown && scopeDarkImage != null)
                scopeDarkImage.Color = new Vector4(0.0f, 0.0f, 0.0f, dark);
        }

        static void Place(UIRect? rect, Vector2 position)
        {
            if (rect != null && rect.Position != position)
                rect.Position = position;
        }

        void SetVisible(bool visible)
        {
            parts ??= new[] { top, bottom, left, right, Ammo?.GetComponent<UIRect>(), FireMode?.GetComponent<UIRect>() };
            foreach (UIRect? rect in parts)
                if (rect != null && rect.Visible != visible)
                    rect.Visible = visible;
            if (!visible)
                foreach (UIRect? rect in hitRects)
                    if (rect != null && rect.Visible)
                        rect.Visible = false;
        }

        UIRect?[]? parts;
    }
}
