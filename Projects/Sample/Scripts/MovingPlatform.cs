using System;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>
    /// A platform that goes from where it is placed to Offset away and back, waiting at each end: a ferry sideways, a lift
    /// up and down. It needs a Rigid Body (Kinematic): it moves in the fixed step, and carries a character standing on it
    /// (and boxes) along. With an Audio Source (looping), the sound plays while it moves; StopSound at each end.
    /// </summary>
    public class MovingPlatform : Entity
    {
        /// <summary>Where the far end is, from the start, in meters.</summary>
        public Vector3 Offset = new Vector3(6, 0, 0);

        /// <summary>Seconds from one end to the other.</summary>
        public float TravelSeconds = 3.0f;

        /// <summary>Seconds it waits at each end.</summary>
        public float WaitSeconds = 1.5f;

        /// <summary>Played where it stops (relative to the project; empty: none).</summary>
        public string StopSound = "";
        public float StopVolume = 0.9f;

        Vector3 start;
        float time;
        bool moving;
        AudioSource? motor;

        protected override void OnCreate()
        {
            start = Translation;
            motor = GetComponent<AudioSource>();
        }

        protected override void OnFixedUpdate(float dt)
        {
            // One round: wait, travel out, wait, travel back; eased in and out so a character doesn't slip
            float travel = MathF.Max(TravelSeconds, 0.1f), wait = MathF.Max(WaitSeconds, 0.0f);
            float round = 2.0f * (travel + wait);
            time = (time + dt) % round;
            bool travelling = (time >= wait && time < wait + travel) || time >= 2.0f * wait + travel;
            float t = time < wait ? 0.0f
                : time < wait + travel ? (time - wait) / travel
                : time < 2.0f * wait + travel ? 1.0f
                : 1.0f - (time - 2.0f * wait - travel) / travel;
            float eased = t * t * (3.0f - 2.0f * t);
            Translation = start + Offset * eased;

            if (travelling != moving)
            {
                moving = travelling;
                if (moving)
                {
                    motor?.Play();
                }
                else
                {
                    motor?.Stop();
                    if (StopSound.Length > 0)
                        Audio.PlayOneShot(StopSound, WorldPosition, StopVolume);
                }
            }
        }
    }
}
