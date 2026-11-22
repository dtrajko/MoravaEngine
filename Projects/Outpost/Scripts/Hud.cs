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
    /// its fire mode; the knife has no ammo and no spread (no bars); down the sights in first person the bars go (the
    /// sights are the aim). Phase 2.5: the sniper's scope, on a canvas of its own matched to the screen's height (so the
    /// lens stays round in any aspect ratio, black bars beside it): shown while the player's view is the scope's, with the
    /// darkening of its coming up; the crosshair's bars go meanwhile, and the zoom shows after the fire mode.
    /// </summary>
    public class Hud : Entity
    {
        public Entity? Player;
        public Entity? CrosshairTop, CrosshairBottom, CrosshairLeft, CrosshairRight;
        public Entity? Ammo, FireMode;
        public Entity? HitTopLeft, HitTopRight, HitBottomLeft, HitBottomRight;

        /// <summary>The scope's overlay: its root (the lens and the bars under it), and the darkening over it.</summary>
        public Entity? Scope, ScopeDark;

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
        }

        protected override void OnUpdate(float dt)
        {
            PlayerWeapon? weapon = player?.Weapon;
            bool shown = weapon != null && player != null && !player.IsFlying;
            SetVisible(shown);
            UpdateScope(shown && player != null && player.ScopeShown);
            if (!shown || weapon == null || player == null)
                return;
            // The bars: not for the knife, nor down the sights in first person, nor through the scope
            bool bars = !weapon.IsMelee && !(player.View == ViewMode.FirstPerson && player.AimBlend > 0.5f && !weapon.Data.HasScope) &&
                !player.ScopeShown;
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
