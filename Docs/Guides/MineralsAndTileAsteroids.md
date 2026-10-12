# Minerals and Tile Asteroids

How mineral asteroids get into the endless map, and how to add a mineral or tune the zones. Background:
[Architecture.md, section 5b](../Architecture.md#5b-minerals-and-zones-target-model).

Two questions are answered in two different places:

| Question | Answered by | Where |
|---|---|---|
| **Where** can asteroids be in a tile, and how many? | the tile map | `AsteroidPlacement` on the `EndlessTileAuthoring` actor, `MineralAsteroidSpawnPoint` markers |
| **Which** mineral and size, i.e. how valuable? | the zone of the tile | `DA_MineralCatalog` (Project Settings > Game > GDTSurvivor > Mineral Catalog) |

Zones are rings around the base tile (0, 0). A tile belongs to the zone with the highest `MinDistanceInTiles`
that is not above the straight-line distance from the base tile center to the tile center. A tile is always
completely in one zone. The further out, the more valuable minerals and bigger asteroids ("high risk, high reward").

---

## A – Build asteroids into a tile map

1. Open the tile map (`Tile_A`, ...) and select the `EndlessTileAuthoring` actor.
2. Set **Asteroid Placement**:

   | Value | Result |
   |---|---|
   | **None** | No asteroids (e.g. the start tile). |
   | **Markers** | Only at the `MineralAsteroidSpawnPoint` markers in this map. |
   | **Random** | Only on random free spots; **Min/Max Random Asteroids** per tile. |
   | **Markers And Random** | Markers first, then random ones on the remaining free spots. |

3. For markers: drag `MineralAsteroidSpawnPoint` (Place Actors panel, search "Mineral") into the map.

   | Field | Meaning |
   |---|---|
   | **Chance** | Chance that an asteroid appears here at all (1 = always). |
   | **Min Size / Max Size** | Allowed size classes; the zone picks one in this range. |
   | **Fixed Mineral** | Optional. Always this mineral, regardless of the zone (e.g. a hidden Iconos in a wreck). |

   The cyan circle shows the biggest asteroid that can appear at this marker. Markers are not checked for
   free space, so keep the circle clear of walls yourself.

4. Press **Bake** on the `EndlessTileAuthoring` actor. The notification lists the number of markers.

## B – Add a mineral

1. Create the small piece (`BP_Mineral_<Name>`, child of `BP_Mineral_Base`) and the big asteroid
   (`BP_MineralAsteroid_<Name>`, child of `MineralAsteroid`) with **Mineral Class** set to the small piece.
2. Content Browser → right-click → **Miscellaneous → Data Asset → MineralDefinition**, name it
   `DA_Mineral_<Name>`:

   | Field | What to enter |
   |---|---|
   | **Mineral Id** | Unique, never change it later (e.g. `Iconos`). |
   | **Display Name**, **Color**, **Icon** | For the HUD and the evaluation. |
   | **Value** | Points per collected piece in the evaluation at the end of the level. |
   | **Asteroid Class** | `BP_MineralAsteroid_<Name>`. |

3. Add it to **Minerals** in `DA_MineralCatalog`, then give it a weight in the **Mineral Weights** of the zones
   where it should appear.
4. Right-click the catalog → **Asset Actions → Validate**: catches missing ids, unsorted zones and minerals
   that are used in a zone but not listed.

## C – Tune zones and sizes

All in `DA_MineralCatalog`:

- **Sizes**: what `Small` … `Huge` mean (actor scale, health, number of pieces).
- **Zones**: sorted by **Min Distance In Tiles**, the first one at 0. Per zone **Mineral Weights** and
  **Size Weights** (relative chances; a mineral or size that is missing never appears in that zone).

With **Show Tile Debug** on the `EndlessTileManager`, every tile shows its zone name.

## Where things live

| What | Where |
|---|---|
| Mineral definitions, zones, sizes | `UMineralDefinition`, `UMineralCatalog` (`Source/GDTSurvivor/Public/Minerals/`) |
| Markers | `AMineralAsteroidSpawnPoint` (`World/`), baked into `UEndlessTileData::AsteroidSlots` |
| Spawning | `AEndlessTile::SpawnMineralAsteroids` (server, deterministic per tile coordinate and seed) |
| Free-spot rules for random asteroids | `Asteroid Placement` on the `EndlessTileManager` (edge margin, clearance, attempts) |
| Carried / delivered minerals per player | `AShipPlayerState`: `CollectMineral` (mineral pickup), `DeliverMinerals` (dropzone), `GetDeliveredMineralValue` (evaluation), event `OnMineralsChanged` |
