using System;
using System.Collections.Generic;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>
    /// A security monitor: the room's security cameras (the entities tagged CameraTag) take turns on the monitor's render
    /// texture, each for SecondsPerCamera (Camera.Target moves from one to the next, in the order of their names). The
    /// texture is the one a camera has as its Target in the scene. The cameras draw only while the player's camera
    /// (Viewer) looks at the screen (Camera.Enabled): a monitor nobody watches keeps its last picture and costs nothing
    /// (the first camera draws once at the start, so the screen never shows the render texture's placeholder). The screen
    /// is the entity's quad, facing along its X axis.
    /// </summary>
    public class SecurityMonitor : Entity
    {
        /// <summary>The camera the game is seen through: the cameras draw while it looks at the screen.</summary>
        public Entity? Viewer;

        /// <summary>The tag of the security cameras.</summary>
        public string CameraTag = "SecurityCamera";

        /// <summary>How long each camera is shown.</summary>
        public float SecondsPerCamera = 4.0f;

        /// <summary>Degrees from the viewer's view direction within which the screen counts as looked at.</summary>
        public float ViewAngle = 50.0f;

        /// <summary>Meters: farther away, the screen isn't watched.</summary>
        public float MaxDistance = 30.0f;

        readonly List<Camera> cameras = new List<Camera>();
        string texture = "";
        int current;
        float shown;
        bool started; // the first frame: the first camera draws its picture, watched or not

        protected override void OnCreate()
        {
            Entity[] found = Scene.FindEntitiesWithTag(CameraTag);
            Array.Sort(found, (a, b) => string.CompareOrdinal(a.Name, b.Name));
            foreach (Entity entity in found)
            {
                Camera? camera = entity.GetComponent<Camera>();
                if (camera == null)
                    continue;
                cameras.Add(camera);
                if (texture.Length == 0)
                    texture = camera.Target;
            }
            if (texture.Length == 0)
            {
                Log.Warn($"{Name}: no camera tagged {CameraTag} has a render texture as its Target");
                return;
            }
            // One camera at a time on the texture: the first
            for (int i = 0; i < cameras.Count; i++)
                cameras[i].Target = i == 0 ? texture : "";
        }

        protected override void OnUpdate(float dt)
        {
            if (cameras.Count == 0 || texture.Length == 0)
                return;

            bool watched = IsWatched();
            if (watched && cameras.Count > 1)
            {
                // The turns go on only while someone looks: the next camera when this one's time is up
                shown += dt;
                if (shown >= SecondsPerCamera)
                {
                    shown = 0.0f;
                    cameras[current].Target = "";
                    current = (current + 1) % cameras.Count;
                    cameras[current].Target = texture;
                }
            }
            if (!started)
            {
                started = true;
                watched = true;
            }
            if (cameras[current].Enabled != watched)
                cameras[current].Enabled = watched;
        }

        // The viewer is in front of the screen, near enough, and looks towards it
        bool IsWatched()
        {
            if (Viewer == null || !Viewer.Exists)
                return true;
            Vector3 eye = Viewer.WorldPosition;
            Vector3 toScreen = WorldPosition - eye;
            float distance = toScreen.Length();
            if (distance > MaxDistance || Vector3.Dot(-toScreen, Right) <= 0.0f)
                return false;
            if (distance < 1e-3f)
                return true;
            float cosine = Vector3.Dot(ViewDirection(Viewer.EulerAngles), toScreen / distance);
            return cosine >= MathF.Cos(ViewAngle * MathF.PI / 180.0f);
        }

        // Where a camera looks, from its rotation as the renderer reads it: X down, Y to the left (as OppositeCamera's)
        static Vector3 ViewDirection(Vector3 rotation)
        {
            float pitch = rotation.X * MathF.PI / 180.0f, yaw = rotation.Y * MathF.PI / 180.0f;
            return new Vector3(-MathF.Sin(yaw) * MathF.Cos(pitch), -MathF.Sin(pitch), -MathF.Cos(yaw) * MathF.Cos(pitch));
        }
    }
}
