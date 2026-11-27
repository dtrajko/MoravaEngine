using System;
using System.Collections.Generic;
using System.Numerics;
using Morava;

namespace Outpost
{
    /// <summary>
    /// The HUD's map (docs/games/Shooter_Plan.html, Phase 3.4): a camera straight above the player draws the level into a
    /// render texture (Minimap.mrtex) that a UI image at the top left shows, turned with the player's view (its facing is up,
    /// "N" on the rim says where north is). Map (M, the D-pad's up) opens the big map over the screen's middle instead: the
    /// whole level, north up (BigMap.mrtex, its own camera), the player's arrow turned to the view; again, closed.
    ///
    /// The plan of the player's floor: the cameras' near clip is a plane CutAbove meters over the floor the player last
    /// stood on, so the roofs and the floors above are cut away and the walls show cut below the doors' tops. The engine
    /// tilts a camera 89 degrees down at most (Pitch), so the cameras lean back a little to keep what they show under
    /// their pictures' middles, and the cut leans 1 degree with them (on the big map, 0.35 m higher at the buildings'
    /// north walls than at their south ones). Each camera draws only while its map shows (Camera.Enabled), every
    /// DrawEvery frames.
    ///
    /// The marks over the pictures are UI images found by name ("Minimap Mark 1".., "Big Map Mark 1"..), given their
    /// texture (Tools/Textures/GenerateHudTextures.py) and color each frame: the pickups while they can be taken, the
    /// training dummies (the guards from Phase 5; grey while down) and the objectives not done, which stay on the
    /// minimap's rim when they are off it; an objective that is a pickup frames the pickup's mark (twice its size, under
    /// it). Marks on another floor than the player's are faded.
    /// </summary>
    public class HudMap : Entity
    {
        public Entity? Player;
        /// <summary>The HUD (its objectives' panel hides the big map while shown).</summary>
        public Entity? Hud;
        public Entity? MinimapCamera, BigMapCamera;
        /// <summary>The minimap (its frame), its picture (the marks' parent), the "N" on its rim and the player's arrow.</summary>
        public Entity? Minimap, MinimapView, MinimapNorth, MinimapPlayer;
        /// <summary>The big map (its frame), its picture, the player's arrow, its title (the floor) and its hint.</summary>
        public Entity? BigMap, BigMapView, BigMapPlayer, BigMapTitle, BigMapHint;

        // ---- The cut and the cameras: CameraHeight over the cut, seeing Depth meters below it
        /// <summary>The cameras' tilt down: the engine's most (src/EnvMapVulkan/EnvMapVulkanSceneData.cpp, PlaceCamera).</summary>
        public const float Pitch = 89.0f;
        public float CutAbove = 1.6f;
        public float StoryHeight = 3.5f;
        public float CameraHeight = 150.0f;
        public float Depth = 30.0f;

        /// <summary>Meters across the minimap.</summary>
        public float MinimapRange = 44.0f;
        /// <summary>The big map's middle (x, z) and the meters across it: the greybox's ground with a margin.</summary>
        public Vector2 BigMapCenter = new Vector2(0.0f, 0.0f);
        public float BigMapRange = 86.0f;

        // ---- The marks
        public string MarkTexture = "Assets/Textures/UI/Map{0}.png";
        public float MarkSize = 22.0f, BigMarkSize = 26.0f;
        /// <summary>A mark this many meters above or below the player's floor is on another floor: its alpha times OtherFloorAlpha.</summary>
        public float OtherFloor = 2.2f;
        public float OtherFloorAlpha = 0.35f;
        public Vector4 GuardColor = new Vector4(0.95f, 0.22f, 0.15f, 1.0f), DownColor = new Vector4(0.6f, 0.6f, 0.6f, 0.8f);
        public Vector4 HealthColor = new Vector4(0.35f, 0.9f, 0.4f, 1.0f), ArmorColor = new Vector4(0.45f, 0.7f, 1.0f, 1.0f);
        public Vector4 AmmoColor = new Vector4(0.95f, 0.85f, 0.35f, 1.0f), ObjectiveColor = new Vector4(1.0f, 1.0f, 1.0f, 1.0f);

