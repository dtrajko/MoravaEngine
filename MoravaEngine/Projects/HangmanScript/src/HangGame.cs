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
        private bool exit = false;

        public HangGame(string word)
        {
            this.wordToGuess = word;

            // Print word
            CGL.GameConsole.clearBuffer();
            // CGL.GameConsole.putString(0, 0, "The word is: " + this.wordToGuess, true);
            CGL.GameConsole.present();
        }

        public void keyEvent(char key)
        {
            // Check if exit
            if (key == 'X')
            {
                exit = true;
            }
            // throw new NotImplementedException();
        }

        public bool tick()
        {
            // Sleep and return
            // Thread.Sleep(4000);
            return !exit;
            // throw new NotImplementedException();
        }
    }
}
