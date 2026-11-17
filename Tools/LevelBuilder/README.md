# Level builder

Makes a level's greybox models from a data file, in Blender without its window (the shooter's plan:
`docs/games/Shooter_Plan.html`, Phase 0.3, version 1). The data file is the source: change it and run the script again.
A change made by hand in Blender, or to the level's entities in the scene, is lost the next run.

```powershell
# From the repository's root (Blender 4.2 or newer; 5.2 is what it was made with)
& "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup `
    --python Tools/LevelBuilder/build_level.py -- Projects/Outpost/Levels/Greybox.level.json
```

It writes, for a level named `<Level>`:

| File | What |
|---|---|
| `Assets/Models/Levels/<Level>/Level_<Level>_<Building>_F<n>.gltf` (and `.bin`) | a building's floor n (0 is the ground floor): its slab, its outer and inner walls, the stairs from it up |
| `Assets/Models/Levels/<Level>/Level_<Level>_<Building>_Roof.gltf` | the roof: a model of its own, to hide it and look inside |
| `Assets/Models/Levels/<Level>/Level_<Level>_Ground.gltf` | the ground around the buildings |
| `Assets/Models/Levels/<Level>/Level_<Level>_<Catwalk>.gltf` | a catwalk: its deck, its steps, its railings |
| `Assets/Models/Levels/Textures/Greybox_<Material>.png` | the greybox materials: a color with a line every meter, shared by all levels |

The models are `.gltf` files with their `.bin` and the textures in a folder of their own (not `.glb`): the engine reads a
model's textures from files, not from inside a `.glb`. A run first deletes the level's models of the last run, so a
building or floor taken out of the data leaves nothing behind.

**The scene.** When the data names a `Scene`, the script puts the level in it: an entity `Level_<Level>` at the origin,
with one child entity per model, each with a `TriangleMesh` collider (the greybox's boxes are their own colliders; the
stairs' steps are 0.18 m at most, under a character's step height of 0.35 m). These entities have the IDs 60000 to 69999
and are replaced on each run; the scene's other entities (the camera, the sun, the environment, what was added by hand)
stay as they are.

The script prints each model's triangles and, per building, its size, floors and stairs (the steps, their height, the
stairs' length).

## The data file (`.level.json`)

Coordinates are the engine's: meters, Y up; North is -Z, East is +X. A building's own coordinates (`From`, `To`, `At`,
`Min` of a hole) count from its `Min` corner (its north-west corner).

```json
{
  "Name": "Greybox",
  "Scene": "../Scenes/Greybox.mscene",
  "Settings": { "StoryHeight": 3.5, "WallThickness": 0.2 },
  "Ground": { "Min": [-40, -40], "Size": [80, 80] },
  "Buildings": [
    {
      "Name": "Barracks",
      "Min": [-12, -14],
      "Size": [16, 10],
      "Floors": 2,
      "Openings": [
        { "Floor": 0, "Wall": "South", "Offset": 4, "Kind": "Door", "Width": 1.2 },
        { "Floor": 1, "Wall": "North", "Offset": 8, "Kind": "Window", "Width": 3 }
      ],
      "Walls": [
        { "Floor": 0, "From": [8, 0.2], "To": [8, 9.8], "Openings": [{ "Offset": 2.5, "Kind": "Door" }] }
      ],
      "Stairs": [{ "From": 0, "At": [2, 8.4], "Direction": "North" }],
      "Holes": [{ "Floor": 1, "Min": [12, 2], "Size": [2, 2] }]
    }
  ]
}
```

- **Buildings**: a box of `Size` [width along X, depth along Z] from `Min` [x, z], with `Floors` stories (each
  `StoryHeight` high) and a roof. Every floor has a slab (its top at the floor's height, `FloorThickness` down from
  there) and the four outer walls, in the footprint's edge (`WallThickness` thick). `Base`: the ground floor's height
  (0: the ground's top).
- **Openings** in the outer walls: `Wall` North, South, East or West, `Floor` (0 if left out), `Kind` Door or Window,
  `Offset` from the building's west edge (North and South walls) or north edge (East and West) to the opening's middle;
  optional `Width`, `Height`, and a window's `Sill` (its bottom above the floor).
- **Walls**: the rooms' walls, straight along X or Z from `From` to `To` (their middle line), on a `Floor`, from the
  floor up to the slab above; optional `Thickness`. Their `Openings` have an `Offset` from the wall's west or north end.
- **Stairs**: straight, from floor `From` to the one above (from the top floor: onto the roof, through a hole in it). `At` is the middle of the first step's front edge,
  `Direction` the way they climb (North, South, East, West), `Width` optional. The steps divide the story's height
  evenly, each at most `StairRiser` high, `StairTread` deep (a 3.5 m story: 20 steps of 17.5 cm, 5.6 m long). The floor
  above gets a hole over the whole stairs. Leave room to walk onto the first step (more than a character's width
  between it and a wall).
- **Holes**: more openings in a floor's slab, `Min` and `Size` in the building's coordinates.
- **Parapet** (a setting, 0 by default): a low wall this high around the roof, in the outer walls' place. Its gaps are
  the roof's `Openings` (`Floor` = the number of floors, `Kind` Door): where a catwalk joins it.
- **Catwalks** (the level's, beside `Buildings`): `{ "Name", "From": [x, y, z], "To": [x, y, z], "Width" }`, along X or
  Z, `From` and `To` the deck's top at each end on its middle line. Ends at different heights: flat at the higher one
  and steps down at the lower end (each at most `StairRiser`). Railings along both sides (`RailingHeight`, 0: none).
  Leave the joins clear: a stairwell's hole next to a parapet's gap leaves too narrow a strip for the navmesh.
- **Ground**: a slab of `Size` from `Min`, its top at 0 and `GroundThickness` deep, around the buildings' footprints.
- **Settings** (the level's, and a building's own to override them): `StoryHeight` (3.5), `FloorThickness` (0.3),
  `WallThickness` (0.2), `DoorWidth` (1.0), `DoorHeight` (2.2), `WindowWidth` (1.6), `WindowSill` (1.0), `WindowHeight`
  (1.2), `StairWidth` (1.2), `StairRiser` (0.18), `StairTread` (0.28), `GroundThickness` (0.5), `Parapet` (0), `CatwalkWidth`
  (1.2), `CatwalkThickness` (0.15), `RailingHeight` (1.0), `RailingThickness` (0.05).

## Not yet (Phase 6: the level builder in full)

Ramps, ladders, lifts, stairs with a landing or a turn, railings on stairs and at holes; catwalks that turn or slope
without steps; props, lights, probe volumes; the spots file
(spawns, patrol routes, cover, pickups, objectives); materials beyond the greybox; collider meshes simpler than the drawn
ones (the engine has no model that only collides yet).