        /// <summary>The big map shows (Map pressed, and nothing in its way).</summary>
        public bool BigMapShown { get; private set; }
        /// <summary>Map was pressed to open it (it may be hidden meanwhile: paused, dead, the objectives' panel).</summary>
        public bool BigMapOpen { get; private set; }
        /// <summary>The floor the map is cut at (the height the player last stood at) and the cut's height.</summary>
        public float FloorHeight => floorY;
        public float CutHeight => floorY + CutAbove;
        /// <summary>The marks shown on each map this frame (a test reads them).</summary>
        public int MinimapMarks { get; private set; }
        public int BigMapMarks { get; private set; }

        Player? player;
        Hud? hud;
        Morava.Camera? minimapCamera, bigMapCamera;
        UIRect? minimapRect, bigMapRect, minimapViewRect, bigMapViewRect, northRect, bigPlayerRect;
        UIText? bigTitle, bigHint;
        readonly List<(UIRect? Rect, UIImage? Image)> minimapMarks = new(), bigMapMarks = new();
        readonly List<(Vector3 Position, string Shape, Vector4 Color, bool Pinned, float Scale)> marks = new();
        float floorY;
        bool hasFloor, warned;

        protected override void OnCreate()
        {
            player = Player as Player;
            hud = Hud as Hud;
            if (player == null)
                Log.Warn($"{Name}: no Player, the map shows nothing");
            minimapCamera = MinimapCamera?.GetComponent<Morava.Camera>();
            bigMapCamera = BigMapCamera?.GetComponent<Morava.Camera>();
            minimapRect = Minimap?.GetComponent<UIRect>();
            bigMapRect = BigMap?.GetComponent<UIRect>();
            minimapViewRect = MinimapView?.GetComponent<UIRect>();
            bigMapViewRect = BigMapView?.GetComponent<UIRect>();
            northRect = MinimapNorth?.GetComponent<UIRect>();
            bigPlayerRect = BigMapPlayer?.GetComponent<UIRect>();
            bigTitle = BigMapTitle?.GetComponent<UIText>();
            bigHint = BigMapHint?.GetComponent<UIText>();
            for (int i = 1; Scene.FindEntityByName($"Minimap Mark {i}") is Entity mark; i++)
                minimapMarks.Add((mark.GetComponent<UIRect>(), mark.GetComponent<UIImage>()));
            for (int i = 1; Scene.FindEntityByName($"Big Map Mark {i}") is Entity mark; i++)
                bigMapMarks.Add((mark.GetComponent<UIRect>(), mark.GetComponent<UIImage>()));
            if (minimapCamera == null || bigMapCamera == null || minimapRect == null || bigMapRect == null || minimapMarks.Count == 0 || bigMapMarks.Count == 0)
                Log.Warn($"{Name}: parts missing (the cameras, the maps, {minimapMarks.Count} minimap marks, {bigMapMarks.Count} big map marks)");
            Setup(minimapCamera, MinimapRange);
            Setup(bigMapCamera, BigMapRange);
            Show(false, false);
        }

        // Down from CameraHeight over the cut: the near plane through the cut where the camera's middle line meets it, Range
        // meters across the picture (square) there
        void Setup(Morava.Camera? camera, float range)
        {
            if (camera == null)
                return;
            camera.FieldOfView = 2.0f * MathF.Atan(0.5f * range / AxisDistance) * 180.0f / MathF.PI;
            camera.NearClip = AxisDistance;
            camera.FarClip = AxisDistance + Depth;
        }

        static float PitchRadians => Pitch * MathF.PI / 180.0f;

        /// <summary>From a camera to the cut along its middle line.</summary>
        public float AxisDistance => CameraHeight / MathF.Sin(PitchRadians);

        /// <summary>Where a map's camera is for its picture's middle to be at (x, z) on the cut, facing the yaw up.</summary>
        public Vector3 CameraPosition(float x, float z, float yaw)
        {
            float back = CameraHeight / MathF.Tan(PitchRadians); // (it looks this far ahead of under itself)
            return new Vector3(x, CutHeight + CameraHeight, z) - LevelForward(yaw) * back;
        }

        static Vector3 LevelForward(float yaw)
        {
            float a = yaw * MathF.PI / 180.0f;
            return new Vector3(-MathF.Sin(a), 0.0f, -MathF.Cos(a));
        }

