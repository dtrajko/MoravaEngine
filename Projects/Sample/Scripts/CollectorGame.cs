namespace Sample
{
    /// <summary>
    /// The coin collector's run from level to level (Milestone 2): what the finished levels added up to. Static fields
    /// stay while scenes load (Scene.Load) and start fresh with each run of the game (the editor loads the scripts again
    /// at Stop). The end screen shows them; Play Again starts a new run.
    /// </summary>
    public static class CollectorGame
    {
        /// <summary>The scene the run starts with (Play Again).</summary>
        public const string FirstLevel = "Scenes/Level1.mscene";

        /// <summary>The coins of the finished levels.</summary>
        public static int Coins;

        /// <summary>Seconds of game time the finished levels took.</summary>
        public static double Seconds;

        /// <summary>The levels finished.</summary>
        public static int Levels;

        /// <summary>The times the player fell and started again from a checkpoint.</summary>
        public static int Falls;

        /// <summary>A level is done: its coins and time count.</summary>
        public static void FinishLevel(int coins, double seconds)
        {
            Coins += coins;
            Seconds += seconds;
            Levels++;
        }

        /// <summary>A new run (Play Again).</summary>
        public static void Reset()
        {
            Coins = 0;
            Seconds = 0;
            Levels = 0;
            Falls = 0;
        }

        /// <summary>Minutes and seconds: 1:07.</summary>
        public static string FormatTime(double seconds)
        {
            int whole = (int)System.Math.Max(0, seconds);
            return $"{whole / 60}:{whole % 60:00}";
        }
    }
}
