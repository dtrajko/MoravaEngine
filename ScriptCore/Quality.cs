namespace Morava
{
    /// <summary>
    /// How much of the renderer's work the game asks of the computer: a Settings menu's Quality. High is the scenes as they
    /// were made in the editor; Ultra asks more (larger shadow maps, ray-traced effects at full resolution), Medium and Low
    /// less (no ray tracing, smaller shadow maps; Low also without ambient occlusion). Until the player chooses, the game
    /// picks a tier for the GPU (automatic: High on a graphics card with ray tracing, Medium on one without, Low on a GPU in
    /// the processor), or the project's default.
    /// </summary>
    public static unsafe class Quality
    {
        /// <summary>
        /// The tier the game is drawn with. Set: from the next frame on, and the player's choice is kept for the next runs
        /// too (in the editor's Play as well: the Viewport shows it).
        /// </summary>
        public static QualityLevel Level
        {
            get => (QualityLevel)Interop.Game->Quality_GetLevel();
            set => Interop.Game->Quality_SetLevel((int)value);
        }

        /// <summary>The player chose no tier: the game's pick for the GPU (or the project's default) draws.</summary>
        public static bool IsAutomatic => Interop.Game->Quality_IsAutomatic() != 0;

        /// <summary>Forgets the player's choice: the game picks again (from the next frame on).</summary>
        public static void SetAutomatic() => Interop.Game->Quality_SetLevel(-1);

        /// <summary>The GPU traces rays: High and Ultra use it for reflections and ambient occlusion.</summary>
        public static bool RayTracingSupported => Interop.Game->Quality_IsRayTracingSupported() != 0;
    }

    /// <summary>A quality tier (Quality.Level).</summary>
    public enum QualityLevel
    {
        /// <summary>No ray tracing, small shadow maps, no ambient occlusion, FXAA.</summary>
        Low = 0,
        /// <summary>No ray tracing, smaller shadow maps.</summary>
        Medium = 1,
        /// <summary>The scenes as they were made in the editor.</summary>
        High = 2,
        /// <summary>Larger shadow maps, ray-traced effects at full resolution.</summary>
        Ultra = 3,
    }
}