        protected override void OnUpdate(float dt)
        {
            if (player == null)
                return;
            // The floor: where the player last stood (not a jump's height)
            if (player.IsGrounded || !hasFloor)
            {
                floorY = player.WorldPosition.Y;
                hasFloor = true;
            }
            if (!PauseMenu.BlocksGame && Input.WasPressed("Map"))
                ToggleBigMap();
            bool hudShown = !player.IsFlying && !player.IsDead;
            bool panel = hud?.ObjectivesPanel?.GetComponent<UIRect>()?.Visible == true;
            bool big = BigMapOpen && hudShown && !PauseMenu.IsPaused && !panel;
            Show(hudShown && !big, big);
            BigMapShown = big;

            Vector3 at = player.WorldPosition;
            float yaw = player.Yaw;
            GatherMarks();
            if (big)
            {
                Vector3 camera = CameraPosition(BigMapCenter.X, BigMapCenter.Y, 0.0f);
                Place(BigMapCamera, camera, 0.0f);
                BigMapMarks = DrawMarks(bigMapMarks, bigMapViewRect, camera, 0.0f, BigMapRange, BigMarkSize, false);
                if (bigPlayerRect != null)
                {
                    bigPlayerRect.Position = ToView(Project(at, camera, 0.0f, BigMapRange), bigMapViewRect);
                    bigPlayerRect.Rotation = -yaw; // (clockwise on the screen; the yaw is positive to the left)
                }
                if (bigTitle != null)
                    bigTitle.Text = $"MAP  ·  FLOOR {FloorNumber}";
                if (bigHint != null)
                    bigHint.Text = $"{Binding("Map")}  CLOSE";
            }
            else
            {
                Vector3 camera = CameraPosition(at.X, at.Z, yaw);
                Place(MinimapCamera, camera, yaw);
                MinimapMarks = DrawMarks(minimapMarks, minimapViewRect, camera, yaw, MinimapRange, MarkSize, true);
                if (northRect != null)
                {
                    // North (-Z) in the turned picture: (sin yaw, cos yaw), on the square's rim
                    float a = yaw * MathF.PI / 180.0f;
                    Vector2 north = new Vector2(MathF.Sin(a), MathF.Cos(a));
                    north /= MathF.Max(MathF.Abs(north.X), MathF.Abs(north.Y));
                    northRect.Position = ToView(north * Edge(minimapViewRect, northRect.Size.Y), minimapViewRect);
                }
            }
        }

        /// <summary>The big map opened, or closed (Map does it; not while dead).</summary>
        public void ToggleBigMap()
        {
            if (player != null && !player.IsDead)
                BigMapOpen = !BigMapOpen;
        }

        /// <summary>1 on the ground floor, 2 a story up..</summary>
        public int FloorNumber => (int)MathF.Floor((floorY + 0.5f) / StoryHeight) + 1;

        void Show(bool minimap, bool big)
        {
            if (minimapRect != null) minimapRect.Visible = minimap;
            if (bigMapRect != null) bigMapRect.Visible = big;
            if (minimapCamera != null) minimapCamera.Enabled = minimap;
            if (bigMapCamera != null) bigMapCamera.Enabled = big;
        }

        static void Place(Entity? camera, Vector3 position, float yaw)
        {
            if (camera == null)
                return;
            camera.Translation = position; // (a root entity)
            camera.EulerAngles = new Vector3(Pitch, yaw, 0.0f); // down, the yaw's forward up in the picture
        }

        void GatherMarks()
        {
            marks.Clear();
            // The objectives first (under the rest): one that is a pickup round the pickup's mark
            foreach (Objective objective in Objectives.All)
                if (!objective.IsComplete)
                    marks.Add(((objective.Target ?? objective).WorldPosition, "Objective", ObjectiveColor, true, objective.Target is Pickup ? 2.0f : 1.0f));
            foreach (Entity entity in MapMarks.All)
            {
                if (entity is Pickup pickup && pickup.IsAvailable)
                    marks.Add((pickup.WorldPosition, pickup.Kind switch { PickupKind.Health => "Cross", PickupKind.Armor => "Shield", _ => "Box" },
                        pickup.Kind switch { PickupKind.Health => HealthColor, PickupKind.Armor => ArmorColor, _ => AmmoColor }, false, 1.0f));
                else if (entity is Dummy dummy)
                    marks.Add((dummy.WorldPosition, "Dot", dummy.IsDown ? DownColor : GuardColor, false, 1.0f));
            }
        }

