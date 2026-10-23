namespace Morava
{
    /// <summary>
    /// The base class of a game's scripts. A script is attached to one of the scene's entities; the engine creates it when
    /// the game starts and calls its methods while the game runs.
    /// </summary>
    public abstract class Entity
    {
        /// <summary>The engine's ID of the entity this script belongs to.</summary>
        public ulong Id { get; internal set; }

        /// <summary>Once, when the game starts, before the first update.</summary>
        protected internal virtual void OnCreate() { }

        /// <summary>Once per frame, with the frame's time in seconds.</summary>
        protected internal virtual void OnUpdate(float dt) { }

        /// <summary>On each fixed step of the game loop (60 per second), with the step in seconds: for physics.</summary>
        protected internal virtual void OnFixedUpdate(float dt) { }
    }
}
