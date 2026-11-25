using System.Collections.Generic;
using Morava;

namespace Outpost
{
    /// <summary>How an objective is done.</summary>
    public enum ObjectiveKind
    {
        /// <summary>The player enters the objective's trigger (a Collider with Is Trigger on).</summary>
        Reach,
        /// <summary>The player takes Target (a Pickup).</summary>
        PickUp,
        /// <summary>A script calls Complete (Phase 7: switches, items, a named guard).</summary>
        Manual,
    }

    /// <summary>
    /// One of the mission's objectives (docs/games/Shooter_Plan.html, Phase 3.2; Phase 7 adds the briefing, the mission's
    /// state in saves and the other kinds): its Title on the HUD, how it is done (Kind), whether it is Optional, and its
    /// place in the list (Order). The HUD lists the ones not done at the top right and all of them while Objectives (Tab)
    /// is held; done, it says so. Done stays done when the player dies and starts again.
    /// </summary>
    public class Objective : Entity
    {
        public string Title = "Objective";
        public ObjectiveKind Kind = ObjectiveKind.Reach;

        /// <summary>The pickup to take (PickUp).</summary>
        public Entity? Target;

        public bool Optional;

        /// <summary>Its place in the list (lower first; the same: by name).</summary>
        public int Order;

        public bool IsComplete { get; private set; }

        Pickup? pickup;

        protected override void OnCreate()
        {
            pickup = Target as Pickup;
            if (Kind == ObjectiveKind.PickUp && pickup == null)
                Log.Warn($"{Name}: Kind PickUp without a Pickup as Target: it can't be done");
            if (Kind == ObjectiveKind.Reach && !HasComponent<Collider>())
                Log.Warn($"{Name}: Kind Reach without a Collider (Is Trigger): it can't be done");
            Objectives.Add(this);
        }

        protected override void OnTriggerEnter(Entity other)
        {
            if (Kind == ObjectiveKind.Reach && other is Player player && !player.IsDead)
                Complete();
        }

        protected override void OnFixedUpdate(float dt)
        {
            if (!IsComplete && Kind == ObjectiveKind.PickUp && pickup != null && pickup.Taken > 0)
                Complete();
        }

        /// <summary>Done (once: later calls do nothing).</summary>
        public void Complete()
        {
            if (IsComplete)
                return;
            IsComplete = true;
            Log.Info($"{Name}: objective complete: {Title}");
            Objectives.OnCompleted(this);
        }
    }

    /// <summary>
    /// The scene's objectives in their order, and the last one done (the HUD's banner). Read by polling, no events: the
    /// scripts' statics may outlive a Play in the editor, and a handler kept from it would call into a gone HUD.
    /// </summary>
    public static class Objectives
    {
        static readonly List<Objective> list = new();

        /// <summary>In their order (Order, then name).</summary>
        public static IReadOnlyList<Objective> All
        {
            get
            {
                list.RemoveAll(o => !o.Exists); // (the last Play's)
                return list;
            }
        }

        /// <summary>Objectives done so far (counting on: compare with the count seen before), and the last one.</summary>
        public static int Completions { get; private set; }
        public static Objective? LastCompleted { get; private set; }

        /// <summary>Every objective that isn't optional is done (and there is one).</summary>
        public static bool AllRequiredComplete
        {
            get
            {
                bool any = false;
                foreach (Objective o in All)
                {
                    if (o.Optional)
                        continue;
                    any = true;
                    if (!o.IsComplete)
                        return false;
                }
                return any;
            }
        }

        internal static void Add(Objective objective)
        {
            list.RemoveAll(o => !o.Exists); // (the last Play's, if the scripts outlived it)
            if (list.Contains(objective))
                return;
            list.Add(objective);
            list.Sort((a, b) => a.Order != b.Order ? a.Order.CompareTo(b.Order) : string.CompareOrdinal(a.Name, b.Name));
        }

        internal static void OnCompleted(Objective objective)
        {
            Completions++;
            LastCompleted = objective;
        }
    }
}
