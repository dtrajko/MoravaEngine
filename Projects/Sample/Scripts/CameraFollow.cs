using System;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>
    /// A camera that follows its target: it keeps the offset it starts with (where it is in the scene, from the target),
    /// and catches up smoothly. It doesn't turn: the view keeps the camera's rotation.
    /// </summary>
    public class CameraFollow : Entity
    {
        /// <summary>The entity followed.</summary>
        public Entity? Target;

        /// <summary>How fast it catches up: higher is tighter (0: it stays exactly at the offset).</summary>
        public float Smoothing = 6.0f;

        Vector3 offset;

        protected override void OnCreate()
        {
            if (Target != null)
                offset = Translation - Target.Translation;
        }

        protected override void OnUpdate(float dt)
        {
            if (Target == null || !Target.Exists)
                return;
            Vector3 wanted = Target.Translation + offset;
            Translation = Smoothing > 0 ? Vector3.Lerp(Translation, wanted, 1.0f - MathF.Exp(-Smoothing * dt)) : wanted;
        }
    }
}
