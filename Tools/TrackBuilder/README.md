# Track builder

Makes a race track's models from a data file, in Blender without its window (the racing game's plan:
`docs/games/Racing_Plan.html`, Phase 1). The data file is the source: change it and run the script again. A change made by
hand in Blender is lost the next run.

```powershell
# From the repository's root (Blender 4.2 or newer; 5.2 is what it was made with)
& "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup `
    --python Tools/TrackBuilder/build_track.py -- Projects/Racing/Tracks/TestTrack.track.json
```

It writes, for a track named `<Name>`:

| File | What | In the scene |
|---|---|---|
| `Assets/Models/Track_<Name>_Road.glb` | the road, the curbs | Model + Collider TriangleMesh, Friction 1 |
| `Assets/Models/Track_<Name>_Walls.glb` | the side walls | Model + Collider TriangleMesh, Friction 0.05 (the car slides along) |
| `Assets/Models/Track_<Name>_Structure.glb` | the road's sides and underside, the supports, the start line (drawn just above the road: not a collider, so it is no step) | Model only |
| `Assets/Tracks/<Name>.centerline.json` | the center line every 2 m (position, right, up, width), the start | read by scripts with `Assets.ReadText` (the test driver, the race); in `Assets` so a built game's pack has it (`CenterlineDirectory` in the data changes the folder) |

Each model is cut into chunks of about 100 m along the track (separate meshes), so the engine culls it chunk by chunk.
The three models go under one Empty (`Track`) placed where the data's origin should be, on the ground (`GroundHeight`).

## The data file (`.track.json`)

Coordinates are the engine's: meters, Y up, a car facing along -Z; relative to the `Track` entity.

```json
{
  "Name": "TestTrack",
  "ModelsDirectory": "../Assets/Models",
  "StartDistance": 12,
  "Settings": { "GroundHeight": 0 },
  "Defaults": { "Width": 16, "Bank": 0, "Walls": true, "Curbs": false, "Supports": true },
  "Points": [
    { "Position": [0, 3, 90] },
    { "Position": [-23, 14, -257], "Bank": -22, "Curbs": true }
  ]
}
```

- **Points** make a closed loop: a centripetal Catmull-Rom spline runs through them, and the first point is the start
  line. Put the first point in the middle of a straight, with points before and after it on the same line, so the start
  is straight.
- **Width**: meters from wall to wall. **Bank**: degrees around the direction of travel; positive raises the left edge
  (for a right turn), negative the right edge (for a left turn). Both blend smoothly to the next point's.
- **Walls**, **Curbs**, **Supports**: hold from a point to the next. Curbs go on the inner side of the turn only (the
  side it turns towards); the road runs up to the wall on the outer side.
- **StartDistance**: how far before the start line the car starts (`Start` in the center line file).
- **Settings** (all optional): `SampleSpacing` (1 m between cross sections), `ChunkLength` (100), `CurbWidth` (1.5),
  `CurbStripe` (3), `WallHeight` (1), `WallThickness` (0.5), `SlabThickness` (0.8), `SupportSpacing` (24),
  `SupportSize` (1.4), `GroundHeight` (0: supports go down to here), `StartLineWidth` (2), `CenterlineSpacing` (2).

The script prints the track's length, its chunks, how far its lowest and highest points are apart, and each model's
triangles.

## Materials

The models carry plain materials (Road, Curb_Red, Curb_White, Wall, Structure, Support, Line_White, Line_Black) with a
base color and roughness. The engine imports them as they are; the project's own materials can replace them per part.

## Flat tire treads (`flatten_tires.py`)

Plain Python, no Blender. Kenney's Car Kit tires are full round only across the middle of their width; widened (a wheel
scaled along its axle) they look as if only their middle touched the road. This writes a copy with the tread flat across
most of the width (the meshes named `wheel...`; the rest of the file as it is):

```powershell
python Tools/TrackBuilder/flatten_tires.py Projects/Racing/Assets/Models/KenneyCarKit/wheel-racing.glb `
    Projects/Racing/Assets/Models/KenneyCarKit/wheel-racing-flat.glb   # [edge]: the chamfer, 0.07 of the width
```
