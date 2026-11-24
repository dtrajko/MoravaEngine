using Morava;

namespace Outpost
{
    /// <summary>
    /// Where the player starts again after dying (docs/games/Shooter_Plan.html, Phase 3.1): the entity's position (its
    /// feet: put it on the floor) facing its Y rotation. It needs a Collider with Is Trigger on: the player passing through
    /// it makes it the respawn (the last one touched; none yet: where the player started). Phase 7 adds the mission's
    /// state to it.
    /// </summary>
    public class Checkpoint : Entity
    {
        /// <summary>The HUD's word when it is reached (empty: none shown).</summary>
        public string Message = "CHECKPOINT";

        protected override void OnTriggerEnter(Entity other)
        {
            if (other is Player player)
                player.SetCheckpoint(this);
        }
    }
}
