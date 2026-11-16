using Morava;

namespace Outpost
{
    /// <summary>
    /// The content check (docs/games/Shooter_Plan.html, Phase 0.2): puts a character's animator (Soldier.manim) in one of
    /// the animation library's clips, by number: 0 Idle, 1 Walk, 2 Jog, 3 Pistol idle, 4 Pistol reload, 5 Crouch idle,
    /// 6 Crouch walk, 7 Death; our rifle clips (Tools/CharacterPoses): 8 Rifle idle, 9 Rifle aim, 10 Rifle aim up,
    /// 11 Rifle reload, 12 Sniper idle, 13 Sniper aim, 14 Sniper bolt, 15 Sniper reload.
    /// </summary>
    public class ContentCheckPose : Entity
    {
        public float Pose = 0.0f;

        protected override void OnCreate()
        {
            GetComponent<Animator>()?.SetFloat("Pose", Pose);
        }
    }
}
