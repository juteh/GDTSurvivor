# GDTSurvivor Architecture

This document defines **who is responsible for what**, **who may talk to whom**, and **what is stored where**.
It describes the *target* architecture. Places where the project does not follow it yet are listed in
[Migration status](#7-migration-status). New code must follow this document; old code is moved over step by step.

---

## 1. Overview

```
 ┌──────────────────────────── GameInstance (whole app session) ────────────────────────────┐
 │  Subsystems: MetaProgression, RunState, Settings, Highscore   ← the only owners of saves │
 └───────────────────────────────────────────────────────────────────────────────────────────┘
        ▲ read / call                                     ▲ read / call
 ┌──────┴────────── Level (one map) ─────────────────────┴──────────────────────────────────┐
 │  GameMode (server)  ──spawns / decides──▶  GameState (replicated, shared match data)     │
 │       │                                                                                   │
 │       ▼ possesses                                                                         │
 │  PlayerController (owner client + server) ──creates──▶ HUD / Widgets (local only)        │
 │       │                                                   ▲ listen to events              │
 │       ▼                                                   │                               │
 │  Pawn ── components (Stats, Weapons, Magnet …) ──events───┘                               │
 │  PlayerState (replicated, survives the pawn) ──events──▶ HUD                             │
 └───────────────────────────────────────────────────────────────────────────────────────────┘
```

Rule of thumb: **calls go down, events go up.**

---

## 2. Responsibilities

Each class has one job. "Must not" lists the most common ways to break that.

### GameInstance + GameInstance Subsystems
*Lives for the whole app session, across map changes. Exists locally on every machine.*

| | |
|---|---|
| **Owns** | Everything that must survive a map change: meta progression, the current run between levels, settings, highscores. |
| **May** | Load and save SaveGames. Provide data and operations to everyone (`GetGameInstance()->GetSubsystem<…>()`). Broadcast change events. |
| **Must not** | Hold references to actors or widgets of a level. Contain gameplay rules. |

Use one **subsystem per topic** (e.g. `UMetaProgressionSubsystem`) instead of growing the GameInstance Blueprint.

### GameMode
*Server only. Clients never have a GameMode, so nothing on a client may depend on it.*

| | |
|---|---|
| **Owns** | The rules of a match: start, win/lose conditions, when a level-up happens, which enemies spawn. |
| **May** | Spawn and possess pawns. Change GameState and PlayerState. Ask subsystems for data (e.g. permanent upgrades). |
| **Must not** | Create or update widgets (`CreateWidget`, `UpdatePlayerHUD`). Store data that clients need to see (that goes to GameState / PlayerState). Load SaveGames directly. |

### GameState
*Exists on server and all clients, replicated. Shared data of the match.*

| | |
|---|---|
| **Owns** | Match-wide state everyone can see: timer, current objective and its progress, difficulty/intensity, list of players (`PlayerArray`). |
| **May** | Hold data and broadcast events like `OnTimerChanged`, `OnObjectiveUpdated`. |
| **Must not** | Make rule decisions (that is the GameMode). Hold PlayerControllers (they do not exist on other clients; use `PlayerArray`). |

### PlayerController
*Exists on the owning client and the server. The player's "will".*

| | |
|---|---|
| **Owns** | Input, the HUD and all menus of this player, pause, mouse/input mode. |
| **May** | Create widgets. Send the player's choices to the server (Server RPCs, e.g. "picked level-up option X"). |
| **Must not** | Contain gameplay rules or stat values. |

### PlayerState
*Exists on server and all clients, replicated. Survives the pawn (death, respawn).*

| | |
|---|---|
| **Owns** | What the player has earned **in this run**: experience, level, chosen level-up upgrades, score, player name. |
| **May** | Hold data and broadcast events (`OnExperienceChanged`, `OnUpgradesChanged`, `OnScoreChanged`). |
| **Must not** | Hold the current health/shield (that belongs to the pawn). Talk to widgets directly. |

### Pawn (`APlayerSpaceShipPawn`) and its components
*The ship in the world. Can die and be replaced.*

| | |
|---|---|
| **Owns** | Movement, shooting, collision, and the **current** combat values via components (health, shield, fire rate, damage, pickup range). |
| **May** | Call its own components directly. Apply damage to other actors (`ApplyDamage`). Broadcast events (`OnHealthChanged`, `OnDeath`). |
| **Must not** | Load SaveGames. Know about widgets. Decide win/lose (it reports `OnDeath`, the GameMode decides). |

The stats component calculates every value as **base value + modifiers**. Each modifier has a source
(level-up, permanent upgrade, later buffs), so the pawn never has to know where a bonus came from.

### HUD and Widgets
*Local only. Pure presentation.*

| | |
|---|---|
| **Owns** | Showing data and turning clicks into requests. |
| **May** | Read data from PlayerState, GameState, pawn components and subsystems. Listen to their events. Call actions like `BuyUpgrade` or "choose option". |
| **Must not** | Change gameplay values directly. Be updated by other classes from the outside (no `GameMode → HUD.UpdateHealthUI`); a widget updates **itself** by listening to events. |

### Other actors (enemies, pickups, projectiles, asteroids)

| | |
|---|---|
| **May** | Deal damage via `ApplyDamage`. Give rewards through the receiving player's PlayerState or pawn component (e.g. XP pickup → `PlayerState.AddExperience`). Report important events (enemy destroyed) via events or the GameMode on the server. |
| **Must not** | Use `GetPlayerPawn(0)` / `GetPlayerController(0)`; use the actor that actually overlapped/hit/instigated. |

### Data Assets / DataTables
Definitions that designers tune: upgrades (stat, value per stack, max stacks, cost, name, icon), enemy types,
tiles. Code reads them; nobody hardcodes these values in graphs.

---

## 3. Communication rules

1. **Calls go down.** An owner may call what it owns directly: GameMode → GameState, Pawn → its components, PlayerController → its widgets.
2. **Events go up and sideways.** A component never calls its owner or the HUD. It broadcasts an event (multicast delegate / event dispatcher) and whoever cares subscribes.
3. **Widgets update themselves.** They subscribe to events in `NativeConstruct` and unsubscribe in `NativeDestruct`.
4. **One source of truth per value.** Every value has exactly one owner (see [section 4](#4-what-is-stored-where)). Everybody else reads it or listens to it, nobody keeps a copy.
5. **No `GetPlayerPawn(0)` / `GetPlayerController(0)` in gameplay code.** Use the owner, the instigator or the overlapping actor. (Index 0 only works in single player.)
6. **No casts to the GameMode from client-side code** (widgets, PlayerController visuals, pawn visuals). The GameMode is server only.
7. **Only subsystems touch SaveGames.** No `LoadGameFromSlot` / `SaveGameToSlot` anywhere else.
8. **C++ first.** New logic is written in C++. Blueprints are for assets, layout, tuning values and thin subclasses of C++ classes.
9. **Data over branches.** No `if Type == 0 … else if Type == 1 …` chains or string keys for upgrade types; look the definition up in data.

---

## 4. What is stored where

### Runtime ownership

| Data | Lifetime | Owner | Why there |
|---|---|---|---|
| Money, permanent upgrades | Forever | `UMetaProgressionSubsystem` | Must survive app restarts and all maps. |
| Settings | Forever | Settings subsystem | Needed before any level is loaded. |
| Highscores | Forever | Highscore subsystem | Shared by all runs, shown in menus. |
| Run progress between campaign levels (score, health carried over, completed levels) | One run, across maps | `RunState` subsystem | Must survive the map change; the PlayerState is destroyed with the level. |
| Experience, level, chosen level-ups, score | One run inside a level | PlayerState | Replicated to everyone (player list), survives pawn death. |
| Current health/shield, final combat values | One life | Pawn stats component | Belongs to the body in the world; recalculated from base + modifiers. |
| Timer, objective, intensity | One match | GameState | Shared by all players, replicated. |
| Match rules state (pending level-ups, spawn logic) | One match | GameMode | Only the server decides. |
| Widget-only state (open tab, hover) | While visible | Widget | Pure presentation. |

### Save slots

| Slot | SaveGame class | Owner | Contents | Written when |
|---|---|---|---|---|
| `metaprogression` | `UMetaProgressionSaveGame` | `UMetaProgressionSubsystem` | Money, spent money, upgrade levels | Immediately on every change |
| `state` | `StateSave` (BP) | → `RunState` subsystem (planned) | Run data between campaign levels | Level completed |
| `settings` | `SettingsSave` (BP) | → Settings subsystem (planned) | Graphics/audio/input settings | Settings applied |
| `highscore` | `HighscoreSave` (BP) | → Highscore subsystem (planned) | Name + score list | Run finished |

**Applying saved data at level start:** the subsystem only *provides* data. The GameMode (or the pawn's stats
component on spawn) asks for it **once**, at a single, documented place, and turns it into modifiers.
Loading the same slot in several classes leads to values overwriting each other (this happened with the
permanent health/shield upgrades).

---

## 5. Upgrades and stats (target model)

```
UpgradeDefinition (Data Asset)          PlayerState                 Pawn: ShipStatsComponent
  Stat = MaxHealth                        chosen level-ups  ──┐       base values
  ValuePerStack = 5                                           ├──▶    + modifiers (source, stat, value)
  MaxStacks = 10, Cost = 100           MetaProgression       ─┘       = current values ──event──▶ HUD
  Name, Icon                             permanent upgrades
```

- A level-up and a permanent upgrade use the **same definition and the same code path**; only the source differs.
- Adding a new upgrade = adding a Data Asset (+ a stat if it is a new kind of value).
- This model maps closely to the Gameplay Ability System (AttributeSet + GameplayEffects), which is planned
  for buffs and abilities later.

---

## 6. Naming and placement

- C++: `Source/GDTSurvivor/Public` (headers) and `Private` (sources). Comments and names in English.
- One subsystem or component per topic, named after the topic (`MetaProgressionSubsystem`, `ShipStatsComponent`).
- Blueprint subclasses of C++ classes keep the `BP_` / `WBP_` prefix and contain no logic beyond tuning and asset references.
- Events are named `On<What>Changed` / `On<Something>Happened`.

---

## 7. Migration status

Known places that do not follow this document yet, in planned order:

- [x] **Stats are spread over four places** → `UShipStatsComponent` on the player ship (health, shield incl. recharge, fire rate, damage, pickup range; level-up and permanent upgrades as modifiers). Enemies still use `BP_HealthComponent`.
- [x] **HUD is updated from outside** → `UPlayerHUDWidget` (parent of `WBP_HUD`) listens to `UShipStatsComponent` (health, shield) and `AShipPlayerState` (score, level, experience, upgrade list). `UpdatePlayerHUD` / `UpdateLevelHUD` are gone.
- [ ] **Upgrades are hardcoded** → upgrade Data Assets. *Remaining: effect values in `UShipStatsComponent::AddUpgradeStack`, names/descriptions in `BP_GameMode_Base.GetUpgradeDisplayName/GetUpgradeDescription` and `UpgradeSelectionWidget`, max stacks (10) in `ShowLevelUpUI`/`PopulateOptionSlot`, upgrade types passed as byte.*
- [ ] **`state` slot is loaded by both `BP_GameMode_Base` (score) and `BP_PlayerSpaceShipPawn` (health/shield via `ShipStats.RestoreFromRunState`).** → `RunState` subsystem, applied once.
- [ ] **GameMode creates widgets** (HUD, tutorial, level-up, result board). → PlayerController / `AHUD`.
- [ ] **`GameState.ALL_PCs` stores PlayerControllers.** → use `PlayerArray`.
- [ ] **`GetPlayerPawn(0)` in gameplay code** (GameMode, components, pickups). → owner / instigator.
- [x] **Score lives in `BP_ScoreComponent` on the pawn** → `AShipPlayerState.AddScore`; pickups reward the overlapping player via `GetShipPlayerState(Actor)`.
- [x] Meta progression in a GameInstance subsystem with its own save slot.
