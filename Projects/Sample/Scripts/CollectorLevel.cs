using System;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>
    /// A level of the coin collector (Milestone 2), on an entity of its own: it counts the level's coins (the entities
    /// tagged Coin), shows the count and the time on the HUD, plays a sound for each coin (a little higher each time) and
    /// scatters sparkles where it was, and when the last one is collected, says the level is done.
    /// </summary>
    public class CollectorLevel : Entity
    {
        /// <summary>The level that runs now (the coins report to it).</summary>
        public static CollectorLevel? Current { get; private set; }

        /// <summary>Shown on the HUD.</summary>
        public string LevelName = "Level 1";

        /// <summary>The sounds, relative to the project (empty: none).</summary>
        public string CoinSound = "Assets/Audio/Pickup.wav";
        public string CompleteSound = "Assets/Audio/Win.wav";

        /// <summary>Placed where a coin was (none: nothing), SparkleCount of them.</summary>
        public Prefab? SparklePrefab;
        public int SparkleCount = 5;

        /// <summary>The HUD (UI Text elements; none: not shown): the coins, the level's name, its time, and the message
        /// at its end (hidden until then).</summary>
        public Entity? CoinText;
        public Entity? LevelText;
        public Entity? TimeText;
        public Entity? MessageText;

        /// <summary>The coins collected in this level (the Inspector shows it while the game runs).</summary>
        public int Collected;

        int total;
        double startTime, endTime = -1;
        readonly Random random = new();

        protected override void OnCreate()
        {
            Current = this;
            total = Scene.FindEntitiesWithTag("Coin").Length;
            startTime = Time.GameTime; // the clock goes on from level to level
            SetText(LevelText, LevelName);
            if (MessageText?.GetComponent<UIRect>() is UIRect rect)
                rect.Visible = false;
            ShowCoins();
            Log.Info($"{LevelName}: collect the {total} coins");
        }

        protected override void OnUpdate(float dt)
        {
            double seconds = (endTime >= 0 ? endTime : Time.GameTime) - startTime;
            SetText(TimeText, FormatTime(seconds));
        }

        /// <summary>A coin the player touched: one more, and the level is done with the last one.</summary>
        public void Collect(Coin coin)
        {
            if (endTime >= 0)
                return;
            Collected++;
            ShowCoins();
            Vector3 where = coin.WorldPosition;
            if (CoinSound.Length > 0)
                Audio.PlayOneShot(CoinSound, where, 0.8f, AudioBus.Effects, 1.0f + 0.03f * (Collected - 1));
            if (SparklePrefab != null)
            {
                for (int i = 0; i < SparkleCount; i++)
                {
                    float angle = (i + (float)random.NextDouble() * 0.5f) * 2.0f * MathF.PI / SparkleCount;
                    var sparkle = (Sparkle)Scene.Instantiate(SparklePrefab, where);
                    sparkle.Velocity = new Vector3(MathF.Cos(angle) * 1.2f, 2.2f + (float)random.NextDouble(), MathF.Sin(angle) * 1.2f);
                }
            }
            Log.Info($"{LevelName}: coin {Collected} / {total} ({coin.Name})");
            if (Collected == total)
                Complete();
        }

        void Complete()
        {
            endTime = Time.GameTime;
            string message = $"{LevelName} complete!\nAll {total} coins in {FormatTime(endTime - startTime)}";
            Log.Info(message.Replace('\n', ' '));
            SetText(MessageText, message);
            if (MessageText?.GetComponent<UIRect>() is UIRect rect)
                rect.Visible = true;
            if (CompleteSound.Length > 0)
                Audio.PlayOneShot(CompleteSound);
        }

        void ShowCoins() => SetText(CoinText, $"Coins {Collected} / {total}");

        static void SetText(Entity? entity, string text)
        {
            if (entity?.GetComponent<UIText>() is UIText ui)
                ui.Text = text;
        }

        static string FormatTime(double seconds)
        {
            int whole = (int)Math.Max(0, seconds);
            return $"{whole / 60}:{whole % 60:00}";
        }
    }
}
