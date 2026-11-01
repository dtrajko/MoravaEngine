using Morava;

namespace Sample
{
    /// <summary>
    /// Where the player starts again after a fall: the entity's position (put it on the platform's top). It needs a
    /// Collider with Is Trigger on, above the platform: the player passing through it makes it the level's checkpoint.
    /// </summary>
    public class Checkpoint : Entity
    {
        protected override void OnTriggerEnter(Entity other)
        {
            if (other.HasTag("Player"))
                CollectorLevel.Current?.SetCheckpoint(this);
        }
    }
}
