using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using CGL;


namespace HangmanScript
{

    public class HangGame : IGame
    {
        private string wordToGuess;

        public HangGame(string word)
        {
            this.wordToGuess = word;
        }

        public void keyEvent(char key)
        {
            // throw new NotImplementedException();
            return;
        }

        public bool tick()
        {
            // throw new NotImplementedException();

            // Print word
            CGL.GameConsole.clearBuffer();
            CGL.GameConsole.putString(0, 0, "The word is: " + this.wordToGuess);
            CGL.GameConsole.present();

            // Sleep and return
            Thread.Sleep(4000);
            return false;
        }
    }
}
