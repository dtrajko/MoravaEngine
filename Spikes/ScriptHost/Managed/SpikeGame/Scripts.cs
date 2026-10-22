using System;
using System.Numerics;
using Morava.Spike;

// Moves its entity along X by Version units per second: the host sees from the distance which build is loaded
public class Mover : Entity
{
#if GAME_V2
    public const int Version = 2;
#else
    public const int Version = 1;
#endif

    protected override void OnCreate() => Log.Info($"Mover v{Version} created on entity {Id}");

    protected override void OnUpdate(float dt) => Translation += new Vector3(Version * dt, 0, 0);
}

// Throws in every update: the host must survive it and see the error
public class Thrower : Entity
{
    protected override void OnUpdate(float dt) => throw new InvalidOperationException("thrown on purpose");
}

// Leaves itself in an engine list: the game can't be collected after that, and unloading must say so
public class Leaker : Entity
{
    protected override void OnCreate() => Registry.Kept.Add(this);
}
