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

        /// <summary>
        /// The game's window fills its monitor. Set, it changes after this frame (as F11 or Alt+Enter do); in the editor's
        /// Play, which runs in the Viewport panel, nothing changes (the log says so). A Settings menu's toggle.
        /// </summary>
        public static bool Fullscreen
        {
            get => Interop.Game->Application_GetFullscreen() != 0;
            set => Interop.Game->Application_SetFullscreen(value ? 1 : 0);
        }
    }
}
