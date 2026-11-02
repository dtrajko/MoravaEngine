using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace Hazel
{
    public class Material
    {
        public void Set(string uniform, float value)
        {
            SetFloat_Native(m_UnmanagedInstance, uniform, value);
        }

        public void Set(string uniform, Texture2D texture)
        {
            SetTexture_Native(m_UnmanagedInstance, uniform, texture.m_UnmanagedInstance);
        }

        public void SetTexture(string uniform, Texture2D texture)
        {
            SetTexture_Native(m_UnmanagedInstance, uniform, texture.m_UnmanagedInstance);
        }

        internal Material(IntPtr unmanagedInstance)
        {
            m_UnmanagedInstance = unmanagedInstance;
        }

        ~Material()
        {
            Destructor_Native(m_UnmanagedInstance);
        }

        internal IntPtr m_UnmanagedInstance;

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Destructor_Native(IntPtr unmanagedInstance);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetFloat_Native(IntPtr unmanagedInstance, string uniform, float value);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetTexture_Native(IntPtr unmanagedInstance, string uniform, IntPtr texture);

    }

    public class MaterialInstance
    {
        public void Set(string uniform, float value)
        {
            SetFloat_Native(m_UnmanagedInstance, uniform, value);
        }

        public void Set(string uniform, Texture2D texture)
        {
            SetTexture_Native(m_UnmanagedInstance, uniform, texture.m_UnmanagedInstance);
        }

        public void Set(string uniform, Vector3 value)
        {
            SetVector3_Native(m_UnmanagedInstance, uniform, ref value);
        }

        public void Set(string uniform, Vector4 value)
        {
            SetVector4_Native(m_UnmanagedInstance, uniform, ref value);
        }

        public void SetTexture(string uniform, Texture2D texture)
        {
            SetTexture_Native(m_UnmanagedInstance, uniform, texture.m_UnmanagedInstance);
        }

        internal MaterialInstance(IntPtr unmanagedInstance)
        {
            m_UnmanagedInstance = unmanagedInstance;
        }

        ~MaterialInstance()
        {
            Destructor_Native(m_UnmanagedInstance);
        }

        internal IntPtr m_UnmanagedInstance;

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Destructor_Native(IntPtr unmanagedInstance);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetFloat_Native(IntPtr unmanagedInstance, string uniform, float value);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetVector3_Native(IntPtr unmanagedInstance, string uniform, ref Vector3 value);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetVector4_Native(IntPtr unmanagedInstance, string uniform, ref Vector4 value);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void SetTexture_Native(IntPtr unmanagedInstance, string uniform, IntPtr texture);
    }
}
