# Outpost: outside assets and their licenses

The shooter uses CC0 assets only, or our own (docs/games/Shooter_Plan.html, the decisions), with one exception: the
HUD's font (SIL OFL, below). Every outside pack is listed here before it is used: where it came from, its license, and
what of it the game uses. Each pack's own license file is kept beside its files in `Assets/`.

| Pack | Author, source | License | Used for | Folder |
|---|---|---|---|---|
| Universal Base Characters (Standard, free) | Quaternius, https://quaternius.itch.io/universal-base-characters | CC0 1.0 | The soldiers' body (`Superhero_Male_FullBody`, its textures: the light skin, `T_Superhero_Male_Ligh.png` in the pack, as `T_Superhero_Male_Light.png`); the gltf's two normal map names fixed to the files the pack has, and its hair material named our dark brown copy of the hair texture (`T_Hair_1_DarkBrown.png`, made by `Tools/CharacterPoses/tint_hair.py`) | `Assets/Models/QuaterniusCharacters` |
| Universal Animation Library (Standard, free; the itch.io version, new bone names) | Quaternius, https://quaternius.itch.io/universal-animation-library | CC0 1.0 | The characters' clips (`UAL1_Standard.glb`: locomotion, crouch, jump, hits, death, pistol) | `Assets/Models/QuaterniusAnimations` |
| Ultimate Gun Pack (July 2019) | Quaternius, https://quaternius.com/packs/ultimategun.html, https://opengameart.org/content/low-poly-guns-pack | CC0 1.0 (the pages; no license file in the download) | The rifle (`AssaultRifle_2`), the sniper rifle (`SniperRifle_3`), the pistol (`Pistol_1`); scale 0.0017 in a scene (the files are in centimeters, about 6 times too large) | `Assets/Models/QuaterniusGuns` |
| Barlow Condensed (SemiBold, Medium) | The Barlow Project Authors (Jeremy Tribby), https://github.com/jpt/barlow, from https://github.com/google/fonts/tree/main/ofl/barlowcondensed | SIL Open Font License 1.1 (`OFL.txt` beside the fonts) | The HUD's font (Phase 3.2). **The one exception to CC0** (the user, 2026-11-24): good condensed fonts are seldom CC0; the OFL lets a game ship the font with it, its files kept as they are (not sold alone, its name not used for a changed version) | `Assets/Fonts` |

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
colors) shares the player's mesh and its normal and roughness maps. The weapon sounds (`Assets/Audio/Rifle_*.wav`), the
bullets' hits (`Impact_*.wav`) and the hit marker's tick (`HitMarker.wav`) are synthesized by
`Tools/Audio/GenerateWeaponSounds.py`, no recordings. The weapon effects' textures (`Assets/Textures/Effects`: the muzzle
flash, the bullet holes) are made by `Tools/Textures/GenerateWeaponEffectTextures.py`, no images.

Phase 2.4: the knife (`Assets/Models/OutpostWeapons/Knife.glb`) is our own, made by `Tools/Models/make_knife.py`, CC0.
The pistol and knife clips (`Soldier_Sidearms.glb`, `Tools/CharacterPoses/make_rifle_poses.py --set sidearms`), the get-up
(`Soldier_GetUp.glb`, `Tools/CharacterPoses/make_get_up.py`)
are our own on the animation library's rig, carrying its mannequin, CC0. The pistol's sounds (`Pistol_*.wav`), the knife's
(`Knife_*.wav`) and taking the weapons out (`Draw_*.wav`) are synthesized by `Tools/Audio/GenerateWeaponSounds.py`.

Phase 3.1: the pickups (the health packs, the vest, the ammo box) and the electric floor (`Assets/Models/OutpostPickups`)
are our own, made by `Tools/Models/make_pickups.py`, CC0. Their sounds, the sparks' and the hum (`Pickup_*.wav`,
`Electric_*.wav`) and the player's hurt and death (`Player_*.wav`) are synthesized by `Tools/Audio/GenerateGameSounds.py`;
the HUD's hit vignette (`Assets/Textures/UI/DamageVignette.png`) is made by `Tools/Textures/GenerateHudTextures.py`.

Phase 3.2: the weapons' silhouettes on the HUD's slots (`Assets/Textures/UI/Weapon_*.png`) are rendered from the
weapons' models above by `Tools/Textures/RenderWeaponIcons.py`; the arc toward a hit (`DamageArc.png`) is made by
`Tools/Textures/GenerateHudTextures.py`, and the objective's tones (`Objective_Complete.wav`) are synthesized by
`Tools/Audio/GenerateGameSounds.py`.
