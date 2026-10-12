# Adding a Tile

How to add a hand-made tile to the endless map (`Level_Endless`), and how to rename one. Background:
`Source/GDTSurvivor/Public/World/EndlessTileAuthoring.h` and `EndlessTileManager.h`. Asteroids in tiles:
[MineralsAndTileAsteroids.md](MineralsAndTileAsteroids.md).

A tile is a small map that is **baked** into a `UEndlessTileData` asset (`DA_<MapName>`). The
`EndlessTileManager` in `Level_Endless` picks tiles from its **Tile Set** by weight (per seed, so every level start
is different) and rotates them by 0/90/180/270 degrees.

---

## A – Add a tile

1. **New map** next to the other tiles: *File → New Level → Empty Level*, save it as
   `Content/GDTSurvivor/Maps/SurvivorMode/Tiles/Tile_<Name>` (or duplicate an existing tile map).
2. Drag an **`EndlessTileAuthoring`** actor into the map and set its location to **0, 0, 0**. It is the tile
   center; everything is stored relative to it.
3. Set it up:

   | Field | Meaning |
   |---|---|
   | **Tile Name** | Shown in the tile debug text. Empty = map name. |
   | **Allow Rotation** | May be placed rotated by 90/180/270 degrees. |
   | **Weight** | How often this tile is picked compared to the others in the Tile Set. |
   | **Asteroid Placement**, **Min/Max Random Asteroids** | See [MineralsAndTileAsteroids.md](MineralsAndTileAsteroids.md). |
   | **Target Data** | Leave empty for a new tile; the bake creates `DA_<MapName>` next to the map. |

4. **Build the content** inside the green box (tile size); keep the orange border strip free, so neighbouring
   tiles never block each other:
   - Static mesh actors → walls and decoration (baked into instanced meshes).
   - Blueprint actors (e.g. `BP_WreckContainer`, pickups) → spawned on the server when the tile is placed.
   - `MineralAsteroidSpawnPoint` → asteroid markers.
   - Lights, volumes, `PlayerStart` and actors tagged `NoBake` are ignored.
5. Press **Bake** on the `EndlessTileAuthoring` actor. The notification lists what was baked and warns about
   actors outside the tile.
6. Open `Level_Endless`, select the **`EndlessTileManager`** and add the new `DA_Tile_<Name>` to its
   **Tile Set**. Save the level.

After changing a tile later, pressing **Bake** again is enough.

## B – Rename a tile

Always rename in the **Content Browser** (never in the file explorer), so Unreal updates all references.

1. Rename the map and its data asset (`Tile_A` → `Tile_<New>`, `DA_Tile_A` → `DA_Tile_<New>`). Unreal updates
   **Target Data** on the authoring actor and the **Tile Set** / **Start Tile** entries on the manager.
2. Check that **Target Data** on the authoring actor points to the renamed asset. If it is empty, the bake
   creates a *new* `DA_<MapName>` and the manager keeps using the old one.
3. Press **Bake** (updates the stored map path and the tile name), then save the tile map and `Level_Endless`.
4. Optional: right-click the folder → **Update Redirector References** to remove leftover redirectors.

If **Tile Name** is set on the authoring actor, change it yourself; it is used instead of the map name.

## Where things live

| What | Where |
|---|---|
| Tile maps and their data assets | `Content/GDTSurvivor/Maps/SurvivorMode/Tiles/` (`Tile_*`, `DA_Tile_*`) |
| Which tiles are used, start tile, seed | `EndlessTileManager` actor in `Level_Endless` |
| Bake | `AEndlessTileAuthoring::Bake` (`World/`) |
