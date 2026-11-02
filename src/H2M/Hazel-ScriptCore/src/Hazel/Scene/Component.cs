using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace Hazel
{
    public abstract class Component
    {
        public Entity Entity { get; set; }
    }

    public class TagComponent : Component
    {
        public string Tag
        {
            get
            {
                return GetTag_Native(Entity.ID);
            }
            set
            {
                SetTag_Native(value);
            }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern string GetTag_Native(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetTag_Native(string tag);

    }

    public class TransformComponent : Component
    {
        public Matrix4 Transform
        {
            get
            {
                Matrix4 result;
                GetTransform_Native(Entity.ID, out result);
                return result;
            }
            set
            {
                SetTransform_Native(Entity.ID, ref value);
            }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void GetTransform_Native(ulong entityID, out Matrix4 result);

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetTransform_Native(ulong entityID, ref Matrix4 result);

    }

    public class MeshComponent : Component
    {
        public Mesh Mesh
        {
            get
            {
                Mesh result = new Mesh(GetMesh_Native(Entity.ID));
                return result;
            }
            set
            {
                IntPtr ptr = value == null ? IntPtr.Zero : value.m_UnmanagedInstance;
                SetMesh_Native(Entity.ID, ptr);
            }
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern IntPtr GetMesh_Native(ulong entityID);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetMesh_Native(ulong entityID, IntPtr unmanagedInstance);

    }

    public class CameraComponent : Component
    {
       // TODO
    }

    public class ScriptComponent : Component
    {
        // TODO
    }

    public class SpriteRendererComponent : Component
    {
        // TODO
    }

    // TODO
    public class RigidBody2DComponent : Component
    {
        public void ApplyLinearImpulse(Vector2 impulse, Vector2 offset, bool wake)
        {
            ApplyLinearImpulse_Native(Entity.ID, ref impulse, ref offset, wake);
        }

        public Vector2 GetLinearVelocity()
        {
            GetLinearVelocity_Native(Entity.ID, out Vector2 velocity);
            return velocity;
        }

        public void SetLinearVelocity(Vector2 velocity)
        {
            SetLinearVelocity_Native(Entity.ID, ref velocity);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void ApplyLinearImpulse_Native(ulong entityID, ref Vector2 impulse, ref Vector2 offset, bool wake);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetLinearVelocity_Native(ulong entityID, out Vector2 velocity);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetLinearVelocity_Native(ulong entityID, ref Vector2 velocity);
    }

    public class BoxCollider2DComponent : Component
    {
    }


}
