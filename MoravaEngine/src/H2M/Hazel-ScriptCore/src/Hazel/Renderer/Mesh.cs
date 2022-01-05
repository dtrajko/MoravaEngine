using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace Hazel
{
    public class Mesh
    {
        public Mesh(string filepath)
        {
            m_UnmanagedInstance = Constructor_Native(filepath);
        }

        internal Mesh(IntPtr unmanagedInstance)
        {
            m_UnmanagedInstance = unmanagedInstance;
        }

        ~Mesh()
        {
            Destructor_Native(m_UnmanagedInstance);
        }

        public Material BaseMaterial
        { 
            get
            {
                return new Material(GetMaterial_Native(m_UnmanagedInstance));
            }
        }

        public MaterialInstance GetMaterial(int index)
        {
            return new MaterialInstance(GetMaterialByIndex_Native(m_UnmanagedInstance, index));
        }

        public int GetMaterialCount()
        {
            return GetMaterialCount_Native(m_UnmanagedInstance);
        }

        internal IntPtr m_UnmanagedInstance;

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern IntPtr Constructor_Native(string filepath);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void Destructor_Native(IntPtr unmanagedInstance);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern IntPtr GetMaterial_Native(IntPtr unmanagedInstance);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern IntPtr GetMaterialByIndex_Native(IntPtr unmanagedInstance, int index);
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern int GetMaterialCount_Native(IntPtr unmanagedInstance);

    }
}
