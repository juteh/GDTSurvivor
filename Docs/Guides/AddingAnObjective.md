# Adding an Objective

How to add a new level objective (e.g. "Destroy 20 asteroids"). Background:
[Architecture.md, section 7](../Architecture.md#7-migration-status) and `Source/GDTSurvivor/Public/Objectives/Objective.h`.

An objective is a Blueprint child of `BP_ObjectiveBase` (C++ parent `AObjective`) that only contains **data**.
The logic (counting, completing, replication, HUD text) is the same for all objectives.

---

## A – The objective counts an existing event (no code)

Existing events (`EObjectiveEvent`): `MineralCollected`, `EnemyDestroyed`.

1. Content Browser → `Content/GDTSurvivor/Core/GameLogic/` → right-click `BP_ObjectiveBase` →
   **Create Child Blueprint Class**, name it `BP_Objective_<Name>`.
2. Open it, **Class Defaults**, section *Objective*:

   | Field | What to enter |
   |---|---|
   | **Counted Event** | The event that adds one progress step. |
   | **Can Complete** | Off for objectives that only collect (like endless mode). |
   | **Progress Format** | HUD text while running. Placeholders: `{Current}`, `{Required}`, `{Stored}`. Example: `Kill all enemies. {Current}/{Required}` |
   | **Completed Text** | HUD text once complete, e.g. `All enemies destroyed.` |

3. In the level, select the `BP_ObjectiveHandler` actor and set **Current Objective** to the new class and
   **Required Items** to the target amount.

The objective is complete when the progress is **greater than** Required Items (same rule as before the C++
refactoring), so "5 enemies" needs 6 kills. Set Required Items accordingly.

## B – The objective needs a new kind of event (C++)

Example: "Destroy asteroids".

1. Add the event to `EObjectiveEvent` in `Source/GDTSurvivor/Public/Objectives/Objective.h`, e.g. `AsteroidDestroyed`.
2. Full build (enum change: Rider + editor restart).
3. Where the event happens **on the server**, report it to the GameMode:
   - Blueprint: `Get Game Mode` → cast to `BP_GameMode_Base` → **Report Objective Event** (`AsteroidDestroyed`).
   - C++: `GetWorld()->GetAuthGameMode<AShipGameModeBase>()->ReportObjectiveEvent(EObjectiveEvent::AsteroidDestroyed)`.
4. Continue with case A.

Do **not** cast to specific objective classes anywhere (no "if CollectItems … else if KillEnemies …").
Every objective reacts to `ReportObjectiveEvent` on its own; see [Architecture.md, rule 9](../Architecture.md#3-communication-rules).

## Where things live

| What | Where |
|---|---|
| Objective classes | `Content/GDTSurvivor/Core/GameLogic/BP_Objective_*` (parent `AObjective`) |
| Which objective a level uses | `BP_ObjectiveHandler` actor in the level |
| Active objective | `AShipGameState::GetActiveObjective` (set by the GameMode in BeginPlay) |
| HUD text | `UObjectiveHUDWidget` → `WBP_HUD_Objective` (created by `AShipPlayerController`) |
| Win check / endless delivery | `BP_GameMode_Base.CheckWinCondition` (`IsComplete`, `StoreProgress`) |
