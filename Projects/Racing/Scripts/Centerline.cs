using System;
using System.Collections.Generic;
using System.Numerics;
using System.Text.Json;
using Morava;

namespace Racing
{
    /// <summary>
    /// A track's center line, as Tools/TrackBuilder writes it (Assets/Tracks/&lt;Name&gt;.centerline.json): a point every
    /// 2 m with its distance along the track, its right, up and width, and the start. Read by the test driver (to follow
    /// it) and the race (checkpoints, respawn, wrong way, fallen off). Positions are in the world: the file's are relative
    /// to the track entity, whose position Load adds.
    /// </summary>
    public sealed class Centerline
    {
        public struct Point
        {
            public float S;            // meters along the track from the start line
            public Vector3 Position;   // on the road's surface, in the middle
            public Vector3 Right, Up;  // the road's (banked) right and up
            public float Width;        // from wall to wall
        }

        public string Name { get; private set; } = "";
        /// <summary>The track's version: 8 hex digits of a hash of the center line's file (its line ends left out). A rebuilt
        /// track with another shape has another one, and so records of its own (RecordKey).</summary>
        public string Version { get; private set; } = "";
        /// <summary>The save data key of one of this version of the track's records: "Name-Version.name".</summary>
        public string RecordKey(string name) => $"{Name}-{Version}.{name}";
        /// <summary>Meters around the track (the start line is at 0).</summary>
        public float Length { get; private set; }
        public IReadOnlyList<Point> Points => points;
        public int Count => points.Count;
        /// <summary>Where the car starts (on the road, behind the line) and its direction.</summary>
        public Vector3 StartPosition { get; private set; }
        public Vector3 StartForward { get; private set; }

        readonly List<Point> points = new();

        Centerline() { }

        /// <summary>The center line in this file of the project (null when it isn't there), for a track entity at trackPosition.</summary>
        public static Centerline? Load(string file, Vector3 trackPosition)
        {
            string? text = Assets.ReadText(file);
            if (text == null)
                return null;
            using JsonDocument document = JsonDocument.Parse(text);
            JsonElement root = document.RootElement;
            var line = new Centerline
            {
                Name = root.GetProperty("Name").GetString() ?? "",
                Length = root.GetProperty("Length").GetSingle(),
                Version = Hash(text),
            };
            foreach (JsonElement point in root.GetProperty("Points").EnumerateArray())
            {
                line.points.Add(new Point
                {
                    S = point.GetProperty("s").GetSingle(),
                    Position = ReadVector(point.GetProperty("Position")) + trackPosition,
                    Right = ReadVector(point.GetProperty("Right")),
                    Up = ReadVector(point.GetProperty("Up")),
                    Width = point.GetProperty("Width").GetSingle(),
                });
            }
            JsonElement start = root.GetProperty("Start");
            line.StartPosition = ReadVector(start.GetProperty("Position")) + trackPosition;
            line.StartForward = Vector3.Normalize(ReadVector(start.GetProperty("Forward")));
            return line;
        }

        // FNV-1a (32 bits) of the text without its '\r's: the same on every platform, whatever Git did to the line ends
        static string Hash(string text)
        {
            uint hash = 2166136261;
            foreach (char c in text)
            {
                if (c == '\r')
                    continue;
                hash = (hash ^ c) * 16777619;
            }
            return hash.ToString("x8");
        }

        static Vector3 ReadVector(JsonElement e) => new(e[0].GetSingle(), e[1].GetSingle(), e[2].GetSingle());

        /// <summary>The index after i, around the track.</summary>
        public int Next(int i) => (i + 1) % points.Count;

        /// <summary>The track's direction at point i (towards the next one), of length 1.</summary>
        public Vector3 Direction(int i) => Vector3.Normalize(points[Next(i)].Position - points[i].Position);

        /// <summary>The point this many meters ahead of point i along the line.</summary>
        public int Ahead(int i, float meters)
        {
            float s = 0.0f;
            while (s < meters)
            {
                int j = Next(i);
                s += Vector3.Distance(points[i].Position, points[j].Position);
                i = j;
            }
            return i;
        }

        /// <summary>
        /// The middle of the road s meters along the track (around it: any s), between the two points either side, and the
        /// road's right there (level: without the bank). For what moves along the track smoothly (the menu's camera).
        /// </summary>
        public Vector3 PositionAt(float s, out Vector3 right)
        {
            s = ((s % Length) + Length) % Length;
            int low = 0, high = points.Count - 1;
            while (low < high) // the last point at or before s
            {
                int middle = (low + high + 1) / 2;
                if (points[middle].S <= s) low = middle; else high = middle - 1;
            }
            int next = Next(low);
            float span = next > low ? points[next].S - points[low].S : Length - points[low].S;
            float t = span > 1e-4f ? Math.Clamp((s - points[low].S) / span, 0.0f, 1.0f) : 0.0f;
            Vector3 forward = points[next].Position - points[low].Position;
            Vector3 level = Vector3.Cross(new Vector3(forward.X, 0.0f, forward.Z), Vector3.UnitY);
            right = level.LengthSquared() > 1e-8f ? Vector3.Normalize(level) : Vector3.UnitX;
            return Vector3.Lerp(points[low].Position, points[next].Position, t);
        }

        /// <summary>The point nearest to s meters along the track.</summary>
        public int IndexAt(float s)
        {
            s = ((s % Length) + Length) % Length;
            int best = 0;
            for (int i = 1; i < points.Count; i++)
                if (MathF.Abs(points[i].S - s) < MathF.Abs(points[best].S - s))
                    best = i;
            return best;
        }

        /// <summary>
        /// The point nearest to position: searched around hint (the last one found, a step earlier), or along the whole
        /// line when that is far off (after a respawn, or with no hint: -1).
        /// </summary>
        public int Nearest(Vector3 position, int hint)
        {
            int n = points.Count;
            int found = -1;
            float best = float.MaxValue;
            if (hint >= 0)
            {
                for (int k = -10; k <= 40; k++)
                {
                    int i = ((hint + k) % n + n) % n;
                    float d = Vector3.DistanceSquared(position, points[i].Position);
                    if (d < best)
                    {
                        best = d;
                        found = i;
                    }
                }
                if (best < 30.0f * 30.0f)
                    return found;
            }
            for (int i = 0; i < n; i++)
            {
                float d = Vector3.DistanceSquared(position, points[i].Position);
                if (d < best)
                {
                    best = d;
                    found = i;
                }
            }
            return found;
        }

        /// <summary>A rotation facing along the track at point i, level with its road (banked with it).</summary>
        public Quaternion RotationAt(int i) => Facing(Direction(i), points[i].Up);

        /// <summary>A rotation whose forward (-Z) is forward and whose up is as near to up as it can be.</summary>
        public static Quaternion Facing(Vector3 forward, Vector3 up)
        {
            Vector3 right = Vector3.Normalize(Vector3.Cross(forward, up));
            Vector3 trueUp = Vector3.Cross(right, forward);
            var m = new Matrix4x4(
                right.X, right.Y, right.Z, 0.0f,
                trueUp.X, trueUp.Y, trueUp.Z, 0.0f,
                -forward.X, -forward.Y, -forward.Z, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f);
            return Quaternion.CreateFromRotationMatrix(m);
        }
    }
}
