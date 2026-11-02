using CGL;


/// <summary>
/// Based on
/// CS #2 - Creating the MonoDomain (and load script) | C++ Scripting Guide [C#]
/// https://www.youtube.com/watch?v=ps9EW_nzs34
/// </summary>

namespace HangmanScript
{
    /// <summary>
    /// Main class
    /// </summary>
    public class HangMain
    {
        /// <summary>
        /// Main function
        /// </summary>
        public static IGame main()
        {
            string word = CGL.RandomWord.getRandomWord();
            return new HangGame(word);
        }
    }
}
