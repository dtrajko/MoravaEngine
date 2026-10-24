using System;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>
    /// Something to collect: it turns where it is, and when the ball rolls into it, it counts for the ball's score and is
    /// gone. The entity needs a Collider with Is Trigger on, and the tag Pickup (the ball counts the pickups by it).
    /// </summary>
    public class Pickup : Entity
    {
        /// <summary>Degrees per second around the world's up axis.</summary>
        public float DegreesPerSecond = 90.0f;

        protected override void OnUpdate(float dt)
        {
            Quaternion turn = Quaternion.CreateFromAxisAngle(Vector3.UnitY, DegreesPerSecond * dt * MathF.PI / 180.0f);
            Rotation = Quaternion.Normalize(turn * Rotation);
        }

        protected override void OnTriggerEnter(Entity other)
        {
            if (other is BallController ball)
            {
                ball.Collect(this);
                Destroy();
            }
        }
    }
}
