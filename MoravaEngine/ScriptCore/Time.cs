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

        /// <summary>This update's time in seconds: the fixed step in OnFixedUpdate, the frame's time in OnUpdate.</summary>
        public static float DeltaTime => Now.DeltaTime;

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
