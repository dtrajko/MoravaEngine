# Outpost: outside assets and their licenses

The shooter uses CC0 assets only, or our own (docs/games/Shooter_Plan.html, the decisions). Every outside pack is listed
here before it is used: where it came from, its license, and what of it the game uses. Each pack's own license file is
kept beside its files in `Assets/`.

| Pack | Author, source | License | Used for | Folder |
|---|---|---|---|---|
| Universal Base Characters (Standard, free) | Quaternius, https://quaternius.itch.io/universal-base-characters | CC0 1.0 | The soldiers' body (`Superhero_Male_FullBody`, its textures: the light skin, `T_Superhero_Male_Ligh.png` in the pack, as `T_Superhero_Male_Light.png`); the gltf's two normal map names fixed to the files the pack has, and its hair material named our dark brown copy of the hair texture (`T_Hair_1_DarkBrown.png`, made by `Tools/CharacterPoses/tint_hair.py`) | `Assets/Models/QuaterniusCharacters` |
| Universal Animation Library (Standard, free; the itch.io version, new bone names) | Quaternius, https://quaternius.itch.io/universal-animation-library | CC0 1.0 | The characters' clips (`UAL1_Standard.glb`: locomotion, crouch, jump, hits, death, pistol) | `Assets/Models/QuaterniusAnimations` |
| Ultimate Gun Pack (July 2019) | Quaternius, https://quaternius.com/packs/ultimategun.html, https://opengameart.org/content/low-poly-guns-pack | CC0 1.0 (the pages; no license file in the download) | The rifle (`AssaultRifle_2`), the sniper rifle (`SniperRifle_3`), the pistol (`Pistol_1`); scale 0.0017 in a scene (the files are in centimeters, about 6 times too large) | `Assets/Models/QuaterniusGuns` |

Checked and kept outside the repository for later (the content check, Phase 0.2; downloads in the ignored
`Library/ContentCheck`): Quaternius's Toon Shooter Game Kit (props for Phase 6, the health kit for Phase 3), Universal
Animation Library 2, Low Poly Animated Guns; Kenney's Blaster Kit (not used: toy sci-fi blasters).

Our own: the scripts' textures and sounds, the level builder's models (`Tools/LevelBuilder`), the greybox materials, the
rifle clips (`Assets/Models/SoldierAnimations/Soldier_Rifles.glb`, made by `Tools/CharacterPoses/make_rifle_poses.py`)
and the player's fast walks forward, back and to the sides (`Assets/Models/SoldierAnimations/Soldier_Locomotion.glb`, made by
`Tools/CharacterPoses/make_fast_walk.py`), both on the animation library's rig; the files carry the library's
mannequin, CC0. The soldier's uniform (`Soldier_Player.gltf`, `Soldier_Player_Arms.gltf` with their `.bin` files and the
`T_Soldier_Player_*` maps in `Assets/Models/QuaterniusCharacters`, made by `Tools/CharacterPoses/make_uniform.py`) is
shaped from the base character's mesh and painted over its maps, CC0. The training dummies' uniform (`Soldier_Training.gltf` and `T_Soldier_Training_Color.png`, the same script's Training
colors) shares the player's mesh and its normal and roughness maps. The weapon sounds (`Assets/Audio/Rifle_*.wav`) are
synthesized by `Tools/Audio/GenerateWeaponSounds.py`, no recordings.
