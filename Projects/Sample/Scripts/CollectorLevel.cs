using System;
using System.Numerics;
using Morava;

namespace Sample
{
    /// <summary>
    /// A level of the coin collector (Milestone 2), on an entity of its own: it counts the level's coins (the entities
    /// tagged Coin), shows the count and the time on the HUD, plays a sound for each coin (a little higher each time) and
    /// scatters sparkles where it was. The level is done with its last coin, or, with a Goal, when the player walks into
    /// the goal that the last coin opened. Then its coins and time count for the run (CollectorGame) and NextScene loads
    /// after NextDelay seconds. Below FallHeight the player starts again at the last checkpoint (Checkpoint), or where the
    /// level began.
    /// </summary>
    public class CollectorLevel : Entity
    {
        /// <summary>The level that runs now (the coins, the checkpoints and the goal report to it).</summary>
        public static CollectorLevel? Current { get; private set; }

        /// <summary>Shown on the HUD.</summary>
        public string LevelName = "Level 1";

        /// <summary>Loaded when the level is done (relative to the project; empty: the game stays here).</summary>
        public string NextScene = "";

        /// <summary>Seconds from the level's end to the next scene.</summary>
        public float NextDelay = 4.0f;

        /// <summary>The level's goal (none: the level is done with its last coin).</summary>
        public Entity? Goal;

        /// <summary>Below this height the player has fallen: back to the last checkpoint.</summary>
        public float FallHeight = -100.0f;

        /// <summary>The sounds, relative to the project (empty: none).</summary>
        public string CoinSound = "Assets/Audio/Pickup.wav";
        public string CompleteSound = "Assets/Audio/Win.wav";
        public string FallSound = "Assets/Audio/Land.wav";

        /// <summary>Launched where a coin was (none: nothing), SparkleCount of them, from a pool of SparklePoolSize placed
        /// at the level's start (placing them during the game would pause it: SparklePool).</summary>
        public Prefab? SparklePrefab;
        public int SparkleCount = 5;
        public int SparklePoolSize = 15;

        /// <summary>The HUD (UI Text elements; none: not shown): the coins, the level's name, its time, and the message
        /// at its end (hidden until then).</summary>
        public Entity? CoinText;
        public Entity? LevelText;
        public Entity? TimeText;
        public Entity? MessageText;

        /// <summary>The coins collected in this level (the Inspector shows it while the game runs).</summary>
        public int Collected;

        int total;
        double startTime, endTime = -1, messageUntil = -1;
        Entity? player;
        Vector3 respawn;
        Entity? checkpoint;
        bool loading;
        SparklePool? sparkles;

        protected override void OnCreate()
        {
            Current = this;
            total = Scene.FindEntitiesWithTag("Coin").Length;
            startTime = Time.GameTime; // the clock goes on from level to level
            Entity[] players = Scene.FindEntitiesWithTag("Player");
            player = players.Length > 0 ? players[0] : null;
            respawn = player?.Translation ?? Vector3.Zero;
            sparkles = new SparklePool(SparklePrefab, SparklePoolSize);
            SetText(LevelText, LevelName);
            ShowMessage(null);
            ShowCoins();
            Log.Info($"{LevelName}: collect the {total} coins" + (Goal != null ? ", then reach the goal" : ""));
        }

        protected override void OnUpdate(float dt)
        {
            double seconds = (endTime >= 0 ? endTime : Time.GameTime) - startTime;
            SetText(TimeText, CollectorGame.FormatTime(seconds));

            if (endTime >= 0 && NextScene.Length > 0)
            {
                double left = endTime + NextDelay - Time.GameTime;
                if (left <= 0 && !loading)
                {
                    loading = true;
                    Scene.Load(NextScene);
                }
            }
            else if (messageUntil >= 0 && Time.GameTime >= messageUntil)
            {
                messageUntil = -1;
                ShowMessage(null);
            }

            if (player != null && endTime < 0 && player.WorldPosition.Y < FallHeight)
                Respawn();
        }

        /// <summary>A coin the player touched: one more; the last one ends the level, or opens its goal.</summary>
        public void Collect(Coin coin)
        {
            if (endTime >= 0)
                return;
            Collected++;
            ShowCoins();
            Vector3 where = coin.WorldPosition;
            if (CoinSound.Length > 0)
                Audio.PlayOneShot(CoinSound, where, 0.8f, AudioBus.Effects, 1.0f + 0.03f * (Collected - 1));
            sparkles?.Burst(where, SparkleCount, 1.2f, 2.2f);
            Log.Info($"{LevelName}: coin {Collected} / {total} ({coin.Name})");
            if (Collected < total)
                return;
            if (Goal is LevelGoal goal)
            {
                goal.Open();
                ShowMessage("All coins! The gate is open", 3.0);
                Log.Info($"{LevelName}: all coins, the goal is open");
            }
            else
            {
                Complete();
            }
        }

        /// <summary>The player walked into the open goal.</summary>
        public void ReachGoal()
        {
            if (endTime < 0 && Collected >= total)
                Complete();
        }

        /// <summary>The player passed a checkpoint: after a fall, the game goes on from there.</summary>
        public void SetCheckpoint(Checkpoint point)
        {
            if (checkpoint == point)
                return;
            checkpoint = point;
            respawn = point.WorldPosition;
            Log.Info($"{LevelName}: checkpoint '{point.Name}'");
        }

        void Respawn()
        {
            CollectorGame.Falls++;
            // Put somewhere: the character is there in the next step, standing (its fall's speed gone)
            player!.Translation = respawn + new Vector3(0, 0.05f, 0);
            player.GetComponent<CharacterController>()?.Move(Vector3.Zero);
            if (FallSound.Length > 0)
                Audio.PlayOneShot(FallSound, respawn, 0.8f);
            ShowMessage("Splash! Back to the checkpoint", 2.0);
            Log.Info($"{LevelName}: fell, back to {(checkpoint != null ? $"the checkpoint '{checkpoint.Name}'" : "the start")}");
        }

        void Complete()
        {
            endTime = Time.GameTime;
            double seconds = endTime - startTime;
            CollectorGame.FinishLevel(total, seconds);
            string message = $"{LevelName} complete!\nAll {total} coins in {CollectorGame.FormatTime(seconds)}";
            Log.Info(message.Replace('\n', ' ') + (NextScene.Length > 0 ? $": next '{NextScene}' in {NextDelay:0} s" : ""));
            ShowMessage(message);
            if (CompleteSound.Length > 0)
                Audio.PlayOneShot(CompleteSound);
        }

        void ShowCoins() => SetText(CoinText, $"Coins {Collected} / {total}");

        // A message in the middle of the screen (null: hidden), for some seconds (0: until the next one)
        void ShowMessage(string? message, double seconds = 0)
        {
            if (message != null)
                SetText(MessageText, message);
            if (MessageText?.GetComponent<UIRect>() is UIRect rect)
                rect.Visible = message != null;
            messageUntil = message != null && seconds > 0 ? Time.GameTime + seconds : -1;
        }

        static void SetText(Entity? entity, string text)
        {
            if (entity?.GetComponent<UIText>() is UIText ui)
                ui.Text = text;
        }
    }
}
