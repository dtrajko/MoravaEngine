using System;
using System.Collections.Generic;
using System.Text;

namespace HangmanScript
{
    public interface IGame
    {
        bool tick();

        void keyEvent(char key);
    }
}
