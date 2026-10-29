namespace Morava
{
    /// <summary>The game's clock.</summary>
    public static unsafe class Time
    {
        static NativeTime Now
        {
            get
            {
                NativeTime time;
                Interop.Game->Time_Get(&time);
                return time;
            }
        }

        /// <summary>
        /// This update's time in seconds: the fixed step in OnFixedUpdate, the frame's time in OnUpdate (times TimeScale: 0
        /// while the game stands still).
        /// </summary>
        public static float DeltaTime => Now.DeltaTime;

        /// <summary>
        /// The frame's real time in seconds, whatever TimeScale is: for what runs while the game is paused (a menu's
        /// animation). In OnFixedUpdate, the fixed step.
        /// </summary>
        public static float UnscaledDeltaTime => Now.UnscaledDeltaTime;

        /// <summary>
        /// Game time per real time: 1 normal, 0.5 slow motion, 0 the game stands still (a pause menu: no fixed steps, no
        /// physics, DeltaTime 0; OnUpdate and the UI still run). Set it here; it counts from the next frame and is kept when
        /// a scene is loaded. A new game starts at 1.
        /// </summary>
        public static float TimeScale
        {
            get => Now.TimeScale;
            set => Interop.Game->Time_SetTimeScale(value);
        }

        /// <summary>Seconds of real time since the game started (a pause counts).</summary>
        public static double UnscaledTime => Now.UnscaledTime;

        /// <summary>The fixed step, in seconds (1/60).</summary>
        public static float FixedDeltaTime => Now.FixedDeltaTime;

        /// <summary>Seconds of game time since the game started (paused time doesn't count).</summary>
        public static double GameTime => Now.Time;

        /// <summary>Frames since the game started.</summary>
        public static ulong FrameCount => Now.FrameCount;

        /// <summary>Fixed steps since the game started.</summary>
        public static ulong FixedStepCount => Now.FixedStepCount;
    }
}