        // The marks on one map; pinned ones (objectives) stay on the rim when off it (pin: the minimap). Returns how many show
        int DrawMarks(List<(UIRect? Rect, UIImage? Image)> pool, UIRect? view, Vector3 camera, float yaw, float range, float size, bool pin)
        {
            int used = 0;
            foreach (var mark in marks)
            {
                float markSize = size * mark.Scale, edge = Edge(view, markSize);
                Vector2 p = Project(mark.Position, camera, yaw, range);
                float far = MathF.Max(MathF.Abs(p.X), MathF.Abs(p.Y));
                if (far > edge)
                {
                    if (!(pin && mark.Pinned))
                        continue;
                    p *= edge / far;
                }
                if (used >= pool.Count)
                {
                    if (!warned)
                        Log.Warn($"{Name}: more marks than the map's {pool.Count}");
                    warned = true;
                    break;
                }
                var (rect, image) = pool[used++];
                if (rect == null || image == null)
                    continue;
                rect.Visible = true;
                rect.Position = ToView(p, view);
                rect.Size = new Vector2(markSize, markSize);
                Vector4 color = mark.Color;
                if (MathF.Abs(mark.Position.Y - floorY) > OtherFloor)
                    color.W *= OtherFloorAlpha;
                image.Color = color;
                image.Texture = string.Format(MarkTexture, mark.Shape);
            }
            for (int i = used; i < pool.Count; i++)
                if (pool[i].Rect is UIRect rect)
                    rect.Visible = false;
            return used;
        }

        // Where a point is in the picture of a map's camera at Camera, tilted Pitch down toward the yaw: -1 to 1 across (x
        // right, y up), in perspective at the point's own depth
        Vector2 Project(Vector3 point, Vector3 camera, float yaw, float range)
        {
            float a = yaw * MathF.PI / 180.0f, s = MathF.Sin(PitchRadians), c = MathF.Cos(PitchRadians);
            Vector3 level = LevelForward(yaw), right = new Vector3(MathF.Cos(a), 0.0f, -MathF.Sin(a));
            Vector3 view = level * c - Vector3.UnitY * s, up = level * s + Vector3.UnitY * c;
            Vector3 d = point - camera;
            float half = 0.5f * range / AxisDistance * MathF.Max(1.0f, Vector3.Dot(d, view)); // the half width seen at its depth
            return new Vector2(Vector3.Dot(d, right), Vector3.Dot(d, up)) / half;
        }

        // The furthest a mark of this size goes from the picture's middle (-1 to 1) and stays whole in it
        static float Edge(UIRect? view, float size)
        {
            float half = view != null ? 0.5f * view.Size.X : 150.0f;
            return half > size ? 1.0f - 0.5f * size / half : 0.5f;
        }

        // -1 to 1 (y up) into the position of a child centered on the picture (the UI's y down)
        static Vector2 ToView(Vector2 p, UIRect? view)
        {
            float half = view != null ? 0.5f * view.Size.X : 150.0f;
            return new Vector2(p.X * half, -p.Y * half);
        }

        static string Binding(string action)
        {
            try
            {
                return Input.GetBindingText(action, 0);
            }
            catch (ArgumentException)
            {
                return action;
            }
        }
    }

    /// <summary>
    /// What the HUD's map marks (Phase 3.4): the pickups and the training dummies add themselves when created. Read by
    /// polling, the last Play's dropped (as Objectives).
    /// </summary>
    public static class MapMarks
    {
        static readonly List<Entity> list = new();

        public static IReadOnlyList<Entity> All
        {
            get
            {
                list.RemoveAll(e => !e.Exists);
                return list;
            }
        }

        internal static void Add(Entity entity)
        {
            list.RemoveAll(e => !e.Exists);
            if (!list.Contains(entity))
                list.Add(entity);
        }
    }
}
