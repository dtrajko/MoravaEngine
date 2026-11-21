using System;
using System.Collections.Generic;
using Morava;

namespace Outpost
{
    /// <summary>
    /// The weapons a player carries and which one is in the hands (docs/games/Shooter_Plan.html, Phase 2.4): switching by
    /// the commands' slot (the number keys), cycle (the wheel, the gamepad's bumpers) and last weapon (Q), in the fixed
    /// step. A switch puts the weapon away (its HolsterSeconds: a reload or a strike stops), then the next one comes out
    /// (its DrawSeconds); meanwhile nothing fires. Each weapon keeps its own magazine and reserve. Only the weapon out
    /// steps; what is seen and heard reacts to the events.
    /// </summary>
    public sealed class PlayerArsenal
    {
        readonly List<PlayerWeapon> weapons = new();
        int current, previous, next = -1;
        float holsterLeft, drawLeft;

        public PlayerArsenal(IEnumerable<PlayerWeapon> carried)
        {
            weapons.AddRange(carried);
            weapons.Sort((a, b) => a.Data.Slot.CompareTo(b.Data.Slot));
        }

        public IReadOnlyList<PlayerWeapon> Weapons => weapons;

        /// <summary>The weapon in the hands (null: none carried); during a switch the one going away, then the one coming out.</summary>
        public PlayerWeapon? Current => weapons.Count > 0 ? weapons[current] : null;

        /// <summary>A switch is under way (putting one away or taking the next out): nothing fires.</summary>
        public bool IsSwitching => holsterLeft > 0.0f || drawLeft > 0.0f;

        /// <summary>Putting the weapon away (the first half of a switch), and how far (0 to 1); taking one out (1 to 0 as it comes up).</summary>
        public bool IsHolstering => holsterLeft > 0.0f;
        public float Lowered
        {
            get
            {
                if (holsterLeft > 0.0f && Current != null)
                    return 1.0f - holsterLeft / MathF.Max(0.01f, Current.Data.HolsterSeconds);
                if (drawLeft > 0.0f && Current != null)
                    return drawLeft / MathF.Max(0.01f, Current.Data.DrawSeconds);
                return 0.0f;
            }
        }

        /// <summary>The next weapon was taken out (the old one's put away): from, to. Its drawing has started.</summary>
        public event Action<PlayerWeapon, PlayerWeapon>? Switched;

        /// <summary>A switch started: the weapon in the hands goes away.</summary>
        public event Action<PlayerWeapon>? Holstering;

        /// <summary>The weapon is out and ready.</summary>
        public event Action<PlayerWeapon>? Drawn;

        /// <summary>The weapon out at once, without a switch (the start): its index in Weapons.</summary>
        public void Choose(int index)
        {
            if (index < 0 || index >= weapons.Count)
                return;
            current = previous = index;
            next = -1;
            holsterLeft = drawLeft = 0.0f;
        }

        /// <summary>The weapon in a slot (1 to 4), or -1.</summary>
        public int IndexOfSlot(int slot) => weapons.FindIndex(w => w.Data.Slot == slot);

        /// <summary>
        /// Each fixed step, before the weapon's own: the command's switch, if any (to the same weapon: none; during a switch
        /// to another: it turns there), then the switch's time.
        /// </summary>
        public void Step(in PlayerCommand command, float dt)
        {
            if (weapons.Count == 0)
                return;
            int wanted = -1;
            if (command.WeaponSlot > 0)
                wanted = IndexOfSlot(command.WeaponSlot);
            else if (command.WeaponCycle != 0)
                wanted = ((next >= 0 ? next : current) + Math.Sign(command.WeaponCycle) + weapons.Count) % weapons.Count;
            else if (command.LastWeapon && previous != current)
                wanted = previous;
            if (wanted >= 0)
                SwitchTo(wanted);

            if (holsterLeft > 0.0f)
            {
                holsterLeft -= dt;
                if (holsterLeft <= 0.0f)
                {
                    holsterLeft = 0.0f;
                    PlayerWeapon from = weapons[current];
                    previous = current;
                    current = next;
                    next = -1;
                    drawLeft = weapons[current].Data.DrawSeconds;
                    Switched?.Invoke(from, weapons[current]);
                }
            }
            else if (drawLeft > 0.0f)
            {
                drawLeft -= dt;
                if (drawLeft <= 0.0f)
                {
                    drawLeft = 0.0f;
                    Drawn?.Invoke(weapons[current]);
                }
            }
        }

        void SwitchTo(int index)
        {
            if (index == (next >= 0 ? next : current))
                return; // (already there, or on its way)
            if (holsterLeft > 0.0f || (drawLeft <= 0.0f && index != current))
            {
                // Putting away: the next one changes; ready: start putting this one away
                if (holsterLeft <= 0.0f)
                {
                    weapons[current].Holster();
                    holsterLeft = weapons[current].Data.HolsterSeconds;
                    Holstering?.Invoke(weapons[current]);
                }
                next = index;
                return;
            }
            // Taking one out: it goes away again (as far as it came out), then the wanted one
            if (index == current)
                return;
            PlayerWeapon coming = weapons[current];
            coming.Holster();
            float out01 = 1.0f - drawLeft / MathF.Max(0.01f, coming.Data.DrawSeconds);
            holsterLeft = MathF.Max(0.01f, coming.Data.HolsterSeconds * out01);
            drawLeft = 0.0f;
            next = index;
            Holstering?.Invoke(coming);
        }
    }
}
