using System;
using System.Numerics;

namespace Morava
{
    /// <summary>
    /// An entity of the running game's scene, and the base class of a game's scripts. A script is attached to one entity:
    /// the engine creates it when the game starts and calls its methods while the game runs. Other entities (found by
    /// name or tag, hit by a ray, touched) are Entity objects too, or the entity's script when it has one (so
    /// <c>other is Coin coin</c> works): two of them are equal when they are the same entity.
    /// </summary>
    /// <remarks>
    /// Translation, Rotation and Scale are relative to the entity's parent (to the world without one).
    /// A script's public fields of type bool, int, float, double, string, Vector2, Vector3, Vector4, Entity (or a script
    /// class) and enums show in the editor's Inspector: a value set there is given to the script before its
    /// OnCreate, and the others keep the value the field's initializer gives them.
    /// </remarks>
    public unsafe class Entity : IEquatable<Entity>
    {
        /// <summary>A script: the engine sets its entity.</summary>
        protected Entity() { }

        internal Entity(ulong id) => Id = id;

        /// <summary>The engine's ID of the entity.</summary>
        public ulong Id { get; internal set; }

        /// <summary>The entity still exists: not destroyed.</summary>
        public bool Exists => Interop.Game->Entity_Exists(Id) != 0;

        /// <summary>The entity's name, as the Scene Hierarchy shows it.</summary>
        public string Name
        {
            get
            {
                char* buffer = stackalloc char[256];
                int length = Interop.Game->Entity_GetName(Id, buffer, 256);
                if (length == -2) throw NoEntity();
                if (length >= 0) return new string(buffer, 0, length);
                char[] large = new char[4096];
                fixed (char* chars = large)
                    length = Interop.Game->Entity_GetName(Id, chars, large.Length);
                return length >= 0 ? new string(large, 0, length) : "";
            }
            set { fixed (char* chars = value) Check(Interop.Game->Entity_SetName(Id, chars, value.Length)); }
        }

        /// <summary>The position, in meters.</summary>
        public Vector3 Translation
        {
            get { Vector3 value; Check(Interop.Game->Entity_GetTranslation(Id, &value)); return value; }
            set => Check(Interop.Game->Entity_SetTranslation(Id, &value));
        }

        /// <summary>The rotation.</summary>
        public Quaternion Rotation
        {
            get { Quaternion value; Check(Interop.Game->Entity_GetRotation(Id, &value)); return value; }
            set => Check(Interop.Game->Entity_SetRotation(Id, &value));
        }

        /// <summary>The rotation as the Inspector shows it: degrees around X, Y and Z (Y turns left and right).</summary>
        public Vector3 EulerAngles
        {
            get { Vector3 value; Check(Interop.Game->Entity_GetEulerAngles(Id, &value)); return value; }
            set => Check(Interop.Game->Entity_SetEulerAngles(Id, &value));
        }

        /// <summary>The size: 1 is the model's own. A physics body keeps the size it had when the game started.</summary>
        public Vector3 Scale
        {
            get { Vector3 value; Check(Interop.Game->Entity_GetScale(Id, &value)); return value; }
            set => Check(Interop.Game->Entity_SetScale(Id, &value));
        }

        /// <summary>Where the entity is in the world, with its parents' transforms.</summary>
        public Matrix4x4 WorldTransform
        {
            get { Matrix4x4 value; Check(Interop.Game->Entity_GetWorldTransform(Id, &value)); return value; }
        }

        /// <summary>The position in the world.</summary>
        public Vector3 WorldPosition => WorldTransform.Translation;

        /// <summary>The direction the entity faces in the world (its -Z axis), of length 1.</summary>
        public Vector3 Forward { get { Matrix4x4 m = WorldTransform; return Direction(-m.M31, -m.M32, -m.M33); } }

        /// <summary>Its right in the world (its X axis), of length 1.</summary>
        public Vector3 Right { get { Matrix4x4 m = WorldTransform; return Direction(m.M11, m.M12, m.M13); } }

        /// <summary>Its up in the world (its Y axis), of length 1.</summary>
        public Vector3 Up { get { Matrix4x4 m = WorldTransform; return Direction(m.M21, m.M22, m.M23); } }

