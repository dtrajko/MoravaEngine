using Morava;

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

        /// <summary>The main menu (the project's start scene).</summary>
        public const string MainMenuScene = "Scenes/MainMenu.mscene";

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

        // The best finished run, kept between the game's runs (SaveData): the most coins, then the shortest time
        const string BestCoinsKey = "BestCoins";
        const string BestSecondsKey = "BestSeconds";
        const string RunsKey = "FinishedRuns";

        /// <summary>
        /// The run is over (the end screen): counted, and kept as the best when it is (saved at once: the player may close
        /// the game from here). True when it is the best so far.
        /// </summary>
        public static bool FinishRun()
        {
            SaveData.SetInt(RunsKey, SaveData.GetInt(RunsKey) + 1);
            bool best = !SaveData.Has(BestCoinsKey) || Coins > SaveData.GetInt(BestCoinsKey) ||
                (Coins == SaveData.GetInt(BestCoinsKey) && Seconds < SaveData.GetFloat(BestSecondsKey, float.MaxValue));
            if (best)
            {
                SaveData.SetInt(BestCoinsKey, Coins);
                SaveData.SetFloat(BestSecondsKey, (float)Seconds);
            }
            SaveData.Save();
            Log.Info($"Run {SaveData.GetInt(RunsKey)} finished{(best ? ": the best so far" : "")} ({SaveData.File})");
            return best;
        }

        /// <summary>"Best: 42 coins in 3:07" (empty before the first finished run).</summary>
        public static string BestText() => SaveData.Has(BestCoinsKey) ?
            $"Best: {SaveData.GetInt(BestCoinsKey)} coins in {FormatTime(SaveData.GetFloat(BestSecondsKey))} ({SaveData.GetInt(RunsKey)} {(SaveData.GetInt(RunsKey) == 1 ? "run" : "runs")})" : "";

        /// <summary>Minutes and seconds: 1:07.</summary>
        public static string FormatTime(double seconds)
        {
            int whole = (int)System.Math.Max(0, seconds);
            return $"{whole / 60}:{whole % 60:00}";
        }
    }
}
