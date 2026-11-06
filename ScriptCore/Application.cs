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
        /// The game's window fills its monitor. Set, it changes after this frame (as F11 or Alt+Enter do), and the player's
        /// choice is kept for the next runs too; in the editor's Play, which runs in the Viewport panel, nothing changes (the
        /// log says so). A Settings menu's toggle.
        /// </summary>
        public static bool Fullscreen
        {
            get => Interop.Game->Application_GetFullscreen() != 0;
            set => Interop.Game->Application_SetFullscreen(value ? 1 : 0);
        }

        /// <summary>
        /// The sizes the monitor can show the game at in fullscreen, largest first (a smaller one draws fewer pixels: faster).
        /// Empty in the editor's Play.
        /// </summary>
        public static Resolution[] Resolutions
        {
            get
            {
                uint* pairs = stackalloc uint[2 * 64];
                int count = Interop.Game->Application_GetResolutions(pairs, 64);
                var resolutions = new Resolution[System.Math.Min(count, 64)];
                for (int i = 0; i < resolutions.Length; i++)
                    resolutions[i] = new Resolution((int)pairs[i * 2], (int)pairs[i * 2 + 1]);
                return resolutions;
            }
        }

        /// <summary>
        /// The size fullscreen shows the game at (one of Resolutions; at first the monitor's own). Set: in fullscreen it
        /// changes after this frame, in a window at the next fullscreen; the player's choice is kept for the next runs too.
        /// </summary>
        public static Resolution Resolution
        {
            get
            {
                uint* size = stackalloc uint[2];
                Interop.Game->Application_GetResolution(size);
                return new Resolution((int)size[0], (int)size[1]);
            }
            set => Interop.Game->Application_SetResolution((uint)System.Math.Max(value.Width, 0), (uint)System.Math.Max(value.Height, 0));
        }

        /// <summary>
        /// The window waits for the display before it shows the next picture: no tearing, and no more frames than the
        /// display shows. Set, it changes after this frame; the player's choice is kept for the next runs too. In the
        /// editor's Play nothing changes (the editor runs without it).
        /// </summary>
        public static bool VSync
        {
            get => Interop.Game->Application_GetVSync() != 0;
            set => Interop.Game->Application_SetVSync(value ? 1 : 0);
        }
    }

    /// <summary>A size in pixels: a fullscreen resolution.</summary>
    public readonly struct Resolution : System.IEquatable<Resolution>
    {
        /// <summary>A size of width x height pixels.</summary>
        public Resolution(int width, int height)
        {
            Width = width;
            Height = height;
        }

        /// <summary>Pixels across.</summary>
        public int Width { get; }

        /// <summary>Pixels down.</summary>
        public int Height { get; }

        /// <summary>The same size.</summary>
        public bool Equals(Resolution other) => Width == other.Width && Height == other.Height;

        /// <summary>The same size.</summary>
        public override bool Equals(object? obj) => obj is Resolution other && Equals(other);

        /// <summary>From the width and the height.</summary>
        public override int GetHashCode() => System.HashCode.Combine(Width, Height);

        /// <summary>The same size.</summary>
        public static bool operator ==(Resolution a, Resolution b) => a.Equals(b);

        /// <summary>Another size.</summary>
        public static bool operator !=(Resolution a, Resolution b) => !a.Equals(b);

        /// <summary>"1920 x 1080".</summary>
        public override string ToString() => $"{Width} x {Height}";
    }
}