        static Vector3 Direction(float x, float y, float z)
        {
            var v = new Vector3(x, y, z);
            return v.LengthSquared() > 0 ? Vector3.Normalize(v) : v;
        }

        /// <summary>The entity has this tag (Inspector: Tags).</summary>
        public bool HasTag(string tag)
        {
            int result;
            fixed (char* chars = tag) result = Interop.Game->Entity_HasTag(Id, chars, tag.Length);
            if (result < 0) throw NoEntity();
            return result != 0;
        }

        /// <summary>Gives the entity a tag, for the rest of the game.</summary>
        public void AddTag(string tag) { fixed (char* chars = tag) Check(Interop.Game->Entity_AddTag(Id, chars, tag.Length)); }

        /// <summary>Takes a tag away.</summary>
        public void RemoveTag(string tag) { fixed (char* chars = tag) Check(Interop.Game->Entity_RemoveTag(Id, chars, tag.Length)); }

        /// <summary>
        /// Removes the entity (and its children) from the game, at the end of this update: until then it is still there.
        /// </summary>
        public void Destroy() => Check(Interop.Game->Entity_Destroy(Id));

        /// <summary>The entity has the component: a dynamic Rigidbody, a CharacterController, a Collider.</summary>
        public bool HasComponent<T>() where T : Component
        {
            int result = Interop.Game->Entity_HasComponent(Id, (int)Component.KindOf(typeof(T)));
            if (result < 0) throw NoEntity();
            return result != 0;
        }

        /// <summary>The entity's component, null when it has none. Ask once (in OnCreate) and keep it.</summary>
        public T? GetComponent<T>() where T : Component, new() =>
            HasComponent<T>() ? new T { Entity = this } : null;

        /// <summary>Once, when the game starts, before the first update.</summary>
        protected internal virtual void OnCreate() { }

        /// <summary>Once per frame, with the frame's time in seconds.</summary>
        protected internal virtual void OnUpdate(float dt) { }

        /// <summary>On each fixed step of the game loop (60 per second), with the step in seconds: for physics.</summary>
        protected internal virtual void OnFixedUpdate(float dt) { }

        /// <summary>The entity's body started touching another one.</summary>
        protected internal virtual void OnCollisionEnter(Collision collision) { }

        /// <summary>The entity's body stopped touching another one.</summary>
        protected internal virtual void OnCollisionExit(Entity other) { }

        /// <summary>Something entered this trigger, or this entity entered a trigger (other is the trigger then).</summary>
        protected internal virtual void OnTriggerEnter(Entity other) { }

        /// <summary>Something left this trigger, or this entity left one.</summary>
        protected internal virtual void OnTriggerExit(Entity other) { }

        void Check(int result)
        {
            if (result == 0) throw NoEntity();
        }

        InvalidOperationException NoEntity() =>
            new InvalidOperationException($"Entity {Id} doesn't exist: it was destroyed, or it isn't in the running game");

        /// <summary>The same entity.</summary>
        public bool Equals(Entity? other) => other is not null && other.Id == Id;

        /// <inheritdoc/>
        public override bool Equals(object? other) => other is Entity entity && Equals(entity);

        /// <inheritdoc/>
        public override int GetHashCode() => Id.GetHashCode();

        /// <summary>The same entity (or both null).</summary>
        public static bool operator ==(Entity? a, Entity? b) => a is null ? b is null : a.Equals(b);

        /// <summary>Not the same entity.</summary>
        public static bool operator !=(Entity? a, Entity? b) => !(a == b);

        /// <summary>Its name and ID, for the log.</summary>
        public override string ToString()
        {
            if (Interop.Native == null || Interop.Native->Entity_Exists == null || Interop.Native->Entity_Exists(Id) == 0)
                return $"Entity {Id}";
            return $"{Name} ({Id})";
        }
    }

    /// <summary>Two bodies that started touching (Entity.OnCollisionEnter).</summary>
    public sealed class Collision
    {
        internal Collision(Entity other, Vector3 point, Vector3 normal) { Other = other; Point = point; Normal = normal; }

        /// <summary>The entity touched.</summary>
        public Entity Other { get; }

        /// <summary>Where they first touched, in the world.</summary>
        public Vector3 Point { get; }

        /// <summary>The direction from this entity towards the other one there, of length 1.</summary>
        public Vector3 Normal { get; }
    }
}
