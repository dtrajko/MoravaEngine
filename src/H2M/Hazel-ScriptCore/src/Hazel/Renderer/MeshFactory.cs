using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace Hazel
{
    public static class MeshFactory
    {

        public static Mesh CreatePlane(float width, float height)
        {
            return new Mesh(CreatePlane_Native(width, height));
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern IntPtr CreatePlane_Native(float width, float height);

    }
}
