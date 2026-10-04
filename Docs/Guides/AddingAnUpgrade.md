# Adding an Upgrade

How to add a new upgrade that players can pick on level-up and/or buy permanently in the
spaceship menu. Background: [Architecture.md, section 5](../Architecture.md#5-upgrades-and-stats-target-model).

There are two cases:

- **A – the upgrade changes an existing stat** (health, shield, fire rate, …): assets only, no code.
- **B – the upgrade needs a new kind of value** (e.g. move speed): small C++ change first, then case A.

---

## A – New upgrade for an existing stat (no code)

### 1. Create the asset

1. Content Browser → `Content/GDTSurvivor/Core/Upgrades/`
2. Right-click → **Miscellaneous → Data Asset** → pick **`UpgradeDefinition`**.
3. Name it `DA_Upgrade_<Name>`, e.g. `DA_Upgrade_CritDamage`.

Tip: duplicating an existing `DA_Upgrade_*` (Ctrl+D) is faster, but **always change the `UpgradeId`** afterwards.

### 2. Fill in the fields

| Field | What to enter |
|---|---|
| **Upgrade Id** | Unique, stable name, e.g. `CritDamage`. Saved in the player's save game. **Never change it** once the upgrade has been released, or bought levels are lost. |
| **Display Name** | Short text on buttons, shop rows and in the HUD list, e.g. `Damage +1`. |
| **Description** | Tooltip on the level-up button, e.g. `Increases the damage per shot by 1.` (English). |
| **Icon** | Optional. Not shown anywhere yet. |
| **Effects** | One entry per stat the upgrade changes, applied **once per stack** (see below). |
| **Offer As Level Up** | Can be rolled as a level-up choice. |
| **Max Level Up Stacks** | How often it can be picked per run. Maxed upgrades are no longer offered. |
| **Sell As Permanent** | Appears in the permanent upgrade shop. |
| **Max Permanent Level** | How often it can be bought in the shop. |
| **Permanent Cost** | Price of every permanent level. |

#### How effects are calculated

Each stat is calculated as

```
value = (base + sum of all Additive) * product of all Multiplier
```

over all stacks of all sources (level-ups and permanent upgrades together), then clamped to a minimum.

- **Additive** for flat bonuses: `MaxHealth` with Additive `5` → +5 health per stack.
- **Multiplier** for percentages. Multipliers **compound**: Multiplier `0.9` on
  `FireIntervalMultiplier` gives 0.9, 0.81, 0.73, … — i.e. "10 % faster" per stack.
- Leave the unused one at its neutral value (Additive `0`, Multiplier `1`).

| Stat | Base | Meaning | Typical effect |
|---|---|---|---|
| `MaxHealth` | 100 | Maximum health (min. 1) | Additive +5 |
| `MaxShield` | 100 | Maximum shield | Additive +5 |
| `ShieldRegenDelay` | 3 s | Pause after a hit before the shield recharges | Multiplier 0.9 |
| `ShieldRegenInterval` | 0.1 s | Seconds per recharged shield point (min. 0.05) — lower is faster | Multiplier 0.9 |
| `FireIntervalMultiplier` | 1 | Multiplier on every weapon's time between shots (min. 0.05) — lower is faster | Multiplier 0.9 |
| `DamageBonus` | 0 | Flat damage added to every player projectile | Additive +1 |
| `PickupRangeMultiplier` | 1 | Multiplier on the pickup radius of XP shards and minerals | Multiplier 1.1 |

Base values for health, shield and shield regeneration can be changed on the `ShipStats` component of
`BP_PlayerSpaceShipPawn` (section *Stats | Base*). Raising a maximum also fills the current value by the same amount.

### 3. Add it to the catalog

Open `Content/GDTSurvivor/Core/Upgrades/DA_UpgradeCatalog` and add the new asset to **Upgrades**.
The order of this list is the order of the rows in the permanent shop.

Without this step the upgrade exists but is never offered or sold.

### 4. Validate

Right-click `DA_UpgradeCatalog` → **Asset Actions → Validate Assets**. It reports empty entries and
missing or duplicate `UpgradeId`s.

### 5. Check the shop rows

`WBP_UpgradeSelection` has **six fixed rows**. Upgrades with *Sell As Permanent* fill them in catalog
order; a seventh one is **not shown**. To add a row:

1. In the `WBP_UpgradeSelection` designer, duplicate the last row and name the new widgets
   `WBP_ButtonBase_Buy_7` and `CommonTextBlock_Upgrade_7` (exact names).
2. In `Source/GDTSurvivor/Public/UpgradeSelectionWidget.h` add both as `BindWidget` properties, like rows 1–6.
3. In `UpgradeSelectionWidget.cpp` (`NativeOnInitialized`) add `{WBP_ButtonBase_Buy_7, CommonTextBlock_Upgrade_7}` to `Rows`.
4. Build (Live Coding is enough).

### 6. Test

- **Level-up:** play until the level-up selection appears (it offers 3 random upgrades that are not maxed).
  Hover the button to see the description, pick it, and check the HUD upgrade list and the effect.
- **Permanent:** buy it in the spaceship menu, start a level, check the effect. Use *Reset Upgrades* to refund.

---

## B – Upgrade for a new kind of value (C++)

Example: a `MoveSpeedMultiplier` stat.

1. **Add the stat** in `Source/GDTSurvivor/Public/ShipStatsComponent.h`, enum `EShipStat`, **before `Count`**,
   with a one-line comment that says what it means and whether lower or higher is better.
2. **Give it a base value** in `UShipStatsComponent::GetBaseValue` (`ShipStatsComponent.cpp`).
   If designers should tune it, add a `Base…` property in the *Stats | Base* section of the header, like `BaseMaxShield`.
3. **Optional minimum** in `UShipStatsComponent::GetMinValue`, if stacked multipliers could make it zero or negative.
4. **Use it** where the value matters: `ShipStats->GetStat(EShipStat::MoveSpeedMultiplier)`.
   If something must react when it changes (e.g. a widget), listen to `UShipStatsComponent::OnStatChanged`
   instead of polling.
5. **Full build** (enum changes need Rider + editor restart, Live Coding is not enough).
6. Continue with **case A** and pick the new stat in the asset's *Effects*.
7. Add the stat to the table in this guide.

Do **not** add upgrade-specific code anywhere else (no `if (Upgrade == …)` branches). Everything an upgrade
does must be expressible as effects on stats; see [Architecture.md, rule 9](../Architecture.md#3-communication-rules).

---

## Where things live

| What | Where |
|---|---|
| Upgrade assets and catalog | `Content/GDTSurvivor/Core/Upgrades/` |
| Which catalog is used | Project Settings → Game → GDTSurvivor → *Upgrade Catalog* (`Config/DefaultGame.ini`) |
| Asset class | `UUpgradeDefinition` (`Source/GDTSurvivor/Public/UpgradeDefinition.h`) |
| Stats and their calculation | `UShipStatsComponent` (`ShipStatsComponent.h/.cpp`) |
| Chosen level-ups in a run | `AShipPlayerState` (`GetUpgradeStacks`, `GetLevelUpOptions`) |
| Bought permanent levels | `UMetaProgressionSubsystem`, save slot `metaprogression` (`Saved/SaveGames/metaprogression.sav`) |
| Level-up selection | `ULevelUpSelectionWidget` → `WBP_LevelUpSelection` |
| Permanent shop | `UUpgradeSelectionWidget` → `WBP_UpgradeSelection` |
