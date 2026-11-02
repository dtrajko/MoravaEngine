using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.CompilerServices;


namespace CGL
{
    public class RandomWord
    {
        /// <summary>
        /// Get a random word
        /// </summary>
        /// <returns>Random word</returns>
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        public extern static string getRandomWord();
    }
}
