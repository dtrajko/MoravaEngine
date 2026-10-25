using System;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>
    /// The ball of Roll a Ball: Move pushes it (forward being away from the camera), Jump hops it off the ground. It needs
    /// a Rigid Body (Dynamic) and a sphere Collider: physics rolls it. It counts the pickups it collects (Pickup calls
    /// Collect), says the score in the log, plays a sound for each pickup, each jump and the win, and scatters sparkles
    /// where a pickup was. After the win, or a fall off the playground, the game starts again (Scene.Reload).
    /// </summary>
    public class BallController : Entity
    {
        /// <summary>Meters per second squared that Move gives it, whatever its mass.</summary>
        public float Acceleration = 12.0f;

        /// <summary>Meters per second, up, when it jumps (0: it doesn't).</summary>
        public float JumpSpeed = 5.0f;

        /// <summary>Forward is where this entity looks (none: along -Z).</summary>
        public Entity? Camera;

        /// <summary>The pickups collected (the Inspector shows it while the game runs).</summary>
        public int Score;

        /// <summary>The sounds, relative to the project (empty: none).</summary>
        public string PickupSound = "Assets/Audio/Pickup.wav";
        public string JumpSound = "Assets/Audio/Jump.wav";
        public string WinSound = "Assets/Audio/Win.wav";

        /// <summary>Placed where a pickup was (none: nothing), SparkleCount of them, each flying off its own way.</summary>
        public Prefab? SparklePrefab;
        public int SparkleCount = 4;

        /// <summary>Seconds after the win until the game starts again (0: it doesn't).</summary>
        public float RestartDelay = 5.0f;

        /// <summary>Below this height the ball has fallen off: the game starts again.</summary>
        public float FallHeight = -10.0f;

        Rigidbody body = null!;
        float radius;
        int total;
        double startTime, restartTime = -1;
        readonly Random random = new();

        protected override void OnCreate()
        {
            body = GetComponent<Rigidbody>() ??
                throw new InvalidOperationException($"{Name} needs a Rigid Body (Dynamic) for BallController");
            radius = Scale.Y; // the sphere model's radius is 1
            total = Scene.FindEntitiesWithTag("Pickup").Length;
            startTime = Time.GameTime; // the game time goes on when the scene starts again
            Log.Info($"Roll a Ball: collect the {total} pickups");
        }

        protected override void OnUpdate(float dt)
        {
            if (restartTime >= 0 && Time.GameTime >= restartTime)
            {
                restartTime = -1;
                Scene.Reload();
            }
            else if (WorldPosition.Y < FallHeight)
            {
                Log.Info("The ball fell off: again");
                Scene.Reload();
            }
        }

        protected override void OnFixedUpdate(float dt)
        {
            Vector2 move = Input.GetAxis2D("Move");
            Vector3 forward = Camera != null ? Camera.Forward : -Vector3.UnitZ;
            forward = Vector3.Normalize(new Vector3(forward.X, 0, forward.Z));
            Vector3 right = Vector3.Cross(forward, Vector3.UnitY);
            body.AddForce((right * move.X + forward * move.Y) * Acceleration, ForceMode.Acceleration);

            // On the ground: something just below the ball
            if (JumpSpeed > 0 && Input.WasPressed("Jump") &&
                Physics.Raycast(WorldPosition, -Vector3.UnitY, radius + 0.1f, out _, PhysicsLayers.Solid, this))
            {
                Vector3 velocity = body.LinearVelocity;
                body.LinearVelocity = new Vector3(velocity.X, JumpSpeed, velocity.Z);
                if (JumpSound.Length > 0)
                    Audio.PlayOneShot(JumpSound, WorldPosition, 0.6f);
            }
        }

        /// <summary>A pickup it touched: one more for the score.</summary>
        public void Collect(Entity pickup)
        {
            Score++;
            Log.Info($"Score: {Score} / {total} ({pickup.Name})");
            // From where the pickup was, a little higher each time
            if (PickupSound.Length > 0)
                Audio.PlayOneShot(PickupSound, pickup.WorldPosition, 0.8f, AudioBus.Effects, 1.0f + 0.04f * (Score - 1));
            if (SparklePrefab != null)
            {
                for (int i = 0; i < SparkleCount; i++)
                {
                    // Out around the pickup, and up
                    float angle = (i + (float)random.NextDouble() * 0.5f) * 2.0f * MathF.PI / SparkleCount;
                    var sparkle = (Sparkle)Scene.Instantiate(SparklePrefab, pickup.WorldPosition);
                    sparkle.Velocity = new Vector3(MathF.Cos(angle) * 1.5f, 2.5f + (float)random.NextDouble(), MathF.Sin(angle) * 1.5f);
                }
            }
            if (Score == total)
            {
                Log.Info($"You win! All {total} pickups in {Time.GameTime - startTime:0.0} s" +
                    (RestartDelay > 0 ? $": again in {RestartDelay:0} s" : ""));
                if (WinSound.Length > 0)
                    Audio.PlayOneShot(WinSound);
                if (RestartDelay > 0)
                    restartTime = Time.GameTime + RestartDelay;
            }
        }
    }
}
