using Morava;

namespace Sample
{
    /// <summary>
    /// The coin collector's end screen (on its canvas): the run's coins, time and falls (CollectorGame), and its buttons:
    /// Play Again (a new run from the first level; On Click: PlayAgain), Main Menu (On Click: MainMenu) and Quit (the
    /// player closes; On Click: Quit). The
    /// mouse is free here and the first button is selected, so the gamepad works at once.
    /// </summary>
    public class EndScreen : Entity
    {
        /// <summary>A UI Text for the run's numbers.</summary>
        public Entity? SummaryText;

        /// <summary>Selected when the screen opens.</summary>
        public Entity? FirstButton;

        protected override void OnCreate()
        {
            Input.CursorLocked = false;
            string summary = CollectorGame.Levels == 0 ? "No levels played yet" :
                $"{CollectorGame.Coins} coins in {CollectorGame.Levels} {(CollectorGame.Levels == 1 ? "level" : "levels")}\nTime {CollectorGame.FormatTime(CollectorGame.Seconds)}" +
                (CollectorGame.Falls > 0 ? $", {CollectorGame.Falls} {(CollectorGame.Falls == 1 ? "splash" : "splashes")}" : ", not a single splash");
            if (SummaryText?.GetComponent<UIText>() is UIText text)
                text.Text = summary;
            FirstButton?.GetComponent<UIButton>()?.Select();
            Log.Info($"End screen: {summary.Replace('\n', ' ')}");
        }

        // The Play Again button's On Click
        void PlayAgain()
        {
            Log.Info("Play again");
            CollectorGame.Reset();
            Scene.Load(CollectorGame.FirstLevel);
        }

        // The Main Menu button's On Click
        void MainMenu()
        {
            Log.Info("Main menu from the end screen");
            Scene.Load(CollectorGame.MainMenuScene);
        }

        // The Quit button's On Click
        void Quit()
        {
            Log.Info("Quit from the end screen");
            Application.Quit();
        }
    }
}
