using System;
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
        UIText? healthValue, armorValue, message, deathTitle, deathHint;
        UIRect? healthBar, armorBar;
        UIImage? healthBarImage, armorBarImage, damageImage, deathDarkImage;
        float healthBarWidth, armorBarWidth;
        float pulse;

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
        }

        protected override void OnUpdate(float dt)
        {
            PlayerWeapon? weapon = player?.Weapon;
            UpdateVitals(dt);
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
                foreach (Entity? e in new[] { HealthBar, ArmorBar, Message, DamageFlash, DeathDark, DeathTitle, DeathHint })
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
