using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.CompilerServices;


namespace CGL
{
    public class GameConsole
    {
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void clearBuffer();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void present();

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void putChar(uint x, uint y, char c);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static void putString(uint x, uint y, string str, bool wrap = true);

    }
}
