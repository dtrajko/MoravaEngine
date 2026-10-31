using System;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>
    /// An elevator platform going between three heights: it waits WaitSeconds at Bottom, rises to Middle, waits, rises to
    /// Top, waits, goes down to Middle, waits, down to Bottom, and again. It speeds up and slows down smoothly
    /// (Acceleration) and doesn't go faster than Speed. The entity needs a kinematic Rigidbody, so whatever stands on it
    /// rides along, and an Audio Source with the motor's loop (Loop on, Play On Start off): it plays while the platform
    /// moves, louder and higher with its speed. StopSound plays where it stops.
    /// </summary>
    public class ElevatorMovement : Entity
    {
        /// <summary>Heights of the platform's center, in meters.</summary>
        public float Bottom = 1.2f;
        public float Middle = 8.8f;
        public float Top = 16.4f;

        /// <summary>Seconds it stays at each stop.</summary>
        public float WaitSeconds = 10.0f;

        /// <summary>Its top speed, meters per second.</summary>
        public float Speed = 1.0f;

        /// <summary>How fast it speeds up and slows down, meters per second per second.</summary>
        public float Acceleration = 0.5f;

        /// <summary>Played where it stops (a sound file, relative to the project).</summary>
        public string StopSound = "Assets/Audio/Elevator_Clank.wav";

        /// <summary>The motor's volume at full speed, and the stop sound's.</summary>
        public float MotorVolume = 0.6f;
        public float StopVolume = 0.9f;

        AudioSource? motor;
        int stop;             // the stop it is at or going to: Bottom, Middle, Top, Middle, then Bottom again
        bool moving;
        float waited;
        float velocity;       // meters per second along the way, never negative

        float StopHeight(int index) => (index % 4) switch { 0 => Bottom, 1 => Middle, 2 => Top, _ => Middle };

        protected override void OnCreate()
        {
            motor = GetComponent<AudioSource>();
            Vector3 position = Translation;
            Translation = new Vector3(position.X, Bottom, position.Z);
        }

        // In the fixed step: the kinematic body follows the translation in the same step, and carries the character
        protected override void OnFixedUpdate(float dt)
        {
            if (!moving)
            {
                waited += dt;
                if (waited < WaitSeconds)
                    return;
                waited = 0.0f;
                stop = (stop + 1) % 4;
                moving = true;
                velocity = 0.0f;
                motor?.Play();
            }

            Vector3 position = Translation;
            float target = StopHeight(stop);
            float remaining = MathF.Abs(target - position.Y);
            // Faster up to Speed, slower when the stop is near: as fast as it can still brake to it
            float braking = MathF.Sqrt(2.0f * Acceleration * remaining);
            velocity = MathF.Min(MathF.Min(velocity + Acceleration * dt, Speed), MathF.Max(braking, 0.05f));
            float step = velocity * dt;
            if (step >= remaining)
            {
                Translation = new Vector3(position.X, target, position.Z);
                Arrive();
                return;
            }
            Translation = new Vector3(position.X, position.Y + MathF.Sign(target - position.Y) * step, position.Z);

            if (motor != null)
            {
                float speed = Speed > 0.0f ? velocity / Speed : 1.0f;
                motor.Volume = MotorVolume * (0.35f + 0.65f * speed);
                motor.Pitch = 0.8f + 0.2f * speed;
            }
        }

        void Arrive()
        {
            moving = false;
            velocity = 0.0f;
            motor?.Stop();
            if (StopSound.Length > 0)
                Audio.PlayOneShot(StopSound, WorldPosition, StopVolume);
        }
    }
}
