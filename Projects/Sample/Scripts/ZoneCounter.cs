using Morava;

namespace Sample
{
    /// <summary>
    /// A trigger that counts what is inside it and says so in the log. The entity needs a Collider with Is Trigger on.
    /// </summary>
    public class ZoneCounter : Entity
    {
        /// <summary>Said when something comes in.</summary>
        public string Message = "Welcome";

        /// <summary>Also say when something leaves.</summary>
        public bool ReportExits = true;

        /// <summary>How many bodies are inside now (the Inspector shows it while the game runs).</summary>
        public int Inside;

        protected override void OnTriggerEnter(Entity other)
        {
            Inside++;
            Log.Info($"{Name}: {Message}, {other.Name}! ({Inside} inside)");
        }

        protected override void OnTriggerExit(Entity other)
        {
            Inside--;
            if (ReportExits)
                Log.Info($"{Name}: {other.Name} left ({Inside} inside)");
        }
    }
}
