namespace Morava
{
    /// <summary>The running game as a whole.</summary>
    public static unsafe class Application
    {
        /// <summary>
        /// Ends the game after this frame: the player closes its window, the editor stops Play (as its Stop button does).
        /// The settings are saved first (Settings). A main menu's Quit button.
        /// </summary>
        public static void Quit() => Interop.Game->Application_Quit();
    }
}
